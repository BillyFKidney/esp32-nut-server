#!/bin/sh
# Compile and exercise strerror.c as if the platform libc lacks strerror().

set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
temporary_directory=$(mktemp -d "${TMPDIR:-/tmp}/nut-strerror-test.XXXXXX")
trap 'rm -rf "$temporary_directory"' EXIT HUP INT TERM

cat > "$temporary_directory/config.h" <<'EOF'
#define HAVE_STDIO_H 1
EOF

cc -std=c11 -Wall -Wextra -Werror -I"$temporary_directory" \
    -Dstrerror=nut_fallback_strerror \
    "$root/src/common/strerror.c" "$root/tests/strerror_fallback_test.c" \
    -o "$temporary_directory/strerror_fallback_test"

"$temporary_directory/strerror_fallback_test"
