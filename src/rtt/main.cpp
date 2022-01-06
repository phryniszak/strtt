#include <vector>
#include <fstream>
#include <experimental/random>

#include "stlink.h"
#include "log.h"
#include "stlink_errors.h"
#include "helper_time_support.h"

#define RAM_START (0x20000000)
#define RAM_START_SAFE (RAM_START + 0x2000)
#define RAM_END (0x20008000)
#define RAM_END_SAFE (RAM_END - 0x100)

#define FLASH_START (0x8000000)

#define SPEED_TEST_RAM_AREA_SIZE (0x1000)
#define SPEED_TEST_READ_CNT (10)

std::vector<uint8_t> readFile(const char *filename)
{
    // open the file:
    std::streampos fileSize;
    std::ifstream file(filename, std::ios::binary);

    // get its size:
    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // read the data:
    std::vector<uint8_t> fileData(fileSize);
    file.read((char *)&fileData[0], fileSize);
    return fileData;
}

int main(int argc, char **argv)
{
    log_init();
    debug_level = log_levels::LOG_LVL_DEBUG_IO;

    LOG_INFO("App started");

    struct hl_interface_param_s param;
    param.transport = hl_transports::HL_TRANSPORT_SWD;

    std::vector<uint16_t> pids{STLINK_V2_PID, STLINK_V2_1_PID, STLINK_V2_1_NO_MSD_PID,
                               STLINK_V3_USBLOADER_PID, STLINK_V3E_PID, STLINK_V3S_PID,
                               STLINK_V3_2VCP_PID, STLINK_V3E_NO_MSD_PID};

    for (std::size_t i = 0; i < HLA_MAX_USB_IDS; ++i)
    {
        if (i < pids.size())
        {
            param.vid[i] = STLINK_VID;
            param.pid[i] = pids[i];
        }
        else
        {
            param.vid[i] = 0;
            param.pid[i] = 0;
        }
    }

    param.use_stlink_tcp = true;
    param.stlink_tcp_port = 7184;
    param.initial_interface_speed = 24000;
    param.connect_under_reset = false;

    void *fd;

    int res = stlink_usb_layout_api.open(&param, &fd);

    if (res != ERROR_OK)
    {
        LOG_ERROR("Can not open stlink error: %d", res);
        exit(-1);
    }

    // sleep(0.1);

    //
    std::vector<uint8_t> buffer_2(16);
    uint8_t buffer_1[16];
    res = stlink_usb_layout_api.read_mem(fd, FLASH_START, 1, 4, buffer_1);
    res = stlink_usb_layout_api.read_mem(fd, FLASH_START, 2, 2, buffer_1);
    res = stlink_usb_layout_api.read_mem(fd, FLASH_START, 4, 1, buffer_1);
    res = stlink_usb_layout_api.read_mem(fd, FLASH_START, 1, 4, buffer_1);
    res = stlink_usb_layout_api.read_mem(fd, FLASH_START, 1, 4, buffer_1);

    res = stlink_usb_layout_api.read_mem(fd, FLASH_START, 1, 4, buffer_2.data());
    res = stlink_usb_layout_api.read_mem(fd, FLASH_START, 1, 4, buffer_1);
    res = stlink_usb_layout_api.read_mem(fd, FLASH_START, 1, 4, buffer_2.data());

    // uint8_t buffer[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    // res = stlink_usb_layout_api.write_mem(fd, RAM_START_SAFE + 1, -1, 9, buffer);


    int i;

    // RANDOM FLASH READ TEST //
    if (argc == 2)
    {
        LOG_USER("*** RANDOM FLASH READ TEST ***");

        std::vector<uint8_t> binFile = readFile(argv[1]);

        for (i = 0; i < SPEED_TEST_READ_CNT * 20; i++)
        {
            uint32_t size = std::experimental::randint(3, (int)binFile.size());
            uint32_t offset = std::experimental::randint(0, (int)(binFile.size() - size));
            std::vector<uint8_t> buffer(size);

            res = stlink_usb_layout_api.read_mem(fd, FLASH_START + offset, -1, size, buffer.data());

            // compare
            bool cmp_result = std::equal(buffer.begin(), buffer.end(), binFile.begin() + offset);
            LOG_USER("compare result: %s (Read %d bytes at offset %d with res %d)", cmp_result ? "PASS" : "FAIL", size, offset, res);
        }
    }

    LOG_USER("*** RANDOM RAM READ/WRITE TEST ***");

    for (i = 0; i < SPEED_TEST_READ_CNT * 20; i++)
    {
        uint32_t size = std::experimental::randint(3, RAM_END_SAFE - RAM_START_SAFE);
        uint32_t offset = std::experimental::randint(0, (int)(RAM_END_SAFE - RAM_START_SAFE - size));

        // size = 12262;
        // offset = 9633;

        std::vector<uint8_t> buffer1(size, 0);
        std::vector<uint8_t> buffer2(size, 0);

        // feel with random values
        for (uint8_t &element : buffer1)
        {
            element = (uint8_t)std::rand();
        }

        // write and read back
        int res1 = stlink_usb_layout_api.write_mem(fd, RAM_START_SAFE + offset, -1, size, buffer1.data());
        int res2 = stlink_usb_layout_api.read_mem(fd, RAM_START_SAFE + offset, -1, size, buffer2.data());

        // compare
        bool cmp_result = std::equal(buffer1.begin(), buffer1.end(), buffer2.begin());
        LOG_USER("compare result: %s (Write/Read %d bytes at offset %d with res %d %d)", cmp_result ? "PASS" : "FAIL", size, offset, res1, res2);
    }

    LOG_USER("*** SPEED TEST ***");

    uint8_t buffer[SPEED_TEST_RAM_AREA_SIZE];

    int64_t start_ts;
    int64_t ts;

    start_ts = timeval_ms();
    for (i = 0; i < SPEED_TEST_READ_CNT; i++)
    {
        stlink_usb_layout_api.read_mem(fd, RAM_START, 1, SPEED_TEST_RAM_AREA_SIZE, buffer);
        ts = timeval_ms() - start_ts;
        LOG_USER("Read cycles %d time: %ldms", i + 1, ts);
    }

    LOG_USER("Average time of %d ONE byte read %d bytes is: %dms", i, SPEED_TEST_RAM_AREA_SIZE, (int)ts / i);

    start_ts = timeval_ms();
    for (i = 0; i < SPEED_TEST_READ_CNT; i++)
    {
        stlink_usb_layout_api.read_mem(fd, RAM_START, -1, SPEED_TEST_RAM_AREA_SIZE, buffer);
        ts = timeval_ms() - start_ts;
        LOG_USER("Read cycles %d time: %ldms", i + 1, ts);
    }

    LOG_USER("Average time of %d AUTOMATIC size read of %d bytes is: %dms", i, SPEED_TEST_RAM_AREA_SIZE, (int)ts / i);

    // CLOSE //

    res = stlink_usb_layout_api.close(fd);
}