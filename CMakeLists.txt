# https://stackoverflow.com/questions/6352123/multiple-directories-under-cmake

cmake_minimum_required(VERSION 3.0.0)
project(strtt VERSION 0.2.0)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)

# build stlink library
add_subdirectory(src/openocd)

# build sample app
add_subdirectory(src/rtt)