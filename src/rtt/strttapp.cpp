#include <atomic>
#include <vector>
#include <chrono>
#include <memory>

#include <signal.h>

#include "strtt.h"
#include "log.h"
#include "inputparser.h"
#include "consoleinput.h"
#include "adapter.h"

// #define SYSVIEW

#ifdef SYSVIEW
#include "sysview.h"
#endif

#ifdef __linux__
#include <sys/resource.h>
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

static void showArgs(const std::string& progName)
{
    std::cout << "usage: " << progName << " [OPTIONS]" << std::endl;
    std::cout << "  -v number\t ... verbosity (debug level) 0..4" << std::endl;
    std::cout << "  -ramsize size\t ... size of RAM, e.g. 0x2000" << std::endl;
    std::cout << "  -ramstart address ... start address of RAM, e.g. 0x08000000" << std::endl;
    std::cout << "  -port number\t ... port number for TCP connection" << std::endl;
    std::cout << "  -t\t\t ... show cycle time " << std::endl;
    std::cout << "  -tcp\t\t ... use TCP connection " << std::endl;
    std::cout << "  -ap number\t ... accessport number" << std::endl;
    std::cout << "  -serial string\t ... ST-LINK serial number to connect to" << std::endl;
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

    signal(SIGINT, signalHandler);
    log_init();

    for( uint32_t i = 1; i < argc; ++i ) {
        if( strcmp(argv[i], "--help") == 0 ) {
            showArgs(argv[0]);
            return EXIT_SUCCESS;
        }
    }

    debug_level            = LOG_LVL_SILENT;
    int         _ramKB        = 16;
    int         port          = SYSVIEW_COMM_SERVER_PORT;
    uint32_t    _ramStart     = RAM_START;
    uint8_t     apNum         = 0;
    bool        useTCP        = false;
    bool        showCycleTime = false;
    std::string serial;

    auto handleOptions = [&argc, argv, &_ramKB, &port, &_ramStart, &apNum, &useTCP, &showCycleTime, &serial]() {
        InputParser input(argc, argv);

        if( input.cmdOptionExists("-v") ) {
            debug_level = std::stoi(input.getCmdOption("-v"));
        }

        if( input.cmdOptionExists("-ramsize") ) {
            std::string opt = input.getCmdOption("-ramsize");
            if( opt.size() > 1 && opt[0] == '0' && (opt[1] == 'x' || opt[1] == 'X') ) {
                _ramKB = std::stoi(opt, nullptr, 16);
            }
            else {
                _ramKB = std::stoi(opt, nullptr, 0);
            }
        }

        if( input.cmdOptionExists("-port") ) {
            port = std::stoi(input.getCmdOption("-port"));
        }

        if( input.cmdOptionExists("-ramstart") ) {
            // get ram start from options, value maybe hex or dec
            std::string opt = input.getCmdOption("-ramstart");
            if( opt.size() > 1 && opt[0] == '0' && (opt[1] == 'x' || opt[1] == 'X') ) {
                _ramStart = std::stoi(opt, nullptr, 16);
            }
            else {
                _ramStart = std::stoi(opt, nullptr, 0);
            }
        }

        if( input.cmdOptionExists("-t") ) {
            showCycleTime = true;
        }

        if( input.cmdOptionExists("-tcp") ) {
            useTCP = true;
        }

        if( input.cmdOptionExists("-ap") ) {
            apNum = std::stoi(input.getCmdOption("-ap"));
        }

        if( input.cmdOptionExists("-serial") ) {
            serial = input.getCmdOption("-serial");
        }
    };

    try {
        handleOptions();
    } catch( const std::exception& e ) {
        std::cerr << "ERROR parsing command-line args:" << e.what() << std::endl;
        showArgs(argv[0]);
        return EXIT_FAILURE;
    }

    if (!serial.empty())
        adapter_set_required_serial(serial.c_str());

    auto strtt = std::make_unique<StRtt>(_ramStart, apNum);

    // open stLink
    int res = strtt->open(useTCP);
    if (res != ERROR_OK)
    {
        LOG_ERROR("failed to open STLINK (%d)", res);
        exit(-1);
    }

    // get idCode
    // uint32_t idCode;
    // res = strtt->getIdCode(&idCode);

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

    strtt->addChannelHandler([&](const int index, const std::vector<uint8_t> *buffer)
                             {
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
                                     LOG_OUTPUT("SysView size: %d ", (int)buffer->size());
                                     _sv->saveFromSTM(buffer);
                                 }
#endif
                             });

    ConsoleInput console;
    std::vector<uint8_t> str;
    double _duration;
    while (!stopApp)
    {
        START_TS;

        // read rtt
        res = strtt->readRtt();

        if (res != ERROR_OK)
        {
            LOG_ERROR("readRtt returned error %d, program is exiting", res);
            stopApp = true;
        }

        // read console
        while (console.isChar())
        {
            uint8_t ch = console.getChar();
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

    return 0;
}
