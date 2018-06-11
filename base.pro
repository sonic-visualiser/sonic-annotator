
TEMPLATE = lib

exists(config.pri) {
    include(config.pri)
}

!exists(config.pri) {
    include(noconfig.pri)
}

include(base.pri)

CONFIG += staticlib
QT += network xml
QT -= gui

TARGET = base

OBJECTS_DIR = o
MOC_DIR = o

include(bq-files.pri)
include(vamp-plugin-sdk-files.pri)
include(svcore/files.pri)

DATAQUAY_SOURCES=$$fromfile(dataquay/lib.pro, SOURCES)
DATAQUAY_HEADERS=$$fromfile(dataquay/lib.pro, HEADERS)

for (file, BQ_SOURCES)       { SOURCES += $$file }
for (file, BQ_HEADERS)       { HEADERS += $$file }

for (file, VAMP_SOURCES)     { SOURCES += $$file }
for (file, VAMP_HEADERS)     { HEADERS += $$file }

for (file, DATAQUAY_SOURCES) { SOURCES += $$sprintf("dataquay/%1", $$file) }
for (file, DATAQUAY_HEADERS) { HEADERS += $$sprintf("dataquay/%1", $$file) }

for (file, SVCORE_SOURCES)   { SOURCES += $$sprintf("svcore/%1", $$file) }
for (file, SVCORE_HEADERS)   { HEADERS += $$sprintf("svcore/%1", $$file) }
