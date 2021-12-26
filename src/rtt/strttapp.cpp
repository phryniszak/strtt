#include <atomic>
#include <vector>
#include <chrono>
#include <signal.h>

#include "strtt.h"
#include "log.h"
#include "inputparser.h"

#ifdef SYSVIEW
#include "sysview.h"
#endif

#ifdef __linux__

#include <sys/resource.h>
#include "kbhit.h"

#elif _WIN32
#include <conio.h>
#else

#endif

// CONST //////////////////////////////////////////////////

const int SYSVIEW_COMM_SERVER_PORT = 19111; // the port users will be connecting to

// GLOBAL VARIABLES ///////////////////////////////////////

std::atomic_bool stopApp;

// DEFINES ////////////////////////////////////////////////

#define START_TS auto __start_ts = std::chrono::high_resolution_clock::now()
#define STOP_TS _duration = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - __start_ts).count()

//
//
//
void signalHandler(int signum)
{
    LOG_INFO("Interrupt signal %d received", signum);

    // cleanup and close up stuff here
    // terminate program
    stopApp = true;
}

// INFO:
// https://stackoverflow.com/questions/12207684/how-do-i-terminate-a-thread-in-c11
// https://www.bo-yang.net/2017/11/19/cpp-kill-detached-thread

int main(int argc, char **argv)
{

#ifdef __linux__
    // Opportunistic call to renice us, so we can keep up under
    // higher load conditions. This may fail when run as non-root.
    setpriority(PRIO_PROCESS, 0, -11);
#endif

    InputParser input(argc, argv);

    signal(SIGINT, signalHandler);

    log_init();
    debug_level = LOG_LVL_SILENT;

    if (input.cmdOptionExists("-v"))
    {
        debug_level = std::stoi(input.getCmdOption("-v"));
    }

    int _ramKB = 16;
    if (input.cmdOptionExists("-ram"))
    {
        _ramKB = std::stoi(input.getCmdOption("-ram"));
    }

    int port = SYSVIEW_COMM_SERVER_PORT;
    if (input.cmdOptionExists("-port"))
    {
        port = std::stoi(input.getCmdOption("-port"));
    }

    bool showCycleTime = false;
    if (input.cmdOptionExists("-t"))
    {
        showCycleTime = true;
    }

    StRtt *strtt = new StRtt();

    // open stLink
    int res = strtt->open();
    if (res != ERROR_OK)
    {
        LOG_ERROR("failed to open STLINK (%d)", res);
        exit(-1);
    }

    // get idCode
    uint32_t idCode;
    res = strtt->getIdCode(&idCode);

    // find rtt
    res = strtt->findRtt(_ramKB);
    if (res != ERROR_OK)
    {
        LOG_ERROR("failed to find RTT (%d)", res);
        exit(-1);
    }

    // get channels description
    strtt->getRttDesc();

    // get buff size
    uint32_t sizeR, sizeW;
    res = strtt->getRttBuffSize(0, &sizeR, &sizeW);

#ifdef SYSVIEW
    SysView *_sv;
    _sv = new SysView(port);
#endif

    strtt->addChannelHandler([&](const int index, const std::vector<uint8_t> *buffer) {
        if (index == 0)
        {
            // TERMINAL, print to console
            for (uint8_t ch : *buffer)
            {
                fputc(ch, stdout);
            }
            fflush(stdout);
        }

#ifdef SYSVIEW
        else if (index == 1)
        {
            // LOG_OUTPUT("SysView size: %d ", (int)buffer->size());
            _sv->saveFromSTM(buffer);
        }
#endif
    });

    std::vector<uint8_t> str;
    double _duration;
    while (!stopApp)
    {
        START_TS;

        // read rtt
        strtt->readRtt();

        // read console
        while (_kbhit())
        {
            uint8_t ch = _getch();
            str.push_back(ch);
        }

        // write rtt
        if (str.size() > 0)
        {
            strtt->writeRtt(0, &str);
        }

#ifdef SYSVIEW
        // write SysView
        if (_sv->dataToSTM())
        {
            auto data = _sv->getDataToSTM();
            strtt->writeRtt(1, &data);
        }
#endif

        if (showCycleTime)
        {
            STOP_TS;
            LOG_USER("Cycle time: %dms", (int)_duration);
        }
    }

    strtt->close();
    return 0;
}
