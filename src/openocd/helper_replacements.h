/***************************************************************************
 *   Copyright (C) 2006 by Dominic Rath                                    *
 *   Dominic.Rath@gmx.de                                                   *
 *                                                                         *
 *   Copyright (C) 2007,2008 Øyvind Harboe                                 *
 *   oyvind.harboe@zylin.com                                               *
 *                                                                         *
 *   Copyright (C) 2008 by Spencer Oliver                                  *
 *   spen@spen-soft.co.uk                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#ifndef OPENOCD_HELPER_REPLACEMENTS_H
#define OPENOCD_HELPER_REPLACEMENTS_H

#include "helper_types.h"

/* MIN,MAX macros */
#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

/* for systems that do not support ENOTSUP
 * win32 being one of them */
#ifndef ENOTSUP
#define ENOTSUP 134 /* Not supported */
#endif

/* for systems that do not support O_BINARY
 * linux being one of them */
#ifndef O_BINARY
#define O_BINARY 0
#endif

/* Windows specific */
#ifdef _WIN32

#include <windows.h>
#include <time.h>

/* Windows does not declare sockaddr_un */
#define UNIX_PATH_LEN 108
struct sockaddr_un
{
	uint16_t sun_family;
	char sun_path[UNIX_PATH_LEN];
};

/* win32 systems do not support ETIMEDOUT */
#ifndef ETIMEDOUT
#define ETIMEDOUT WSAETIMEDOUT
#endif

/* GNU extensions to the C library that may be missing on some systems */
#ifdef __MINGW32__
char *strndup(const char *s, size_t n);
#endif	/* HAVE_STRNDUP */


#if IS_MINGW == 1
static inline unsigned char inb(unsigned short int port)
{
	unsigned char _v;
	__asm__ __volatile__("inb %w1,%0"
						 : "=a"(_v)
						 : "Nd"(port));
	return _v;
}

static inline void outb(unsigned char value, unsigned short int port)
{
	__asm__ __volatile__("outb %b0,%w1"
						 :
						 : "a"(value), "Nd"(port));
}

/* mingw does not have ffs, so use gcc builtin types */
#define ffs __builtin_ffs

#endif /* IS_MINGW */

int win_select(int max_fd, fd_set *rfds, fd_set *wfds, fd_set *efds, struct timeval *tv);

#endif /* _WIN32 */

/* generic socket functions for Windows and Posix */
static inline int write_socket(int handle, const void *buffer, unsigned int count)
{
#ifdef _WIN32
	return send(handle, buffer, count, 0);
#else
	return write(handle, buffer, count);
#endif
}

static inline int read_socket(int handle, void *buffer, unsigned int count)
{
#ifdef _WIN32
	return recv(handle, buffer, count, 0);
#else
	return read(handle, buffer, count);
#endif
}

static inline int close_socket(int sock)
{
#ifdef _WIN32
	return closesocket(sock);
#else
	return close(sock);
#endif
}

static inline void socket_block(int fd)
{
#ifdef _WIN32
	unsigned long nonblock = 0;
	ioctlsocket(fd, FIONBIO, &nonblock);
#else
	int oldopts = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, oldopts & ~O_NONBLOCK);
#endif
}

static inline void socket_nonblock(int fd)
{
#ifdef _WIN32
	unsigned long nonblock = 1;
	ioctlsocket(fd, FIONBIO, &nonblock);
#else
	int oldopts = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, oldopts | O_NONBLOCK);
#endif
}

static inline int socket_select(int max_fd,
								fd_set *rfds,
								fd_set *wfds,
								fd_set *efds,
								struct timeval *tv)
{
#ifdef _WIN32
	return win_select(max_fd, rfds, wfds, efds, tv);
#else
	return select(max_fd, rfds, wfds, efds, tv);
#endif
}

#endif /* OPENOCD_HELPER_REPLACEMENTS_H */
