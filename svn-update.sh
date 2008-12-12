#!/bin/sh

svn update || exit

for x in audioio base data plugin rdf system transform; do
    svn update $x || exit
done

