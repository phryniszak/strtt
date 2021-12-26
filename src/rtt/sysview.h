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
