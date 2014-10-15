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

#include "version.h"

JAMSFeatureWriter::JAMSFeatureWriter() :
    FileFeatureWriter(SupportOneFilePerTrackTransform |
                      SupportOneFilePerTrack |
                      SupportOneFileTotal |
		      SupportStdOut,
                      "json"),
    m_network(false),
    m_networkRetrieved(false)
{
}

JAMSFeatureWriter::~JAMSFeatureWriter()
{
}

string
JAMSFeatureWriter::getDescription() const
{
    return "Write features to JSON files in JAMS (JSON Annotated Music Specification) format.";
}

JAMSFeatureWriter::ParameterList
JAMSFeatureWriter::getSupportedParameters() const
{
    ParameterList pl = FileFeatureWriter::getSupportedParameters();
    Parameter p;

    p.name = "network";
    p.description = "Attempt to retrieve RDF descriptions of plugins from network, if not available locally";
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
        }
    }
}

void
JAMSFeatureWriter::setTrackMetadata(QString trackId, TrackMetadata metadata)
{
    m_metadata[trackId] = metadata;
}

static double
realTime2Sec(const Vamp::RealTime &r)
{
    return r / Vamp::RealTime(1, 0);
}

void
JAMSFeatureWriter::write(QString trackId,
			 const Transform &transform,
			 const Plugin::OutputDescriptor& ,
			 const Plugin::FeatureList& features,
			 std::string /* summaryType */)
{
    QString transformId = transform.getIdentifier();

    QTextStream *sptr = getOutputStream(trackId, transformId);
    if (!sptr) {
        throw FailedToOpenOutputStream(trackId, transformId);
    }

    QTextStream &stream = *sptr;

    TrackTransformPair tt(trackId, transformId);
    TrackTransformPair targetKey = getFilenameKey(trackId, transformId);

    if (m_startedTargets.find(targetKey) == m_startedTargets.end()) {
        // Need to write track-level preamble
        stream << "{\n";
        stream << QString("\"file_metadata\": {\n"
                          "  \"filename\": \"%1\"")
            .arg(QFileInfo(trackId).fileName());

        if (m_metadata.find(trackId) != m_metadata.end()) {
            if (m_metadata[trackId].maker != "") {
                stream << QString(",\n  \"artist\": \"%1\"")
                    .arg(m_metadata[trackId].maker);
            }
            if (m_metadata[trackId].title != "") {
                stream << QString(",\n  \"title\": \"%1\"")
                    .arg(m_metadata[trackId].title);
            }
        }

        stream << "\n},\n";

        m_startedTargets.insert(targetKey);
    }

    bool justBegun = false;
    
    if (m_data.find(tt) == m_data.end()) {

	identifyTask(transform);

        QString json
            ("\"%1\": [ { \n"
             "  \"annotation_metadata\": {\n"
             "    \"annotation_tools\": \"Sonic Annotator v%2\",\n"
             "    \"data_source\": \"Automatic feature extraction\",\n"
             "    \"annotator\": {\n"
             "%3"
             "    },\n"
             "  },\n"
             "  \"data\": [");
        m_data[tt] = json
            .arg(getTaskKey(m_tasks[transformId]))
            .arg(RUNNER_VERSION)
            .arg(writeTransformToObjectContents(transform));
        justBegun = true;
    }

    QString d = m_data[tt];

    for (int i = 0; i < int(features.size()); ++i) {

        if (i > 0 || !justBegun) {
            d += ",\n";
        } else {
            d += "\n";
        }
        
        d += "    { ";
	
        Plugin::Feature f(features[i]);

        switch (m_tasks[transformId]) {

        case ChordTask:
        case SegmentTask:
        case NoteTask:
        case UnknownTask:
            if (f.hasDuration) {
                d += QString
                    ("\"start\": { \"value\": %1 }, "
                     "\"end\": { \"value\": %2 }")
                    .arg(realTime2Sec(f.timestamp))
                    .arg(realTime2Sec
                         (f.timestamp +
                          (f.hasDuration ? f.duration : Vamp::RealTime::zeroTime)));
                break;
            } else {
                // don't break; fall through to simpler no-duration case
            }
            
        case BeatTask:
        case KeyTask:
        case OnsetTask:
            d += QString("\"time\": { \"value\": %1 }")
                .arg(realTime2Sec(f.timestamp));
            break;

        case MelodyTask:
        case PitchTask:
            //!!!
            break;
        }
        
        if (f.label != "") {
            d += QString(", \"label\": { \"value\": \"%2\" }")
                .arg(f.label.c_str());
        } else if (f.values.size() > 0) {
            d += QString(", \"label\": { \"value\": \"%2\" }")
                .arg(f.values[0]);
        }
            
        d += " }";
    }	

    m_data[tt] = d;
}

void
JAMSFeatureWriter::finish()
{
    cerr << "Finish called on " << this << endl;

    set<QTextStream *> startedStreams;

    for (DataMap::const_iterator i = m_data.begin();
         i != m_data.end(); ++i) {

        TrackTransformPair tt = i->first;
        QString data = i->second;

        QTextStream *sptr = getOutputStream(tt.first, tt.second);
        if (!sptr) {
            throw FailedToOpenOutputStream(tt.first, tt.second);
        }

        if (startedStreams.find(sptr) != startedStreams.end()) {
            *sptr << "," << endl;
        }
        startedStreams.insert(sptr);
        
        *sptr << data << "\n  ]\n} ]";
    }
        
    for (FileStreamMap::const_iterator i = m_streams.begin();
	 i != m_streams.end(); ++i) {
	*(i->second) << endl << "}" << endl;
    }

    m_data.clear();
    m_startedTargets.clear();

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
	    cerr << "      Consider using the --json-network option to retrieve plugin descriptions"  << endl;
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

	    cerr << "WARNING: Cannot currently write dense or track-level outputs to JSON format (only sparse ones). Will proceed using UnknownTask type, but this probably isn't going to work" << endl;
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
        json += ntpl.arg("start").arg(t.getStartTime().toDouble());
    }

    if (t.getDuration() != RealTime::zeroTime) {
        json += ntpl.arg("duration").arg(t.getDuration().toDouble());
    }

    if (t.getSampleRate() != 0) {
        json += ntpl.arg("sample_rate").arg(t.getSampleRate());
    }

    if (!t.getParameters().empty()) {
        json += QString("      \"parameters\": {\n");
        Transform::ParameterMap parameters = t.getParameters();
        for (Transform::ParameterMap::const_iterator i = parameters.begin();
             i != parameters.end(); ++i) {
            QString name = i->first;
            float value = i->second;
            json += QString("        \"%1\": %2\n").arg(name).arg(value);
        }
        json += QString("      },\n");
    }

    // no trailing comma on final property:
    json += QString("      \"transform_id\": \"%1\"\n").arg(t.getIdentifier());

    return json;
}

