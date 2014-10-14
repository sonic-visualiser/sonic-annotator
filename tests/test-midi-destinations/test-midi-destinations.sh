#!/bin/bash

. ../include.sh

infile1=$audiopath/3clicks8.wav
infile2=$audiopath/6clicks8.wav

infile1dot=$audiopath/3.clicks.8.wav

outfile1=3clicks8.mid
outfile2=6clicks8.mid

outfile3=3clicks8_vamp_vamp-example-plugins_percussiononsets_onsets.mid
outfile4=3clicks8_vamp_vamp-example-plugins_percussiononsets_detectionfunction.mid
outfile5=6clicks8_vamp_vamp-example-plugins_percussiononsets_onsets.mid
outfile6=6clicks8_vamp_vamp-example-plugins_percussiononsets_detectionfunction.mid

outfile1dot=3.clicks.8.mid

tmpmid=$mypath/tmp_1_$$.mid

trap "rm -f $tmpmid $outfile1 $outfile2 $outfile3 $outfile4 $outfile5 $outfile6 $infile1dot $outfile1dot $audiopath/$outfile1 $audiopath/$outfile2 $audiopath/$outfile3 $audiopath/$outfile4 $audiopath/$outfile5 $audiopath/$outfile6 $audiopath/$outfile1dot" 0

transformdir=$mypath/transforms

check_midi() {
    test -f $1 || \
	fail "Fails to write output to expected location $1 for $2"
    case $(strings $1 | head -2 | fmt -80) in
	MThd\ MTrk) ;;
	*) fail "MIDI output does not look like MIDI in $2";;
    esac
    rm -f $1
}    


ctx="onsets transform, one audio file, default MIDI writer destination"

rm -f $audiopath/$outfile1

$r -t $transformdir/onsets.n3 -w midi $infile1 2>/dev/null || \
    fail "Fails to run with $ctx"

check_midi $audiopath/$outfile1 "$ctx"


ctx="onsets transform, one audio file with dots in filename, default MIDI writer destination"

rm -f $audiopath/$outfile1

cp $infile1 $infile1dot

$r -t $transformdir/onsets.n3 -w midi $infile1dot 2>/dev/null || \
    fail "Fails to run with $ctx"

check_midi $audiopath/$outfile1dot "$ctx"

rm -f $infile1dot $audiopath/$outfile1dot


ctx="onsets and df transforms, one audio file, default MIDI writer destination"

rm -f $audiopath/$outfile1

$r -t $transformdir/onsets.n3 -t $transformdir/detectionfunction.n3 -w midi $infile1 2>/dev/null || \
    fail "Fails to run with $ctx"

check_midi $audiopath/$outfile1 "$ctx"


ctx="onsets transform, two audio files, default MIDI writer destination"

rm -f $audiopath/$outfile1
rm -f $audiopath/$outfile2

$r -t $transformdir/onsets.n3 -w midi $infile1 $infile2 2>/dev/null || \
    fail "Fails to run with $ctx"

check_midi $audiopath/$outfile1 "$ctx"
check_midi $audiopath/$outfile2 "$ctx"


ctx="onsets transform, two audio files, one-file MIDI writer"

$r -t $transformdir/onsets.n3 -w midi --midi-one-file $tmpmid $infile1 $infile2 2>/dev/null || \
    fail "Fails to run with $ctx"

check_midi $tmpmid "$ctx"


ctx="onsets transform, two audio files, stdout MIDI writer"

# stdout writer is not supported for midi

$r -t $transformdir/onsets.n3 -w midi --midi-stdout $infile1 $infile2 2>/dev/null >$tmpmid && \
    fail "Fails by completing successfully with $ctx"


ctx="onsets transform, one audio file, many-files MIDI writer"

rm -f $audiopath/$outfile3

$r -t $transformdir/onsets.n3 -w midi --midi-many-files $infile1 2>/dev/null || \
    fail "Fails to run with $ctx"

check_midi $audiopath/$outfile3 "$ctx"


ctx="onsets transform, two audio files, many-files MIDI writer"

rm -f $audiopath/$outfile3
rm -f $audiopath/$outfile5

$r -t $transformdir/onsets.n3 -w midi --midi-many-files $infile1 $infile2 2>/dev/null || \
    fail "Fails to run with $ctx"

check_midi $audiopath/$outfile3 "$ctx"
check_midi $audiopath/$outfile5 "$ctx"


ctx="onsets and df transforms, two audio files, many-files MIDI writer"

rm -f $audiopath/$outfile3
rm -f $audiopath/$outfile4
rm -f $audiopath/$outfile5
rm -f $audiopath/$outfile6

$r -t $transformdir/onsets.n3 -t $transformdir/detectionfunction.n3 -w midi --midi-many-files $infile1 $infile2 2>/dev/null || \
    fail "Fails to run with $ctx"

check_midi $audiopath/$outfile3 "$ctx"
check_midi $audiopath/$outfile4 "$ctx"
check_midi $audiopath/$outfile5 "$ctx"
check_midi $audiopath/$outfile6 "$ctx"


ctx="output base directory"

rm -f ./$outfile1

$r -t $transformdir/onsets.n3 -t $transformdir/detectionfunction.n3 -w midi --midi-basedir . $infile1 2>/dev/null || \
    fail "Fails to run with $ctx"

check_midi ./$outfile1 "$ctx"


ctx="output base directory and many-files"

rm -f ./$outfile3
rm -f ./$outfile5

$r -t $transformdir/onsets.n3 -w midi --midi-basedir . --midi-many-files $infile1 $infile2 2>/dev/null || \
    fail "Fails to run with $ctx"

check_midi ./$outfile3 "$ctx"
check_midi ./$outfile5 "$ctx"


ctx="nonexistent output base directory"

$r -t $transformdir/onsets.n3 -w midi --midi-basedir ./DOES_NOT_EXIST $infile1 2>/dev/null && \
    fail "Fails with $ctx by completing successfully (should refuse and bail out)"


ctx="existing output file and no --midi-force"

touch $audiopath/$outfile1

$r -t $transformdir/onsets.n3 -w midi $infile1 2>/dev/null && \
    fail "Fails by completing successfully when output file already exists (should refuse and bail out)"


ctx="existing output file and --midi-force"

touch $audiopath/$outfile1

$r -t $transformdir/onsets.n3 -w midi --midi-force $infile1 2>/dev/null || \
    fail "Fails to run with $ctx"

check_midi $audiopath/$outfile1 "$ctx"


exit 0
