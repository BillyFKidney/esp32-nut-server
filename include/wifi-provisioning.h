#pragma once

/**
 * Connect to saved Wi-Fi or start the open fallback provisioning portal.
 *
 * This function initializes NVS, esp-netif, the default event loop, and Wi-Fi.
 */
void wifi_provisioning_init(void);
