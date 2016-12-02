
TEMPLATE = app

exists(config.pri) {
    include(config.pri)
}

!exists(config.pri) {
    include(noconfig.pri)
}

include(base.pri)

CONFIG += console
QT += xml network
QT -= gui

win32-x-g++:QMAKE_LFLAGS += -Wl,-subsystem,console
macx*: CONFIG -= app_bundle

TARGET = sonic-annotator

OBJECTS_DIR = o
MOC_DIR = o

HEADERS += \
	runner/AudioDBFeatureWriter.h \
        runner/FeatureWriterFactory.h  \
        runner/DefaultFeatureWriter.h \
        runner/FeatureExtractionManager.h \
        runner/JAMSFeatureWriter.h \
        runner/LabFeatureWriter.h \
        runner/MIDIFeatureWriter.h \
        runner/MultiplexedReader.h

SOURCES += \
	runner/main.cpp \
	runner/DefaultFeatureWriter.cpp \
	runner/FeatureExtractionManager.cpp \
        runner/AudioDBFeatureWriter.cpp \
        runner/FeatureWriterFactory.cpp \
        runner/JAMSFeatureWriter.cpp \
        runner/LabFeatureWriter.cpp \
        runner/MIDIFeatureWriter.cpp \
        runner/MultiplexedReader.cpp

!win32 {
    QMAKE_POST_LINK=/bin/bash tests/test.sh
}

