/** @file wifi-diagnostics.c @brief Collect bounded Wi-Fi and DHCP diagnostics. @see wifi-diagnostics.h, esp_netif.h, lwip/dhcp.h, lwip/netif.h */
#include "wifi-diagnostics.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "esp_netif_net_stack.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "lwip/dhcp.h"
#include "lwip/netif.h"
#include "lwip/prot/dhcp.h"
#include "lwip/tcpip.h"

typedef struct
{
    struct netif *network_interface;
    bool available;
    bool offer_received;
    uint8_t state;
    uint8_t tries;
} WifiDhcpSnapshot;

static char connection_diagnostic[WIFI_CONNECTION_DIAGNOSTIC_LENGTH];
static portMUX_TYPE wifi_diagnostic_lock = portMUX_INITIALIZER_UNLOCKED;

void wifi_set_connection_diagnostic(const char *message)
{
    taskENTER_CRITICAL(&wifi_diagnostic_lock);
    snprintf(connection_diagnostic, sizeof(connection_diagnostic), "%s", message);
    taskEXIT_CRITICAL(&wifi_diagnostic_lock);
}

void wifi_get_connection_diagnostic(char *destination, size_t destination_size)
{
    taskENTER_CRITICAL(&wifi_diagnostic_lock);
    snprintf(destination, destination_size, "%s", connection_diagnostic);
    taskEXIT_CRITICAL(&wifi_diagnostic_lock);
}

static const char *wifi_dhcp_state_name(uint8_t state)
{
    switch (state)
    {
    case DHCP_STATE_REQUESTING:
        return "REQUESTING";
    case DHCP_STATE_INIT:
        return "INIT";
    case DHCP_STATE_REBOOTING:
        return "REBOOTING";
    case DHCP_STATE_REBINDING:
        return "REBINDING";
    case DHCP_STATE_RENEWING:
        return "RENEWING";
    case DHCP_STATE_SELECTING:
        return "SELECTING";
    case DHCP_STATE_INFORMING:
        return "INFORMING";
    case DHCP_STATE_CHECKING:
        return "CHECKING";
    case DHCP_STATE_BOUND:
        return "BOUND";
    case DHCP_STATE_BACKING_OFF:
        return "BACKING_OFF";
    case DHCP_STATE_OFF:
    default:
        return "OFF";
    }
}

static void wifi_capture_dhcp_snapshot_callback(void *argument)
{
    WifiDhcpSnapshot *snapshot = argument;
    const struct dhcp *dhcp = netif_dhcp_data(snapshot->network_interface);
    if (dhcp == NULL)
    {
        return;
    }

    snapshot->available = true;
    snapshot->state = dhcp->state;
    snapshot->tries = dhcp->tries;
    snapshot->offer_received = !ip4_addr_isany_val(dhcp->offered_ip_addr);
}

void wifi_diagnostics_capture_dhcp(esp_netif_t *station_network_interface)
{
    WifiDhcpSnapshot snapshot = {
        .network_interface =
            (struct netif *)esp_netif_get_netif_impl(station_network_interface),
    };
    if (snapshot.network_interface == NULL ||
        tcpip_callback_wait(wifi_capture_dhcp_snapshot_callback, &snapshot) != ERR_OK)
    {
        wifi_set_connection_diagnostic("Wi-Fi associated, but the DHCP client state could not be inspected.");
        return;
    }

    if (!snapshot.available)
    {
        wifi_set_connection_diagnostic("Wi-Fi associated, but the DHCP client was not running.");
        return;
    }

    if ((snapshot.state == DHCP_STATE_SELECTING || snapshot.state == DHCP_STATE_BACKING_OFF) &&
        !snapshot.offer_received)
    {
        char message[WIFI_CONNECTION_DIAGNOSTIC_LENGTH];
        snprintf(message, sizeof(message),
                 "Wi-Fi associated, but no DHCP offer was received (%s after %u attempts).",
                 wifi_dhcp_state_name(snapshot.state), snapshot.tries);
        wifi_set_connection_diagnostic(message);
        return;
    }

    if (snapshot.state == DHCP_STATE_REQUESTING && snapshot.offer_received)
    {
        wifi_set_connection_diagnostic("A DHCP offer was received, but its lease acknowledgement did not arrive.");
        return;
    }

    if (snapshot.state == DHCP_STATE_CHECKING && snapshot.offer_received)
    {
        wifi_set_connection_diagnostic("A DHCP acknowledgement was received; the offered address is being checked for a conflict.");
        return;
    }

    char message[WIFI_CONNECTION_DIAGNOSTIC_LENGTH];
    snprintf(message, sizeof(message),
             "Wi-Fi associated, but DHCP did not complete (state %s, %u attempts, offer %s).",
             wifi_dhcp_state_name(snapshot.state), snapshot.tries,
             snapshot.offer_received ? "received" : "not received");
    wifi_set_connection_diagnostic(message);
}
