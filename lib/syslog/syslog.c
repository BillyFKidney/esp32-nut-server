#include "syslog.h"

void closelog (void) {}

void openlog (__const char *__ident, int __option, int __facility) {}

int setlogmask (int __mask) { return 0;}

void syslog (int __pri, __const char *__fmt, ...) {}
