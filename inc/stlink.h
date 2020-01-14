#ifndef _STLINK_H
#define _STLINK_H

#include <vector>
#include <string>
#include <functional>

#include "hl.h"

#define RAM_START 0x20000000

#define SEGGER_RTT_MODE_NO_BLOCK_SKIP (0)      // Skip. Do not block, output nothing. (Default)
#define SEGGER_RTT_MODE_NO_BLOCK_TRIM (1)      // Trim: Do not block, output as much as fits.
#define SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL (2) // Block: Wait until there is space in the buffer.

//
// Description for a circular buffer (also called "ring buffer")
// which is used as up-buffer (T->H)
//
typedef struct __attribute__((__packed__))
{
    uint32_t sName;        // Optional name. Standard names so far are: "Terminal", "SysView", "J-Scope_t4i4"
    uint32_t pBuffer;      // Pointer to start of buffer
    uint32_t SizeOfBuffer; // Buffer size in bytes. Note that one byte is lost, as this implementation does not fill up the buffer in order to avoid the problem of being unable to distinguish between full and empty.
    uint32_t WrOff;        // Position of next item to be written by either target.
    uint32_t RdOff;        // Position of next item to be read by host. Must be volatile since it may be modified by host.
    uint32_t Flags;        // Contains configuration flags
} SEGGER_RTT_BUFFER;

//
// RTT control block which describes the number of buffers available
// as well as the configuration for each buffer
//
//
typedef struct __attribute__((__packed__))
{
    char acID[16];                // Initialized to "SEGGER RTT"
    uint32_t MaxNumUpBuffers;     // Initialized to SEGGER_RTT_MAX_NUM_UP_BUFFERS (type. 2)
    uint32_t MaxNumDownBuffers;   // Initialized to SEGGER_RTT_MAX_NUM_DOWN_BUFFERS (type. 2)
    SEGGER_RTT_BUFFER buffDesc[]; // Up/Down buffers, transferring information up/down from target via debug probe to host
} SEGGER_RTT_CB;

typedef struct
{
    SEGGER_RTT_CB *pRttDescription;
    uint32_t offset;
} SEGGER_RTT_INFO;

typedef std::function<void(const int, const std::vector<uint8_t> *)> CallbackFunction;

class StLink
{
private:
    struct hl_layout_api_s *_api;
    struct hl_interface_param_s _param = {0};
    void *_handle = nullptr;
    std::vector<uint8_t> _memory;
    SEGGER_RTT_INFO _rtt_info = {0};
    std::vector<std::string> _rtt_info_names;
    double _duration;
    CallbackFunction _callback;
    std::vector<uint8_t> _wrMemory;
    void init();
    int readRttEx(uint32_t index);
    unsigned _GetAvailWriteSpace(SEGGER_RTT_BUFFER *pRing);

public:
    StLink(/* args */);
    ~StLink();

    int open();
    int close();

    int findRtt(uint32_t ramKbytes, uint32_t ramStart = RAM_START);
    int getRttDesc();
    int getRttBuffSize(uint32_t buffIndex, uint32_t *sizeRead, uint32_t *sizeWrite);

    int readRtt();
    int readRttFromBuff(int buffIndex, std::vector<uint8_t> *buffer);
    int writeRtt(int buffIndex, std::vector<uint8_t> *buffer);

    int getIdCode(uint32_t *idCode);

    void addChannelHandler(CallbackFunction callback);
};

#endif