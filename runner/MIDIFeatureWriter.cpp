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

//#define DEBUG_MIDI_FEATURE_WRITER 1

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

string
MIDIFeatureWriter::getDescription() const
{
    return "Write features to MIDI files. All features are written as MIDI notes. If a feature has at least one value, its first value will be used as the note pitch, the second value (if present) for velocity. If a feature has units of Hz, then its pitch will be converted from frequency to an integer value in MIDI range; otherwise it will simply be rounded to the nearest integer and written directly. (Be aware that MIDI cannot represent overlapping notes of equal pitch, so features with durations may be misrepresented if they do not have distinct enough values.) Multiple (up to 16) transforms can be written to a single MIDI file, where they will be given separate MIDI channel numbers.";
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

    if (m_fileTransforms[filename].find(transform) == 
        m_fileTransforms[filename].end()) {

        // This transform is new to the file, give it a channel number

        int channel = m_nextChannels[filename];
        m_nextChannels[filename] = channel + 1;

        m_fileTransforms[filename].insert(transform);
        m_channels[transform] = channel;
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

#ifdef DEBUG_MIDI_FEATURE_WRITER
        cerr << "feature timestamp = " << feature.timestamp << ", sampleRate = " << sampleRate << ", frame = " << frame << endl;
        cerr << "feature duration = " << feature.duration << ", sampleRate = " << sampleRate << ", duration = " << duration << endl;
#endif
        
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

        note.channel = m_channels[transform];

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

            if (!writer.isOK()) {
		cerr << "ERROR: Failed to write to MIDI file: " 
		     << writer.getError() << endl;
		throw FileOperationFailed(filename, "MIDI write");
            }
	}
    }

    m_notes.clear();

    FileFeatureWriter::finish();
}

