#!/bin/bash

fail() {
    echo "Test failed: $1"
    exit 1
}

xmllint --version 2>/dev/null || \
    fail "Can't find required xmllint program"

rapper --version >/dev/null || \
    fail "Can't find required rapper program"

exit 0

