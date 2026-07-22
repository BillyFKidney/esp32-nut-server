#include "syslog.h"

#include "management.h"

void closelog (void) {}

void openlog (const char *ident, int option, int facility)
{
	(void)ident;
	(void)option;
	(void)facility;
}

int setlogmask (int mask)
{
	(void)mask;
	return 0;
}

void syslog (int priority, const char *format, ...)
{
	va_list arguments;
	va_start(arguments, format);
	management_log_capture_syslog(priority, format, arguments);
	va_end(arguments);
}

void vsyslog (int priority, const char *format, va_list arguments)
{
	management_log_capture_syslog(priority, format, arguments);
}
