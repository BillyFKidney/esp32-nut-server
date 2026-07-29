#pragma once

#include <stddef.h>

#include "esp_netif.h"

#define WIFI_CONNECTION_DIAGNOSTIC_LENGTH 192U

/** Store the current user-facing Wi-Fi connection diagnostic. */
void wifi_set_connection_diagnostic(const char *message);

/** Copy the current user-facing Wi-Fi connection diagnostic. */
void wifi_get_connection_diagnostic(char *destination, size_t destination_size);

/**
 * Capture a read-only DHCP snapshot for the supplied station interface and
 * update the current connection diagnostic without changing Wi-Fi behavior.
 */
void wifi_diagnostics_capture_dhcp(esp_netif_t *station_network_interface);
