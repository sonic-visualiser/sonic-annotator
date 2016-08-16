/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Annotator
    A utility for batch feature extraction from audio files.
    Mark Levy, Chris Sutton and Chris Cannam, Queen Mary, University of London.
    Copyright 2007-2014 QMUL.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "JAMSFeatureWriter.h"

using namespace std;
using Vamp::Plugin;
using Vamp::PluginBase;

#include "base/Exceptions.h"
#include "rdf/PluginRDFIndexer.h"

#include <QFileInfo>
#include <QTextCodec>

#include "version.h"

JAMSFeatureWriter::JAMSFeatureWriter() :
    FileFeatureWriter(SupportOneFilePerTrackTransform |
                      SupportOneFilePerTrack |
                      SupportOneFileTotal |
		      SupportStdOut,
                      "json"), // file extension is json even with jams writer
    m_network(false),
    m_networkRetrieved(false),
    m_n(1),
    m_m(1),
    m_digits(6)
{
}

JAMSFeatureWriter::~JAMSFeatureWriter()
{
}

string
JAMSFeatureWriter::getDescription() const
{
    return "Write features to JSON files in JAMS (JSON Annotated Music Specification) format. WARNING: This is a provisional implementation! The output format may change in future releases to comply more effectively with the specification. Please report any problems you find with the current implementation.";
}

JAMSFeatureWriter::ParameterList
JAMSFeatureWriter::getSupportedParameters() const
{
    ParameterList pl = FileFeatureWriter::getSupportedParameters();
    Parameter p;

    p.name = "digits";
    p.description = "Specify the number of significant digits to use when printing transform outputs. Outputs are represented internally using single-precision floating-point, so digits beyond the 8th or 9th place are usually meaningless. The default is 6.";
    p.hasArg = true;
    pl.push_back(p);

    p.name = "network";
    p.description = "Attempt to retrieve RDF descriptions of plugins from network, if not available locally.";
    p.hasArg = false;
    pl.push_back(p);

    return pl;
}

void
JAMSFeatureWriter::setParameters(map<string, string> &params)
{
    FileFeatureWriter::setParameters(params);

    for (map<string, string>::iterator i = params.begin();
         i != params.end(); ++i) {
        if (i->first == "network") {
            m_network = true;
        } else if (i->first == "digits") {
            int digits = atoi(i->second.c_str());
            if (digits <= 0 || digits > 100) {
                cerr << "JAMSFeatureWriter: ERROR: Invalid or out-of-range value for number of significant digits: " << i->second << endl;
                cerr << "JAMSFeatureWriter: NOTE: Continuing with default settings" << endl;
            } else {
                m_digits = digits;
            }
        }
    }
}

void
JAMSFeatureWriter::setTrackMetadata(QString trackId, TrackMetadata metadata)
{
    m_trackMetadata[trackId] = metadata;
}

void
JAMSFeatureWriter::write(QString trackId,
			 const Transform &transform,
			 const Plugin::OutputDescriptor& ,
			 const Plugin::FeatureList& features,
			 std::string /* summaryType */)
{
    QString transformId = transform.getIdentifier();

    QTextStream *sptr = getOutputStream
        (trackId, transformId, QTextCodec::codecForName("UTF-8"));
    if (!sptr) {
        throw FailedToOpenOutputStream(trackId, transformId);
    }

    DataId did(trackId, transform);

    if (m_data.find(did) == m_data.end()) {
	identifyTask(transform);
        m_streamTracks[sptr].insert(trackId);
        m_streamTasks[sptr].insert(m_tasks[transformId]);
        m_streamData[sptr].insert(did);
    }

    QString d = m_data[did];

    for (int i = 0; i < int(features.size()); ++i) {

        if (d != "") {
            d += ",\n";
        }
        
        d += "    { ";
	
        Plugin::Feature f(features[i]);

        QString timestr = f.timestamp.toString().c_str();
        timestr.replace(QRegExp("^ +"), "");

        QString durstr = "0.0";
        if (f.hasDuration) {
            durstr = f.duration.toString().c_str();
            durstr.replace(QRegExp("^ +"), "");
        }
        
        d += QString("\"time\": %1, \"duration\": %2, \"confidence\": 1.0")
            .arg(timestr).arg(durstr);

        // here we have to differ from the JAMS 0.2.0 spec. It allows
        // a single "value" element which can be either a number or a
        // string, depending on the selected task. But we may have
        // many values and may have a label as well, and no way to
        // know whether these can be made to conform to the JAMS task
        // schema. We should just write what we have. If we only have
        // a label, we can write that out as "value" as JAMS requests,
        // but if we have a (numerical) value and a label, we really
        // have to write them separately, and if we have multiple
        // values we'll have to use an array. The chances of actually
        // ending up with a schema-compliant JAMS format are quite
        // small, which suggests JAMS isn't a great idea for this
        // after all!
        
        if (f.label != "") {
            if (f.values.empty()) {
                d += QString(", \"value\": \"%2\"").arg(f.label.c_str());
            } else {
                d += QString(", \"label\": \"%2\"").arg(f.label.c_str());
            }
        }

        if (!f.values.empty()) {
            d += QString(", \"value\": ");
            if (f.values.size() > 1) {
                d += "[ ";
            }
            for (int j = 0; j < int(f.values.size()); ++j) {
                if (isnan(f.values[j])) {
                    d += "\"NaN\"";
                } else if (isinf(f.values[j])) {
                    d += "\"Inf\"";
                } else {
                    d += QString("%1").arg(f.values[j], 0, 'g', m_digits);
                }
                if (j + 1 < int(f.values.size())) {
                    d += ", ";
                }
            }
            if (f.values.size() > 1) {
                d += " ]";
            }
        }
            
        d += " }";
    }	

    m_data[did] = d;
}

void
JAMSFeatureWriter::setNofM(int n, int m)
{
    if (m_singleFileName != "" || m_stdout) {
        m_n = n;
        m_m = m;
    } else {
        m_n = 1;
        m_m = 1;
    }
}

void
JAMSFeatureWriter::finish()
{
    for (FileStreamMap::const_iterator stri = m_streams.begin();
	 stri != m_streams.end(); ++stri) {

        QTextStream *sptr = stri->second;
        QTextStream &stream = *sptr;

        bool firstInStream = true;

        for (TrackIds::const_iterator tri = m_streamTracks[sptr].begin();
             tri != m_streamTracks[sptr].end(); ++tri) {

            TrackId trackId = *tri;

            if (firstInStream) {
                if (m_streamTracks[sptr].size() > 1 || (m_m > 1 && m_n == 1)) {
                    stream << "[\n";
                }
            }

            if (!firstInStream || (m_m > 1 && m_n > 1)) {
                stream << ",\n";
            }

            stream << "{\n"
                   << QString("\"file_metadata\": {\n"
                              "  \"jams_version\": \"0.2.0\",\n"
                              "  \"identifiers\": { \"filename\": \"%1\" }")
                .arg(QFileInfo(trackId).fileName());

            if (m_trackMetadata.find(trackId) != m_trackMetadata.end()) {

                QString durstr = m_trackMetadata[trackId].duration.toString().c_str();
                durstr.replace(QRegExp("^ +"), "");
                stream << QString(",\n  \"duration\": %1").arg(durstr);

                if (m_trackMetadata[trackId].maker != "") {
                    stream << QString(",\n  \"artist\": \"%1\"")
                        .arg(m_trackMetadata[trackId].maker);
                }
                if (m_trackMetadata[trackId].title != "") {
                    stream << QString(",\n  \"title\": \"%1\"")
                        .arg(m_trackMetadata[trackId].title);
                }
            }

            stream << "\n},\n";
            stream << "\"annotations\": [\n";

            bool firstInTrack = true;

            for (Tasks::const_iterator ti = m_streamTasks[sptr].begin();
                 ti != m_streamTasks[sptr].end(); ++ti) {
                
                Task task = *ti;

                for (DataIds::const_iterator di = m_streamData[sptr].begin();
                     di != m_streamData[sptr].end(); ++di) {
                    
                    DataId did = *di;

                    QString trackId = did.first;
                    Transform transform = did.second;

                    if (m_tasks[transform.getIdentifier()] != task) continue;

                    QString data = m_data[did];

                    if (!firstInTrack) {
                        stream << ",\n";
                    }

                    stream << "{\n  \"namespace\": \"" << getTaskKey(task) << "\",\n";

                    stream << QString
                        ("  \"annotation_metadata\": {\n"
                         "    \"annotation_tools\": \"Sonic Annotator v%2\",\n"
                         "    \"data_source\": \"Automatic feature extraction\",\n"
                         "    \"annotator\": {\n"
                         "%3"
                         "    }\n"
                         "  },\n"
                         "  \"data\": [\n")
                        .arg(RUNNER_VERSION)
                        .arg(writeTransformToObjectContents(transform));

                    stream << data;

                    stream << "\n  ]\n}";
                    firstInTrack = false;
                }
            }

            stream << "\n]";

            stream << "\n}";
            firstInStream = false;
        }

        if (!firstInStream) {
            if (m_streamTracks[sptr].size() > 1 || (m_m > 1 && m_n == m_m)) {
                stream << "\n]";
            }
            stream << "\n";
        }
    }
        
    m_streamTracks.clear();
    m_streamTasks.clear();
    m_streamData.clear();
    m_data.clear();

    FileFeatureWriter::finish();
}

void
JAMSFeatureWriter::loadRDFDescription(const Transform &transform)
{
    QString pluginId = transform.getPluginIdentifier();
    if (m_rdfDescriptions.find(pluginId) != m_rdfDescriptions.end()) return;

    if (m_network && !m_networkRetrieved) {
	PluginRDFIndexer::getInstance()->indexConfiguredURLs();
	m_networkRetrieved = true;
    }

    m_rdfDescriptions[pluginId] = PluginRDFDescription(pluginId);
    
    if (m_rdfDescriptions[pluginId].haveDescription()) {
	cerr << "NOTE: Have RDF description for plugin ID \""
	     << pluginId << "\"" << endl;
    } else {
	cerr << "NOTE: No RDF description for plugin ID \""
	     << pluginId << "\"" << endl;
	if (!m_network) {
	    cerr << "      Consider using the --jams-network option to retrieve plugin descriptions"  << endl;
	    cerr << "      from the network where possible." << endl;
	}
    }
}

void
JAMSFeatureWriter::identifyTask(const Transform &transform)
{
    QString transformId = transform.getIdentifier();
    if (m_tasks.find(transformId) != m_tasks.end()) return;

    loadRDFDescription(transform);
    
    Task task = UnknownTask;

    QString pluginId = transform.getPluginIdentifier();
    QString outputId = transform.getOutput();

    const PluginRDFDescription &desc = m_rdfDescriptions[pluginId];
    
    if (desc.haveDescription()) {

	PluginRDFDescription::OutputDisposition disp = 
	    desc.getOutputDisposition(outputId);

	QString af = "http://purl.org/ontology/af/";
	    
	if (disp == PluginRDFDescription::OutputSparse) {

	    QString eventUri = desc.getOutputEventTypeURI(outputId);

	    //!!! todo: allow user to prod writer for task type

	    if (eventUri == af + "Note") {
		task = NoteTask;
	    } else if (eventUri == af + "Beat") {
		task = BeatTask;
	    } else if (eventUri == af + "ChordSegment") {
		task = ChordTask;
	    } else if (eventUri == af + "KeyChange") {
		task = KeyTask;
	    } else if (eventUri == af + "KeySegment") {
		task = KeyTask;
	    } else if (eventUri == af + "Onset") {
		task = OnsetTask;
	    } else if (eventUri == af + "NonTonalOnset") {
		task = OnsetTask;
	    } else if (eventUri == af + "Segment") {
		task = SegmentTask;
	    } else if (eventUri == af + "SpeechSegment") {
		task = SegmentTask;
	    } else if (eventUri == af + "StructuralSegment") {
		task = SegmentTask;
	    } else {
		cerr << "WARNING: Unsupported event type URI <" 
		     << eventUri << ">, proceeding with UnknownTask type"
		     << endl;
	    }

	} else {

	    cerr << "WARNING: Cannot currently write dense or track-level outputs to JAMS format (only sparse ones). Will proceed using UnknownTask type, but this probably isn't going to work" << endl;
	}
    }	    

    m_tasks[transformId] = task;
}

QString
JAMSFeatureWriter::getTaskKey(Task task) 
{
    switch (task) {
    case UnknownTask: return "unknown";
    case BeatTask: return "beat";
    case OnsetTask: return "onset";
    case ChordTask: return "chord";
    case SegmentTask: return "segment";
    case KeyTask: return "key";
    case NoteTask: return "note";
    case MelodyTask: return "melody";
    case PitchTask: return "pitch";
    }
    return "unknown";
}

QString
JAMSFeatureWriter::writeTransformToObjectContents(const Transform &t)
{
    QString json;
    QString stpl("      \"%1\": \"%2\",\n");
    QString ntpl("      \"%1\": %2,\n");

    json += stpl.arg("plugin_id").arg(t.getPluginIdentifier());
    json += stpl.arg("output_id").arg(t.getOutput());

    if (t.getSummaryType() != Transform::NoSummary) {
        json += stpl.arg("summary_type")
            .arg(Transform::summaryTypeToString(t.getSummaryType()));
    }

    if (t.getPluginVersion() != QString()) {
        json += stpl.arg("plugin_version").arg(t.getPluginVersion());
    }

    if (t.getProgram() != QString()) {
        json += stpl.arg("program").arg(t.getProgram());
    }

    if (t.getStepSize() != 0) {
        json += ntpl.arg("step_size").arg(t.getStepSize());
    }

    if (t.getBlockSize() != 0) {
        json += ntpl.arg("block_size").arg(t.getBlockSize());
    }

    if (t.getWindowType() != HanningWindow) {
        json += stpl.arg("window_type")
            .arg(Window<float>::getNameForType(t.getWindowType()).c_str());
    }

    if (t.getStartTime() != RealTime::zeroTime) {
        json += ntpl.arg("start")
            .arg(t.getStartTime().toDouble(), 0, 'g', 9);
    }

    if (t.getDuration() != RealTime::zeroTime) {
        json += ntpl.arg("duration")
            .arg(t.getDuration().toDouble(), 0, 'g', 9);
    }

    if (t.getSampleRate() != 0) {
        json += ntpl.arg("sample_rate").arg(t.getSampleRate());
    }

    if (!t.getParameters().empty()) {
        json += QString("      \"parameters\": {\n");
        Transform::ParameterMap parameters = t.getParameters();
        for (Transform::ParameterMap::const_iterator i = parameters.begin();
             i != parameters.end(); ++i) {
            if (i != parameters.begin()) {
                json += ",\n";
            }
            QString name = i->first;
            float value = i->second;
            json += QString("        \"%1\": %2")
                .arg(name)
                .arg(value, 0, 'g', 8); // parameter values always to high precision
        }
        json += QString("\n      },\n");
    }

    // no trailing comma on final property:
    json += QString("      \"transform_id\": \"%1\"\n").arg(t.getIdentifier());

    return json;
}

