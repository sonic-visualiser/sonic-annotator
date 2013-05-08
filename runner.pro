TEMPLATE = app

include(config.pri)

CONFIG += qt thread warn_on stl rtti exceptions console
QT += xml network
QT -= gui widgets

# Using the "console" CONFIG flag above should ensure this happens for
# normal Windows builds, but the console feature doesn't get picked up
# in my local cross-compile setup because qmake itself doesn't know to
# look for win32 features
win32-x-g++:QMAKE_LFLAGS += -Wl,-subsystem,console

# If you have compiled your Vamp plugin SDK with FFTW (using its
# HAVE_FFTW3 flag), you can define the same flag here to ensure the
# program saves and restores FFTW wisdom in its configuration properly
#
#DEFINES += HAVE_FFTW3

TARGET = sonic-annotator

DEPENDPATH += . svcore
INCLUDEPATH += . dataquay svcore

QMAKE_LIBDIR = svcore $$QMAKE_LIBDIR

QMAKE_CXXFLAGS_RELEASE += -fmessage-length=80 -fdiagnostics-show-location=every-line

OBJECTS_DIR = o
MOC_DIR = o

contains(DEFINES, BUILD_STATIC):LIBS -= -ljack

MY_LIBS = -Lsvcore -Ldataquay -lsvcore -ldataquay

linux* {
MY_LIBS = -Wl,-Bstatic $$MY_LIBS -Wl,-Bdynamic
}

LIBS = $$MY_LIBS $$LIBS

PRE_TARGETDEPS += svcore/libsvcore.a

HEADERS += \
	runner/AudioDBFeatureWriter.h \
        runner/FeatureWriterFactory.h  \
        runner/DefaultFeatureWriter.h \
        runner/FeatureExtractionManager.h

SOURCES += \
	runner/main.cpp \
	runner/DefaultFeatureWriter.cpp \
	runner/FeatureExtractionManager.cpp \
        runner/AudioDBFeatureWriter.cpp \
        runner/FeatureWriterFactory.cpp

!win32 {
    QMAKE_POST_LINK=/bin/bash tests/test.sh
}

