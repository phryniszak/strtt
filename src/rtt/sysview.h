/*
 * Author(s): Pawel Hryniszak <phryniszak@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SYSVIEW_H
#define _SYSVIEW_H

#include <sockpp/tcp_acceptor.h>

#include "blockingconcurrentqueue.h"

class SysView
{
public:
    SysView(int port);
    bool saveFromSTM(const std::vector<uint8_t> *buffer);
    size_t dataToSTM();
    std::vector<unsigned char> getDataToSTM();

private:
    moodycamel::BlockingConcurrentQueue<unsigned char> _q_from_uc;
    moodycamel::BlockingConcurrentQueue<unsigned char> _q_to_uc;
    int _port;
    std::thread _th;
    bool _connected;

    void run_server();
    void run_socket(sockpp::tcp_socket sock);
};

#endif
