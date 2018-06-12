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

#ifndef _MIDI_FEATURE_WRITER_H_
#define _MIDI_FEATURE_WRITER_H_

#include "transform/FileFeatureWriter.h"
#include "data/model/NoteData.h"

class MIDIFileWriter;

class MIDIFeatureWriter : public FileFeatureWriter
{
public:
    MIDIFeatureWriter();
    virtual ~MIDIFeatureWriter();

    string getDescription() const;

    virtual ParameterList getSupportedParameters() const;
    virtual void setParameters(map<string, string> &params);

    virtual void setTrackMetadata(QString trackid, TrackMetadata metadata);

    virtual void write(QString trackid,
                       const Transform &transform,
                       const Vamp::Plugin::OutputDescriptor &output,
                       const Vamp::Plugin::FeatureList &features,
                       std::string summaryType = "");

    virtual void finish();

    virtual QString getWriterTag() const { return "midi"; }

private:
    class TrivialNoteExportable : public NoteExportable {
    public:
	TrivialNoteExportable(NoteList notes) : m_notes(notes) { }
	virtual NoteList getNotes() const {
	    return m_notes;
	}
	virtual NoteList getNotesWithin(sv_frame_t, sv_frame_t) const {
	    // Not required by MIDIFileWriter, not supported
	    return NoteList();
	}
    private:
	NoteList m_notes;
    };

    typedef map<QString, NoteList> NoteMap; // output filename -> notes
    NoteMap m_notes;
    
    typedef map<QString, set<Transform> > FileTransformMap;
    FileTransformMap m_fileTransforms;

    typedef map<QString, sv_samplerate_t> SampleRateMap; // NoteData uses sample timing
    SampleRateMap m_rates;

    typedef map<Transform, int> ChannelMap;
    ChannelMap m_channels;
    
    typedef map<QString, int> NextChannelMap;
    NextChannelMap m_nextChannels;
};

#endif

