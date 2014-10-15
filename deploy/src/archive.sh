#!/bin/bash

tag=`hg tags | grep '^sonic-annotator-' | head -1 | awk '{ print $1; }'`

v=`echo "$tag" |sed 's/sonic-annotator-//'`

echo "Packaging up version $v from tag $tag..."

hg archive -r"$tag" --subrepos --exclude sv-dependency-builds /tmp/sonic-annotator-"$v".tar.gz

