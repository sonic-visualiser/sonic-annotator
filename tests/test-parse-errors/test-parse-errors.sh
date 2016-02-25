#!/bin/bash

. ../include.sh

infile=$audiopath/3clicks8.wav
tmpfile=$mypath/tmp_$$
trap "rm -f $tmpfile" 0

for transform in "$mypath"/inputs/* ; do

    base=$(basename "$transform")
    expected="$mypath"/expected/"$base".txt

    if [ ! -f "$expected" ]; then
	fail "Internal error: Expected file $expected not found for transform $transform"
    fi

    if $r -t "$transform" -w csv --csv-one-file /dev/null "$infile" 2>"$tmpfile" ; then
	fail "Erroneously succeeds in running bogus transform $transform"
    fi

    cat "$expected" | while read line; do
	if ! fgrep -q "$line" "$tmpfile" ; then
	    fail "Expected output text \"$line\" not found in diagnostic output for transform $base"
	fi
    done
    
done

