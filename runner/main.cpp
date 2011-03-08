/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Annotator
    A utility for batch feature extraction from audio files.
    Mark Levy, Chris Sutton and Chris Cannam, Queen Mary, University of London.
    Copyright 2007-2008 QMUL.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <vector>
#include <string>
#include <iostream>

#include <QCoreApplication>
#include <QSettings>
#include <QStringList>
#include <QString>
#include <QFileInfo>
#include <QDir>
#include <QSet>

using std::cout;
using std::cerr;
using std::endl;
using std::vector;
using std::string;

#include "../version.h"

#include "base/Exceptions.h"
#include "base/TempDirectory.h"

#include "data/fileio/AudioFileReaderFactory.h"
#include "data/fileio/PlaylistFileReader.h"

#include "transform/Transform.h"
#include "transform/TransformFactory.h"

#include "FeatureExtractionManager.h"
#include "transform/FeatureWriter.h"
#include "FeatureWriterFactory.h"

#include "rdf/RDFTransformFactory.h"

#include <vamp-hostsdk/PluginSummarisingAdapter.h>

#ifdef HAVE_FFTW3
#include <fftw3.h>
#endif

// Desired options:
//
// * output preference:
//   - all data in one file
//   - one file per input file
//   - one file per input file per transform
//   - (any use for: one file per transform?)
//
// * output location:
//   - same directory as input file
//   - current directory
//
// * output filename: 
//   - based on input (obvious choice for one file per input file modes)
//   - specified on command line (obvious choice for all in one file mode)
//
// * output format: one or more of
//   - RDF
//   - AudioDB
//   - Vamp Simple Host format
//   - CSV
//
// * input handling:
//   - run each transform on each input file separately
//   - provide all input files to the same transform, one per channel
//
// * format-specific options:
//   - RDF format: fancy/plain RDF
//   - CSV format: separator, timestamp type
//   note: do the output file/location also count as format-specific options?
//   an output writer that wrote to a database would have different options...
//
// * debug level and progress output
// 
// * other potential options:
//   - ignore version mismatches in Transform specifications
//   - sample rate: force a given rate; use file rate instead of rate in
//     Transform spec
// 
// * other potential instructions:
//   - write out a skeleton Transform file for a specified plugin
//   - write out skeleton RDF for a plugin library (i.e. do the job of
//     RDF template_generator)
//   - verify that RDF for a plugin library matches the plugin
//
// MAYBE:
// * transform(s) to run:
//   - supply transform file names on command line
//   - use all transforms found in a given directory?
//
// MAYBE:
// * input files to transform:
//   - supply file names or URIs on command line
//   - use all files in a given directory or tree

static QString
wrap(QString s, int len, int pfx = 0)
{
    QString ws;
    QStringList sl(s.split(' '));
    int i = 0, c = 0;
    while (i < sl.size()) {
        int wl = sl[i].length();
        if (c + wl < len) {
            if (c > 0) {
                ws += ' ';
                ++c;
            }
        } else {
            if (c > 0) {
                ws += '\n';
                for (int j = 0; j < pfx; ++j) ws += ' ';
                c = 0;
            }
        }
        ws += sl[i];
        c += wl;
        ++i;
    }
    return ws;
}

void usage(QString myname)
{
    set<string> writers = FeatureWriterFactory::getWriterTags();
        
    cerr << endl;
    cerr << "Sonic Annotator v" << RUNNER_VERSION << endl;
    cerr << "A utility for batch feature extraction from audio files." << endl;
    cerr << "Mark Levy, Chris Sutton and Chris Cannam, Queen Mary, University of London." << endl;
    cerr << "Copyright 2007-2011 Queen Mary, University of London." << endl;
    cerr << endl;
    cerr << "This program is free software.  You may redistribute copies of it under the" << endl;
    cerr << "terms of the GNU General Public License <http://www.gnu.org/licenses/gpl.html>." << endl;
    cerr << "This program is supplied with NO WARRANTY, to the extent permitted by law." << endl;
    cerr << endl;
    cerr << "  Usage: " << myname.toStdString()
         << " [-mr] -t trans.xml [...] -w <writer> [...] <audio> [...]" << endl;
    cerr << "         " << myname.toStdString()
         << " [-mr] -T trans.txt [...] -w <writer> [...] <audio> [...]" << endl;
    cerr << "         " << myname.toStdString()
         << " -s <transform>" << endl;
    cerr << "         " << myname.toStdString()
         << " [-lhv]" << endl;
    cerr << endl;
    cerr << "Where <audio> is an audio file or URL to use as input: either a local file" << endl;
    cerr << "path, local \"file://\" URL, or remote \"http://\" or \"ftp://\" URL." << endl;
    cerr << endl;

    QString extensions = AudioFileReaderFactory::getKnownExtensions();
    QStringList extlist = extensions.split(" ", QString::SkipEmptyParts);
    if (!extlist.empty()) {
        cerr << "The following audio file extensions are recognised:" << endl;
        cerr << "  ";
        int c = 2;
        for (int i = 0; i < extlist.size(); ++i) {
            QString ext = extlist[i];
            if (ext.startsWith("*.")) ext = ext.right(ext.length()-2);
            c += ext.length() + 2;
            if (c >= 80) {
                cerr << "\n  ";
                c -= 78;
            }
            cerr << ext.toStdString();
            if (i + 1 == extlist.size()) cerr << ".";
            else cerr << ", ";
        }
        cerr << endl;
    }

    cerr << "Playlist files in M3U format are also supported." << endl;
    cerr << endl;
    cerr << "Transformation options:" << endl;
    cerr << endl;
    cerr << "  -t, --transform <T> Apply transform described in transform file <T> to" << endl;
    cerr << "                      all input audio files.  You may supply this option" << endl;
    cerr << "                      multiple times.  You must supply this option or -T at" << endl;
    cerr << "                      least once for any work to be done.  Transform format" << endl;
    cerr << "                      may be SV transform XML or Vamp transform RDF.  See" << endl;
    cerr << "                      documentation for examples." << endl;
    cerr << endl;
    cerr << "  -T, --transforms <T> Apply all transforms described in transform files" << endl;
    cerr << "                      whose names are listed in text file <T>.  You may supply" << endl;
    cerr << "                      this option multiple times." << endl;
    cerr << endl;
    cerr << "  -d, --default <I>   Apply the default transform for transform id <I>.  This" << endl;
    cerr << "                      is equivalent to generating a skeleton transform for this" << endl;
    cerr << "                      id (using the -s option, below) and then applying that," << endl;
    cerr << "                      unmodified, with the -t option in the normal way.  Note" << endl;
    cerr << "                      that the results may vary as the implementation's default" << endl;
    cerr << "                      processing parameters are not guaranteed.  Do not use" << endl;
    cerr << "                      this in production systems.  You may supply this option" << endl;
    cerr << "                      multiple times, and mix it with -t and -T." << endl;
    cerr << endl;
    cerr << "  -w, --writer <W>    Write output using writer type <W>." << endl;
    cerr << "                      Supported writer types are: ";
    for (set<string>::const_iterator i = writers.begin();
         i != writers.end(); ) {
        cerr << *i;
        if (++i != writers.end()) cerr << ", ";
        else cerr << ".";
    }
    cerr << endl;
    cerr << "                      You may supply this option multiple times.  You must" << endl;
    cerr << "                      supply this option at least once for any work to be done." << endl;
    cerr << endl;
    cerr << "  -S, --summary <S>   In addition to the result features, write summary feature" << endl;
    cerr << "                      of summary type <S>." << endl;
    cerr << "                      Supported summary types are: min, max, mean, median, mode," << endl;
    cerr << "                      sum, variance, sd, count." << endl;
    cerr << "                      You may supply this option multiple times." << endl;
    cerr << endl;
    cerr << "      --summary-only  Write only summary features; do not write the regular" << endl;
    cerr << "                      result features." << endl;
    cerr << endl;
    cerr << "      --segments <A>,<B>[,...]" << endl;
    cerr << "                      Summarise in segments, with segment boundaries" << endl;
    cerr << "                      at A, B, ... seconds." << endl;
    cerr << endl;

/*!!! This feature not implemented yet (sniff)
    cerr << "  -m, --multiplex     If multiple input audio files are given, use mono" << endl;
    cerr << "                      mixdowns of all files as the input channels for a single" << endl;
    cerr << "                      invocation of each transform, instead of running the" << endl;
    cerr << "                      transform against all files separately." << endl;
    cerr << endl;
*/

    cerr << "  -r, --recursive     If any of the <audio> arguments is found to be a local" << endl;
    cerr << "                      directory, search the tree starting at that directory" << endl;
    cerr << "                      for all supported audio files and take all of those as" << endl;
    cerr << "                      input instead." << endl;
    cerr << endl;
    cerr << "  -f, --force         Continue with subsequent files following an error." << endl;
    cerr << endl;
    cerr << "Housekeeping options:" << endl;
    cerr << endl;
    cerr << "  -l, --list          List all known transform ids to standard output." << endl;
    cerr << endl;
    cerr << "  -s, --skeleton <I>  Generate a skeleton transform file for transform id <I>" << endl;
    cerr << "                      and write it to standard output." << endl;
    cerr << endl;
    cerr << "  -v, --version       Show the version number and exit." << endl;
    cerr << "  -h, --help          Show this help." << endl;

    cerr << endl;
    cerr << "If no -w (or --writer) options are supplied, either the -l -s -v or -h option" << endl;
    cerr << "(or long equivalent) must be given instead." << endl;

    for (set<string>::const_iterator i = writers.begin();
         i != writers.end(); ++i) {
        FeatureWriter *w = FeatureWriterFactory::createWriter(*i);
        if (!w) {
            cerr << "  (Internal error: failed to create writer of this type)" << endl;
            continue;
        }
        FeatureWriter::ParameterList params = w->getSupportedParameters();
        delete w;
        if (params.empty()) {
            continue;
        }
        cerr << endl;
        cerr << "Additional options for writer type \"" << *i << "\":" << endl;
        cerr << endl;
        for (FeatureWriter::ParameterList::const_iterator j = params.begin();
             j != params.end(); ++j) {
            cerr << "  --" << *i << "-" << j->name << " ";
            int spaceage = 16 - int(i->length()) - int(j->name.length());
            if (j->hasArg) { cerr << "<X> "; spaceage -= 4; }
            for (int k = 0; k < spaceage; ++k) cerr << " ";
            QString s(j->description.c_str());
            s = wrap(s, 56, 22);
            cerr << s.toStdString() << endl;
        }
    }

    cerr << endl;
    exit(0);
}

void
listTransforms()
{
    TransformList transforms =
        TransformFactory::getInstance()->getAllTransformDescriptions();

    for (TransformList::const_iterator iter = transforms.begin();
         iter != transforms.end(); ++iter) {
        const TransformDescription &transform = *iter;
        if (transform.type == TransformDescription::Analysis) {
            cout << transform.identifier.toStdString() << endl;
        }
    }
}    

void
printSkeleton(QString id)
{
    Transform transform =
        TransformFactory::getInstance()->getDefaultTransformFor(id);
    cout << "@prefix xsd:      <http://www.w3.org/2001/XMLSchema#> ." << endl
         << "@prefix vamp:     <http://purl.org/ontology/vamp/> ." << endl
         << "@prefix :         <#> ." << endl << endl;
    QString rdf = RDFTransformFactory::writeTransformToRDF
        (transform, ":transform");
    cout << rdf.toStdString();
}    

void
findSourcesRecursive(QString dirname, QStringList &addTo, int &found)
{
    QDir dir(dirname);

    QString printable = dir.dirName().left(20);
    cerr << "\rScanning \"" << printable.toStdString() << "\"..."
         << QString("                    ").left(20 - printable.length()).toStdString()
         << " [" << found << " audio file(s)]";

    QString extensions = AudioFileReaderFactory::getKnownExtensions();
    QStringList extlist = extensions.split(" ", QString::SkipEmptyParts);

    QStringList files = dir.entryList
        (extlist, QDir::Files | QDir::Readable);
    for (int i = 0; i < files.size(); ++i) {
        addTo.push_back(dir.filePath(files[i]));
        ++found;
    }

    QStringList subdirs = dir.entryList
        (QStringList(), QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);
    for (int i = 0; i < subdirs.size(); ++i) {
        findSourcesRecursive(dir.filePath(subdirs[i]), addTo, found);
    }
}


int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);

    QCoreApplication::setOrganizationName("QMUL");
    QCoreApplication::setOrganizationDomain("qmul.ac.uk");
    QCoreApplication::setApplicationName("Sonic Annotator");

    QStringList args = application.arguments();
    set<string> requestedWriterTags;
    set<string> requestedTransformFiles;
    set<string> requestedTransformListFiles;
    set<string> requestedDefaultTransforms;
    set<string> requestedSummaryTypes;
    bool force = false;
//!!!    bool multiplex = false;
    bool recursive = false;
    bool list = false;
    bool summaryOnly = false;
    QString skeletonFor = "";
    QString myname = args[0];
    myname = QFileInfo(myname).baseName();
    QStringList otherArgs;
    Vamp::HostExt::PluginSummarisingAdapter::SegmentBoundaries boundaries;

    QString helpStr = myname + ": use -h or --help option for help";

    for (int i = 1; i < args.size(); ++i) {

        QString arg = args[i];
        bool last = ((i + 1) == args.size());
        
        if (arg == "-h" || arg == "--help" || arg == "-?") {
            usage(myname);
        }

        if (arg == "-v" || arg == "--version") {
            std::cout << RUNNER_VERSION << std::endl;
            return 0;
        }

        if (arg == "-w" || arg == "--writer") {
            if (last || args[i+1].startsWith("-")) {
                cerr << myname.toStdString() << ": argument expected for \""
                     << arg.toStdString() << "\" option" << endl;
                cerr << helpStr.toStdString() << endl;
                exit(2);
            } else {
                string tag = args[++i].toStdString();
                if (requestedWriterTags.find(tag) != requestedWriterTags.end()) {
                    cerr << myname.toStdString() << ": NOTE: duplicate specification of writer type \"" << tag << "\" ignored" << endl;
                } else {
                    requestedWriterTags.insert(tag);
                }
                continue;
            }
        } else if (arg == "-t" || arg == "--transform") {
            if (last || args[i+1].startsWith("-")) {
                cerr << myname.toStdString() << ": argument expected for \""
                     << arg.toStdString() << "\" option" << endl;
                cerr << helpStr.toStdString() << endl;
                exit(2);
            } else {
                string transform = args[++i].toStdString();
                if (requestedTransformFiles.find(transform) !=
                    requestedTransformFiles.end()) {
                    cerr << myname.toStdString() << ": NOTE: duplicate specification of transform file \"" << transform << "\" ignored" << endl;
                } else {
                    requestedTransformFiles.insert(transform);
                }
                continue;
            }
        } else if (arg == "-T" || arg == "--transforms") {
            if (last || args[i+1].startsWith("-")) {
                cerr << myname.toStdString() << ": argument expected for \""
                     << arg.toStdString() << "\" option" << endl;
                cerr << helpStr.toStdString() << endl;
                exit(2);
            } else {
                string transform = args[++i].toStdString();
                if (requestedTransformListFiles.find(transform) !=
                    requestedTransformListFiles.end()) {
                    cerr << myname.toStdString() << ": NOTE: duplicate specification of transform list file \"" << transform << "\" ignored" << endl;
                } else {
                    requestedTransformListFiles.insert(transform);
                }
                continue;
            }
        } else if (arg == "-d" || arg == "--default") {
            if (last || args[i+1].startsWith("-")) {
                cerr << myname.toStdString() << ": argument expected for \""
                     << arg.toStdString() << "\" option" << endl;
                cerr << helpStr.toStdString() << endl;
                exit(2);
            } else {
                string deft = args[++i].toStdString();
                if (requestedDefaultTransforms.find(deft) !=
                    requestedDefaultTransforms.end()) {
                    cerr << myname.toStdString() << ": NOTE: duplicate specification of default transform \"" << deft << "\" ignored" << endl;
                } else {
                    requestedDefaultTransforms.insert(deft);
                }
                continue;
            }
        } else if (arg == "-S" || arg == "--summary") {
            if (last || args[i+1].startsWith("-")) {
                cerr << myname.toStdString() << ": argument expected for \""
                     << arg.toStdString() << "\" option" << endl;
                cerr << helpStr.toStdString() << endl;
                exit(2);
            } else {
                string summary = args[++i].toStdString();
                requestedSummaryTypes.insert(summary);
                continue;
            }
        } else if (arg == "--summary-only") {
            summaryOnly = true;
            continue;
        } else if (arg == "--segments") {
            if (last) {
                cerr << myname.toStdString() << ": argument expected for \""
                     << arg.toStdString() << "\" option" << endl;
                cerr << helpStr.toStdString() << endl;
                exit(2);
            } else {
                string segmentSpec = args[++i].toStdString();
                QStringList segmentStrs = QString(segmentSpec.c_str()).split(',');
                for (int j = 0; j < segmentStrs.size(); ++j) {
                    bool good = false;
                    boundaries.insert(Vamp::RealTime::fromSeconds
                                      (segmentStrs[j].toDouble(&good)));
                    if (!good) {
                        cerr << myname.toStdString() << ": segment boundaries must be numeric" << endl;
                        cerr << helpStr.toStdString() << endl;
                        exit(2);
                    }
                }
            }
/*!!!
        } else if (arg == "-m" || arg == "--multiplex") {
            multiplex = true;
            cerr << myname.toStdString()
                 << ": WARNING: Multiplex argument not yet implemented" << endl; //!!!
            continue;
*/
        } else if (arg == "-r" || arg == "--recursive") {
            recursive = true;
            continue;
        } else if (arg == "-f" || arg == "--force") {
            force = true;
            continue;
        } else if (arg == "-l" || arg == "--list") {
            list = true;
            continue;
        } else if (arg == "-s" || arg == "--skeleton") {
            if (last || args[i+1].startsWith("-")) {
                cerr << myname.toStdString() << ": usage: "
                     << myname.toStdString() << " " << arg.toStdString()
                     << " <transform>" << endl;
                cerr << helpStr.toStdString() << endl;
                exit(2);
            } else {
                skeletonFor = args[++i];
                continue;
            }
        } else {
            otherArgs.push_back(args[i]);
        }
    }

    if (list) {
        if (!requestedWriterTags.empty() || skeletonFor != "") {
            cerr << helpStr.toStdString() << endl;
            exit(2);
        }
        listTransforms();
        exit(0);
    }
    if (skeletonFor != "") {
        if (!requestedWriterTags.empty()) {
            cerr << helpStr.toStdString() << endl;
            exit(2);
        }
        printSkeleton(skeletonFor);
        exit(0);
    }

    if (requestedTransformFiles.empty() &&
        requestedTransformListFiles.empty() &&
        requestedDefaultTransforms.empty()) {
        cerr << myname.toStdString()
             << ": no transform(s) specified" << endl;
        cerr << helpStr.toStdString() << endl;
        exit(2);
    }

    if (requestedWriterTags.empty()) {
        cerr << myname.toStdString()
             << ": no writer(s) specified" << endl;
        cerr << helpStr.toStdString() << endl;
        exit(2);
    }

    if (!boundaries.empty()) {
        if (requestedSummaryTypes.empty()) {
            cerr << myname.toStdString()
                 << ": summary segment boundaries provided, but no summary type specified"
                 << endl;
            cerr << helpStr.toStdString() << endl;
            exit(2);
        }
    }

    QSettings settings;

#ifdef HAVE_FFTW3
    settings.beginGroup("FFTWisdom");
    QString wisdom = settings.value("wisdom").toString();
    if (wisdom != "") {
        fftw_import_wisdom_from_string(wisdom.toLocal8Bit().data());
    }
    settings.endGroup();
#endif

    settings.beginGroup("RDF");
    if (!settings.contains("rdf-indices")) {
        QStringList list;
        list << "http://www.vamp-plugins.org/rdf/plugins/index.txt";
        settings.setValue("rdf-indices", list);
    }
    settings.endGroup();

    FeatureExtractionManager manager;

    if (!requestedSummaryTypes.empty()) {
        if (!manager.setSummaryTypes(requestedSummaryTypes,
                                     summaryOnly,
                                     boundaries)) {
            cerr << myname.toStdString()
                 << ": failed to set requested summary types" << endl;
            exit(1);
        }
    }
    
    vector<FeatureWriter *> writers;

    for (set<string>::const_iterator i = requestedWriterTags.begin();
         i != requestedWriterTags.end(); ++i) {

        FeatureWriter *writer = FeatureWriterFactory::createWriter(*i);

        if (!writer) {
            cerr << myname.toStdString() << ": unknown feature writer \""
                 << *i << "\"" << endl;
            cerr << helpStr.toStdString() << endl;
            exit(2);
        }

        map<string, string> writerArgs;
        FeatureWriter::ParameterList pl(writer->getSupportedParameters());

        for (int k = 0; k < pl.size(); ++k) {
            
            string argbase = pl[k].name;
            QString literal = QString("--%1-%2")
                .arg(i->c_str()).arg(argbase.c_str());
            
            for (int j = 0; j < otherArgs.size(); ) {
                
                if (otherArgs[j] != literal) {
                    ++j;
                    continue;
                }
                    
                otherArgs.removeAt(j);
                    
                if (pl[k].hasArg) {
                    if (j < otherArgs.size()) {
                        writerArgs[argbase] = otherArgs[j].toStdString();
                        otherArgs.removeAt(j);
                    } else {
                        cerr << myname.toStdString() << ": "
                             << "argument required for \""
                             << literal.toStdString() << "\" option"
                             << endl;
                        cerr << helpStr.toStdString() << endl;
                        exit(2);
                    }
                } else {
                    writerArgs[argbase] = "";
                }
            }
        }
        
        writer->setParameters(writerArgs);
        
        writers.push_back(writer);
    }

    for (int i = 0; i < otherArgs.size(); ++i) {
        if (otherArgs[i].startsWith("-")) {
            cerr << myname.toStdString() << ": unknown option \""
                 << otherArgs[i].toStdString() << "\"" << endl;
            cerr << helpStr.toStdString() << endl;
            exit(2);
        }
    }

    if (otherArgs.empty()) {
        cerr << myname.toStdString() << ": no input(s) specified" << endl;
        cerr << helpStr.toStdString() << endl;
        exit(2);
    }    

    for (set<string>::const_iterator i = requestedTransformListFiles.begin();
         i != requestedTransformListFiles.end(); ++i) {
        PlaylistFileReader reader(i->c_str());
        if (reader.isOK()) {
            vector<QString> files = reader.load();
            for (int j = 0; j < files.size(); ++j) {
                requestedTransformFiles.insert(files[j].toStdString());
            }
        } else {
            cerr << myname.toStdString() << ": failed to read template list file \"" << *i << "\"" << endl;
            exit(2);
        }
    }

    QStringList sources;
    if (!recursive) {
        sources = otherArgs;
    } else {
        for (QStringList::const_iterator i = otherArgs.begin();
             i != otherArgs.end(); ++i) {
            if (QDir(*i).exists()) {
                cerr << "Directory found and recursive flag set, scanning for audio files..." << endl;
                int found = 0;
                findSourcesRecursive(*i, sources, found);
                cerr << "\rDone, found " << found << " supported audio file(s)                    " << endl;
            } else {
                sources.push_back(*i);
            }
        }
    }

    bool good = true;
    QSet<QString> badSources;

    for (QStringList::const_iterator i = sources.begin();
         i != sources.end(); ++i) {
        try {
            manager.addSource(*i);
        } catch (const std::exception &e) {
            badSources.insert(*i);
            cerr << "ERROR: Failed to process file \"" << i->toStdString()
                 << "\": " << e.what() << endl;
            if (force) {
                // print a note only if we have more files to process
                QStringList::const_iterator j = i;
                if (++j != sources.end()) {
                    cerr << "NOTE: \"--force\" option was provided, continuing (more errors may occur)" << endl;
                }
            } else {
                cerr << "NOTE: If you want to continue with processing any further files after an" << endl
                     << "error like this, use the --force option" << endl;
                good = false;
                break;
            }
        }
    }

    if (good) {
    
        bool haveFeatureExtractor = false;
    
        for (set<string>::const_iterator i = requestedTransformFiles.begin();
             i != requestedTransformFiles.end(); ++i) {
            if (manager.addFeatureExtractorFromFile(i->c_str(), writers)) {
                haveFeatureExtractor = true;
            }
        }

        for (set<string>::const_iterator i = requestedDefaultTransforms.begin();
             i != requestedDefaultTransforms.end(); ++i) {
            if (manager.addDefaultFeatureExtractor(i->c_str(), writers)) {
                haveFeatureExtractor = true;
            }
        }

        if (!haveFeatureExtractor) {
            cerr << myname.toStdString() << ": no feature extractors added" << endl;
            good = false;
        }
    }

    if (good) {
        for (QStringList::const_iterator i = sources.begin();
             i != sources.end(); ++i) {
            if (badSources.contains(*i)) continue;
            std::cerr << "Extracting features for: \"" << i->toStdString() << "\"" << std::endl;
            try {
                manager.extractFeatures(*i, force);
            } catch (const std::exception &e) {
                cerr << "ERROR: Feature extraction failed for \"" << i->toStdString()
                     << "\": " << e.what() << endl;
                if (force) {
                    // print a note only if we have more files to process
                    QStringList::const_iterator j = i;
                    if (++j != sources.end()) {
                        cerr << "NOTE: \"--force\" option was provided, continuing (more errors may occur)" << endl;
                    }
                } else {
                    cerr << "NOTE: If you want to continue with processing any further files after an" << endl
                         << "error like this, use the --force option" << endl;
                    good = false;
                    break;
                }
            }
        }
    }
    
    for (int i = 0; i < writers.size(); ++i) delete writers[i];

#ifdef HAVE_FFTW3
    settings.beginGroup("FFTWisdom");
    char *cwisdom = fftw_export_wisdom_to_string();
    if (cwisdom) {
        settings.setValue("wisdom", cwisdom);
        fftw_free(cwisdom);
    }
    settings.endGroup();
#endif

    TempDirectory::getInstance()->cleanup();
    
    if (good) return 0;
    else return 1;
}


