#ifndef _PH_TEST_MOCK_STLINK_H
#define _PH_TEST_MOCK_STLINK_H

#include <cstdint>
#include <vector>

// Simulated target RAM used by the mocked read_mem()/write_mem() callbacks
// in mock_stlink.cpp. The test fills this in before driving StRtt through
// its public API (open/findRtt/readRtt/writeRtt), exactly as if a real
// probe had returned this data over USB. write_mem() writes through to
// this same buffer, so tests can inspect the "device" state afterward.
extern std::vector<uint8_t> g_fakeMemory;
extern uint32_t g_fakeMemoryBase;

// Single-shot failure injection: when g_failReadMemAtAddr (or
// g_failWriteMemAtAddr) is non-zero and a read_mem()/write_mem() call's
// address matches it, the call fails (returns ERROR_FAIL) without touching
// g_fakeMemory, and the matching *Remaining counter is decremented. Once it
// reaches 0, matching calls succeed normally again. Simulates a single
// transient USB error on a specific memory region/address.
extern uint32_t g_failReadMemAtAddr;
extern int g_failReadMemAtAddrRemaining;
extern uint32_t g_failWriteMemAtAddr;
extern int g_failWriteMemAtAddrRemaining;

#endif
