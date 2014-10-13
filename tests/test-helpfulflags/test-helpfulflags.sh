#!/bin/bash

. ../include.sh

$r >/dev/null 2>&1 && \
    fail "Return code 0 when run without args (should be a failure code)"

$r 2>&1 >/dev/null | grep -q "for help" || \
    fail "Improper response when run without args"

$r --help 2>&1 | grep -q Copy || \
    fail "Expected help not printed when run with --help"

$r --list >/dev/null 2>&1 || \
    fail "Fails to run with --list"

$r --list 2>/dev/null | grep -q $percplug || \
    fail "Fails to print $percplug in plugin list (if you haven't got it, install it -- it's needed for other tests)"

$r --list 2>/dev/null | grep -q $testplug || \
    fail "Fails to print $testplug in plugin list (if you haven't got it, install it -- it's needed for other tests)"

$r --skeleton $percplug >/dev/null || \
    fail "Fails to run with --skeleton $percplug"

$r -s $percplug >/dev/null || \
    fail "Fails to run with -s $percplug"

$r --skeleton $percplug >/dev/null || \
    fail "Fails to run with --skeleton $percplug"

$r --skeleton $percplug | rapper -i turtle - test >/dev/null 2>&1 || \
    fail "Invalid XML skeleton produced with --skeleton $percplug"

$r --minversion $version || \
    fail "Returned failure code when run with --minversion $version"

$r --minversion $nextversion 2>/dev/null && \
    fail "Returned success code when run with --minversion $nextversion"

$r --minversion 63.9 2>/dev/null && \
    fail "Returned success code when run with --minversion 63.9"

exit 0
