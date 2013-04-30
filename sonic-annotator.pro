TEMPLATE = subdirs
SUBDIRS = dataquay svcore svcore/data/fileio/test sub_runner

sub_runner.file = runner.pro
sub_runner.depends = svcore
