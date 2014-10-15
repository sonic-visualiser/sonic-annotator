/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.

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

#ifndef _LAB_FEATURE_WRITER_H_
#define _LAB_FEATURE_WRITER_H_

#include <string>
#include <map>
#include <set>

#include <QString>

#include "transform/FileFeatureWriter.h"

using std::string;
using std::map;

class QTextStream;
class QFile;

class LabFeatureWriter : public FileFeatureWriter
{
public:
    LabFeatureWriter();
    virtual ~LabFeatureWriter();

    virtual string getDescription() const;

    virtual ParameterList getSupportedParameters() const;
    virtual void setParameters(map<string, string> &params);

    virtual void write(QString trackid,
                       const Transform &transform,
                       const Vamp::Plugin::OutputDescriptor &output,
                       const Vamp::Plugin::FeatureList &features,
                       std::string summaryType = "");

    virtual void finish();

    virtual QString getWriterTag() const { return "lab"; }

private:
    bool m_forceEnd;

    typedef pair<QString, Transform> DataId; // track id, transform
    typedef map<DataId, Vamp::Plugin::Feature> PendingFeatures;
    PendingFeatures m_pending;

    void writeFeature(QTextStream &,
                      const Vamp::Plugin::Feature &f,
                      const Vamp::Plugin::Feature *optionalNextFeature);
};

#endif
