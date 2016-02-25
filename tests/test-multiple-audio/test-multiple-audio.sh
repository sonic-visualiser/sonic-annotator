#!/bin/bash

. ../include.sh

tmpfile1=$mypath/tmp_1_$$
tmpfile2=$mypath/tmp_2_$$

trap "rm -f $tmpfile1 $tmpfile2" 0

transform=$mypath/transforms/detectionfunction.n3 

urlbase=http://vamp-plugins.org/sonic-annotator/testfiles

have_network=yes
if ! ping -c 1 8.8.8.8 2>/dev/null ; then
    echo "(network appears unavailable, skipping networking tests)"
    have_network=no
fi


# 1. Recursive local directory

# Note, the output here depends on all the audio files present -- we
# would have to regenerate it if we added more test audio files. Note
# that the -r flag is not supposed to pick up playlist files, only
# audio files
$r -t $transform -w csv --csv-stdout -r --summary-only $audiopath > $tmpfile1 2>/dev/null || \
    fail "Fails to run transform $transform with recursive dir option"

expected=$mypath/expected/all-files
csvcompare $tmpfile1 $expected.csv || \
    faildiff "Output mismatch for transform $transform with summaries and recursive dir option" $tmpfile $expected.csv


# 2. Local playlist file referring to local audio files

# Here we strip any leading path from the audio file in the output,
# because the playlist reader will have resolved files to absolute
# paths and those will differ between systems
$r -t $transform -w csv --csv-stdout $audiopath/playlist.m3u --summary-only 2>/dev/null > "$tmpfile2" || \
    fail "Fails to run transform $transform with playlist input"

cat "$tmpfile2" | sed 's,^"[^"]*/,",' > "$tmpfile1"

expected=$mypath/expected/playlist
csvcompare $tmpfile1 $expected.csv || \
    faildiff "Output mismatch for transform $transform with summaries and playlist input" $tmpfile1 $expected.csv


# 3. Multiple files supplied directly on command line

# Strip paths again, just so we can use the same output comparison
# file as above
$r -t $transform -w csv --csv-stdout $audiopath/3clicks.mp3 $audiopath/6clicks.ogg --summary-only 2>/dev/null > $tmpfile2 || \
    fail "Fails to run transform $transform with 2-file input"

cat "$tmpfile2" | sed 's,^"[^"]*/,",' > "$tmpfile1"

expected=$mypath/expected/playlist
csvcompare $tmpfile1 $expected.csv || \
    faildiff "Output mismatch for transform $transform with summaries and 2-file input" $tmpfile1 $expected.csv


# 4. Multiple files supplied directly on command line, with file: URL

$r -t $transform -w csv --csv-stdout $audiopath/3clicks.mp3 file://`pwd`/$audiopath/6clicks.ogg --summary-only 2>/dev/null > $tmpfile2 || \
    fail "Fails to run transform $transform with 2-file input"

cat "$tmpfile2" | sed 's,^"[^"]*/,",' > "$tmpfile1"

expected=$mypath/expected/playlist
csvcompare $tmpfile1 $expected.csv || \
    faildiff "Output mismatch for transform $transform with summaries and 2-file input using file:// URL" $tmpfile1 $expected.csv


if [ "$have_network" = "yes" ]; then

    # 5. Remote playlist file referring to remote audio files

    $r -t $transform -w csv --csv-stdout $urlbase/playlist.m3u --summary-only 2>/dev/null > $tmpfile2 || \
	fail "Fails to run transform $transform with remote playlist input"

    cat "$tmpfile2" | sed 's,^"[^"]*/,",' > "$tmpfile1"

    expected=$mypath/expected/playlist
    csvcompare $tmpfile1 $expected.csv || \
	faildiff "Output mismatch for transform $transform with summaries and remote playlist input" $tmpfile1 $expected.csv


    # 6. Local playlist file referring to mixture of remote and local audio files

    $r -t $transform -w csv --csv-stdout $audiopath/remote-playlist.m3u --summary-only 2>/dev/null > $tmpfile2 || \
	fail "Fails to run transform $transform with playlist of remote files"

    cat "$tmpfile2" | sed 's,^"[^"]*/,",' > "$tmpfile1"

    expected=$mypath/expected/playlist
    csvcompare $tmpfile1 $expected.csv || \
	faildiff "Output mismatch for transform $transform with summaries and remote playlist input" $tmpfile1 $expected.csv


    # 7. Multiple remote files supplied directly on command line

    $r -t $transform -w csv --csv-stdout $urlbase/3clicks.mp3 $urlbase/6clicks.ogg --summary-only 2>/dev/null > $tmpfile2 || \
	fail "Fails to run transform $transform with 2-file remote input"

    cat "$tmpfile2" | sed 's,^"[^"]*/,",' > "$tmpfile1"

    expected=$mypath/expected/playlist
    csvcompare $tmpfile1 $expected.csv || \
	faildiff "Output mismatch for transform $transform with summaries and 2-file input" $tmpfile1 $expected.csv


    # 8. Mixture of remote and local files supplied on command line

    $r -t $transform -w csv --csv-stdout $audiopath/3clicks.mp3 $urlbase/6clicks.ogg --summary-only 2>/dev/null > $tmpfile2 || \
	fail "Fails to run transform $transform with 2-file remote input"

    cat "$tmpfile2" | sed 's,^"[^"]*/,",' > "$tmpfile1"

    expected=$mypath/expected/playlist
    csvcompare $tmpfile1 $expected.csv || \
	faildiff "Output mismatch for transform $transform with summaries and mixed local/remote 2-file input" $tmpfile1 $expected.csv

fi

# 9. As 3, but multiplexing rather than extracting separately from each file

$r -t $transform --multiplex -w csv --csv-stdout $audiopath/3clicks.mp3 $audiopath/6clicks.ogg --summary-only 2>/dev/null > $tmpfile2 || \
    fail "Fails to run transform $transform with 2-file input"

cat "$tmpfile2" | sed 's,^"[^"]*/,",' > "$tmpfile1"

expected=$mypath/expected/multiplexed
csvcompare $tmpfile1 $expected.csv || \
    faildiff "Output mismatch for transform $transform with summaries and 2-file multiplexed input" $tmpfile1 $expected.csv

