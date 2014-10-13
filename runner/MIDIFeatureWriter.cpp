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

#include "MIDIFeatureWriter.h"

using namespace std;
using Vamp::Plugin;
using Vamp::PluginBase;

#include "base/Exceptions.h"
#include "data/fileio/MIDIFileWriter.h"

MIDIFeatureWriter::MIDIFeatureWriter() :
    FileFeatureWriter(SupportOneFilePerTrackTransform |
                      SupportOneFilePerTrack |
                      SupportOneFileTotal,
                      "mid")
{
}

MIDIFeatureWriter::~MIDIFeatureWriter()
{
}

MIDIFeatureWriter::ParameterList
MIDIFeatureWriter::getSupportedParameters() const
{
    ParameterList pl = FileFeatureWriter::getSupportedParameters();
    return pl;
}

void
MIDIFeatureWriter::setParameters(map<string, string> &params)
{
    FileFeatureWriter::setParameters(params);
}

void
MIDIFeatureWriter::setTrackMetadata(QString, TrackMetadata)
{
    cerr << "MIDIFeatureWriter::setTrackMetadata: not supported (yet?)" << endl;
}

void
MIDIFeatureWriter::write(QString trackId,
			 const Transform &transform,
			 const Plugin::OutputDescriptor& output,
			 const Plugin::FeatureList& features,
			 std::string /* summaryType */)
{
    QString transformId = transform.getIdentifier();

    QString filename = getOutputFilename(trackId, transformId);
    if (filename == "") {
	throw FailedToOpenOutputStream(trackId, transformId);
    }

    int sampleRate = transform.getSampleRate();

    if (m_rates.find(filename) == m_rates.end()) {
        m_rates[filename] = sampleRate;
    }

    if (m_fileTransforms[filename].find(transformId) == 
        m_fileTransforms[filename].end()) {

        // This transform is new to the file, give it a channel number

        int channel = m_nextChannels[filename];
        m_nextChannels[filename] = channel + 1;

        m_fileTransforms[filename].insert(transformId);
        m_channels[transformId] = channel;
    }

    NoteList notes = m_notes[filename];

    bool freq = (output.unit == "Hz" || 
                 output.unit == "hz" || 
                 output.unit == "HZ");

    for (int i = 0; i < (int)features.size(); ++i) {

        const Plugin::Feature &feature(features[i]);

        Vamp::RealTime timestamp = feature.timestamp;
        int frame = Vamp::RealTime::realTime2Frame(timestamp, sampleRate);

        int duration = 1;
        if (feature.hasDuration) {
            duration = Vamp::RealTime::realTime2Frame(feature.duration, sampleRate);
        }
        
        int pitch = 60;
        if (feature.values.size() > 0) {
            float pval = feature.values[0];
            if (freq) {
                pitch = Pitch::getPitchForFrequency(pval);
            } else {
                pitch = int(pval + 0.5);
            }
        }

        int velocity = 100;
        if (feature.values.size() > 1) {
            float vval = feature.values[1];
            if (vval < 128) {
                velocity = int(vval + 0.5);
            }
        }

        NoteData note(frame, duration, pitch, velocity);

        note.channel = m_channels[transformId];

        notes.push_back(note);
    }

    m_notes[filename] = notes;
}

void
MIDIFeatureWriter::finish()
{
    for (NoteMap::const_iterator i = m_notes.begin(); i != m_notes.end(); ++i) {

	QString filename = i->first;
	NoteList notes = i->second;
	float rate = m_rates[filename];

	TrivialNoteExportable exportable(notes);

	{
	    MIDIFileWriter writer(filename, &exportable, rate);
	    if (!writer.isOK()) {
		cerr << "ERROR: Failed to create MIDI writer: " 
		     << writer.getError() << endl;
		throw FileOperationFailed(filename, "create MIDI writer");
	    }
	    writer.write();
	}
    }
}

