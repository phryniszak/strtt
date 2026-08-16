// Mock implementation of the stlink_usb_layout_api global used by strtt.cpp.
// Lets the StRtt class be exercised without a real ST-LINK probe attached:
// read_mem()/write_mem() are backed by an in-memory image of the target RAM
// instead of a USB transfer.
#include <cstring>

#include "mock_stlink.h"
#include "stlink.h"
#include "stlink_errors.h"

std::vector<uint8_t> g_fakeMemory;
uint32_t g_fakeMemoryBase = 0;

static int mock_open(struct hl_interface_param_s *, void **handle)
{
    static int dummy_handle;
    *handle = &dummy_handle;
    return ERROR_OK;
}

static int mock_close(void *)
{
    return ERROR_OK;
}

// Mirrors the real probe: copies `count` bytes from the simulated target RAM
// into the caller-supplied buffer. If `buffer` points outside of valid
// process memory (as happens in issue #6 due to a pointer underflow in
// StRtt::readRtt()), this write segfaults exactly like the real read_mem
// would when writing received USB data into the same bad pointer.
static int mock_read_mem(void *, uint32_t addr, uint32_t /*size*/, uint32_t count, uint8_t *buffer)
{
    uint32_t offset = addr - g_fakeMemoryBase;
    if (offset < g_fakeMemory.size() && offset + count <= g_fakeMemory.size())
        memcpy(buffer, g_fakeMemory.data() + offset, count);
    else
        memset(buffer, 0, count);

    return ERROR_OK;
}

static int mock_idcode(void *, uint32_t *idcode)
{
    *idcode = 0;
    return ERROR_OK;
}

struct hl_layout_api_s stlink_usb_layout_api = {};

namespace
{
struct MockRegistration
{
    MockRegistration()
    {
        stlink_usb_layout_api.open = mock_open;
        stlink_usb_layout_api.close = mock_close;
        stlink_usb_layout_api.read_mem = mock_read_mem;
        stlink_usb_layout_api.idcode = mock_idcode;
    }
} registration;
} // namespace
