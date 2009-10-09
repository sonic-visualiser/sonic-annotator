
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

