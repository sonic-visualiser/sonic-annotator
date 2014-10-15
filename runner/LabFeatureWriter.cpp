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

#include "LabFeatureWriter.h"

#include <iostream>

#include <QRegExp>
#include <QTextStream>

using namespace std;
using namespace Vamp;

LabFeatureWriter::LabFeatureWriter() :
    FileFeatureWriter(SupportOneFilePerTrackTransform |
                      SupportStdOut,
                      "lab"),
    m_forceEnd(false)
{
}

LabFeatureWriter::~LabFeatureWriter()
{
}

string
LabFeatureWriter::getDescription() const
{
    return "Write features in .lab, a tab-separated columnar format. The first column is always the feature start time in seconds. If the features have duration, the second column will be the feature end time in seconds. Remaining columns are the feature values (if any) and finally the feature label (if any). There is no identification of the audio file or the transform, so confusion will result if features from different audio or transforms are mixed. (For more control over the output, consider using the more general CSV writer.)";
}

LabFeatureWriter::ParameterList
LabFeatureWriter::getSupportedParameters() const
{
    ParameterList pl = FileFeatureWriter::getSupportedParameters();

    Parameter p;

    p.name = "fill-ends";
    p.description = "Include end times even for features without duration, by using the gap to the next feature instead.";
    p.hasArg = false;
    pl.push_back(p);

    return pl;
}

void
LabFeatureWriter::setParameters(map<string, string> &params)
{
    FileFeatureWriter::setParameters(params);

    for (map<string, string>::iterator i = params.begin();
         i != params.end(); ++i) {
        if (i->first == "fill-ends") {
            m_forceEnd = true;
        }
    }
}

void
LabFeatureWriter::write(QString trackId,
                        const Transform &transform,
                        const Plugin::OutputDescriptor& ,
                        const Plugin::FeatureList& features,
                        std::string)
{
    // Select appropriate output file for our track/transform
    // combination

    TransformId transformId = transform.getIdentifier();

    QTextStream *sptr = getOutputStream(trackId, transformId);
    if (!sptr) {
        throw FailedToOpenOutputStream(trackId, transformId);
    }

    QTextStream &stream = *sptr;

    int n = features.size();

    if (n == 0) return;

    TrackTransformPair tt(trackId, transformId);

    if (m_pending.find(tt) != m_pending.end()) {
        writeFeature(stream, m_pending[tt], &features[0]);
        m_pending.erase(tt);
    }

    if (m_forceEnd) {
        // can't write final feature until we know its end time
        --n;
        m_pending[tt] = features[n];
    }

    for (int i = 0; i < n; ++i) {
        writeFeature(stream, features[i], m_forceEnd ? &features[i+1] : 0);
    }
}

void
LabFeatureWriter::finish()
{
    for (PendingFeatures::const_iterator i = m_pending.begin();
         i != m_pending.end(); ++i) {
        TrackTransformPair tt = i->first;
        Plugin::Feature f = i->second;
        QTextStream *sptr = getOutputStream(tt.first, tt.second);
        if (!sptr) {
            throw FailedToOpenOutputStream(tt.first, tt.second);
        }
        QTextStream &stream = *sptr;
        // final feature has its own time as end time (we can't
        // reliably determine the end of audio file, and because of
        // the nature of block processing, the feature could even
        // start beyond that anyway)
        writeFeature(stream, f, &f);
    }

    m_pending.clear();
}

void
LabFeatureWriter::writeFeature(QTextStream &stream,
                               const Plugin::Feature &f,
                               const Plugin::Feature *optionalNextFeature)
{
    QString sep = "\t";

    QString timestamp = f.timestamp.toString().c_str();
    timestamp.replace(QRegExp("^ +"), "");
    stream << timestamp;

    Vamp::RealTime endTime;
    bool haveEndTime = true;

    if (f.hasDuration) {
        endTime = f.timestamp + f.duration;
    } else if (optionalNextFeature) {
        endTime = optionalNextFeature->timestamp;
    } else {
        haveEndTime = false;
    }

    if (haveEndTime) {
        QString e = endTime.toString().c_str();
        e.replace(QRegExp("^ +"), "");
        stream << sep << e;
    }
    
    for (unsigned int j = 0; j < f.values.size(); ++j) {
        stream << sep << f.values[j];
    }
    
    if (f.label != "") {
        stream << sep << "\"" << f.label.c_str() << "\"";
    }
    
    stream << "\n";
}


