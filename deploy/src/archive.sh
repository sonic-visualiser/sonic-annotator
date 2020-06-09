#!/bin/bash

tag=`hg tags | grep '^sonic-annotator-' | head -1 | awk '{ print $1; }'`

v=`echo "$tag" |sed 's/sonic-annotator-//'`

echo -n "Package up source code for version $v from tag $tag [Yn] ? "
read yn
case "$yn" in "") ;; [Yy]) ;; *) exit 3;; esac
echo "Proceeding"

current=$(hg id | awk '{ print $1; }')

case "$current" in
    *+) echo "ERROR: Current working copy has been modified - unmodified copy required so we can update to tag and back again safely"; exit 2;;
    *);;
esac
          
echo
echo -n "Packaging up version $v from tag $tag... "

mkdir -p packages

hg update -r"$tag"

./repoint archive "$(pwd)"/packages/sonic-annotator-"$v".tar.gz --exclude sv-dependency-builds repoint.pri

hg update -r"$current"

echo Done
echo
