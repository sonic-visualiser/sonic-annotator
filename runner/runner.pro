
TEMPLATE = app

SV_UNIT_PACKAGES = vamp vamp-hostsdk samplerate mad id3tag oggz fishsound sndfile lrdf redland rasqal raptor

#linux-g++:LIBS += -Wl,-Bstatic
#linux-g++:DEFINES += BUILD_STATIC

load(../sonic-visualiser/sv.prf)

LIBPATH += /usr/local/lib

CONFIG += sv qt thread warn_on stl rtti exceptions console
QT += xml network
QT -= gui

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

DEPENDPATH += . ../sonic-visualiser i18n main
INCLUDEPATH += . ../sonic-visualiser main
LIBPATH = ../sonic-visualiser/audioio ../sonic-visualiser/data ../sonic-visualiser/plugin ../sonic-visualiser/rdf ../sonic-visualiser/transform ../sonic-visualiser/base ../sonic-visualiser/system $$LIBPATH

QMAKE_CXXFLAGS_RELEASE += -fmessage-length=80 -fdiagnostics-show-location=every-line

contains(DEFINES, BUILD_STATIC):LIBS -= -ljack

#LIBS = -lsvaudioio -lsvdata -lsvtransform -lsvplugin -lsvrdf -lsvbase -lsvsystem  $$LIBS
LIBS = -lsvdata -lsvtransform -lsvplugin -lsvrdf -lsvdata -lsvbase -lsvsystem  $$LIBS

PRE_TARGETDEPS += ../sonic-visualiser/audioio/libsvaudioio.a \
                  ../sonic-visualiser/data/libsvdata.a \
                  ../sonic-visualiser/transform/libsvtransform.a \
                  ../sonic-visualiser/plugin/libsvplugin.a \
                  ../sonic-visualiser/rdf/libsvrdf.a \
                  ../sonic-visualiser/base/libsvbase.a \
                  ../sonic-visualiser/system/libsvsystem.a

OBJECTS_DIR = tmp_obj
MOC_DIR = tmp_moc

# Input
HEADERS += \
	AudioDBFeatureWriter.h \
        FeatureWriterFactory.h  \
        DefaultFeatureWriter.h \
        FeatureExtractionManager.h

SOURCES += \
	main.cpp \
	DefaultFeatureWriter.cpp \
	FeatureExtractionManager.cpp \
        AudioDBFeatureWriter.cpp \
        FeatureWriterFactory.cpp




# Restore dynamic linkage, in case we went static earlier
linux-g++:LIBS += -Wl,-Bdynamic -lpthread -ldl -lz
