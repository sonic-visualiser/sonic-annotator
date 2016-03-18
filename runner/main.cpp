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
#include "base/ProgressPrinter.h"

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

static QString wrapCol(QString s) {
    return wrap(s, 56, 22);
}

static bool
isVersionNewerThan(QString a, QString b) // from VersionTester in svapp
{
    QRegExp re("[._-]");
    QStringList alist = a.split(re, QString::SkipEmptyParts);
    QStringList blist = b.split(re, QString::SkipEmptyParts);
    int ae = alist.size();
    int be = blist.size();
    int e = std::max(ae, be);
    for (int i = 0; i < e; ++i) {
        int an = 0, bn = 0;
        if (i < ae) {
            an = alist[i].toInt();
            if (an == 0 && alist[i] != "0") {
                an = -1; // non-numeric field -> "-pre1" etc
            }
        }
        if (i < be) {
            bn = blist[i].toInt();
            if (bn == 0 && blist[i] != "0") {
                bn = -1;
            }
        }
        if (an < bn) return false;
        if (an > bn) return true;
    }
    return false;
}

static int
checkMinVersion(QString myname, QString v)
{
    if (v == RUNNER_VERSION) {
        return 0;
    } else if (isVersionNewerThan(RUNNER_VERSION, v)) {
        return 0;
    } else {
        cerr << myname << ": version " 
             << RUNNER_VERSION << " is less than requested min version "
             << v << ", failing" << endl;
        return 1;
    }
}

void printUsage(QString myname)
{
    cerr << endl;
    cerr << "Sonic Annotator v" << RUNNER_VERSION << endl;
    cerr << "A utility for batch feature extraction from audio files." << endl;
    cerr << "Mark Levy, Chris Sutton, and Chris Cannam, Queen Mary, University of London." << endl;
    cerr << "Copyright 2007-2016 Queen Mary, University of London." << endl;
    cerr << endl;
    cerr << "This program is free software.  You may redistribute copies of it under the" << endl;
    cerr << "terms of the GNU General Public License <http://www.gnu.org/licenses/gpl.html>." << endl;
    cerr << "This program is supplied with NO WARRANTY, to the extent permitted by law." << endl;
    cerr << endl;
    cerr << "Usage: " << endl;
    cerr << "  " << myname
         << " [-mrnf] -t transform.ttl [..] -w <writer> [..] <audio> [..]" << endl;
    cerr << "  " << myname
         << " [-mrnf] -T translist.txt [..] -w <writer> [..] <audio> [..]" << endl;
    cerr << "  " << myname
         << " [-mrnf] -d <id> [..] -w <writer> [..] <audio> [...]" << endl;
    cerr << "  " << myname
         << " -s <transform>" << endl;
    cerr << "  " << myname
         << " [-lhv]" << endl;
    cerr << endl;
    cerr << "Where <audio> is an audio file or URL to use as input: either a local file" << endl;
    cerr << "path, local \"file://\" URL, or remote \"http://\" or \"ftp://\" URL;" << endl;
    cerr << "and <id> is a transform id of the form vamp:libname:plugin:output." << endl;
    cerr << endl;
}

void printOptionHelp(std::string writer, FeatureWriter::Parameter &p)
{
    cerr << "  --" << writer << "-" << p.name << " ";
    int spaceage = 16 - int(writer.length()) - int(p.name.length());
    if (p.hasArg) { cerr << "<X> "; spaceage -= 4; }
    for (int k = 0; k < spaceage; ++k) cerr << " ";
    QString s(p.description.c_str());
    s = wrapCol(s);
    cerr << s << endl;
}

void printHelp(QString myname, QString w)
{
    std::string writer = w.toStdString();

    printUsage(myname);

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
            cerr << ext;
            if (i + 1 == extlist.size()) cerr << ".";
            else cerr << ", ";
        }
        cerr << endl;
    }

    cerr << "Playlist files in M3U format are also supported." << endl;
    cerr << endl;

    set<string> writers = FeatureWriterFactory::getWriterTags();

    QString writerText = "Supported writer types are: ";
    for (set<string>::const_iterator i = writers.begin();
         i != writers.end(); ) {
        writerText += i->c_str();
        if (++i != writers.end()) writerText += ", ";
        else writerText += ".";
    }
    writerText = wrapCol(writerText);

    if (writer == "" || writers.find(writer) == writers.end()) {

        cerr << "Transformation options:" << endl;
        cerr << endl;
        cerr << "  -t, --transform <T> "
             << wrapCol("Apply transform described in transform file <T> to"
                        " all input audio files. You may supply this option" 
                        " multiple times. You must supply this option, -T, or -d" 
                        " at least once for any work to be done. Transform format" 
                        " may be SV transform XML or Vamp transform RDF/Turtle."
                        " A skeleton transform file for"
                        " a given transform id can be generated using the"
                        " -s option (see below). See accompanying"
                        " documentation for transform examples.")
             << endl << endl;
        cerr << "  -T, --transforms <T> "
             << wrapCol("Apply all transforms described in transform files"
                        " whose names are listed in text file <T>. You may supply"
                        " this option multiple times.")
             << endl << endl;
        cerr << "  -d, --default <I>   "
             << wrapCol("Apply the default transform for transform id <I>. This"
                        " is equivalent to generating a skeleton transform for the"
                        " id (using the -s option, below) and then applying that,"
                        " unmodified, with the -t option in the normal way. Note"
                        " that results may vary, as default"
                        " processing parameters may change between releases of "
                        + myname + " as well as of individual plugins. Do not use"
                        " this in production systems. You may supply this option"
                        " multiple times, and mix it with -t and -T.")
             << endl << endl;
        cerr << "  -w, --writer <W>    Write output using writer type <W>.\n"
             << "                      " << writerText << endl
             << "                      "
             << wrapCol("You may supply this option multiple times. You must"
                        " supply this option at least once for any work to be done.")
             << endl << endl;
        cerr << "  -S, --summary <S>   "
             << wrapCol("In addition to the result features, write summary feature"
                        " of summary type <S>.") << endl
             << "                      "
             << wrapCol("Supported summary types are min, max, mean, median, mode,"
                        " sum, variance, sd, count.") << endl
             << "                      You may supply this option multiple times."
             << endl << endl;
        cerr << "      --summary-only  "
             << wrapCol("Write only summary features; do not write the regular"
                        " result features.")
             << endl << endl;
        cerr << "      --segments <A>,<B>[,...]\n                      "
             << wrapCol("Summarise in segments, with segment boundaries"
                        " at A, B, ... seconds.")
             << endl << endl;
        cerr << "      --segments-from <F>\n                      "
             << wrapCol("Summarise in segments, with segment boundaries"
                        " at times read from the text file <F>. (one time per"
                        " line, in seconds).")
             << endl << endl;
        cerr << "  -m, --multiplex     "
             << wrapCol("If multiple input audio files are given, use mono"
                        " mixdowns of the files as the input channels for a single"
                        " invocation of each transform, instead of running the"
                        " transform against all files separately. The first file"
                        " will be used for output reference name and sample rate.")
             << endl << endl;
        cerr << "  -r, --recursive     "
             << wrapCol("If any of the <audio> arguments is found to be a local"
                        " directory, search the tree starting at that directory"
                        " for all supported audio files and take all of those as"
                        " input in place of it.")
             << endl << endl;
        cerr << "  -n, --normalise     "
             << wrapCol("Normalise each input audio file to signal abs max = 1.f.")
             << endl << endl;
        cerr << "  -f, --force         "
             << wrapCol("Continue with subsequent files following an error.")
             << endl << endl;
        cerr << "Housekeeping options:"
             << endl << endl;
        cerr << "  -l, --list          List available transform ids to standard output." << endl;
        cerr << "      --list-writers  List supported writer types to standard output." << endl;
        cerr << "      --list-formats  List supported input audio formats to standard output." << endl;
        cerr << endl;
        cerr << "  -s, --skeleton <I>  "
             << wrapCol("Generate a skeleton RDF transform file for transform id"
                        " <I>, with default parameters for that transform, and write it"
                        " to standard output.")
             << endl << endl;
        cerr << "  -v, --version       Show the version number and exit." << endl;
        cerr << endl;
        cerr << "      --minversion <V> "
             << wrapCol("Exit with successful return code if the version of "
                        + myname + " is at least <V>, failure otherwise."
                        " For scripts that depend on certain option support.")
             << endl << endl;
        cerr << "  -h, --help          Show help." << endl;
        cerr << "  -h, --help <W>      Show help for writer type W." << endl;
        cerr << "                      " << writerText << endl;

        cerr << endl
             << wrap("If no -w (or --writer) options are supplied, one of the"
                     " housekeeping options (-l -s -v -h or long equivalent) must"
                     " be given instead.", 78, 0)
             << endl;

    } else {

        FeatureWriter *w = FeatureWriterFactory::createWriter(writer);
        if (!w) {
            cerr << "  (Internal error: failed to create writer of known type \""
                 << writer << "\")" << endl;
            return;
        }
        cerr << "Feature writer \"" << writer << "\":" << endl << endl;
        cerr << "  " << wrap(w->getDescription().c_str(), 76, 2) << endl << endl;
        FeatureWriter::ParameterList params = w->getSupportedParameters();
        delete w;
        if (params.empty()) {
            cerr << "  No additional options are available for this writer." << endl << endl;
            return;
        }
        FeatureWriter::ParameterList mandatory;
        bool haveOptional = false;
        for (auto &p: params) {
            if (p.mandatory) mandatory.push_back(p);
            else haveOptional = true;
        }
        if (!mandatory.empty()) {
            cerr << "Mandatory parameters for writer type \"" << writer << "\":" << endl;
            cerr << endl;
            for (auto &p: mandatory) {
                printOptionHelp(writer, p);
            }
            cerr << endl;
        }
        if (haveOptional) {
            cerr << "Additional options for writer type \"" << writer << "\":" << endl;
            cerr << endl;
            for (auto &p: params) {
                if (p.mandatory) continue;
                printOptionHelp(writer, p);
            }
        }
    }

    cerr << endl;
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
            cout << transform.identifier << endl;
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
    cout << rdf;
}    

void
findSourcesRecursive(QString dirname, QStringList &addTo, int &found)
{
    QDir dir(dirname);

    QString printable = dir.dirName().left(20);
    cerr << "\rScanning \"" << printable << "\"..."
         << QString("                    ").left(20 - printable.length())
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

QStringList
expandPlaylists(QStringList sources)
{
    QStringList expanded;
    foreach (QString path, sources) {
        if (QFileInfo(path).suffix().toLower() == "m3u") {
            ProgressPrinter retrievalProgress("Opening playlist file...");
            FileSource source(path, &retrievalProgress);
            if (!source.isAvailable()) {
                // Don't fail or throw an exception here, just keep
                // the file in the list -- it will be tested again
                // when adding it as a source and that's the proper
                // time to fail. All we're concluding here is that it
                // isn't a valid playlist
                expanded.push_back(path);
                continue;
            }
            source.waitForData();
            PlaylistFileReader reader(source);
            if (reader.isOK()) {
                vector<QString> files = reader.load();
                for (int i = 0; i < (int)files.size(); ++i) {
                    expanded.push_back(files[i]);
                }
            }
        } else {
            // not a playlist
            expanded.push_back(path);
        }
    }
    return expanded;
}

bool
readSegmentBoundaries(QString url, 
                      Vamp::HostExt::PluginSummarisingAdapter::SegmentBoundaries &boundaries)
{
    FileSource source(url);
    if (!source.isAvailable()) {
        cerr << "File or URL \"" << url << "\" could not be retrieved" << endl;
        return false;
    }
    source.waitForData();

    QString filename = source.getLocalFilename();
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        cerr << "File \"" << filename << "\" could not be read" << endl;
        return false;
    }

    QTextStream in(&file);
    int lineNo = 0;
    
    while (!in.atEnd()) {

        ++lineNo;
        
        QString line = in.readLine();
        if (line.startsWith("#")) continue;

        QStringList bits = line.split(",", QString::SkipEmptyParts);
        QString importantBit;
        if (!bits.empty()) {
            bits = bits[0].split(" ", QString::SkipEmptyParts);
        }
        if (!bits.empty()) {
            importantBit = bits[0];
        }
        if (importantBit == QString()) {
            cerr << "WARNING: Skipping line " << lineNo << " (no content found)"
                 << endl;
            continue;
        }
        bool good = false;
        boundaries.insert(Vamp::RealTime::fromSeconds
                          (importantBit.toDouble(&good)));
        if (!good) {
            cerr << "Unparseable or non-numeric segment boundary at line "
                 << lineNo << endl;
            return false;
        }
    }

    return true;
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
    bool multiplex = false;
    bool recursive = false;
    bool normalise = false;
    bool list = false;
    bool listWriters = false;
    bool listFormats = false;
    bool summaryOnly = false;
    QString skeletonFor = "";
    QString minVersion = "";
    QString myname = args[0];
    myname = QFileInfo(myname).baseName();
    QStringList otherArgs;
    Vamp::HostExt::PluginSummarisingAdapter::SegmentBoundaries boundaries;

    QString helpStr = myname + ": use -h or --help option for help";

    for (int i = 1; i < args.size(); ++i) {

        QString arg = args[i];
        bool last = ((i + 1) == args.size());
        
        if (arg == "-h" || arg == "--help" || arg == "-?") {
            QString writer;
            if (!last) {
                writer = args[i+1];
            }
            printHelp(myname, writer);
            return 0;
        }

        if (arg == "-v" || arg == "--version") {
            std::cout << RUNNER_VERSION << std::endl;
            return 0;
        }

        if (arg == "-w" || arg == "--writer") {
            if (last || args[i+1].startsWith("-")) {
                cerr << myname << ": argument expected for \""
                     << arg << "\" option" << endl;
                cerr << helpStr << endl;
                exit(2);
            } else {
                string tag = args[++i].toStdString();
                if (requestedWriterTags.find(tag) != requestedWriterTags.end()) {
                    cerr << myname << ": NOTE: duplicate specification of writer type \"" << tag << "\" ignored" << endl;
                } else {
                    requestedWriterTags.insert(tag);
                }
                continue;
            }
        } else if (arg == "-t" || arg == "--transform") {
            if (last || args[i+1].startsWith("-")) {
                cerr << myname << ": argument expected for \""
                     << arg << "\" option" << endl;
                cerr << helpStr << endl;
                exit(2);
            } else {
                string transform = args[++i].toStdString();
                if (requestedTransformFiles.find(transform) !=
                    requestedTransformFiles.end()) {
                    cerr << myname << ": NOTE: duplicate specification of transform file \"" << transform << "\" ignored" << endl;
                } else {
                    requestedTransformFiles.insert(transform);
                }
                continue;
            }
        } else if (arg == "-T" || arg == "--transforms") {
            if (last || args[i+1].startsWith("-")) {
                cerr << myname << ": argument expected for \""
                     << arg << "\" option" << endl;
                cerr << helpStr << endl;
                exit(2);
            } else {
                string transform = args[++i].toStdString();
                if (requestedTransformListFiles.find(transform) !=
                    requestedTransformListFiles.end()) {
                    cerr << myname << ": NOTE: duplicate specification of transform list file \"" << transform << "\" ignored" << endl;
                } else {
                    requestedTransformListFiles.insert(transform);
                }
                continue;
            }
        } else if (arg == "-d" || arg == "--default") {
            if (last || args[i+1].startsWith("-")) {
                cerr << myname << ": argument expected for \""
                     << arg << "\" option" << endl;
                cerr << helpStr << endl;
                exit(2);
            } else {
                string deft = args[++i].toStdString();
                if (requestedDefaultTransforms.find(deft) !=
                    requestedDefaultTransforms.end()) {
                    cerr << myname << ": NOTE: duplicate specification of default transform \"" << deft << "\" ignored" << endl;
                } else {
                    requestedDefaultTransforms.insert(deft);
                }
                continue;
            }
        } else if (arg == "-S" || arg == "--summary") {
            if (last || args[i+1].startsWith("-")) {
                cerr << myname << ": argument expected for \""
                     << arg << "\" option" << endl;
                cerr << helpStr << endl;
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
                cerr << myname << ": argument expected for \""
                     << arg << "\" option" << endl;
                cerr << helpStr << endl;
                exit(2);
            } else {
                string segmentSpec = args[++i].toStdString();
                QStringList segmentStrs = QString(segmentSpec.c_str()).split(',');
                for (int j = 0; j < segmentStrs.size(); ++j) {
                    bool good = false;
                    boundaries.insert(Vamp::RealTime::fromSeconds
                                      (segmentStrs[j].toDouble(&good)));
                    if (!good) {
                        cerr << myname << ": segment boundaries must be numeric" << endl;
                        cerr << helpStr << endl;
                        exit(2);
                    }
                }
            }
        } else if (arg == "--segments-from") {
            if (last) {
                cerr << myname << ": argument expected for \""
                     << arg << "\" option" << endl;
                cerr << helpStr << endl;
                exit(2);
            } else {
                QString segmentFilename = args[++i];
                if (!readSegmentBoundaries(segmentFilename, boundaries)) {
                    cerr << myname << ": failed to read segment boundaries from file" << endl;
                    cerr << helpStr << endl;
                    exit(2);
                }
            }
        } else if (arg == "-m" || arg == "--multiplex") {
            multiplex = true;
            continue;
        } else if (arg == "-r" || arg == "--recursive") {
            recursive = true;
            continue;
        } else if (arg == "-n" || arg == "--normalise") {
            normalise = true;
            continue;
        } else if (arg == "-f" || arg == "--force") {
            force = true;
            continue;
        } else if (arg == "--list-writers") {
            listWriters = true;
            continue;
        } else if (arg == "--list-formats") {
            listFormats = true;
            continue;
        } else if (arg == "-l" || arg == "--list") {
            list = true;
            continue;
        } else if (arg == "--minversion") {
            if (last || args[i+1].startsWith("-")) {
                cerr << myname << ": usage: "
                     << myname << " " << arg << " <version>" << endl;
                cerr << helpStr << endl;
                exit(2);
            }
            minVersion = args[++i];
            continue;
        } else if (arg == "-s" || arg == "--skeleton") {
            if (last || args[i+1].startsWith("-")) {
                cerr << myname << ": usage: "
                     << myname << " " << arg
                     << " <transform>" << endl;
                cerr << helpStr << endl;
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
            cerr << helpStr << endl;
            exit(2);
        }
        listTransforms();
        exit(0);
    }
    if (listWriters) {
        if (!requestedWriterTags.empty() || skeletonFor != "") {
            cerr << helpStr << endl;
            exit(2);
        }
        set<string> writers = FeatureWriterFactory::getWriterTags();
        bool first = true;
        for (set<string>::const_iterator i = writers.begin();
             i != writers.end(); ++i) {
            if (!first) cout << " ";
            cout << *i;
            first = false;
        }
        cout << endl;
        exit(0);
    }
    if (listFormats) {
        if (!requestedWriterTags.empty() || skeletonFor != "") {
            cerr << helpStr << endl;
            exit(2);
        }
        QString extensions = AudioFileReaderFactory::getKnownExtensions();
        QStringList extlist = extensions.split(" ", QString::SkipEmptyParts);
        bool first = true;
        foreach (QString s, extlist) {
            if (!first) cout << " ";
            s.replace("*.", "");
            cout << s;
            first = false;
        }
        cout << endl;
        exit(0);
    }
    if (list) {
        if (!requestedWriterTags.empty() || skeletonFor != "") {
            cerr << helpStr << endl;
            exit(2);
        }
        listTransforms();
        exit(0);
    }
    if (skeletonFor != "") {
        if (!requestedWriterTags.empty()) {
            cerr << helpStr << endl;
            exit(2);
        }
        printSkeleton(skeletonFor);
        exit(0);
    }
    if (minVersion != "") {
        if (!requestedWriterTags.empty()) {
            cerr << helpStr << endl;
            exit(2);
        }
        exit(checkMinVersion(myname, minVersion));
    }

    if (requestedTransformFiles.empty() &&
        requestedTransformListFiles.empty() &&
        requestedDefaultTransforms.empty()) {
        cerr << myname
             << ": no transform(s) specified" << endl;
        cerr << helpStr << endl;
        exit(2);
    }

    if (requestedWriterTags.empty()) {
        cerr << myname
             << ": no writer(s) specified" << endl;
        cerr << helpStr << endl;
        exit(2);
    }

    if (!boundaries.empty()) {
        if (requestedSummaryTypes.empty()) {
            cerr << myname
                 << ": summary segment boundaries provided, but no summary type specified"
                 << endl;
            cerr << helpStr << endl;
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

    manager.setNormalise(normalise);

    if (!requestedSummaryTypes.empty()) {
        if (!manager.setSummaryTypes(requestedSummaryTypes,
                                     boundaries)) {
            cerr << myname << ": failed to set requested summary types" << endl;
            exit(1);
        }
    }

    manager.setSummariesOnly(summaryOnly);

    vector<FeatureWriter *> writers;

    for (set<string>::const_iterator i = requestedWriterTags.begin();
         i != requestedWriterTags.end(); ++i) {

        FeatureWriter *writer = FeatureWriterFactory::createWriter(*i);

        if (!writer) {
            cerr << myname << ": unknown feature writer \""
                 << *i << "\"" << endl;
            cerr << helpStr << endl;
            exit(2);
        }

        map<string, string> writerArgs;
        FeatureWriter::ParameterList pl(writer->getSupportedParameters());

        for (int k = 0; k < (int)pl.size(); ++k) {
            
            string argbase = pl[k].name;
            QString literal = QString("--%1-%2")
                .arg(i->c_str()).arg(argbase.c_str());
            
            for (int j = 0; j < (int)otherArgs.size(); ) {
                
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
                        cerr << myname << ": "
                             << "argument required for \""
                             << literal << "\" option"
                             << endl;
                        cerr << helpStr << endl;
                        exit(2);
                    }
                } else {
                    writerArgs[argbase] = "";
                }
            }
        }

        for (auto &p: pl) {
            if (p.mandatory) {
                bool found = false;
                for (auto &w: writerArgs) {
                    if (w.first == p.name) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    QString literal = QString("--%1-%2")
                        .arg(i->c_str()).arg(p.name.c_str());
                    cerr << myname << ": "
                         << "the \"" << literal << "\" parameter is mandatory"
                         << endl;
                    cerr << helpStr << endl;
                    exit(2);
                }
            }
        }

        try {
            writer->setParameters(writerArgs);
        } catch (std::exception &ex) {
            cerr << myname << ": " << ex.what() << endl;
            cerr << helpStr << endl;
            exit(2);
        }
        
        writers.push_back(writer);
    }

    for (int i = 0; i < otherArgs.size(); ++i) {
        if (otherArgs[i].startsWith("-")) {
            cerr << myname << ": unknown option \""
                 << otherArgs[i] << "\"" << endl;
            cerr << helpStr << endl;
            exit(2);
        }
    }

    if (otherArgs.empty()) {
        cerr << myname << ": no input(s) specified" << endl;
        cerr << helpStr << endl;
        exit(2);
    }    

    for (set<string>::const_iterator i = requestedTransformListFiles.begin();
         i != requestedTransformListFiles.end(); ++i) {
        PlaylistFileReader reader(i->c_str());
        if (reader.isOK()) {
            vector<QString> files = reader.load();
            for (int j = 0; j < (int)files.size(); ++j) {
                requestedTransformFiles.insert(files[j].toStdString());
            }
        } else {
            cerr << myname << ": failed to read template list file \"" << *i << "\"" << endl;
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

    sources = expandPlaylists(sources);
        
    bool good = true;
    QSet<QString> badSources;

    for (QStringList::const_iterator i = sources.begin();
         i != sources.end(); ++i) {
        try {
            manager.addSource(*i, multiplex);
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
            } else {
                cerr << "ERROR: Failed to add feature extractor from transform file \"" << *i << "\"" << endl;
                good = false;
            }
        }

        for (set<string>::const_iterator i = requestedDefaultTransforms.begin();
             i != requestedDefaultTransforms.end(); ++i) {
            if (manager.addDefaultFeatureExtractor(i->c_str(), writers)) {
                haveFeatureExtractor = true;
            } else {
                cerr << "ERROR: Failed to add default feature extractor for transform \"" << *i << "\"" << endl;
                good = false;
            }
        }

        if (!haveFeatureExtractor) {
            cerr << myname << ": no feature extractors added" << endl;
            good = false;
        }
    }

    if (good) {
        QStringList goodSources;
        foreach (QString source, sources) {
            if (!badSources.contains(source)) {
                goodSources.push_back(source);
            }
        }
        if (multiplex) {
            try {
                for (int i = 0; i < (int)writers.size(); ++i) {
                    writers[i]->setNofM(1, 1);
                }
                manager.extractFeaturesMultiplexed(goodSources);
            } catch (const std::exception &e) {
                cerr << "ERROR: Feature extraction failed: "
                     << e.what() << endl;
            }
        } else {
            int n = 0;
            for (QStringList::const_iterator i = goodSources.begin();
                 i != goodSources.end(); ++i) {
                std::cerr << "Extracting features for: \"" << i->toStdString()
                          << "\"" << std::endl;
                ++n;
                try {
                    for (int j = 0; j < (int)writers.size(); ++j) {
                        writers[j]->setNofM(n, goodSources.size());
                    }
                    manager.extractFeatures(*i);
                } catch (const std::exception &e) {
                    cerr << "ERROR: Feature extraction failed for \""
                         << i->toStdString() << "\": " << e.what() << endl;
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
    }
    
    for (int i = 0; i < (int)writers.size(); ++i) delete writers[i];

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


