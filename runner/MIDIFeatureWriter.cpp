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
			 std::string summaryType)
{
    QString filename = getOutputFilename(trackId, transform.getIdentifier());
    if (filename == "") {
	throw FailedToOpenOutputStream(trackId, transform.getIdentifier());
    }

    //!!! implement!
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

