#!/bin/bash

. ../include.sh

infile=$audiopath/3clicks8.wav
tmpdir=$mypath/tmp_1_$$.dir
tmpwav=$tmpdir/test.wav

trap "rm -rf $tmpdir" 0

types=`$r --list-writers`
[ -n "$types" ] || \
    fail "Fails to report list of writers"

onsets=$mypath/transforms/percussiononsets-onsets.n3
df=$mypath/transforms/percussiononsets-detectionfunction.n3

adbdir=$tmpdir/audiodb-test

for type in $types; do

    mkdir -p $tmpdir
    cp $infile $tmpwav

    # Some of these are special cases:
    #
    # * The "default" writer type always prints to stdout instead of
    # to a file.
    #
    # * The "audiodb" writer will not print any output for features
    # that have no values (but are only point events).  I don't know
    # how reasonable that is, but it's clearly intentional.  It also
    # writes to a subdirectory $basedir/$catid/$trackid.$output
    #
    # * The "json" reader has a mandatory --json-format parameter that
    # currently only accepts one argument ("jams"). It should fail if
    # run with any other value or without this parameter.

    case $type in
	audiodb) 
	    mkdir -p $adbdir
	    $r -t $df -w $type $tmpwav --audiodb-basedir $tmpdir --audiodb-catid `basename $adbdir` 2>/dev/null || \
		fail "Fails to run with reader type \"$type\" and default options"
	    ;;
	default) 
	    $r -t $onsets -w $type $tmpwav > $tmpdir/test.out 2>/dev/null || \
		fail "Fails to run with reader type \"$type\" and default options"
	    ;;
	json)
	    $r -t $onsets -w $type $tmpwav 2>/dev/null && \
		fail "Wrongly succeeds in running with reader type \"$type\" and default options"
	    $r -t $onsets -w $type $tmpwav --json-format blah 2>/dev/null && \
		fail "Wrongly succeeds in running with reader type \"$type\" and unknown json-format option"
	    $r -t $onsets -w $type $tmpwav --json-format 2>/dev/null && \
		fail "Wrongly succeeds in running with reader type \"$type\" and empty json-format option"
	    $r -t $onsets -w $type $tmpwav --json-format jams 2>/dev/null || \
		fail "Fails to run with reader type \"$type\" and correct json-format option"
	    ;;
	*)
	    $r -t $onsets -w $type $tmpwav 2>/dev/null || \
		fail "Fails to run with reader type \"$type\" and default options"
	    ;;
    esac
    newfiles=`ls $tmpdir | fgrep -v .wav`
    if [ "$type" = audiodb ]; then newfiles=`ls $adbdir`; fi

    [ -n "$newfiles" ] || \
	fail "Fails to create output file for reader \"$type\" with default options"

    case `echo $newfiles | wc -w` in
	[2-9])
	if [ "$type" != audiodb ]; then
	    fail "Produces more than one output file for reader \"$type\" with default options"
	fi
	;;
	1)
	if [ "$type" = audiodb ]; then
	    fail "Produces only one output file for reader \"$type\" with default options (expected two)"
	fi
	;;
    esac

    rm -r $tmpdir
done

exit 0

