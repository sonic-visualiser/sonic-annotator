TEMPLATE = subdirs
SUBDIRS = sub_dataquay svcore sub_runner

!win* {
    # We should build and run the tests on any platform,
    # but doing it automatically doesn't work so well from
    # within an IDE on Windows, so remove that from here
    SUBDIRS += svcore/data/fileio/test
}

sub_dataquay.file = dataquay/lib.pro

sub_runner.file = runner.pro
sub_runner.depends = svcore
