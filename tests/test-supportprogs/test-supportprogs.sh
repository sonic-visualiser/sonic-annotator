#!/bin/bash

fail() {
    echo "Test failed: $1"
    exit 1
}

xmllint --version 2>/dev/null || \
    fail "Can't find required xmllint program"

rapper --version >/dev/null || \
    fail "Can't find required rapper program"

uconv --version >/dev/null || \
    fail "Can't find required uconv program"

echo '{}' | json_verify >/dev/null || \
    fail "Can't find required json_verify program, or it doesn't seem to work"

exit 0

