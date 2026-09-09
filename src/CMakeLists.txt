# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

# Keep focused modules explicit because ESP-IDF does not reconfigure a legacy
# recursive source glob when a new file is added during an incremental build.
FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)
list(APPEND app_sources
    ${CMAKE_SOURCE_DIR}/src/api-token-store.c
    ${CMAKE_SOURCE_DIR}/src/management-auth-routes.c
    ${CMAKE_SOURCE_DIR}/src/management-certificates.c
    ${CMAKE_SOURCE_DIR}/src/management-credentials.c
    ${CMAKE_SOURCE_DIR}/src/management-diagnostics-routes.c
    ${CMAKE_SOURCE_DIR}/src/management-http.c
    ${CMAKE_SOURCE_DIR}/src/management-log.c
    ${CMAKE_SOURCE_DIR}/src/management-log-routes.c
    ${CMAKE_SOURCE_DIR}/src/management-pages.c
    ${CMAKE_SOURCE_DIR}/src/management-session.c
    ${CMAKE_SOURCE_DIR}/src/management-session-routes.c
    ${CMAKE_SOURCE_DIR}/src/management-status.c
    ${CMAKE_SOURCE_DIR}/src/management-status-routes.c
    ${CMAKE_SOURCE_DIR}/src/management-time-routes.c
    ${CMAKE_SOURCE_DIR}/src/management-token-routes.c
    ${CMAKE_SOURCE_DIR}/src/management-wifi-routes.c
    ${CMAKE_SOURCE_DIR}/src/management-ota-routes.c
    ${CMAKE_SOURCE_DIR}/src/management-routes.c
    ${CMAKE_SOURCE_DIR}/src/management-authorization.c
    ${CMAKE_SOURCE_DIR}/src/management-device-config.c
    ${CMAKE_SOURCE_DIR}/src/management-device-routes.c
    ${CMAKE_SOURCE_DIR}/src/nut-diagnostics.c
    ${CMAKE_SOURCE_DIR}/src/wifi-credentials.c
    ${CMAKE_SOURCE_DIR}/src/wifi-diagnostics.c
    ${CMAKE_SOURCE_DIR}/src/wifi-provisioning-web.c
)
list(REMOVE_DUPLICATES app_sources)

idf_component_register(
    SRCS ${app_sources} ../lib/af_unix_socket/af_unix_socket.c ../lib/syslog/syslog.c
    INCLUDE_DIRS ../include ../lib/af_unix_socket ../lib/libusb ../lib/syslog
    PRIV_REQUIRES usb nvs_flash openthread esp_coex esp_event driver esp_driver_tsens esp_hw_support heap spi_flash
    REQUIRES fatfs vfs esp_wifi esp_http_server esp_https_server esp_netif esp_timer lwip app_update
    EMBED_FILES ${CMAKE_SOURCE_DIR}/docs/images/nut-logo.png)

# The administration page is embedded JavaScript; validate its generated script
# before compiling on macOS, where JXA is available without a Node dependency.
if(CMAKE_HOST_APPLE)
    find_program(OSASCRIPT_EXECUTABLE osascript)
    if(OSASCRIPT_EXECUTABLE)
        add_custom_target(management_page_js_validation ALL
            COMMAND ${OSASCRIPT_EXECUTABLE} -l JavaScript
                    ${CMAKE_SOURCE_DIR}/tools/validate-management-page-js.mjs
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
        add_dependencies(${COMPONENT_LIB} management_page_js_validation)
    endif()
endif()

fatfs_create_spiflash_image(var ../fatfs/var FLASH_IN_PROJECT)
fatfs_create_spiflash_image(usr ../fatfs/usr FLASH_IN_PROJECT)
