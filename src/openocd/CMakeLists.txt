include(CMakePrintHelpers)

set(stlink_source_files
    log.h
    log.c
    stlink.h
    stlink.c
    helper_time_support.h
    helper_time_support.c
    adapter.c
    libusb_helper.c
    helper_replacement.c
    )

# add libusb library


find_path(LIBUSB_INCLUDE_DIR NAMES libusb.h PATH_SUFFIXES "libusb-1.0")
cmake_print_variables(LIBUSB_INCLUDE_DIR)

find_library(LIBUSB_LIBRARY NAMES libusb-1.0 usb-1.0)
cmake_print_variables(LIBUSB_LIBRARY)

#find_package(libusb-1.0 REQUIRED)

add_library(stlink OBJECT ${stlink_source_files})

IF (WIN32)
    target_link_libraries(stlink PRIVATE "${LIBUSB_LIBRARY}" ws2_32)
ELSE()
    target_link_libraries(stlink PRIVATE "${LIBUSB_LIBRARY}")
ENDIF()


# Make sure the compiler can find include files for our "common" library
# when other libraries or executables link to "common"
target_include_directories (stlink PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories (stlink PRIVATE ${LIBUSB_INCLUDE_DIR})

# target_compile_options(stlink PRIVATE -fPIC)
# add_definitions(-D__USE_MINGW_ANSI_STDIO=1)
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Werror=incompatible-pointer-types")
