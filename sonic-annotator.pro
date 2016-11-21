TEMPLATE = subdirs

!win* {
    # We should build and run the tests on any platform,
    # but doing it automatically doesn't work so well from
    # within an IDE on Windows, so remove that from here
    SUBDIRS += \
	sub_test_svcore_base \
        sub_test_svcore_data_fileio
}

SUBDIRS += sub_runner

sub_test_svcore_base.file = test-svcore-base.pro
sub_test_svcore_data_fileio.file = test-svcore-data-fileio.pro

sub_runner.file = runner.pro


