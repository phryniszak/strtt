#include <atomic>
#include <vector>
#include <chrono>
#include <signal.h>

#include "stlink.h"
#include "log.h"
#include "InputParser.h"

#ifdef __linux__
#include "kbhit.h"
#elif _WIN32
#include <conio.h>
#else

#endif

std::atomic_bool stopApp;
std::vector<uint8_t> str;
double _duration;
bool _showCycleTime;
int ramKB = 16;

#define START_TS auto __start_ts = std::chrono::high_resolution_clock::now()
#define STOP_TS _duration = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - __start_ts).count()

void signalHandler(int signum)
{
    LOG_INFO("Interrupt signal %d received", signum);

    stopApp = true;
}

int main(int argc, char **argv)
{

    InputParser input(argc, argv);

    signal(SIGINT, signalHandler);

    log_init();
    debug_level = LOG_LVL_WARNING;
    if (input.cmdOptionExists("-v"))
    {
        debug_level = std::stoi(input.getCmdOption("-v"));
    }

    if (input.cmdOptionExists("-ram"))
    {
        ramKB = std::stoi(input.getCmdOption("-ram"));
    }

    StLink *stlink = new StLink();

    int res = stlink->open();
    if (res != ERROR_OK)
    {
        LOG_ERROR("failed to open STLINK (%d)", res);
        exit(-1);
    }

    uint32_t idCode;
    res = stlink->getIdCode(&idCode);

    res = stlink->findRtt(ramKB);
    if (res != ERROR_OK)
    {
        LOG_ERROR("failed to find RTT (%d)", res);
        exit(-1);
    }

    stlink->getRttDesc();

    uint32_t sizeR, sizeW;
    res = stlink->getRttBuffSize(0, &sizeR, &sizeW);

    stlink->addChannelHandler([](const int index, const std::vector<uint8_t> *buffer) {
        if (index == 0)
        {

            for (uint8_t ch : *buffer)
            {
                fputc(ch, stdout);
            }
            fflush(stdout);
        }
        else if (index == 1)
        {
        }
    });

    while (!stopApp)
    {
        START_TS;

        stlink->readRtt();

        while (_kbhit())
        {
            uint8_t ch = _getch();
            str.push_back(ch);
        }

        if (str.size() > 0)
        {
            stlink->writeRtt(0, &str);
        }

        if (_showCycleTime)
        {
            STOP_TS;
            LOG_INFO("Cycle time: %dms", (int)_duration);
        }
    }

    stlink->close();
    return 0;
}
