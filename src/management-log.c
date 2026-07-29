#include "management-log.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

#define MANAGEMENT_LOG_ENTRY_CAPACITY 24U
#define MANAGEMENT_LOG_MESSAGE_LENGTH 192U
#define MANAGEMENT_LOG_CHUNK_LENGTH 256U
#define MANAGEMENT_LOG_RESPONSE_LIMIT 6U
#define MANAGEMENT_LOG_VALID_EPOCH 1704067200LL

typedef struct
{
    uint64_t uptime_ms;
    time_t epoch_seconds;
    char level;
    char message[MANAGEMENT_LOG_MESSAGE_LENGTH];
} ManagementLogEntry;

static portMUX_TYPE management_log_lock = portMUX_INITIALIZER_UNLOCKED;
static ManagementLogEntry management_log_entries[MANAGEMENT_LOG_ENTRY_CAPACITY];
static char management_log_pending[MANAGEMENT_LOG_CHUNK_LENGTH];
static size_t management_log_pending_length;
static size_t management_log_next;
static size_t management_log_count;
static vprintf_like_t management_previous_log_vprintf;
static bool management_log_capture_started;

static char management_log_level_from_line(const char *line)
{
    if (line != NULL && line[1] == ' ')
    {
        switch (line[0])
        {
        case 'E':
        case 'W':
        case 'I':
        case 'D':
        case 'V':
            return line[0];
        default:
            break;
        }
    }
    return 'I';
}

static const char *management_log_message_from_line(const char *line)
{
    if (line == NULL)
    {
        return "";
    }

    const char *tag_separator = strchr(line, ':');
    if (tag_separator != NULL && tag_separator[1] == ' ')
    {
        return tag_separator + 2;
    }
    if (line[0] != '\0' && line[1] == ' ')
    {
        return line + 2;
    }
    return line;
}

static const char *management_log_level_name(char level)
{
    switch (level)
    {
    case 'E':
        return "error";
    case 'W':
        return "warning";
    case 'D':
    case 'V':
        return "debug";
    default:
        return "info";
    }
}

static time_t management_log_current_epoch(void)
{
    const time_t now = time(NULL);
    return now >= MANAGEMENT_LOG_VALID_EPOCH ? now : (time_t)0;
}

static void management_log_store_locked(char level, const char *message,
                                        uint64_t uptime_ms, time_t epoch_seconds)
{
    if (message == NULL || message[0] == '\0')
    {
        return;
    }

    ManagementLogEntry *entry = &management_log_entries[management_log_next];
    entry->uptime_ms = uptime_ms;
    entry->epoch_seconds = epoch_seconds;
    entry->level = level;
    snprintf(entry->message, sizeof(entry->message), "%s", message);
    management_log_next = (management_log_next + 1U) % MANAGEMENT_LOG_ENTRY_CAPACITY;
    if (management_log_count < MANAGEMENT_LOG_ENTRY_CAPACITY)
    {
        management_log_count++;
    }
}

static void management_log_commit_pending_locked(uint64_t uptime_ms,
                                                 time_t epoch_seconds)
{
    if (management_log_pending_length == 0)
    {
        return;
    }

    management_log_pending[management_log_pending_length] = '\0';
    management_log_store_locked(
        management_log_level_from_line(management_log_pending),
        management_log_message_from_line(management_log_pending),
        uptime_ms, epoch_seconds);
    management_log_pending_length = 0;
    management_log_pending[0] = '\0';
}

static void management_log_capture_chunk(const char *chunk)
{
    if (chunk == NULL)
    {
        return;
    }

    const uint64_t uptime_ms = (uint64_t)(esp_timer_get_time() / 1000LL);
    const time_t epoch_seconds = management_log_current_epoch();
    taskENTER_CRITICAL(&management_log_lock);
    for (const char *cursor = chunk; *cursor != '\0'; cursor++)
    {
        if (*cursor == '\n')
        {
            management_log_commit_pending_locked(uptime_ms, epoch_seconds);
        }
        else if (*cursor != '\r')
        {
            if (management_log_pending_length < sizeof(management_log_pending) - 1U)
            {
                management_log_pending[management_log_pending_length++] = *cursor;
                management_log_pending[management_log_pending_length] = '\0';
            }
            else
            {
                management_log_pending[sizeof(management_log_pending) - 4U] = '.';
                management_log_pending[sizeof(management_log_pending) - 3U] = '.';
                management_log_pending[sizeof(management_log_pending) - 2U] = '.';
            }
        }
    }
    taskEXIT_CRITICAL(&management_log_lock);
}

static int management_log_vprintf(const char *format, va_list arguments)
{
    int output_result = 0;
    if (management_previous_log_vprintf != NULL)
    {
        va_list output_arguments;
        va_copy(output_arguments, arguments);
        output_result = management_previous_log_vprintf(format, output_arguments);
        va_end(output_arguments);
    }

    char chunk[MANAGEMENT_LOG_CHUNK_LENGTH];
    va_list capture_arguments;
    va_copy(capture_arguments, arguments);
    const int written = vsnprintf(chunk, sizeof(chunk), format, capture_arguments);
    va_end(capture_arguments);
    if (written >= 0)
    {
        chunk[sizeof(chunk) - 1U] = '\0';
        management_log_capture_chunk(chunk);
        if (written >= (int)sizeof(chunk) - 1)
        {
            management_log_capture_chunk("\n");
        }
    }
    return output_result;
}

void management_log_capture_start(void)
{
    if (management_log_capture_started)
    {
        return;
    }
    management_previous_log_vprintf = esp_log_set_vprintf(management_log_vprintf);
    management_log_capture_started = true;
}

void management_log_capture_syslog(int priority, const char *format,
                                   va_list arguments)
{
    if (format == NULL)
    {
        return;
    }

    char message[MANAGEMENT_LOG_MESSAGE_LENGTH];
    va_list capture_arguments;
    va_copy(capture_arguments, arguments);
    const int written = vsnprintf(message, sizeof(message), format, capture_arguments);
    va_end(capture_arguments);
    if (written < 0)
    {
        return;
    }
    message[sizeof(message) - 1U] = '\0';

    char line[MANAGEMENT_LOG_MESSAGE_LENGTH];
    snprintf(line, sizeof(line), "NUT: %.*s",
             (int)(sizeof(line) - sizeof("NUT: ")), message);
    char level = 'D';
    if (priority <= 3)
    {
        level = 'E';
    }
    else if (priority == 4)
    {
        level = 'W';
    }
    else if (priority <= 5)
    {
        level = 'I';
    }

    const uint64_t uptime_ms = (uint64_t)(esp_timer_get_time() / 1000LL);
    const time_t epoch_seconds = management_log_current_epoch();
    taskENTER_CRITICAL(&management_log_lock);
    management_log_store_locked(level, line, uptime_ms, epoch_seconds);
    taskEXIT_CRITICAL(&management_log_lock);
}

static bool management_log_json_append(char *destination, size_t destination_size,
                                       size_t *used, const char *format, ...)
{
    if (destination == NULL || used == NULL || *used >= destination_size)
    {
        return false;
    }

    va_list arguments;
    va_start(arguments, format);
    const int written = vsnprintf(destination + *used,
                                  destination_size - *used, format, arguments);
    va_end(arguments);
    if (written < 0 || (size_t)written >= destination_size - *used)
    {
        *used = destination_size;
        return false;
    }
    *used += (size_t)written;
    return true;
}

static bool management_log_json_append_string(char *destination, size_t destination_size,
                                              size_t *used, const char *value)
{
    if (!management_log_json_append(destination, destination_size, used, "\""))
    {
        return false;
    }

    if (value == NULL)
    {
        value = "";
    }
    for (const unsigned char *cursor = (const unsigned char *)value; *cursor != '\0'; cursor++)
    {
        switch (*cursor)
        {
        case '\\':
            if (!management_log_json_append(destination, destination_size, used, "\\\\"))
            {
                return false;
            }
            break;
        case '"':
            if (!management_log_json_append(destination, destination_size, used, "\\\""))
            {
                return false;
            }
            break;
        case '\b':
            if (!management_log_json_append(destination, destination_size, used, "\\b"))
            {
                return false;
            }
            break;
        case '\f':
            if (!management_log_json_append(destination, destination_size, used, "\\f"))
            {
                return false;
            }
            break;
        case '\n':
            if (!management_log_json_append(destination, destination_size, used, "\\n"))
            {
                return false;
            }
            break;
        case '\r':
            if (!management_log_json_append(destination, destination_size, used, "\\r"))
            {
                return false;
            }
            break;
        case '\t':
            if (!management_log_json_append(destination, destination_size, used, "\\t"))
            {
                return false;
            }
            break;
        default:
            if (*cursor < 0x20U &&
                !management_log_json_append(destination, destination_size, used,
                                             "\\u%04x", (unsigned int)*cursor))
            {
                return false;
            }
            else if (*cursor >= 0x20U &&
                     !management_log_json_append(destination, destination_size, used,
                                                  "%c", (char)*cursor))
            {
                return false;
            }
            break;
        }
    }
    return management_log_json_append(destination, destination_size, used, "\"");
}

static void management_log_flush_pending(void)
{
    const uint64_t uptime_ms = (uint64_t)(esp_timer_get_time() / 1000LL);
    const time_t epoch_seconds = management_log_current_epoch();
    taskENTER_CRITICAL(&management_log_lock);
    management_log_commit_pending_locked(uptime_ms, epoch_seconds);
    taskEXIT_CRITICAL(&management_log_lock);
}

static bool management_log_format_timestamps(time_t epoch_seconds,
                                             char *utc, size_t utc_size,
                                             char *local, size_t local_size)
{
    if (epoch_seconds == 0 || utc == NULL || local == NULL ||
        utc_size == 0 || local_size == 0)
    {
        return false;
    }

    struct tm utc_time;
    struct tm local_time;
    if (gmtime_r(&epoch_seconds, &utc_time) == NULL ||
        localtime_r(&epoch_seconds, &local_time) == NULL ||
        strftime(utc, utc_size, "%Y-%m-%dT%H:%M:%SZ", &utc_time) == 0 ||
        strftime(local, local_size, "%Y-%m-%dT%H:%M:%S%z", &local_time) == 0)
    {
        utc[0] = '\0';
        local[0] = '\0';
        return false;
    }
    return true;
}

bool management_log_append_snapshot(char *destination, size_t destination_size,
                                    size_t *used)
{
    ManagementLogEntry entries[MANAGEMENT_LOG_RESPONSE_LIMIT];
    management_log_flush_pending();

    taskENTER_CRITICAL(&management_log_lock);
    const size_t entry_count = management_log_count < MANAGEMENT_LOG_RESPONSE_LIMIT
                                   ? management_log_count
                                   : MANAGEMENT_LOG_RESPONSE_LIMIT;
    const size_t first_entry =
        (management_log_next + MANAGEMENT_LOG_ENTRY_CAPACITY - entry_count) %
        MANAGEMENT_LOG_ENTRY_CAPACITY;
    for (size_t index = 0; index < entry_count; index++)
    {
        entries[index] = management_log_entries[
            (first_entry + index) % MANAGEMENT_LOG_ENTRY_CAPACITY];
    }
    taskEXIT_CRITICAL(&management_log_lock);

    if (!management_log_json_append(destination, destination_size, used, ",\"logs\":["))
    {
        return false;
    }
    for (size_t index = 0; index < entry_count; index++)
    {
        if (index > 0 &&
            !management_log_json_append(destination, destination_size, used, ","))
        {
            return false;
        }

        char utc[40] = {0};
        char local[40] = {0};
        const bool timestamps_available = management_log_format_timestamps(
            entries[index].epoch_seconds, utc, sizeof(utc), local, sizeof(local));
        if (!management_log_json_append(destination, destination_size, used,
                                        "{\"uptime_ms\":%llu,\"timestamp_utc\":",
                                        (unsigned long long)entries[index].uptime_ms) ||
            (timestamps_available
                 ? !management_log_json_append_string(destination, destination_size, used, utc)
                 : !management_log_json_append(destination, destination_size, used, "null")) ||
            !management_log_json_append(destination, destination_size, used,
                                        ",\"timestamp_local\":") ||
            (timestamps_available
                 ? !management_log_json_append_string(destination, destination_size, used, local)
                 : !management_log_json_append(destination, destination_size, used, "null")) ||
            !management_log_json_append(destination, destination_size, used, ",\"level\":") ||
            !management_log_json_append_string(destination, destination_size, used,
                                               management_log_level_name(entries[index].level)) ||
            !management_log_json_append(destination, destination_size, used, ",\"message\":") ||
            !management_log_json_append_string(destination, destination_size, used,
                                               entries[index].message) ||
            !management_log_json_append(destination, destination_size, used, "}"))
        {
            return false;
        }
    }
    return management_log_json_append(destination, destination_size, used, "]");
}
