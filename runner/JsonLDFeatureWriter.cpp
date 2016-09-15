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

#include "JsonLDFeatureWriter.h"

using namespace std;
using Vamp::Plugin;
using Vamp::PluginBase;

#include "base/Exceptions.h"
#include "rdf/PluginRDFIndexer.h"

#include <QFileInfo>
#include <QTextCodec>
#include <QUuid>

#include "version.h"

JsonLDFeatureWriter::JsonLDFeatureWriter() :
    FileFeatureWriter(SupportOneFilePerTrackTransform |
                      SupportOneFilePerTrack |
                      SupportOneFileTotal |
		      SupportStdOut,
                      "json"),
    m_network(false),
    m_networkRetrieved(false),
    m_n(1),
    m_m(1),
    m_digits(6)
{
}

JsonLDFeatureWriter::~JsonLDFeatureWriter()
{
}

string
JsonLDFeatureWriter::getDescription() const
{
    return "Write features to JSON files in JSON-LD format. WARNING: This is a provisional implementation! The output format may change in future releases to comply more effectively with the specification. Please report any problems you find with the current implementation.";
}

JsonLDFeatureWriter::ParameterList
JsonLDFeatureWriter::getSupportedParameters() const
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
JsonLDFeatureWriter::setParameters(map<string, string> &params)
{
    FileFeatureWriter::setParameters(params);

    for (map<string, string>::iterator i = params.begin();
         i != params.end(); ++i) {
        if (i->first == "network") {
            m_network = true;
        } else if (i->first == "digits") {
            int digits = atoi(i->second.c_str());
            if (digits <= 0 || digits > 100) {
                cerr << "JsonLDFeatureWriter: ERROR: Invalid or out-of-range value for number of significant digits: " << i->second << endl;
                cerr << "JsonLDFeatureWriter: NOTE: Continuing with default settings" << endl;
            } else {
                m_digits = digits;
            }
        }
    }
}

void
JsonLDFeatureWriter::setTrackMetadata(QString trackId, TrackMetadata metadata)
{
    m_trackMetadata[trackId] = metadata;
}

static double
realTime2Sec(const Vamp::RealTime &r)
{
    return r / Vamp::RealTime(1, 0);
}

void
JsonLDFeatureWriter::write(QString trackId,
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

    if (m_trackTimelineGuids.find(trackId) == m_trackTimelineGuids.end()) {
        QUuid uuid = QUuid::createUuid();
        m_trackTimelineGuids[trackId] = QString(uuid.toString().replace("{", "").replace("}", ""));
    }

    QString d = m_data[did];

    for (int i = 0; i < int(features.size()); ++i) {

        if (d != "") {
            d += ",\n";
        }
        
        d += "\t\t\t{ ";
	
        Plugin::Feature f(features[i]);

        QString timestr = f.timestamp.toString().c_str();
        timestr.replace(QRegExp("^ +"), "");

        QString durstr = "0.0";
        if (f.hasDuration) {
            durstr = f.duration.toString().c_str();
            durstr.replace(QRegExp("^ +"), "");
            d += " \"@type\": \"tl:Interval\", ";
        }
        else{
            d += " \"@type\": \"tl:Instant\", ";
        }
        
        d += QString("\"tl:at\": %1 ")
            .arg(timestr);

        d += QString(", \"tl:timeline\": \"%1\" ")
            .arg(m_trackTimelineGuids[trackId]);

        if (f.hasDuration) {
            d += QString(", \"tl:duration\": %2")
                .arg(durstr);
        }
        
        if (f.label != "") {
            if (f.values.empty()) {
                d += QString(", \"afo:value\": \"%2\"").arg(f.label.c_str());
            } else {
                d += QString(", \"rdfs:label\": \"%2\"").arg(f.label.c_str());
            }
        }

        if (!f.values.empty()) {
            d += QString(", \"afo:value\": ");
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
JsonLDFeatureWriter::setNofM(int n, int m)
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
JsonLDFeatureWriter::finish()
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

            stream << "{\n" << writeContext();
            stream << "\t\"@type\": \"mo:Track\",\n"
                   << QString("\t\"mo:available_as\": \"%1\"").arg(QFileInfo(trackId).filePath());

            if (m_trackMetadata.find(trackId) != m_trackMetadata.end()) {

                if (m_trackMetadata[trackId].title != "") {
                    stream << QString(",\n\t\"dc:title\": \"%1\"")
                        .arg(m_trackMetadata[trackId].title);
                }

                if (m_trackMetadata[trackId].maker != "") {
                    stream << QString(",\n\t\"mo:artist\": { "
                                      "\t\t\"@type\": \"mo:MusicArtist\",\n"
                                      "\t\t\"foaf:name\": \"%1\" "
                                      "\t}")
                        .arg(m_trackMetadata[trackId].maker);
                }

                QString durstr = m_trackMetadata[trackId].duration.toString().c_str();
                durstr.replace(QRegExp("^ +"), "");
                stream << QString(",\n\t\"mo:encodes\": {\n"
                                  "\t\t\"@type\": \"mo:Signal\",\n"
                                  "\t\t\"mo:time\": {\n "
                                  "\t\t\t\"@type\": \"tl:Interval\",\n"
                                  "\t\t\t\"tl:duration\": \"PT%1S\",\n"
                                  "\t\t\t\"tl:timeline\": { \"@type\": \"tl:Timeline\", \"@id\": \"%2\" } "
                                  "\n\t\t}").arg(durstr).arg(m_trackTimelineGuids[trackId]);
            }

            stream << "\n\t},\n";
            stream << "\t\"afo:features\": [\n";

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

                    stream << QString
                        ("\t{\n"
                         "\t\t\"@type\": \"afv:%1\",\n"
                         "\t\t\"afo:computed_by\": {\n"
                         "%2\t\t},\n"
                         "\t\t\"afo:values\": [\n")
                        .arg(transform.getOutput().replace(0, 1, transform.getOutput().at(0).toUpper()))
                        .arg(writeTransformToObjectContents(transform));

                    stream << data;

                    stream << "\n\t\t]\n\t}";
                    firstInTrack = false;
                }
            }

            stream << "\n\t]";

            stream << "\n}";
            firstInStream = false;
        }

        if (!firstInStream) {
            if (m_streamTracks[sptr].size() > 1 || (m_m > 1 && m_n == m_m)) {
                stream << "\n\t]";
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
JsonLDFeatureWriter::loadRDFDescription(const Transform &transform)
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
	    cerr << "      Consider using the --jsld-network option to retrieve plugin descriptions"  << endl;
	    cerr << "      from the network where possible." << endl;
	}
    }
}

void
JsonLDFeatureWriter::identifyTask(const Transform &transform)
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

	    cerr << "WARNING: Cannot currently write dense or track-level outputs to JSON-LD format (only sparse ones). Will proceed using UnknownTask type, but this probably isn't going to work" << endl;
	}
    }	    

    m_tasks[transformId] = task;
}

QString
JsonLDFeatureWriter::getTaskKey(Task task)
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
JsonLDFeatureWriter::writeTransformToObjectContents(const Transform &t)
{
    QString json;
    QString stpl("\t\t\t\"%1\": \"%2\",\n");
    QString ntpl("\t\t\t\"%1\": %2,\n");

    json += stpl.arg("@type").arg("vamp:Transform");
    json += stpl.arg("vamp:plugin_id").arg(t.getPluginIdentifier());
    json += stpl.arg("vamp:output_id").arg(t.getOutput());

    if (t.getSummaryType() != Transform::NoSummary) {
        json += stpl.arg("vamp:summary_type")
            .arg(Transform::summaryTypeToString(t.getSummaryType()));
    }

    if (t.getPluginVersion() != QString()) {
        json += stpl.arg("vamp:plugin_version").arg(t.getPluginVersion());
    }

    if (t.getProgram() != QString()) {
        json += stpl.arg("vamp:program").arg(t.getProgram());
    }

    if (t.getStepSize() != 0) {
        json += ntpl.arg("vamp:step_size").arg(t.getStepSize());
    }

    if (t.getBlockSize() != 0) {
        json += ntpl.arg("vamp:block_size").arg(t.getBlockSize());
    }

    if (t.getWindowType() != HanningWindow) {
        json += stpl.arg("vamp:window_type")
            .arg(Window<float>::getNameForType(t.getWindowType()).c_str());
    }

    if (t.getStartTime() != RealTime::zeroTime) {
        json += ntpl.arg("tl:start")
            .arg(t.getStartTime().toDouble(), 0, 'g', 9);
    }

    if (t.getDuration() != RealTime::zeroTime) {
        json += ntpl.arg("tl:duration")
            .arg(t.getDuration().toDouble(), 0, 'g', 9);
    }

    if (t.getSampleRate() != 0) {
        json += ntpl.arg("vamp:sample_rate").arg(t.getSampleRate());
    }

    if (!t.getParameters().empty()) {
        json += QString("\t\t\t\"vamp:parameter_binding\": [\n");
        Transform::ParameterMap parameters = t.getParameters();
        for (Transform::ParameterMap::const_iterator i = parameters.begin();
             i != parameters.end(); ++i) {
            if (i != parameters.begin()) {
                json += ",\n";
            }
            QString name = i->first;
            float value = i->second;
            json += QString("\t\t\t\t{\n");
            json += QString("\t\t\t\t\t\"@type\": \"vamp:Parameter\",\n");
            json += QString("\t\t\t\t\t\"vamp:identifier\": \"%1\",\n").arg(name);
            json += QString("\t\t\t\t\t\"vamp:value\": %1").arg(value, 0, 'g', 8);
            json += QString("\n\t\t\t\t}");
        }
        json += QString("\n\t\t\t],\n");
    }

    // no trailing comma on final property:
    json += QString("\t\t\t\"vamp:transform_id\": \"%1\",\n").arg(t.getIdentifier());
    json += QString("\t\t\t\"afo:implemented_in\": {\n");
    json += QString("\t\t\t\t\"@type\": \"afo:SoftwareAgent\",\n");
    json += QString("\t\t\t\t\"afo:name\": \"Sonic Annotator\",\n");
    json += QString("\t\t\t\t\"afo:version\": \"%1\" \n").arg(RUNNER_VERSION);
    json += QString("\t\t\t}\n");

    return json;
}

QString
JsonLDFeatureWriter::writeContext() {
    QString context;
    context += QString("\t\"@context\": {\n");
    context += QString("\t\t\"foaf\": \"http://xmlns.com/foaf/0.1/\",\n");
    context += QString("\t\t\"afo\": \"http://sovarr.c4dm.eecs.qmul.ac.uk/af/ontology/1.1#\",\n");
    context += QString("\t\t\"afv\": \"http://sovarr.c4dm.eecs.qmul.ac.uk/af/vocabulary/1.1#\",\n");
    context += QString("\t\t\"mo\": \"http://purl.org/ontology/mo/\",\n");
    context += QString("\t\t\"dc\": \"http://purl.org/dc/elements/1.1/\",\n");
    context += QString("\t\t\"tl\": \"http://purl.org/NET/c4dm/timeline.owl#\",\n");
    context += QString("\t\t\"vamp\": \"http://purl.org/ontology/vamp/\"\n");
    context += QString("\t},\n");
    return context;
}
