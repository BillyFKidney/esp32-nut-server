# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

# Keep focused modules explicit because ESP-IDF does not reconfigure a legacy
# recursive source glob when a new file is added during an incremental build.
FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)
list(APPEND app_sources
    ${CMAKE_SOURCE_DIR}/src/management-auth-routes.c
    ${CMAKE_SOURCE_DIR}/src/management-certificates.c
    ${CMAKE_SOURCE_DIR}/src/management-credentials.c
    ${CMAKE_SOURCE_DIR}/src/management-http.c
    ${CMAKE_SOURCE_DIR}/src/management-log.c
    ${CMAKE_SOURCE_DIR}/src/management-pages.c
    ${CMAKE_SOURCE_DIR}/src/management-session.c
    ${CMAKE_SOURCE_DIR}/src/management-status.c
    ${CMAKE_SOURCE_DIR}/src/management-authorization.c
    ${CMAKE_SOURCE_DIR}/src/wifi-credentials.c
    ${CMAKE_SOURCE_DIR}/src/wifi-diagnostics.c
    ${CMAKE_SOURCE_DIR}/src/wifi-provisioning-web.c
)
list(REMOVE_DUPLICATES app_sources)

idf_component_register(
    SRCS ${app_sources} ../lib/af_unix_socket/af_unix_socket.c ../lib/syslog/syslog.c
    INCLUDE_DIRS ../include ../lib/af_unix_socket ../lib/libusb ../lib/syslog
    PRIV_REQUIRES usb nvs_flash openthread esp_coex esp_event driver esp_driver_tsens esp_hw_support heap spi_flash
    REQUIRES fatfs vfs esp_wifi esp_http_server esp_https_server esp_netif esp_timer lwip app_update)

fatfs_create_spiflash_image(var ../fatfs/var FLASH_IN_PROJECT)
fatfs_create_spiflash_image(usr ../fatfs/usr FLASH_IN_PROJECT)
