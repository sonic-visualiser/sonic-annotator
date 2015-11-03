
set -e

mypath=$(dirname $0)

case "$(pwd)/$mypath" in
    *" "*)
	echo 1>&2
	echo "ERROR: Test scripts do not handle paths containing spaces (yes, I know)" 1>&2
	echo "(Path is: \"$(pwd)/$mypath\")" 1>&2
	exit 1;;
    *)
    ;;
esac

version=1.3
nextversion=1.4

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

midicompare() {
    a="$1"
    b="$2"
    od -c "$a" > "${a}__"
    od -c "$b" > "${b}__"
    cmp -s "${a}__" "${b}__"
    rv=$?
    rm "${a}__" "${b}__"
    return $rv
}

jsoncompare() {
    a="$1"
    b="$2"
    cmp -s "$a" "$b"
}

faildiff() {
    echo "Test failed: $1"
    if [ -n "$2" -a -n "$3" ]; then
	echo "Output follows:"
	echo "--"
	cat "$2"
	echo "--"
	echo "Expected output follows:"
	echo "--"
	cat "$3"
	echo "--"
	echo "Diff:"
	echo "--"
	sdiff -w78 "$2" "$3"
	echo "--"
    fi
    exit 1
}

faildiff_od() {
    echo "Test failed: $1"
    if [ -n "$2" -a -n "$3" ]; then
	echo "Output follows:"
	echo "--"
	od -c "$2"
	echo "--"
	echo "Expected output follows:"
	echo "--"
	od -c "$3"
	echo "--"
	echo "Diff:"
	echo "--"
	od -w8 -c "$3" > "${3}__"
	od -w8 -c "$2" | sdiff -w78 - "${3}__"
	rm "${3}__"
	echo "--"
    fi
    exit 1
}

failshow() {
    echo "Test failed: $1"
    if [ -n "$2" ]; then
	echo "Output follows:"
	echo "--"
	cat $2
	echo "--"
    fi
    exit 1
}	

check_json() {
    test -f $1 || \
	fail "Fails to write output to expected location $1 for $2"
    cat $1 | json_verify -q || \
	failshow "Writes invalid JSON to location $1 for $2" $1
    rm -f $1
}    


