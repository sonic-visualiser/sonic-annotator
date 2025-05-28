#!/bin/bash

set -e

version="$1"

if [ -z "$version" ]; then
    echo "Usage: $0 <version>"
    exit 2
fi

set -u

if [ ! -d deploy/osx ]; then
    echo "This must be run from the project root"
    exit 1
fi

deploy/osx/configure.sh

for arch in arm64 x86_64 ; do

    ninja -C build_"$arch"
    
done

dir="sonic-annotator-$version-macos"
mkdir -p "$dir"

lipo build_arm64/sonic-annotator build_x86_64/sonic-annotator -create -output "$dir"/sonic-annotator

deploy/osx/sign.sh "$dir"/sonic-annotator
deploy/osx/notarize.sh "$dir"/sonic-annotator

cp README.md CHANGELOG COPYING CITATION "$dir/"
tar cvzf "$dir.tar.gz" "$dir"
rm -rf "$dir"

mv "$dir.tar.gz" packages/
