#!/bin/bash

fail() {
    echo "Test failed: $1"
    exit 1
}

xmllint --version 2>/dev/null || \
    fail "Can't find required xmllint program (from libxml2 distribution)"

rapper --version >/dev/null || \
    fail "Can't find required rapper program (from raptor/redland distribution)"

iconv --version >/dev/null || \
    fail "Can't find required iconv program (usually associated with glibc or libiconv)"

echo '{}' | json_verify >/dev/null || \
    fail "Can't find required json_verify program (from yajl distribution), or it doesn't seem to work"

echo '{}' | json_reformat >/dev/null || \
    fail "Can't find required json_reformat program (from yajl distribution), or it doesn't seem to work"

exit 0

