TEMPLATE = subdirs
SUBDIRS = svcore sub_runner

sub_runner.file = runner.pro
sub_runner.depends = svcore
