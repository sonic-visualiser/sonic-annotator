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

#ifndef JAMS_FEATURE_WRITER_H
#define JAMS_FEATURE_WRITER_H

#include "transform/FileFeatureWriter.h"

#include "rdf/PluginRDFDescription.h"

namespace sv {

class JAMSFileWriter;

class JAMSFeatureWriter : public FileFeatureWriter
{
public:
    JAMSFeatureWriter();
    virtual ~JAMSFeatureWriter();

    std::string getDescription() const;

    virtual ParameterList getSupportedParameters() const;
    virtual void setParameters(std::map<std::string, std::string> &params);

    virtual void setTrackMetadata(QString trackid, TrackMetadata metadata);

    virtual void setNofM(int, int);
    
    virtual void write(QString trackid,
                       const Transform &transform,
                       const Vamp::Plugin::OutputDescriptor &output,
                       const Vamp::Plugin::FeatureList &features,
                       std::string summaryType = "");

    virtual void finish();

    virtual QString getWriterTag() const { return "jams"; }

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

    typedef std::map<QString, PluginRDFDescription> RDFDescriptionMap; // by plugin id
    RDFDescriptionMap m_rdfDescriptions;

    typedef QString TrackId;
    typedef std::pair<TrackId, Transform> DataId;

    typedef std::map<TrackId, TrackMetadata> TrackMetadataMap;
    TrackMetadataMap m_trackMetadata;

    typedef std::set<TrackId> TrackIds;
    typedef std::map<QTextStream *, TrackIds> StreamTrackMap;
    StreamTrackMap m_streamTracks;

    typedef std::set<Task> Tasks;
    typedef std::map<QTextStream *, Tasks> StreamTaskMap;
    StreamTaskMap m_streamTasks;

    typedef std::set<DataId> DataIds;
    typedef std::map<QTextStream *, DataIds> StreamDataMap;
    StreamDataMap m_streamData;

    typedef std::map<DataId, QString> DataMap;
    DataMap m_data;

    typedef std::map<TransformId, Task> TaskMap;
    TaskMap m_tasks;

    void loadRDFDescription(const Transform &);
    void identifyTask(const Transform &);

    QString getTaskKey(Task);

    QString writeTransformToObjectContents(const Transform &);

    std::string m_format;
    bool m_network;
    bool m_networkRetrieved;
    int m_n;
    int m_m;
    int m_digits;
};

}

#endif

