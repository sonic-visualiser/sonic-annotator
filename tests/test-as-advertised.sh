#!/bin/bash

mypath=`dirname $0`
r=$mypath/../runner/sonic-annotator

infile=$mypath/audio/3clicks8.wav
testplug=vamp:vamp-example-plugins:percussiononsets
tmpdir=$mypath/tmp_1_$$.dir
tmpwav=$tmpdir/test.wav

trap "rm -rf $tmpdir" 0

fail() {
    echo "Test failed: $1"
    exit 1
}

types=`\
    $r --help 2>&1 | \
    grep 'Supported writer types are:' | \
    sed -e 's/^.*://' -e 's/[,\.]//g' \
    `
[ -n "$types" ] || \
    fail "Fails to report sensible list of writers in help text?"

onsets=$mypath/transforms/transforms-as-advertised-percussiononsets-onsets.n3
df=$mypath/transforms/transforms-as-advertised-percussiononsets-detectionfunction.n3

adbdir=$tmpdir/audiodb-test
mkdir -p $adbdir

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

    case $type in
	audiodb) 
	    $r -t $df -w $type $tmpwav --audiodb-basedir $tmpdir --audiodb-catid `basename $adbdir` 2>/dev/null || \
		fail "Fails to run with reader type \"$type\" and default options"
	    ;;
	default) 
	    $r -t $onsets -w $type $tmpwav > $tmpdir/test.out 2>/dev/null || \
		fail "Fails to run with reader type \"$type\" and default options"
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

