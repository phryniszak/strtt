#ifndef _PH_TEST_MOCK_STLINK_H
#define _PH_TEST_MOCK_STLINK_H

#include <cstdint>
#include <vector>

// Simulated target RAM used by the mocked read_mem()/write_mem() callbacks
// in mock_stlink.cpp. The test fills this in before driving StRtt through
// its public API (open/findRtt/readRtt), exactly as if a real probe had
// returned this data over USB.
extern std::vector<uint8_t> g_fakeMemory;
extern uint32_t g_fakeMemoryBase;

#endif
