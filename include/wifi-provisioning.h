#pragma once

#include <stdbool.h>

/**
 * Connect to saved Wi-Fi or start the open fallback provisioning portal.
 *
 * This function initializes NVS, esp-netif, the default event loop, and Wi-Fi.
 */
void wifi_provisioning_init(void);

/** Return true when the station interface has obtained an IPv4 address. */
bool wifi_provisioning_is_connected(void);
