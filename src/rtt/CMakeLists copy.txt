include(CMakePrintHelpers)

set(THREADS_PREFER_PTHREAD_FLAG)
include(FindThreads)
cmake_print_variables(CMAKE_USE_PTHREADS_INIT)

set(strtt_source_files 
    strtt.cpp
    sysview.cpp
    strttapp.cpp)

find_library(sockpp NAME sockpp REQUIRED)

add_executable(strtt ${strtt_source_files})

target_link_libraries(strtt stlink sockpp Threads::Threads)
