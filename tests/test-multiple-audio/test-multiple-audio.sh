#!/bin/bash

. ../include.sh

tmpfile=$mypath/tmp_1_$$

trap "rm -f $tmpfile" 0

transform=$mypath/transforms/detectionfunction.n3 

urlbase=http://vamp-plugins.org/sonic-annotator/testfiles


# 1. Recursive local directory

# Note, the output here depends on all the audio files present -- we
# would have to regenerate it if we added more test audio files. Note
# that the -r flag is not supposed to pick up playlist files, only
# audio files
$r -t $transform -w csv --csv-stdout -r --summary-only $audiopath > $tmpfile 2>/dev/null || \
    fail "Fails to run transform $transform with recursive dir option"

expected=$mypath/expected/all-files
csvcompare $tmpfile $expected.csv || \
    faildiff "Output mismatch for transform $transform with summaries and recursive dir option" $tmpfile $expected.csv


# 2. Local playlist file referring to local audio files

# Here we strip any leading path from the audio file in the output,
# because the playlist reader will have resolved files to absolute
# paths and those will differ between systems
$r -t $transform -w csv --csv-stdout $audiopath/playlist.m3u --summary-only 2>/dev/null | sed 's,^"[^"]*/,",' > $tmpfile || \
    fail "Fails to run transform $transform with playlist input"

expected=$mypath/expected/playlist
csvcompare $tmpfile $expected.csv || \
    faildiff "Output mismatch for transform $transform with summaries and playlist input" $tmpfile $expected.csv


# 3. Multiple files supplied directly on command line

# Strip paths again, just so we can use the same output comparison
# file as above
$r -t $transform -w csv --csv-stdout $audiopath/3clicks.mp3 $audiopath/6clicks.ogg --summary-only 2>/dev/null | sed 's,^"[^"]*/,",' > $tmpfile || \
    fail "Fails to run transform $transform with 2-file input"

expected=$mypath/expected/playlist
csvcompare $tmpfile $expected.csv || \
    faildiff "Output mismatch for transform $transform with summaries and 2-file input" $tmpfile $expected.csv


# 4. Remote playlist file referring to remote audio files

$r -t $transform -w csv --csv-stdout $urlbase/playlist.m3u --summary-only 2>/dev/null | sed 's,^"[^"]*/,",' > $tmpfile || \
    fail "Fails to run transform $transform with remote playlist input"

expected=$mypath/expected/playlist
csvcompare $tmpfile $expected.csv || \
    faildiff "Output mismatch for transform $transform with summaries and remote playlist input" $tmpfile $expected.csv


# 5. Local playlist file referring to mixture of remote and local audio files

$r -t $transform -w csv --csv-stdout $audiopath/remote-playlist.m3u --summary-only 2>/dev/null | sed 's,^"[^"]*/,",' > $tmpfile || \
    fail "Fails to run transform $transform with playlist of remote files"

expected=$mypath/expected/playlist
csvcompare $tmpfile $expected.csv || \
    faildiff "Output mismatch for transform $transform with summaries and remote playlist input" $tmpfile $expected.csv


# 6. Multiple remote files supplied directly on command line

$r -t $transform -w csv --csv-stdout $urlbase/3clicks.mp3 $urlbase/6clicks.ogg --summary-only 2>/dev/null | sed 's,^"[^"]*/,",' > $tmpfile || \
    fail "Fails to run transform $transform with 2-file remote input"

expected=$mypath/expected/playlist
csvcompare $tmpfile $expected.csv || \
    faildiff "Output mismatch for transform $transform with summaries and 2-file input" $tmpfile $expected.csv


# 7. Mixture of remote and local files supplied on command line

$r -t $transform -w csv --csv-stdout $audiopath/3clicks.mp3 $urlbase/6clicks.ogg --summary-only 2>/dev/null | sed 's,^"[^"]*/,",' > $tmpfile || \
    fail "Fails to run transform $transform with 2-file remote input"

expected=$mypath/expected/playlist
csvcompare $tmpfile $expected.csv || \
    faildiff "Output mismatch for transform $transform with summaries and mixed local/remote 2-file input" $tmpfile $expected.csv

