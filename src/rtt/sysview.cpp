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

#include <vector>

#include <sockpp/version.h>

#include "log.h"
#include "sysview.h"

using namespace moodycamel;
using namespace std::chrono_literals;

extern std::atomic_bool stopApp;

#define DEQ_SIZE 1400

#define SYSVIEW_COMM_TARGET_HELLO_SIZE 32
#define SYSVIEW_COMM_APP_HELLO_SIZE 32
#define SEGGER_SYSVIEW_MAJOR 3
#define SEGGER_SYSVIEW_MINOR 10
#define SEGGER_SYSVIEW_REV 0

static const char _abHelloMsg[SYSVIEW_COMM_TARGET_HELLO_SIZE] = {'S', 'E', 'G', 'G', 'E', 'R', ' ', 'S', 'y', 's', 't', 'e', 'm', 'V', 'i', 'e', 'w', ' ',
                                                                 'V', '0' + SEGGER_SYSVIEW_MAJOR, '.', '0' + (SEGGER_SYSVIEW_MINOR / 10), '0' + (SEGGER_SYSVIEW_MINOR % 10), '.',
                                                                 '0' + (SEGGER_SYSVIEW_REV / 10), '0' + (SEGGER_SYSVIEW_REV % 10), '\0', 0, 0, 0, 0, 0};

/**
 * @brief Construct a new Sys View:: Sys View object
 * 
 * @param port 
 */
SysView::SysView(int port)
{
    this->_connected = false;
    this->_port = port;
    this->_th = std::thread(&SysView::run_server, this);
}

/**
 * @brief 
 * 
 * @param buffer 
 * @return true 
 * @return false 
 */
bool SysView::saveFromSTM(const std::vector<uint8_t> *buffer)
{
    if (this->_connected)
    {
        return this->_q_from_uc.enqueue_bulk(buffer->data(), buffer->size());
    }
    return false;
}

/**
 * @brief 
 * 
 * @return size_t 
 */
size_t SysView::dataToSTM()
{
    return this->_q_to_uc.size_approx();
}

/**
 * @brief 
 * 
 * @return std::vector<unsigned char> 
 */
std::vector<unsigned char> SysView::getDataToSTM()
{
    size_t n = this->_q_to_uc.size_approx();
    std::vector<unsigned char> data;
    data.resize(n);
    this->_q_to_uc.try_dequeue_bulk(data.data(), data.size());
    return data;
}

/**
 * @brief 
 * 
 */
void SysView::run_server()
{
    sockpp::socket_initializer sockInit;
    sockpp::tcp_acceptor acc(this->_port);

    if (!acc)
    {
        LOG_ERROR("Error creating the acceptor: %s", acc.last_error_str().c_str());
        exit(EXIT_FAILURE);
    }

    LOG_USER("Acceptor bound to address: %s", acc.address().to_string().c_str());
    LOG_USER("Awaiting connections on port %d ...", this->_port);

    while (!stopApp)
    {
        sockpp::inet_address peer;

        // Accept a new sysview connection
        sockpp::tcp_socket sock = acc.accept(&peer);
        LOG_USER("Received a connection request from %s", peer.to_string().c_str());

        if (!sock)
        {
            LOG_ERROR("Error accepting incoming connection: %s", acc.last_error_str().c_str());
        }
        else
        {
            this->run_socket(std::move(sock));
        }
    }
}

/**
 * @brief 
 * 
 * @param sock 
 */
void SysView::run_socket(sockpp::tcp_socket sock)
{
    sock.set_non_blocking();

    ssize_t n;
    char buf[1024 * 2];

    // first we should get HELLO message
    n = sock.read(buf, sizeof(buf));

    if (n != SYSVIEW_COMM_APP_HELLO_SIZE)
    {
        LOG_ERROR("Received HELLO message with incorrect size: %d (should be %d)", (int)n, SYSVIEW_COMM_APP_HELLO_SIZE);
        return;
    }

    LOG_USER("Received HELLO message");

    // answer to HELLO message
    n = sock.write_n(_abHelloMsg, sizeof(_abHelloMsg));
    if (n != SYSVIEW_COMM_TARGET_HELLO_SIZE)
    {
        LOG_ERROR("Failed to send Hello message to SystemView App");
        return;
    }

    LOG_USER("Answered HELLO message");

    // purge all data from uc
    n = this->_q_from_uc.size_approx();
    std::vector<unsigned char> data;
    data.resize(n);
    this->_q_from_uc.try_dequeue_bulk(data.data(), data.size());

    // no looks like we are connected
    this->_connected = true;

    int errnum;
    unsigned char deq_buff[DEQ_SIZE];

    while (!stopApp)
    {
        // write to socket if we get data from uc
        n = this->_q_from_uc.wait_dequeue_bulk_timed(deq_buff, sizeof(deq_buff), 10ms);
        if (n)
        {
            ssize_t nn = sock.write_n(deq_buff, n);
            LOG_USER("Socket send %d from %d", (int)nn, (int)n);
        }

        // read socket
        n = sock.read(buf, sizeof(buf));

        // returned error
        if (n == -1)
        {
            errnum = errno;

            // if no data received
            if (errnum == EWOULDBLOCK)
            {
                continue;
            }

            // if error return
            LOG_ERROR("Socket received error: %s", std::strerror(errnum));
            return;
        }
        else if (n == 0)
        {
            goto closed;
        }
        else
        {
            // received data
            if (n > 1)
            {
                LOG_USER("Socket received: %d, 0x%02x 0x%02x", (int)n, buf[0], buf[1]);
            }
            else
            {
                LOG_USER("Socket received: %d, 0x%02x", (int)n, buf[0]);
            }

            // write to que and later to uc
            if (!this->_q_to_uc.enqueue_bulk(buf, n))
            {
                LOG_ERROR("Can't enqueue data from SystemView");
            }
        }
    }

closed:
    this->_connected = false;
    LOG_USER("Connection closed from %s", sock.peer_address().to_string().c_str());
}