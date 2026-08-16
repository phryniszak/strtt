// Regression test for https://github.com/phryniszak/strtt/issues/6
//
// Reported scenario: when the SEGGER RTT up-buffer (e.g. the linker-placed
// _acUpBuffer) lives at an address LOWER than the -ramstart value passed on
// the command line, StRtt::readRtt() computes
//
//     start = bufferDesc.pBuffer - ramStart;
//
// with both operands as uint32_t. Since pBuffer < ramStart, the subtraction
// underflows to a value close to UINT32_MAX. That huge "start" is then used
// to index into this->_memory (a std::vector<uint8_t> sized only to
// ramKbytes*1024), producing a wild pointer that is handed to read_mem() as
// the destination buffer -- which segfaults when it writes into it.
//
// This test recreates that exact memory layout against a mocked ST-LINK
// backend (no hardware required) and calls the same public StRtt sequence
// (open -> findRtt -> readRtt) that strttapp.cpp uses. Before the underlying
// bug is fixed, this test is expected to crash with SIGSEGV, proving the
// repro. After the fix, readRtt() should handle the bad buffer without
// crashing.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include "mock_stlink.h"
#include "strtt.h"
#include "stlink_errors.h"

namespace
{
constexpr uint32_t kRamStart = 0x20010000;
constexpr uint32_t kRamKBytes = 1;
constexpr uint32_t kRttCbOffset = 16; // keep nonzero, offset 0 is ambiguous with "not found" in findRtt()

void writeU32(std::vector<uint8_t> &mem, size_t offset, uint32_t value)
{
    memcpy(mem.data() + offset, &value, sizeof(value));
}
} // namespace

int main()
{
    g_fakeMemoryBase = kRamStart;
    g_fakeMemory.assign(kRamKBytes * 1024, 0);

    // SEGGER_RTT_CB header at kRttCbOffset: acID[16] + MaxNumUpBuffers + MaxNumDownBuffers
    memcpy(g_fakeMemory.data() + kRttCbOffset, "SEGGER RTT", 11);
    writeU32(g_fakeMemory, kRttCbOffset + 16, 1); // MaxNumUpBuffers
    writeU32(g_fakeMemory, kRttCbOffset + 20, 1); // MaxNumDownBuffers

    // buffDesc[0]: up-buffer ("Terminal"), placed BELOW ramStart -- the bug trigger
    size_t up = kRttCbOffset + 24;
    writeU32(g_fakeMemory, up + 0, 0);                   // sName
    writeU32(g_fakeMemory, up + 4, kRamStart - 0x1000);  // pBuffer  (< ramStart!)
    writeU32(g_fakeMemory, up + 8, 64);                  // SizeOfBuffer
    writeU32(g_fakeMemory, up + 12, 10);                 // WrOff
    writeU32(g_fakeMemory, up + 16, 0);                  // RdOff (!= WrOff -> data pending)
    writeU32(g_fakeMemory, up + 20, 0);                  // Flags

    // buffDesc[1]: down-buffer, left all-zero (SizeOfBuffer == 0 -> inactive, skipped)

    StRtt rtt(kRamStart, 0);

    int res = rtt.open(false);
    if (res != ERROR_OK)
    {
        printf("FAIL: open() returned %d\n", res);
        return 1;
    }

    res = rtt.findRtt(kRamKBytes);
    if (res != ERROR_OK)
    {
        printf("FAIL: findRtt() returned %d\n", res);
        return 1;
    }

    printf("findRtt() OK, calling readRtt() (used to SIGSEGV here per issue #6)...\n");

    res = rtt.readRtt();

    printf("PASS: readRtt() returned %d without crashing\n", res);
    return 0;
}
