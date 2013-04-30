#!/bin/bash

mypath=`dirname $0`
r=$mypath/../sonic-annotator

testplug=vamp:vamp-example-plugins:percussiononsets
testplug2=vamp:vamp-test-plugin:vamp-test-plugin

fail() {
    echo "Test failed: $1"
    exit 1
}

$r >/dev/null 2>&1 && \
    fail "Return code 0 when run without args (should be a failure code)"

$r 2>&1 >/dev/null | grep -q "for help" || \
    fail "Improper response when run without args"

$r --help 2>&1 | grep -q Copy || \
    fail "Expected help not printed when run with --help"

$r --list >/dev/null 2>&1 || \
    fail "Fails to run with --list"

$r --list 2>/dev/null | grep -q $testplug || \
    fail "Fails to print $testplug in plugin list (if you haven't got it, install it -- it's needed for other tests)"

$r --list 2>/dev/null | grep -q $testplug2 || \
    fail "Fails to print $testplug2 in plugin list (if you haven't got it, install it -- it's needed for other tests)"

$r --skeleton $testplug >/dev/null || \
    fail "Fails to run with --skeleton $testplug"

$r -s $testplug >/dev/null || \
    fail "Fails to run with -s $testplug"

$r --skeleton $testplug >/dev/null || \
    fail "Fails to run with --skeleton $testplug"

$r --skeleton $testplug | rapper -i turtle - test >/dev/null 2>&1 || \
    fail "Invalid XML skeleton produced with --skeleton $testplug"

exit 0
