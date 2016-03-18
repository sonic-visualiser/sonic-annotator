TEMPLATE = app

INCLUDEPATH += vamp-plugin-sdk

win32-g++ {
    INCLUDEPATH += sv-dependency-builds/win32-mingw/include
    LIBS += -Lsv-dependency-builds/win32-mingw/lib
}
win32-msvc* {
    INCLUDEPATH += sv-dependency-builds/win32-msvc/include
    LIBS += -Lsv-dependency-builds/win32-msvc/lib
}
mac* {
    INCLUDEPATH += sv-dependency-builds/osx/include
    LIBS += -Lsv-dependency-builds/osx/lib
}

exists(config.pri) {
    include(config.pri)
}

!exists(config.pri) {

    CONFIG += release
    DEFINES += NDEBUG BUILD_RELEASE NO_TIMING

    DEFINES += HAVE_BZ2 HAVE_FFTW3 HAVE_FFTW3F HAVE_SNDFILE HAVE_SAMPLERATE HAVE_DATAQUAY HAVE_MAD HAVE_ID3TAG

    LIBS += -lbz2 -lfftw3 -lfftw3f -lsndfile -lFLAC -logg -lvorbis -lvorbisenc -lvorbisfile -logg -lmad -lid3tag -lsamplerate -lz -lsord-0 -lserd-0

    win* {
        DEFINES += _USE_MATH_DEFINES
        LIBS += -lwinmm -lws2_32
    }
    macx* {
        DEFINES += HAVE_COREAUDIO
        LIBS += -framework CoreAudio -framework CoreMidi -framework AudioUnit -framework AudioToolbox -framework CoreFoundation -framework CoreServices -framework Accelerate
    }
    linux* {
        LIBS += -ldl
    }
}

CONFIG += qt thread warn_on stl rtti exceptions console c++11
QT += xml network
QT -= gui widgets

# Using the "console" CONFIG flag above should ensure this happens for
# normal Windows builds, but the console feature doesn't get picked up
# in my local cross-compile setup because qmake itself doesn't know to
# look for win32 features
win32-x-g++:QMAKE_LFLAGS += -Wl,-subsystem,console

DEFINES += HAVE_FFTW3 HAVE_VAMP HAVE_VAMPHOSTSDK

TARGET = sonic-annotator

DEPENDPATH += . svcore runner
INCLUDEPATH += . dataquay svcore runner

QMAKE_LIBDIR = svcore $$QMAKE_LIBDIR

QMAKE_CXXFLAGS_RELEASE += -fmessage-length=80 -fdiagnostics-show-location=every-line

OBJECTS_DIR = o
MOC_DIR = o

contains(DEFINES, BUILD_STATIC):LIBS -= -ljack

MY_LIBS = -Lsvcore -Ldataquay -lsvcore -ldataquay

linux* {
MY_LIBS = -Wl,-Bstatic $$MY_LIBS -Wl,-Bdynamic
}

win* {
MY_LIBS = -Lsvcore/release -Ldataquay/release $$MY_LIBS
}

LIBS = $$MY_LIBS $$LIBS

win* {
PRE_TARGETDEPS += svcore/release/libsvcore.a
}

!win* {
PRE_TARGETDEPS += svcore/libsvcore.a
}

HEADERS += \
        vamp-plugin-sdk/vamp-hostsdk/PluginBase.h \
        vamp-plugin-sdk/vamp-hostsdk/PluginBufferingAdapter.h \
        vamp-plugin-sdk/vamp-hostsdk/PluginChannelAdapter.h \
        vamp-plugin-sdk/vamp-hostsdk/Plugin.h \
        vamp-plugin-sdk/vamp-hostsdk/PluginHostAdapter.h \
        vamp-plugin-sdk/vamp-hostsdk/PluginInputDomainAdapter.h \
        vamp-plugin-sdk/vamp-hostsdk/PluginLoader.h \
        vamp-plugin-sdk/vamp-hostsdk/PluginSummarisingAdapter.h \
        vamp-plugin-sdk/vamp-hostsdk/PluginWrapper.h \
        vamp-plugin-sdk/vamp-hostsdk/RealTime.h \
        vamp-plugin-sdk/src/vamp-hostsdk/Window.h \
	runner/AudioDBFeatureWriter.h \
        runner/FeatureWriterFactory.h  \
        runner/DefaultFeatureWriter.h \
        runner/FeatureExtractionManager.h \
        runner/JAMSFeatureWriter.h \
        runner/LabFeatureWriter.h \
        runner/MIDIFeatureWriter.h \
        runner/MultiplexedReader.h

SOURCES += \
        vamp-plugin-sdk/src/vamp-hostsdk/PluginBufferingAdapter.cpp \
        vamp-plugin-sdk/src/vamp-hostsdk/PluginChannelAdapter.cpp \
        vamp-plugin-sdk/src/vamp-hostsdk/PluginHostAdapter.cpp \
        vamp-plugin-sdk/src/vamp-hostsdk/PluginInputDomainAdapter.cpp \
        vamp-plugin-sdk/src/vamp-hostsdk/PluginLoader.cpp \
        vamp-plugin-sdk/src/vamp-hostsdk/PluginSummarisingAdapter.cpp \
        vamp-plugin-sdk/src/vamp-hostsdk/PluginWrapper.cpp \
        vamp-plugin-sdk/src/vamp-hostsdk/RealTime.cpp \
        vamp-plugin-sdk/src/vamp-hostsdk/Files.cpp \
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

