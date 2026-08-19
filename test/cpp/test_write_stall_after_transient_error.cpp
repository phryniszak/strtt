// Regression test for a permanent RTT write "stall" that only a full
// strtt restart would previously clear.
//
// StRtt::writeRtt() caches a one-time shadow copy of the down-buffer's
// on-device content in `_wrMemory`, so it only has to download it once and
// can thereafter merge in new bytes locally before writing the whole buffer
// back (writing the whole buffer, rather than just the changed bytes, is
// deliberate -- see the comment above the shadow-copy block in strtt.cpp
// for why a "write only the new bytes" version was tried and reverted: on
// real ST-LINK V3 hardware, small/unaligned AP memory writes reliably wedge
// the USB bulk transfer into a permanent timeout loop).
//
// The bug this test targets: `_wrMemory.resize(...)` used to run *before*
// the read_mem() that was supposed to populate it, so a single transient
// read failure (a USB hiccup -- rare, but effectively guaranteed over a
// long-running session) still left `_wrMemory` non-empty. Every later call
// saw a non-empty `_wrMemory` and skipped ever fetching a real snapshot
// again, so it kept blasting the zero-filled shadow buffer over the real
// device memory -- silently zeroing whatever the target hadn't yet
// consumed, forever, until the process (and therefore `_wrMemory`) was
// torn down and restarted.
//
// This test reproduces that with a mocked ST-LINK backend: it injects
// exactly one failure on the down-buffer's snapshot read, performs two
// writes, and checks that bytes the write never touched are still intact
// on the "device" afterward. Before the fix, the second write silently
// zeroes them; after the fix, the retried snapshot read preserves them.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include "mock_stlink.h"
#include "strtt.h"
#include "stlink_errors.h"

namespace
{
constexpr uint32_t kRamStart = 0x20000000;
constexpr uint32_t kRamKBytes = 2;
constexpr uint32_t kRttCbOffset = 16; // keep nonzero, offset 0 is ambiguous with "not found" in findRtt()

constexpr uint32_t kUpBufferOffset = 200;
constexpr uint32_t kUpBufferSize = 64;

constexpr uint32_t kDownBufferOffset = 300;
constexpr uint32_t kDownBufferSize = 16;
constexpr uint8_t kCanary = 0xAA; // stand-in for "whatever was really in that RAM"

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

    // buffDesc[0]: up-buffer ("Terminal"), empty (RdOff == WrOff)
    size_t up = kRttCbOffset + 24;
    writeU32(g_fakeMemory, up + 0, 0);
    writeU32(g_fakeMemory, up + 4, kRamStart + kUpBufferOffset);
    writeU32(g_fakeMemory, up + 8, kUpBufferSize);
    writeU32(g_fakeMemory, up + 12, 0);
    writeU32(g_fakeMemory, up + 16, 0);
    writeU32(g_fakeMemory, up + 20, 0);

    // buffDesc[1]: down-buffer ("Terminal"), empty and ready for host writes
    size_t down = up + 24;
    uint32_t downBufferAddr = kRamStart + kDownBufferOffset;
    writeU32(g_fakeMemory, down + 0, 0);
    writeU32(g_fakeMemory, down + 4, downBufferAddr);
    writeU32(g_fakeMemory, down + 8, kDownBufferSize);
    writeU32(g_fakeMemory, down + 12, 0); // WrOff
    writeU32(g_fakeMemory, down + 16, 0); // RdOff
    writeU32(g_fakeMemory, down + 20, 0);

    // Fill the down-buffer's on-device content with a recognizable pattern.
    // A correct implementation must preserve the bytes our writes never
    // touch; the bug zeroes them.
    memset(g_fakeMemory.data() + kDownBufferOffset, kCanary, kDownBufferSize);

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

    // Simulate exactly one transient USB error on the down-buffer's
    // one-time snapshot read.
    g_failReadMemAtAddr = downBufferAddr;
    g_failReadMemAtAddrRemaining = 1;

    std::vector<uint8_t> firstWrite = {'h', 'i'};
    int firstRet = rtt.writeRtt(0, &firstWrite);
    if (firstRet >= 0)
    {
        printf("FAIL: first writeRtt() should have failed (injected error), returned %d\n", firstRet);
        return 1;
    }
    if (firstWrite.size() != 2)
    {
        printf("FAIL: failed write should not have consumed any bytes, %zu left\n", firstWrite.size());
        return 1;
    }
    printf("First write correctly failed (%d) with the injected transient error, retrying...\n", firstRet);

    // The injected failure was single-shot; this retry should succeed as
    // if nothing had happened.
    int secondRet = rtt.writeRtt(0, &firstWrite);
    if (secondRet != 2)
    {
        printf("FAIL: retried writeRtt() should have written 2 bytes, returned %d\n", secondRet);
        return 1;
    }

    // The two bytes we wrote land at offset 0-1. Everything from offset 2
    // onward was never part of any write and must still hold the canary
    // (used to catch strtt silently zeroing content it doesn't own).
    bool corrupted = false;
    for (uint32_t i = 2; i < kDownBufferSize; i++)
    {
        if (g_fakeMemory[kDownBufferOffset + i] != kCanary)
        {
            corrupted = true;
            break;
        }
    }
    if (corrupted)
    {
        printf("FAIL: down-buffer bytes beyond the write were zeroed -- stale/uninitialized shadow "
               "buffer was written back to the device (this is the restart-fixes-it bug)\n");
        return 1;
    }

    if (memcmp(g_fakeMemory.data() + kDownBufferOffset, "hi", 2) != 0)
    {
        printf("FAIL: written bytes not found at the expected offset\n");
        return 1;
    }

    printf("PASS: transient read failure was retried correctly; unrelated buffer content preserved\n");
    return 0;
}
