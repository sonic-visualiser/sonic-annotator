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

#ifndef _JAMS_FEATURE_WRITER_H_
#define _JAMS_FEATURE_WRITER_H_

#include "transform/FileFeatureWriter.h"

#include "rdf/PluginRDFDescription.h"

class JAMSFileWriter;

class JAMSFeatureWriter : public FileFeatureWriter
{
public:
    JAMSFeatureWriter();
    virtual ~JAMSFeatureWriter();

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

    virtual QString getWriterTag() const { return "json"; }

private:
    enum Task {
	UnknownTask,
	BeatTask,
	OnsetTask,
	ChordTask,
	SegmentTask,
	KeyTask,
	NoteTask,
	MelodyTask,
	PitchTask,
    };

    typedef map<QString, PluginRDFDescription> RDFDescriptionMap; // by plugin id
    RDFDescriptionMap m_rdfDescriptions;

    typedef map<QString, QString> TrackMetadataMap; // track id -> json object
    TrackMetadataMap m_metadata;

    typedef map<TrackTransformPair, QString> DataMap;
    DataMap m_data;

    typedef map<QString, Task> TaskMap; // by transform id
    TaskMap m_tasks;

    typedef set<TrackTransformPair> StartedSet;
    StartedSet m_startedTargets;

    void loadRDFDescription(const Transform &);
    void identifyTask(const Transform &);

    QString getTaskKey(Task);

    QString writeTransformToObjectContents(const Transform &);

    bool m_network;
    bool m_networkRetrieved;
};

#endif

