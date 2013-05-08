TEMPLATE = subdirs
SUBDIRS = sub_dataquay svcore svcore/data/fileio/test sub_runner

sub_dataquay.file = dataquay/lib.pro

sub_runner.file = runner.pro
sub_runner.depends = svcore
