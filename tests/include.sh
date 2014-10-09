
mypath=`dirname $0`

testdir=$mypath/..
r=$testdir/../sonic-annotator

audiopath=$testdir/audio

percplug=vamp:vamp-example-plugins:percussiononsets
amplplug=vamp:vamp-example-plugins:amplitudefollower
testplug=vamp:vamp-test-plugin:vamp-test-plugin

fail() {
    echo "Test failed: $1"
    exit 1
}

csvcompare() {
    # permit some fuzz in final few digits
    a="$1"
    b="$2"
    perl -p -e 's/(\d+\.\d{6})\d+/$1/' "$a" > "${a}__"
    perl -p -e 's/(\d+\.\d{6})\d+/$1/' "$b" > "${b}__"
    cmp -s "${a}__" "${b}__"
    rv=$?
    rm "${a}__" "${b}__"
    return $rv
}

csvcompare_ignorefirst() {
    # a bit like the above, but ignoring first column (and without temp files)
    out=`cat "$1" "$2" | cut -d, -f2- | perl -p -e 's/(\d+\.\d{6})\d+/$1/' | sort | uniq -c | grep -v ' 2 '`
    return `[ -z "$out" ]`
}

faildiff() {
    echo "Test failed: $1"
    if [ -n "$2" -a -n "$3" ]; then
	echo "Output follows:"
	echo "--"
	cat $2
	echo "--"
	echo "Expected output follows:"
	echo "--"
	cat $3
	echo "--"
	echo "Diff:"
	echo "--"
	sdiff -w78 $2 $3
	echo "--"
    fi
    exit 1
}


