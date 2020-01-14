/***************************************************************************
 *   Copyright (C) 2005 by Dominic Rath                                    *
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

#ifndef _HL_H
#define _HL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*
 * TARGET_UNKNOWN = 0: we don't know anything about the target yet
 * TARGET_RUNNING = 1: the target is executing user code
 * TARGET_HALTED  = 2: the target is not executing code, and ready to talk to the
 * debugger. on an xscale it means that the debug handler is executing
 * TARGET_RESET   = 3: the target is being held in reset (only a temporary state,
 * not sure how this is used with all the recent changes)
 * TARGET_DEBUG_RUNNING = 4: the target is running, but it is executing code on
 * behalf of the debugger (e.g. algorithm for flashing)
 *
 * also see: target_state_name();
 */

enum target_state {
	TARGET_UNKNOWN = 0,
	TARGET_RUNNING = 1,
	TARGET_HALTED = 2,
	TARGET_RESET = 3,
	TARGET_DEBUG_RUNNING = 4,
};


enum tpiu_pin_protocol
{
    TPIU_PIN_PROTOCOL_SYNC,             /**< synchronous trace output */
    TPIU_PIN_PROTOCOL_ASYNC_MANCHESTER, /**< asynchronous output with Manchester coding */
    TPIU_PIN_PROTOCOL_ASYNC_UART        /**< asynchronous output with NRZ coding */
};

enum hl_transports
{
    HL_TRANSPORT_UNKNOWN = 0,
    HL_TRANSPORT_SWD,
    HL_TRANSPORT_JTAG,
    HL_TRANSPORT_SWIM
};

#define HLA_MAX_USB_IDS 8

struct hl_interface_param_s
{
    /** */
    const char *device_desc;
    /** */
    const char *serial;
    /** List of recognised VIDs */
    uint16_t vid[HLA_MAX_USB_IDS + 1];
    /** List of recognised PIDs */
    uint16_t pid[HLA_MAX_USB_IDS + 1];
    /** */
    unsigned api;
    /** */
    enum hl_transports transport;
    /** */
    bool connect_under_reset;
    /** Initial interface clock clock speed */
    int initial_interface_speed;
};

struct hl_layout_api_s
{
    /** */
    int (*open)(struct hl_interface_param_s *param, void **handle);
    /** */
    int (*close)(void *handle);
    /** */
    int (*reset)(void *handle);
    /** */
    int (*assert_srst)(void *handle, int srst);
    /** */
    int (*run)(void *handle);
    /** */
    int (*halt)(void *handle);
    /** */
    int (*step)(void *handle);
    /** */
    int (*read_regs)(void *handle);
    /** */
    int (*read_reg)(void *handle, int num, uint32_t *val);
    /** */
    int (*write_reg)(void *handle, int num, uint32_t val);
    /** */
    int (*read_mem)(void *handle, uint32_t addr, uint32_t size,
                    uint32_t count, uint8_t *buffer);
    /** */
    int (*write_mem)(void *handle, uint32_t addr, uint32_t size,
                     uint32_t count, const uint8_t *buffer);
    /** */
    int (*write_debug_reg)(void *handle, uint32_t addr, uint32_t val);
    /**
	 * Read the idcode of the target connected to the adapter
	 *
	 * If the adapter doesn't support idcode retrieval, this callback should
	 * store 0 to indicate a wildcard match.
	 *
	 * @param handle A pointer to the device-specific handle
	 * @param idcode Storage for the detected idcode
	 * @returns ERROR_OK on success, or an error code on failure.
	 */
    int (*idcode)(void *handle, uint32_t *idcode);
    /** */
    int (*override_target)(const char *targetname);
    /** */
    int (*custom_command)(void *handle, const char *command);
    /** */
    int (*speed)(void *handle, int khz, bool query);
    /**
	 * Configure trace parameters for the adapter
	 *
	 * @param handle A handle to adapter
	 * @param enabled Whether to enable trace
	 * @param pin_protocol Configured pin protocol
	 * @param port_size Trace port width for sync mode
	 * @param trace_freq A pointer to the configured trace
	 * frequency; if it points to 0, the adapter driver must write
	 * its maximum supported rate there
	 * @returns ERROR_OK on success, an error code on failure.
	 */
    int (*config_trace)(void *handle, bool enabled, enum tpiu_pin_protocol pin_protocol,
                        uint32_t port_size, unsigned int *trace_freq);
    /**
	 * Poll for new trace data
	 *
	 * @param handle A handle to adapter
	 * @param buf A pointer to buffer to store received data
	 * @param size A pointer to buffer size; must be filled with
	 * the actual amount of bytes written
	 *
	 * @returns ERROR_OK on success, an error code on failure.
	 */
    int (*poll_trace)(void *handle, uint8_t *buf, size_t *size);
    /** */
    target_state (*state)(void *fd);
};

extern struct hl_layout_api_s stlink_usb_layout_api;

#endif