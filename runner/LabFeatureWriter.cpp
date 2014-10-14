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
    return "Write features in .lab, a tab-separated columnar format. The first column is always the feature start time in seconds. If the features have duration, the second column will be the feature end time in seconds. Remaining columns are the feature values (if any) and finally the feature label (if any). There is no identification of the audio file or the transform, so confusion will result if features from different audio or transforms are mixed. For more control over the output, consider using the CSV writer.";
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
                        std::string summaryType)
{
    // Select appropriate output file for our track/transform
    // combination

    QTextStream *sptr = getOutputStream(trackId, transform.getIdentifier());
    if (!sptr) {
        throw FailedToOpenOutputStream(trackId, transform.getIdentifier());
    }

    QTextStream &stream = *sptr;

    QString sep = "\t";

    for (unsigned int i = 0; i < features.size(); ++i) {

        QString timestamp = features[i].timestamp.toString().c_str();
        timestamp.replace(QRegExp("^ +"), "");
        stream << timestamp;

        Vamp::RealTime endTime;
        bool haveEndTime = true;

        if (features[i].hasDuration) {
            endTime = features[i].timestamp + features[i].duration;
        } else if (m_forceEnd) {
            if (i+1 < features.size()) {
                endTime = features[i+1].timestamp;
            } else {
                //!!! what to do??? can we get the end time of the input file?
                endTime = features[i].timestamp;
            }
        } else {
            haveEndTime = false;
        }

        if (haveEndTime) {
            QString e = endTime.toString().c_str();
            e.replace(QRegExp("^ +"), "");
            stream << sep << e;
        }

        for (unsigned int j = 0; j < features[i].values.size(); ++j) {
            stream << sep << features[i].values[j];
        }

        if (features[i].label != "") {
            stream << sep << "\"" << features[i].label.c_str() << "\"";
        }

        stream << "\n";
    }
}


