include(CMakePrintHelpers)
include(FetchContent)
cmake_print_variables(CMAKE_VERSION)

# pthread
set(THREADS_PREFER_PTHREAD_FLAG)
include(FindThreads)
cmake_print_variables(CMAKE_USE_PTHREADS_INIT)

if (SYSVIEW)

    # sockpp
    FetchContent_Declare(
      sockpp_content
      GIT_REPOSITORY    https://github.com/fpagliughi/sockpp.git
      GIT_TAG           999ad87296e34d5a8e4edf15d985315d0d84eda7
    )

    FetchContent_MakeAvailable(sockpp_content)
    cmake_print_variables(sockpp_content_SOURCE_DIR)
    cmake_print_variables(sockpp_content_BINARY_DIR)
    include_directories(${sockpp_content_SOURCE_DIR}/include)

    # sources
    set(strtt_source_files
        strtt.cpp
        sysview.cpp
        strttapp.cpp)

    add_executable(strtt ${strtt_source_files})

    IF (WIN32)
        target_link_libraries(strtt stlink sockpp-static ws2_32 Threads::Threads)
    ELSE()
        target_link_libraries(strtt stlink sockpp Threads::Threads)
    ENDIF()

else()
    # sources
    set(strtt_source_files
        strtt.cpp
        strttapp.cpp)

    add_executable(strtt ${strtt_source_files})

    IF (WIN32)
        target_link_libraries(strtt stlink ws2_32 Threads::Threads)
    ELSE()
        target_link_libraries(strtt stlink Threads::Threads)
    ENDIF()

endif()