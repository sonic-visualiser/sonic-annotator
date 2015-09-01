#!/bin/bash

. ../include.sh

infile=$audiopath/3clicks8.wav
tmpttl=$mypath/tmp_1_$$.ttl

silentfile=$audiopath/20sec-silence.wav

trap "rm -f $tmpttl" 0

transformdir=$mypath/transforms

for rdfarg in "" "--rdf-plain" ; do

    note=""
    [ -n "$rdfarg" ] && note=" with $rdfarg"

    rm -f "$tmpttl"

    $r -t $transformdir/onsets.n3 -w rdf $infile $rdfarg --rdf-one-file "$tmpttl" 2>/dev/null || \
	fail "Fails to run with onsets transform and RDF writer$note"

    rapper -i turtle "$tmpttl" >/dev/null 2>&1 || \
	fail "Fails to produce parseable RDF/TTL for onsets transform$note"

    rapper -i turtle -c "$tmpttl" 2>&1 | egrep -q 'Parsing returned [1-9][0-9]+ triples' ||
	fail "RDF output contains no triples (?) for onsets transform$note"

    rm -f "$tmpttl"
    
    $r -t $transformdir/detectionfunction.n3 -w rdf $infile $rdfarg --rdf-one-file "$tmpttl" 2>/dev/null || \
	fail "Fails to run with detectionfunction transform and RDF writer$note"

    rapper -i turtle "$tmpttl" >/dev/null 2>&1 || \
	fail "Fails to produce parseable RDF/TTL for detectionfunction transform$note"

    rapper -i turtle -c "$tmpttl" 2>&1 | egrep -q 'Parsing returned [1-9][0-9]+ triples' ||
	fail "RDF output contains no triples (?) for detectionfunction transform$note"

    rm -f "$tmpttl"

    $r -t $transformdir/onsets.n3 -t $transformdir/detectionfunction.n3 -w rdf $infile $rdfarg --rdf-one-file "$tmpttl" 2>/dev/null || \
	fail "Fails to run with detectionfunction and onsets transforms together and RDF writer$note"

    rapper -i turtle "$tmpttl" >/dev/null 2>&1 || \
	fail "Fails to produce parseable RDF/TTL for detectionfunction and onsets transforms together$note"

    rapper -i turtle -c "$tmpttl" 2>&1 | egrep -q 'Parsing returned [1-9][0-9]+ triples' ||
	fail "RDF output contains no triples (?) for detectionfunction and onsets transforms together$note"

    # And the various structures emitted by the Vamp test plugin
    for output in instants curve-oss curve-fsr curve-fsr-timed curve-vsr grid-oss grid-fsr notes-regions; do

	$r -d "$testplug:$output" -w rdf --rdf-one-file "$tmpttl" --rdf-force "$silentfile" 2>/dev/null || \
	fail "Failed to run for plugin $testplug with output $output"

	rapper -i turtle "$tmpttl" >/dev/null 2>&1 || \
	    fail "Fails to produce parseable RDF/TTL for test plugin output $output"

	rapper -i turtle -c "$tmpttl" 2>&1 | egrep -q 'Parsing returned [1-9][0-9]+ triples' ||
	    fail "RDF output contains no triples (?) for test plugin output $output"
    done
done

# Check the output encoding -- should be valid UTF-8 always

for code in ucs-2 iso-8859-1; do
    $r -t $transformdir/onsets.n3 -w rdf --rdf-stdout $audiopath/id3v2-$code.mp3 2>/dev/null | iconv --from-code=utf-8 --to-code=utf-8 >/dev/null ||
	fail "RDF/Turtle output from $code input is not valid utf-8 according to iconv"
done

exit 0
