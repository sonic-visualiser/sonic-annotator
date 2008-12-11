#!/bin/bash

mypath=`dirname $0`
r=$mypath/../sonic-annotator

infile=$mypath/audio/3clicks8.wav
testplug=vamp:vamp-example-plugins:percussiononsets
tmpttl=$mypath/tmp_1_$$.ttl

trap "rm -f $tmpttl" 0

fail() {
    echo "Test failed: $1"
    exit 1
}

transformpfx=$mypath/transforms/transforms-rdf-writer-percussiononsets

for rdfarg in "" "--rdf-plain" ; do

    note=""
    [ -n "$rdfarg" ] && note=" with $rdfarg"

    rm -f $tmpttl

    $r -t $transformpfx-onsets.n3 -w rdf $infile $rdfarg --rdf-one-file $tmpttl 2>/dev/null || \
	fail "Fails to run with onsets transform and RDF writer$note"

    rapper -i turtle $tmpttl >/dev/null 2>&1 || \
	fail "Fails to produce parseable RDF/TTL for onsets transform$note"

    rapper -i turtle -c $tmpttl 2>&1 | egrep -q 'Parsing returned [1-9][0-9]+ triples' ||
	fail "RDF output contains no triples (?) for onsets transform$note"

    rm -f $tmpttl
    
    $r -t $transformpfx-detectionfunction.n3 -w rdf $infile $rdfarg --rdf-one-file $tmpttl 2>/dev/null || \
	fail "Fails to run with detectionfunction transform and RDF writer$note"

    rapper -i turtle $tmpttl >/dev/null 2>&1 || \
	fail "Fails to produce parseable RDF/TTL for detectionfunction transform$note"

    rapper -i turtle -c $tmpttl 2>&1 | egrep -q 'Parsing returned [1-9][0-9]+ triples' ||
	fail "RDF output contains no triples (?) for detectionfunction transform$note"

    rm -f $tmpttl

    $r -t $transformpfx-onsets.n3 -t $transformpfx-detectionfunction.n3 -w rdf $infile $rdfarg --rdf-one-file $tmpttl 2>/dev/null || \
	fail "Fails to run with detectionfunction and onsets transforms together and RDF writer$note"

    rapper -i turtle $tmpttl >/dev/null 2>&1 || \
	fail "Fails to produce parseable RDF/TTL for detectionfunction and onsets transforms together$note"

    rapper -i turtle -c $tmpttl 2>&1 | egrep -q 'Parsing returned [1-9][0-9]+ triples' ||
	fail "RDF output contains no triples (?) for detectionfunction and onsets transforms together$note"

done

exit 0
