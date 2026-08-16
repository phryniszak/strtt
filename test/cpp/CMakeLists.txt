set(test_readrtt_underflow_sources
    test_readrtt_underflow.cpp
    mock_stlink.cpp
    ${CMAKE_SOURCE_DIR}/src/rtt/strtt.cpp
    ${CMAKE_SOURCE_DIR}/src/openocd/log.c
    ${CMAKE_SOURCE_DIR}/src/openocd/helper_time_support.c
    )

add_executable(test_readrtt_underflow ${test_readrtt_underflow_sources})

target_include_directories(test_readrtt_underflow PRIVATE
    ${CMAKE_SOURCE_DIR}/src/rtt
    ${CMAKE_SOURCE_DIR}/src/openocd
    )

add_test(NAME readrtt_underflow COMMAND test_readrtt_underflow)
