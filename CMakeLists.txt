cmake_minimum_required(VERSION 3.16.0)
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
