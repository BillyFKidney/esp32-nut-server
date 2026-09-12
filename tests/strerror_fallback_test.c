/* Verify the inherited strerror fallback under an explicitly forced host build. */

#include <errno.h>
#include <stdio.h>
#include <string.h>

char *nut_fallback_strerror(int errnum);

static int check_text(const char *name, int value, const char *expected)
{
    const char *actual = nut_fallback_strerror(value);
    if (strcmp(actual, expected) == 0)
    {
        return 0;
    }

    fprintf(stderr, "%s: expected '%s', got '%s'\n", name, expected, actual);
    return 1;
}

int main(void)
{
    int failed = 0;
    failed |= check_text("EACCES", EACCES, "Permission denied");

#if defined(EWOULDBLOCK)
    failed |= check_text("EWOULDBLOCK", EWOULDBLOCK, "Operation would block");
#endif

#if defined(EAGAIN) && (!defined(EWOULDBLOCK) || EAGAIN != EWOULDBLOCK)
    failed |= check_text("EAGAIN", EAGAIN, "No more processes");
#endif

    char *first = nut_fallback_strerror(-1);
    failed |= check_text("unknown errno", -2, "Error -2");
    if (first != nut_fallback_strerror(-3))
    {
        fprintf(stderr, "unknown errno fallback did not retain static-buffer semantics\n");
        failed = 1;
    }

    return failed;
}
