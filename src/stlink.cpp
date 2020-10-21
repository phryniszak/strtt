// cpp
#include <chrono>
#include <algorithm>
#include <list>

// c
#include <string.h>

// local
#include "stlink.h"
#include "log.h"

#define MAX_STR_LENGTH 64

#define START_TS auto __start_ts = std::chrono::high_resolution_clock::now()
#define STOP_TS this->_duration = std::chrono::duration<double, std::micro>(std::chrono::high_resolution_clock::now() - __start_ts).count()

StLink::StLink(/* args */)
{
    this->_api = &stlink_usb_layout_api;

    this->init();
}

StLink::~StLink()
{
    this->_api->close(this->_handle);
}

void StLink::init()
{
    this->_param.device_desc = "ST-LINK";
    this->_param.serial = NULL;
    this->_param.transport = HL_TRANSPORT_SWD;

    // hla_vid_pid 0x0483 0x3744
    this->_param.vid[0] = 0x0483;
    this->_param.pid[0] = 0x3744;
    // 0x0483 0x3748
    this->_param.vid[1] = 0x0483;
    this->_param.pid[1] = 0x3748;
    // 0x0483 0x374b
    this->_param.vid[2] = 0x0483;
    this->_param.pid[2] = 0x374b;
    // 0x0483 0x374d
    this->_param.vid[3] = 0x0483;
    this->_param.pid[3] = 0x374d;
    // 0x0483 0x374e
    this->_param.vid[4] = 0x0483;
    this->_param.pid[4] = 0x374e;
    // 0x0483 0x374f
    this->_param.vid[5] = 0x0483;
    this->_param.pid[5] = 0x374f;
    // 0x0483 0x3752
    this->_param.vid[6] = 0x0483;
    this->_param.pid[6] = 0x3752;
    // 0x0483 0x3753
    this->_param.vid[7] = 0x0483;
    this->_param.pid[7] = 0x3753;

    this->_param.initial_interface_speed = 24 * 1000;

    this->_param.connect_under_reset = false;
}

int StLink::open()
{
    return this->_api->open(&this->_param, &this->_handle);
}

int StLink::close()
{
    return this->_api->close(this->_handle);
}

int StLink::getIdCode(uint32_t *idCode)
{
    START_TS;

    int ret;

    ret = this->_api->read_mem(this->_handle, 0xE0042000, 4, 1, (uint8_t *)idCode);

    STOP_TS;

    return ret;
}

int StLink::findRtt(uint32_t ramKbytes, uint32_t ramStart)
{
    START_TS;

    // read the whole RAM ---------------------------------------------------------------
    this->_memory.resize(ramKbytes * 1024);
    int ret = this->_api->read_mem(this->_handle, RAM_START, 4, ramKbytes * 0x100, this->_memory.data());
    if (ret != ERROR_OK)
    {
        STOP_TS;
        return ret;
    }

    // find SEGGER_RTT_CB address -------------------------------------------------------

    const char *strSeggerRtt = "SEGGER RTT";

    for (uint32_t offset = 0; offset < (ramKbytes * 1024) - 16; offset++)
    {
        if (strncmp((char *)&this->_memory[offset], strSeggerRtt, 16) == 0)
        {
            LOG_DEBUG("RTT addr = 0x%x", RAM_START + offset);

            // sizezof(acID[16] + MaxNumUpBuffers +  MaxNumDownBuffers)
            this->_rtt_info.pRttDescription = (SEGGER_RTT_CB *)&this->_memory[offset];
            this->_rtt_info.offset = offset;
            break;
        }
    }

    // check results --------------------------------------------------------------------
    if (this->_rtt_info.offset == 0)
    {
        LOG_ERROR("RTT not found");
        STOP_TS;
        return -1;
    }

    if ((this->_rtt_info.pRttDescription->MaxNumUpBuffers == 0) || (this->_rtt_info.pRttDescription->MaxNumDownBuffers == 0))
    {
        LOG_ERROR("Max number of UP buffers: %d and DOWN buffers: %d",
                  this->_rtt_info.pRttDescription->MaxNumUpBuffers,
                  this->_rtt_info.pRttDescription->MaxNumDownBuffers);
        STOP_TS;
        return -1;
    }

    LOG_DEBUG("Max number of buffers UP: %d and DOWN: %d",
              this->_rtt_info.pRttDescription->MaxNumUpBuffers,
              this->_rtt_info.pRttDescription->MaxNumDownBuffers);

    STOP_TS;
    return ERROR_OK;
}

//
//
//
int StLink::getRttDesc()
{
    START_TS;

    // TODO: sanity checks (pointers)
    unsigned int size = this->_rtt_info.pRttDescription->MaxNumUpBuffers + this->_rtt_info.pRttDescription->MaxNumDownBuffers;
    this->_rtt_info_names.resize(size);

    for (size_t i = 0; i < size; i++)
    {
        char strChannelName[MAX_STR_LENGTH];
        if (this->_rtt_info.pRttDescription->buffDesc[i].sName)
        {
            // we have to read it from flash
            this->_api->read_mem(this->_handle, this->_rtt_info.pRttDescription->buffDesc[i].sName, 1, MAX_STR_LENGTH, (uint8_t *)strChannelName);
            _rtt_info_names[i] = std::string(strChannelName);
            LOG_INFO("%d. Channel name: %s\tsize: %d\tmode: %d", (int)i, strChannelName,
                     this->_rtt_info.pRttDescription->buffDesc[i].SizeOfBuffer,
                     this->_rtt_info.pRttDescription->buffDesc[i].Flags);
        }
        else
        {
            LOG_INFO("%d. Channel name: -------\tsize: %d\tmode: %d", (int)i,
                     this->_rtt_info.pRttDescription->buffDesc[i].SizeOfBuffer,
                     this->_rtt_info.pRttDescription->buffDesc[i].Flags);
        }
    }

    STOP_TS;
    return ERROR_OK;
}

//
//
//
int StLink::getRttBuffSize(uint32_t index, uint32_t *sizeRead, uint32_t *sizeWrite)
{
    // TODO: sanity checks (pointers)
    *sizeRead = this->_rtt_info.pRttDescription->buffDesc[index].SizeOfBuffer;
    *sizeWrite = this->_rtt_info.pRttDescription->buffDesc[index + this->_rtt_info.pRttDescription->MaxNumUpBuffers].SizeOfBuffer;
    return ERROR_OK;
}

int StLink::readRtt()
{
    START_TS;

    // 1. read rtt desc
    uint32_t start = this->_rtt_info.offset;
    unsigned int buffersCnt = this->_rtt_info.pRttDescription->MaxNumUpBuffers + this->_rtt_info.pRttDescription->MaxNumDownBuffers;
    uint32_t size = sizeof(SEGGER_RTT_CB) + sizeof(SEGGER_RTT_BUFFER) * buffersCnt;
    int ret = this->_api->read_mem(this->_handle, start + RAM_START, 1, size, &this->_memory[start]);
    if (ret < 0)
    {
        STOP_TS;
        return ret;
    }

    // establish memory range which we have to read
    // start address and size
    std::list<std::pair<uint32_t, uint32_t>> blocks;

    // 2. Enumerate buffers start and size
    for (size_t i = 0; i < buffersCnt; i++)
    {
        // check only valid channels, eg. with size > 0 AND and something to read
        SEGGER_RTT_BUFFER bufferDesc = this->_rtt_info.pRttDescription->buffDesc[i];
        if ((bufferDesc.SizeOfBuffer) && (bufferDesc.RdOff != bufferDesc.WrOff))
        {
            start = bufferDesc.pBuffer - RAM_START;
            size = bufferDesc.SizeOfBuffer;
            blocks.push_back(std::make_pair(start, size));
        }
    }

    // 3. If nothing to be read return
    if (blocks.empty())
    {
        STOP_TS;
        return ERROR_OK;
    }

    // 4. Find Min/Max range that we need to read
    blocks.sort();
    start = blocks.front().first;
    size = blocks.back().first + blocks.back().second - start;

    // 5. Read memory
    ret = this->_api->read_mem(this->_handle, start + RAM_START, 1, size, &this->_memory[start]);
    if (ret < 0)
    {
        STOP_TS;
        return ret;
    }

    // 6. Read RTT channels
    buffersCnt = this->_rtt_info.pRttDescription->MaxNumUpBuffers;
    for (size_t i = 0; i < buffersCnt; i++)
    {
        // check only valid channels, eg. with size > 0 AND and something to read
        SEGGER_RTT_BUFFER bufferDesc = this->_rtt_info.pRttDescription->buffDesc[i];
        if ((bufferDesc.SizeOfBuffer) && (bufferDesc.RdOff != bufferDesc.WrOff))
        {
            std::vector<uint8_t> buffer;
            buffer.reserve(bufferDesc.SizeOfBuffer);
            int amount = this->readRttFromBuff(i, &buffer);
            if (amount)
            {
                LOG_DEBUG("Chanel: %d readed: %d", (int)i, amount);
                this->_callback((int)i, &buffer);
            }
        }
    }

    STOP_TS;
    return ERROR_OK;
}

/*********************************************************************
*    Reads characters from SEGGER real-time-terminal control block
*    which have been previously stored by the uc.
*
*  Parameters
*    BufferIndex  Index of Down-buffer to be used (e.g. 0 for "Terminal").
*    buffer       std::vector<uint8_t> with size at least of size of chanel
*
*  Return value
*    Number of bytes that have been read.
*/
int StLink::readRttFromBuff(int index, std::vector<uint8_t> *buffer)
{
    SEGGER_RTT_BUFFER *pRing = &this->_rtt_info.pRttDescription->buffDesc[index];
    unsigned int WrOff = pRing->WrOff; // Position of next item to be written by either target.
    unsigned int RdOff = pRing->RdOff; // Position of next item to be read by host. Must be volatile since it may be modified by host.

    size_t memoryIndex = pRing->pBuffer - RAM_START;

    // we start reading from position in memory RdOff until we reach WrOff
    while (RdOff != WrOff)
    {
        buffer->push_back(this->_memory[memoryIndex + RdOff]);

        RdOff++;

        // Handle wrap-around of buffer
        if (RdOff >= pRing->SizeOfBuffer)
        {
            RdOff = 0;
        }
    }

    if (buffer->size())
    {
        // now save information about amount we read
        uint32_t addrRdOff = (uint8_t *)&pRing->RdOff - this->_memory.data() + RAM_START;

        // we read up to readed value of data - we can use it (WrOff)
        // other way we should do some maths with wrap-around logic
        int ret = this->_api->write_mem(this->_handle, addrRdOff, 4, 1, (uint8_t *)&WrOff);
        if (ret < 0)
        {
            return ret;
        }
    }

    return buffer->size();
}

// https://sudomakeinstall.io/posts/2017/11/30/callbacks-in-cpp11/
// https://stackoverflow.com/questions/14189440/c-callback-using-class-member
// https://en.cppreference.com/w/cpp/utility/functional/function
// https://en.cppreference.com/w/cpp/utility/functional/function/deduction_guides
void StLink::addChannelHandler(CallbackFunction callback)
{
    this->_callback = callback;
}

//
// as buffer in uc can be smaller than what we want to write, we
// in one go write up to uc buffer size
int StLink::writeRtt(int buffIndex, std::vector<uint8_t> *buffer)
{
    START_TS;

    SEGGER_RTT_BUFFER *pRing = &this->_rtt_info.pRttDescription->buffDesc[buffIndex + this->_rtt_info.pRttDescription->MaxNumUpBuffers];
    unsigned int WrOff = pRing->WrOff; // Position of next item to be written by host. Must be volatile since it may be modified by host.
    // unsigned int RdOff = pRing->RdOff; // Position of next item to be read by target (down-buffer).

    // how much we can write in non blocking mode
    unsigned available = this->_GetAvailWriteSpace(pRing);
    unsigned numWritten = std::min(available, (unsigned)buffer->size());

    // if nothing can be written return 0
    if (numWritten == 0)
    {
        return 0;
    }

    // read memory from uc to shadow memory
    // we read it once, because its read only memory and it is not modified by uc
    if (!_wrMemory.size())
    {
        this->_wrMemory.resize(pRing->SizeOfBuffer);

        int ret = this->_api->read_mem(this->_handle, pRing->pBuffer, 1, pRing->SizeOfBuffer, this->_wrMemory.data());
        if (ret != ERROR_OK)
        {
            STOP_TS;
            return ret;
        }
    }

    // write buffer to shadow memory, which next we write to uc
    for (size_t i = 0; i < numWritten; i++)
    {
        this->_wrMemory[WrOff++] = buffer->at(0);

        // pop front
        buffer->erase(buffer->begin());

        // Handle wrap-around of buffer
        if (WrOff >= pRing->SizeOfBuffer)
        {
            WrOff = 0;
        }
    }

    // write shadow memory to uc
    int ret = this->_api->write_mem(this->_handle, pRing->pBuffer, 1, pRing->SizeOfBuffer, this->_wrMemory.data());
    if (ret != ERROR_OK)
    {
        STOP_TS;
        return ret;
    }

    // update WrOff
    uint32_t addrWrOff = (uint8_t *)&pRing->WrOff - this->_memory.data() + RAM_START;
    ret = this->_api->write_mem(this->_handle, addrWrOff, 4, 1, (uint8_t *)&WrOff);
    if (ret < 0)
    {
        STOP_TS;
        return ret;
    }

    STOP_TS;
    return numWritten;
}

/*********************************************************************
*
*       _GetAvailWriteSpace()
*
*  Function description
*    Returns the number of bytes that can be written to the ring
*    buffer without blocking.
*
*  Parameters
*    pRing        Ring buffer to check.
*
*  Return value
*    Number of bytes that are free in the buffer.
*/
unsigned StLink::_GetAvailWriteSpace(SEGGER_RTT_BUFFER *pRing)
{
    unsigned RdOff;
    unsigned WrOff;
    unsigned r;
    //
    // Avoid warnings regarding volatile access order.  It's not a problem
    // in this case, but dampen compiler enthusiasm.
    //
    RdOff = pRing->RdOff;
    WrOff = pRing->WrOff;
    if (RdOff <= WrOff)
    {
        r = pRing->SizeOfBuffer - 1u - WrOff + RdOff;
    }
    else
    {
        r = RdOff - WrOff - 1u;
    }
    return r;
}