#!/bin/bash
set -eu
if [ ! -d deploy/osx ]; then
    echo "This must be run from the project root"
    exit 1
fi

qtversion="6.6.3"
qtsuffix="-static"

for arch in arm64 x86_64 ; do

    echo
    echo "For arch: $arch"
    echo

    qarch=arm64
    if [ "$arch" = "x86_64" ]; then
	qarch=amd64
    fi
    
    export QTDIR="$HOME/Qt/$qtversion$qtsuffix"
    export PATH=/usr/local/$arch/bin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin:$QTDIR/bin:$QTDIR/libexec

    fake_pkgconfig="$(pwd)/deploy/osx/fake-qt-pkg-config"
    mkdir -p "$fake_pkgconfig"

    for module in Core Xml Network Test; do
	library="Qt6$module"
	if [ ! -f "$QTDIR/lib/lib$library.a" ]; then
	    echo "Library corresponding to module $module not found in $QTDIR/lib/$library.a [sanity check, we don't actually use it]"
	    exit 1
	fi
	infile="$(pwd)/deploy/osx/skeleton.pc"
	outfile="$fake_pkgconfig/$library.pc"
	if [ ! -f "$infile" ]; then
	    echo "Failed to find input file $infile"
	    exit 1
	fi
	cat "$infile" |
	    sed -e 's|@qtdir@|'"$QTDIR"'|' \
		-e 's|@version@|'"$qtversion"'|' \
		-e 's|@library@|'"$library"'|' \
		-e 's|@module@|'"$module"'|' \
		> "$outfile"
    done

    export PKG_CONFIG_PATH="$fake_pkgconfig"

    buildprefix=build
    
    dir="$buildprefix"_"$arch"

    rm -rf "$dir"
    buildtype=release
    further_args=""

    meson setup "$dir" -Dstatic_qtdir="$QTDIR" -Dbuildtype="$buildtype" $further_args --cross-file ./deploy/cross/macos-"$arch".txt

done
