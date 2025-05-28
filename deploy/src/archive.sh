#!/bin/bash

set -eu

tag=`git tag --list --sort=creatordate | grep '^sonic-annotator-' | tail -1 | awk '{ print $1; }'`

v=`echo "$tag" | sed 's/sonic-annotator-//' | sed 's/_.*$//'`

echo -n "Package up source code for version $v from tag $tag [Yn] ? "
read yn
case "$yn" in "") ;; [Yy]) ;; *) exit 3;; esac
echo "Proceeding"

case $(git status --porcelain --untracked-files=no) in
    "") ;;
    *) echo "ERROR: Current working copy has been modified - unmodified copy required so we can update to tag and back again safely"; exit 2;;
esac

echo
echo -n "Packaging up version $v from tag $tag... "

current=$(git rev-parse --short HEAD)

mkdir -p packages

git checkout "$tag"

./repoint archive "$(pwd)"/packages/sonic-annotator-"$v".tar.gz --exclude sv-dependency-builds .gitignore .github .hgignore .appveyor.yml .hgtags

git checkout "$current"

echo Done
echo
