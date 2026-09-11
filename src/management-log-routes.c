/** @file management-log-routes.c @brief Serve the ADMIN full retained log response. @see management-log-routes.h, management-authorization.h, management-http.h, management-log.h */
#include "management-log-routes.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "management-authorization.h"
#include "management-http.h"
#include "management-log.h"

static bool management_log_append_timestamp_pair(char *destination, size_t destination_size,
                                                 size_t *used, time_t epoch_seconds)
{
    if (epoch_seconds == 0)
    {
        return management_json_append(destination, destination_size, used, "null") &&
               management_json_append(destination, destination_size, used,
                                     ",\"timestamp_local\":null");
    }

    struct tm utc_time;
    struct tm local_time;
    char utc[40];
    char local[40];
    if (gmtime_r(&epoch_seconds, &utc_time) == NULL ||
        localtime_r(&epoch_seconds, &local_time) == NULL ||
        strftime(utc, sizeof(utc), "%Y-%m-%dT%H:%M:%SZ", &utc_time) == 0 ||
        strftime(local, sizeof(local), "%Y-%m-%dT%H:%M:%S%z", &local_time) == 0)
    {
        return management_json_append(destination, destination_size, used, "null") &&
               management_json_append(destination, destination_size, used,
                                      ",\"timestamp_local\":null");
    }

    return management_json_append_string(destination, destination_size, used, utc) &&
           management_json_append(destination, destination_size, used,
                                  ",\"timestamp_local\":") &&
           management_json_append_string(destination, destination_size, used, local);
}

static bool management_log_format_entry_json(const ManagementLogSnapshotEntry *entry,
                                             char *destination, size_t destination_size)
{
    size_t used = 0;
    if (!management_json_append(destination, destination_size, &used,
                                "{\"uptime_ms\":%llu,\"timestamp_utc\":",
                                (unsigned long long)entry->uptime_ms) ||
        !management_log_append_timestamp_pair(destination, destination_size, &used,
                                              entry->epoch_seconds) ||
        !management_json_append(destination, destination_size, &used, ",\"level\":") ||
        !management_json_append_string(destination, destination_size, &used,
                                       management_log_level_name(entry->level)) ||
        !management_json_append(destination, destination_size, &used, ",\"message\":") ||
        !management_json_append_string(destination, destination_size, &used,
                                       entry->message) ||
        !management_json_append(destination, destination_size, &used, "}"))
    {
        return false;
    }
    return used < destination_size;
}

static bool management_log_format_tail_json(char *destination, size_t destination_size,
                                            size_t *used, size_t entry_count)
{
    return management_json_append(destination, destination_size, used, "]") &&
           management_json_append(destination, destination_size, used,
                                  ",\"returned_count\":%u,\"retained_capacity\":%u,"
                                  "\"status_window\":%u,\"complete_retained_window\":true}",
                                  (unsigned int)entry_count,
                                  (unsigned int)MANAGEMENT_LOG_ENTRY_CAPACITY,
                                  (unsigned int)MANAGEMENT_LOG_STATUS_WINDOW);
}

static esp_err_t management_log_send_response(httpd_req_t *request,
                                              const ManagementLogSnapshotEntry *entries,
                                              size_t entry_count)
{
    char entry_json[2048U];
    char tail_json[128U];

    for (size_t index = 0; index < entry_count; index++)
    {
        if (!management_log_format_entry_json(&entries[index], entry_json, sizeof(entry_json)))
        {
            return management_send_json(
                request, "500 Internal Server Error",
                "{\"error\":\"Unable to prepare log response.\"}");
        }
    }
    size_t tail_used = 0;
    if (!management_log_format_tail_json(tail_json, sizeof(tail_json), &tail_used, entry_count))
    {
        return management_send_json(
            request, "500 Internal Server Error",
            "{\"error\":\"Unable to prepare log response.\"}");
    }

    if (management_start_json_chunks(request, "200 OK") != ESP_OK)
    {
        return management_send_json(
            request, "500 Internal Server Error",
            "{\"error\":\"Unable to prepare log response.\"}");
    }

    if (management_send_json_chunk(request, "{\"logs\":[") != ESP_OK)
    {
        return ESP_FAIL;
    }
    for (size_t index = 0; index < entry_count; index++)
    {
        if (index > 0U && management_send_json_chunk(request, ",") != ESP_OK)
        {
            return ESP_FAIL;
        }
        if (!management_log_format_entry_json(&entries[index], entry_json, sizeof(entry_json)))
        {
            return ESP_FAIL;
        }
        if (management_send_json_chunk(request, entry_json) != ESP_OK)
        {
            return ESP_FAIL;
        }
    }
    if (management_send_json_chunk(request, tail_json) != ESP_OK)
    {
        return ESP_FAIL;
    }

    return management_end_json_chunks(request);
}

esp_err_t management_logs_handler(httpd_req_t *request)
{
    if (!management_require_session_without_activity(request))
    {
        return ESP_OK;
    }

    ManagementLogSnapshotEntry *entries =
        calloc(MANAGEMENT_LOG_ENTRY_CAPACITY, sizeof(*entries));
    if (entries == NULL)
    {
        return management_send_json(
            request, "500 Internal Server Error",
            "{\"error\":\"Unable to allocate log response.\"}");
    }

    size_t entry_count = 0;
    const bool snapshot_ok =
        management_log_copy_snapshot(entries, MANAGEMENT_LOG_ENTRY_CAPACITY,
                                     &entry_count);
    if (!snapshot_ok)
    {
        free(entries);
        return management_send_json(
            request, "500 Internal Server Error",
            "{\"error\":\"Unable to prepare log response.\"}");
    }

    const esp_err_t result = management_log_send_response(request, entries, entry_count);
    free(entries);
    return result;
}
