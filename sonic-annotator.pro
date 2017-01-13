TEMPLATE = subdirs

# We build the tests on every platform, though at the time of
# writing they are only automatically run on non-Windows platforms
# (because of the difficulty of getting them running nicely in the
# IDE without causing great confusion if a test fails).
SUBDIRS += \
        sub_test_svcore_base \
        sub_test_svcore_data_fileio

SUBDIRS += sub_runner

sub_test_svcore_base.file = test-svcore-base.pro
sub_test_svcore_data_fileio.file = test-svcore-data-fileio.pro

sub_runner.file = runner.pro

CONFIG += ordered

