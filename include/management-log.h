#pragma once

#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>

/** Install the bounded runtime log capture used by the authenticated console. */
void management_log_capture_start(void);

/** Capture an embedded NUT syslog message without writing it to storage. */
void management_log_capture_syslog(int priority, const char *format,
                                   va_list arguments);

/** Append the most recent bounded log entries to a JSON response buffer. */
bool management_log_append_snapshot(char *destination, size_t destination_size,
                                    size_t *used);
