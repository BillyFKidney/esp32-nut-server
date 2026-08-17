/** @file unsetenv.c @brief Provide a portable unsetenv compatibility implementation. @see nut_common.h, proto.h, config.h */
/* unsetenv.c Jim Klimov <jimklimov+nut@gmail.com> */
#include "config.h" /* must be first */

#ifndef HAVE_UNSETENV
#include <stdlib.h>
#include <string.h>
#include "nut_common.h"
#include "proto.h"

int nut_unsetenv(const char *name)
{
	return setenv(name, "", 1);
}
#endif	/* !HAVE_UNSETENV */
