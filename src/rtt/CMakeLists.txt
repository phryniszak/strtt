include(CMakePrintHelpers)
include(FetchContent)
cmake_print_variables(CMAKE_VERSION)

# pthread
set(THREADS_PREFER_PTHREAD_FLAG)
include(FindThreads)
cmake_print_variables(CMAKE_USE_PTHREADS_INIT)

# sockpp
FetchContent_Declare(
  sockpp_content
  GIT_REPOSITORY    https://github.com/fpagliughi/sockpp.git
  GIT_TAG           master
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
