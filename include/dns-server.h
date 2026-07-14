/*
 * SPDX-FileCopyrightText: 2021-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#pragma once

#include "esp_netif.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DnsServerHandle *DnsServerHandle;

/**
 * Start a DNS server that resolves every IPv4 hostname to the named interface.
 *
 * Adapted from the ESP-IDF captive portal example.
 */
DnsServerHandle dns_server_start(const char *interface_key);

/** Stop a DNS server created by dns_server_start(). */
void dns_server_stop(DnsServerHandle handle);

#ifdef __cplusplus
}
#endif
