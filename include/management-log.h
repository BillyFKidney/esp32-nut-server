#pragma once

#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#define MANAGEMENT_LOG_ENTRY_CAPACITY 24U
#define MANAGEMENT_LOG_STATUS_WINDOW 6U
#define MANAGEMENT_LOG_MESSAGE_LENGTH 192U

typedef struct
{
    uint64_t uptime_ms;
    time_t epoch_seconds;
    char level;
    char message[MANAGEMENT_LOG_MESSAGE_LENGTH];
} ManagementLogSnapshotEntry;

const char *management_log_level_name(char level);

/** Install the bounded runtime log capture used by the authenticated console. */
void management_log_capture_start(void);

/** Capture an embedded NUT syslog message without writing it to storage. */
void management_log_capture_syslog(int priority, const char *format,
                                   va_list arguments);

/**
 * Copy the volatile retained management-log ring in oldest-to-newest order.
 * The caller must provide space for MANAGEMENT_LOG_ENTRY_CAPACITY entries.
 */
bool management_log_copy_snapshot(ManagementLogSnapshotEntry *entries,
                                  size_t entries_capacity, size_t *entry_count);

/** Append the most recent bounded log entries to a JSON response buffer. */
bool management_log_append_snapshot(char *destination, size_t destination_size,
                                    size_t *used);
