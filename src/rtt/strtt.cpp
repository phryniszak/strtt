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

// cpp
#include <chrono>
#include <algorithm>
#include <list>

// c
#include <string.h>

// local
#include "strtt.h"
#include "log.h"

#define MAX_STR_LENGTH 64

#define START_TS auto __start_ts = std::chrono::high_resolution_clock::now()
#define STOP_TS this->_duration = std::chrono::duration<double, std::micro>(std::chrono::high_resolution_clock::now() - __start_ts).count()

/**
 * @brief Construct a new St Rtt:: St Rtt object
 *
 */
StRtt::StRtt(uint32_t start)
    :    ramStart(start)
{
    this->init();
}

/**
 * @brief Destroy the St Rtt:: St Rtt object
 *
 */
StRtt::~StRtt()
{
    stlink_usb_layout_api.close(this->_handle);
}

/**
 * @brief
 *
 */
void StRtt::init()
{
    log_init();

    this->_param.device_desc = "ST-LINK";
    this->_param.transport = hl_transports::HL_TRANSPORT_SWD;

    std::vector<uint16_t> pids{STLINK_V2_PID, STLINK_V2_1_PID, STLINK_V2_1_NO_MSD_PID,
                               STLINK_V3_USBLOADER_PID, STLINK_V3E_PID, STLINK_V3S_PID, STLINK_V3_2VCP_PID, STLINK_V3E_NO_MSD_PID};

    for (std::size_t i = 0; i < HLA_MAX_USB_IDS; ++i)
    {
        if (i < pids.size())
        {
            this->_param.vid[i] = STLINK_VID;
            this->_param.pid[i] = pids[i];
        }
        else
        {
            this->_param.vid[i] = 0;
            this->_param.pid[i] = 0;
        }
    }

    this->_param.use_stlink_tcp = false;

    this->_param.initial_interface_speed = STLINK_SPEED;

    this->_param.connect_under_reset = false;
}

/**
 * @brief
 *
 * @param use_tcp
 * @param port_tcp
 * @return int
 */
int StRtt::open(bool use_tcp = false, uint16_t port_tcp)
{
    this->_param.use_stlink_tcp = use_tcp;
    this->_param.stlink_tcp_port = port_tcp;
    return stlink_usb_layout_api.open(&this->_param, &this->_handle);
}

/**
 * @brief
 *
 * @return int
 */
int StRtt::close()
{
    return stlink_usb_layout_api.close(this->_handle);
}

/**
 * @brief
 *
 * @param pIdCode
 * @return int
 */
int StRtt::getIdCode(uint32_t *pIdCode)
{
    START_TS;

    int ret;

    ret = stlink_usb_layout_api.idcode(this->_handle, pIdCode);

    STOP_TS;

    return ret;
}

/**
 * @brief
 *
 * @param ramKbytes
 * @param ramStart
 * @return int
 */
int StRtt::findRtt(uint32_t ramKbytes)
{
    START_TS;

    // read the whole RAM ---------------------------------------------------------------
    this->_memory.resize(ramKbytes * 1024);
    int ret = stlink_usb_layout_api.read_mem(this->_handle, ramStart, -1, ramKbytes * 0x400, this->_memory.data());
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
            LOG_DEBUG("RTT addr = 0x%x", ramStart + offset);

            // sizeof(acID[16] + MaxNumUpBuffers +  MaxNumDownBuffers)
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

/**
 * @brief
 *
 * @return int
 */
int StRtt::getRttDesc()
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
            stlink_usb_layout_api.read_mem(this->_handle, this->_rtt_info.pRttDescription->buffDesc[i].sName, -1, MAX_STR_LENGTH, (uint8_t *)strChannelName);
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

/**
 * @brief
 *
 * @param index
 * @param sizeRead
 * @param sizeWrite
 * @return int
 */
int StRtt::getRttBuffSize(uint32_t index, uint32_t *sizeRead, uint32_t *sizeWrite)
{
    // TODO: sanity checks (pointers)
    *sizeRead = this->_rtt_info.pRttDescription->buffDesc[index].SizeOfBuffer;
    *sizeWrite = this->_rtt_info.pRttDescription->buffDesc[index + this->_rtt_info.pRttDescription->MaxNumUpBuffers].SizeOfBuffer;
    return ERROR_OK;
}

/**
 * @brief
 *
 * @return int
 */
int StRtt::readRtt()
{
    START_TS;

    // 1. read rtt desc
    uint32_t start = this->_rtt_info.offset;
    unsigned int buffersCnt = this->_rtt_info.pRttDescription->MaxNumUpBuffers + this->_rtt_info.pRttDescription->MaxNumDownBuffers;
    uint32_t size = sizeof(SEGGER_RTT_CB) + sizeof(SEGGER_RTT_BUFFER) * buffersCnt;
    int ret = stlink_usb_layout_api.read_mem(this->_handle, start + ramStart, -1, size, &this->_memory[start]);
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
            start = bufferDesc.pBuffer - ramStart;
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

    // heppens during target debuging, after stopping/starting
    if (size > SANE_SIZE_MAX)
    {
        LOG_ERROR("Read rtt memory size is insane: %d", size);
        STOP_TS;
        return ERROR_OK;
    }

    // 5. Read memory
    // ret = stlink_usb_layout_api.read_mem(this->_handle, start + RAM_START, -1, size, &this->_memory[start]);
    ret = stlink_usb_layout_api.read_mem(this->_handle, start + ramStart, -1, ((size / 4) * 4) + 4, &this->_memory[start]);
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
int StRtt::readRttFromBuff(int index, std::vector<uint8_t> *buffer)
{
    SEGGER_RTT_BUFFER *pRing = &this->_rtt_info.pRttDescription->buffDesc[index];
    unsigned int WrOff = pRing->WrOff; // Position of next item to be written by either target.
    unsigned int RdOff = pRing->RdOff; // Position of next item to be read by host. Must be volatile since it may be modified by host.

    size_t memoryIndex = pRing->pBuffer - ramStart;

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
        uint32_t addrRdOff = (uint8_t *)&pRing->RdOff - this->_memory.data() + ramStart;

        // we read up to read value of data - we can use it (WrOff)
        // other way we should do some maths with wrap-around logic
        int ret = stlink_usb_layout_api.write_mem(this->_handle, addrRdOff, -1, 4, (uint8_t *)&WrOff);
        if (ret < 0)
        {
            return ret;
        }
    }

    return buffer->size();
}

/**
 * @brief
 *
 * @param callback
 */
void StRtt::addChannelHandler(CallbackFunction callback)
{
    this->_callback = callback;
}

/**
 * @brief
 *
 * @param buffIndex
 * @param buffer
 * @return int
 *
 * as buffer in uc can be smaller than what we want to write in one go, we
 * write up to uc buffer size
 */
int StRtt::writeRtt(int buffIndex, std::vector<uint8_t> *buffer)
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

        int ret = stlink_usb_layout_api.read_mem(this->_handle, pRing->pBuffer, -1, pRing->SizeOfBuffer, this->_wrMemory.data());
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
    int ret = stlink_usb_layout_api.write_mem(this->_handle, pRing->pBuffer, -1, pRing->SizeOfBuffer, this->_wrMemory.data());
    if (ret != ERROR_OK)
    {
        STOP_TS;
        return ret;
    }

    // update WrOff
    uint32_t addrWrOff = (uint8_t *)&pRing->WrOff - this->_memory.data() + ramStart;
    ret = stlink_usb_layout_api.write_mem(this->_handle, addrWrOff, -1, 4, (uint8_t *)&WrOff);
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
unsigned StRtt::_GetAvailWriteSpace(SEGGER_RTT_BUFFER *pRing)
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