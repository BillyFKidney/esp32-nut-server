cmake_minimum_required(VERSION 3.16.0)

# Keep release provenance in version.txt, but make local development builds
# identify the exact Git state that produced the firmware. This is evaluated
# during CMake configure/reconfigure, so a changed branch or dirty state must
# be followed by a reconfigure before the displayed build identity changes.
set(ESP32_NUT_GIT_VERSION "")
execute_process(
    COMMAND git describe --tags --dirty --always --match=v[0-9]*.[0-9]*.[0-9]*
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE ESP32_NUT_GIT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    RESULT_VARIABLE ESP32_NUT_GIT_VERSION_RESULT)
if(ESP32_NUT_GIT_VERSION_RESULT EQUAL 0 AND ESP32_NUT_GIT_VERSION)
    set(PROJECT_VER "${ESP32_NUT_GIT_VERSION}")
endif()
unset(ESP32_NUT_GIT_VERSION)
unset(ESP32_NUT_GIT_VERSION_RESULT)

set(EXTRA_COMPONENT_DIRS "src" "lib/libusb" "lib/syslog")
set(COMPONENTS "src")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(nut-esp32s3)

# The ESP32 port carries the NUT configure results as templates. Generate the
# headers into ESP-IDF's build-local config include directory so generated
# files do not need to be tracked in the source tree.
configure_file(
    "${CMAKE_SOURCE_DIR}/include/config.h.sample"
    "${CMAKE_BINARY_DIR}/config/config.h"
    COPYONLY)
configure_file(
    "${CMAKE_SOURCE_DIR}/include/nut_version.h.sample"
    "${CMAKE_BINARY_DIR}/config/nut_version.h"
    COPYONLY)
