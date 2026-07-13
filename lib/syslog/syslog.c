#include "syslog.h"

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
	(void)priority;
	(void)format;
}

void vsyslog (int priority, const char *format, va_list arguments)
{
	(void)priority;
	(void)format;
	(void)arguments;
}
