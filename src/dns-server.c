/*
 * SPDX-FileCopyrightText: 2021-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 *
 * Adapted from ESP-IDF's protocols/http_server/captive_portal example.
 */

#include "dns-server.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"

#define DNS_PORT 53
#define DNS_PACKET_MAX_LENGTH 512
#define DNS_HEADER_LENGTH 12
#define DNS_TYPE_A 1
#define DNS_CLASS_IN 1
#define DNS_ANSWER_TTL_SECONDS 30

static const char *TAG = "nut-dns";

typedef struct __attribute__((packed))
{
    uint16_t id;
    uint16_t flags;
    uint16_t question_count;
    uint16_t answer_count;
    uint16_t authority_count;
    uint16_t additional_count;
} DnsHeader;

typedef struct __attribute__((packed))
{
    uint16_t name_pointer;
    uint16_t type;
    uint16_t class_code;
    uint32_t ttl;
    uint16_t address_length;
    uint32_t address;
} DnsAnswer;

struct DnsServerHandle
{
    volatile bool running;
    TaskHandle_t task;
    SemaphoreHandle_t stopped;
    char interface_key[16];
};

static int dns_prepare_reply(const uint8_t *request, size_t request_length,
                             uint8_t *reply, size_t reply_capacity,
                             const char *interface_key)
{
    if (request_length < DNS_HEADER_LENGTH || request_length > reply_capacity)
    {
        return -1;
    }

    const DnsHeader *request_header = (const DnsHeader *)request;
    if (ntohs(request_header->question_count) == 0)
    {
        return -1;
    }

    const uint16_t request_flags = ntohs(request_header->flags);
    if ((request_flags & 0x7800U) != 0)
    {
        return -1;
    }

    size_t offset = DNS_HEADER_LENGTH;
    while (offset < request_length && request[offset] != 0)
    {
        const size_t label_length = request[offset];
        if (label_length > 63 || offset + label_length + 1 >= request_length)
        {
            return -1;
        }
        offset += label_length + 1;
    }

    if (offset + 5 > request_length)
    {
        return -1;
    }

    offset++;
    uint16_t question_type;
    uint16_t question_class;
    memcpy(&question_type, request + offset, sizeof(question_type));
    memcpy(&question_class, request + offset + sizeof(question_type), sizeof(question_class));
    question_type = ntohs(question_type);
    question_class = ntohs(question_class);

    if (question_type != DNS_TYPE_A || question_class != DNS_CLASS_IN)
    {
        return -1;
    }

    const size_t question_end = offset + sizeof(uint16_t) + sizeof(uint16_t);
    const size_t reply_length = question_end + sizeof(DnsAnswer);
    if (reply_length > reply_capacity)
    {
        return -1;
    }

    esp_netif_t *network_interface = esp_netif_get_handle_from_ifkey(interface_key);
    esp_netif_ip_info_t ip_info;
    if (network_interface == NULL || esp_netif_get_ip_info(network_interface, &ip_info) != ESP_OK)
    {
        return -1;
    }

    memcpy(reply, request, question_end);
    DnsHeader *reply_header = (DnsHeader *)reply;
    reply_header->flags = htons(request_flags | 0x8000U | 0x0400U);
    reply_header->question_count = htons(1);
    reply_header->answer_count = htons(1);
    reply_header->authority_count = 0;
    reply_header->additional_count = 0;

    DnsAnswer answer = {
        .name_pointer = htons(0xC00CU),
        .type = htons(DNS_TYPE_A),
        .class_code = htons(DNS_CLASS_IN),
        .ttl = htonl(DNS_ANSWER_TTL_SECONDS),
        .address_length = htons(sizeof(ip_info.ip.addr)),
        .address = ip_info.ip.addr,
    };
    memcpy(reply + question_end, &answer, sizeof(answer));

    return (int)reply_length;
}

static void dns_server_task(void *parameter)
{
    DnsServerHandle handle = parameter;
    int socket_handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (socket_handle < 0)
    {
        ESP_LOGE(TAG, "Unable to create DNS socket: errno %d", errno);
        goto finished;
    }

    const struct timeval receive_timeout = {
        .tv_sec = 0,
        .tv_usec = 250000,
    };
    setsockopt(socket_handle, SOL_SOCKET, SO_RCVTIMEO, &receive_timeout, sizeof(receive_timeout));

    const struct sockaddr_in address = {
        .sin_family = AF_INET,
        .sin_port = htons(DNS_PORT),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };
    if (bind(socket_handle, (const struct sockaddr *)&address, sizeof(address)) < 0)
    {
        ESP_LOGE(TAG, "Unable to bind DNS socket: errno %d", errno);
        close(socket_handle);
        goto finished;
    }

    ESP_LOGI(TAG, "Captive portal DNS redirect active");
    while (handle->running)
    {
        uint8_t request[DNS_PACKET_MAX_LENGTH];
        struct sockaddr_storage source_address;
        socklen_t source_length = sizeof(source_address);
        const int request_length = recvfrom(socket_handle, request, sizeof(request), 0,
                                            (struct sockaddr *)&source_address, &source_length);
        if (request_length < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                continue;
            }
            ESP_LOGW(TAG, "DNS receive failed: errno %d", errno);
            continue;
        }

        uint8_t reply[DNS_PACKET_MAX_LENGTH];
        const int reply_length = dns_prepare_reply(request, (size_t)request_length,
                                                   reply, sizeof(reply), handle->interface_key);
        if (reply_length > 0)
        {
            if (sendto(socket_handle, reply, (size_t)reply_length, 0,
                       (const struct sockaddr *)&source_address, source_length) < 0)
            {
                ESP_LOGW(TAG, "DNS send failed: errno %d", errno);
            }
        }
    }

    close(socket_handle);

finished:
    handle->task = NULL;
    xSemaphoreGive(handle->stopped);
    vTaskDelete(NULL);
}

DnsServerHandle dns_server_start(const char *interface_key)
{
    if (interface_key == NULL || strlen(interface_key) >= sizeof(((DnsServerHandle)0)->interface_key))
    {
        return NULL;
    }

    DnsServerHandle handle = calloc(1, sizeof(*handle));
    if (handle == NULL)
    {
        return NULL;
    }

    handle->stopped = xSemaphoreCreateBinary();
    if (handle->stopped == NULL)
    {
        free(handle);
        return NULL;
    }

    strlcpy(handle->interface_key, interface_key, sizeof(handle->interface_key));
    handle->running = true;
    if (xTaskCreate(dns_server_task, "dns-server", 4096, handle, 5, &handle->task) != pdPASS)
    {
        vSemaphoreDelete(handle->stopped);
        free(handle);
        return NULL;
    }

    return handle;
}

void dns_server_stop(DnsServerHandle handle)
{
    if (handle == NULL)
    {
        return;
    }

    handle->running = false;
    if (xSemaphoreTake(handle->stopped, pdMS_TO_TICKS(1000)) != pdTRUE && handle->task != NULL)
    {
        vTaskDelete(handle->task);
    }
    vSemaphoreDelete(handle->stopped);
    free(handle);
}
