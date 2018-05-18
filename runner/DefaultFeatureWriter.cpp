/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
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

#include <iostream>
#include <map>

using namespace std;

#include "DefaultFeatureWriter.h"

#include <QTextStream>
#include <QTextCodec>

DefaultFeatureWriter::DefaultFeatureWriter() :
    FileFeatureWriter(SupportStdOut,
                      "xml")
{
}

DefaultFeatureWriter::~DefaultFeatureWriter()
{
}

string
DefaultFeatureWriter::getDescription() const
{
    return "Write features in a generic XML format, with <feature> or <summary> elements containing output name and some or all of timestamp, duration, values, and label.";
}

static QString
toQStringAsStream(const RealTime &rt)
{
    // just for historical compatibility, get the same formatting as
    // when streaming to an iostream
    std::stringstream out;
    out << rt;
    std::string s = out.str();
    return QString::fromStdString(s);
}

void DefaultFeatureWriter::write(QString trackId,
                                 const Transform &transform,
                                 const Vamp::Plugin::OutputDescriptor& output,
                                 const Vamp::Plugin::FeatureList& features,
                                 std::string summaryType)
{
    // Select appropriate output file for our track/transform
    // combination

    TransformId transformId = transform.getIdentifier();

    QTextStream *sptr = getOutputStream
        (trackId, transformId, QTextCodec::codecForName("UTF-8"));
    if (!sptr) {
        throw FailedToOpenOutputStream(trackId, transformId);
    }

    QTextStream &stream = *sptr;

    int n = int(features.size());

    if (n == 0) return;
    
    /* we write a generic XML output of the form
     
       <feature>
          <name>output.name</name>
          <timestamp>feature.timestamp</timestamp>    
          <values>output.binName[0]:feature.value[0]...</values>
          <label>feature.label</label>
       </feature>
     */
    
    for (int i = 0; i < n; ++i) {
        if (summaryType == "") {
            stream << "<feature>" << endl;
        } else {
            stream << "<summary type=\"" << QString::fromStdString(summaryType)
                   << "\">" << endl;
        }
        stream << "\t<name>" << QString::fromStdString(output.name)
               << "</name>" << endl;
        if (features[i].hasTimestamp) {
            stream << "\t<timestamp>"
                   << toQStringAsStream(features[i].timestamp)
                   << "</timestamp>" << endl;    
        }
        if (features[i].hasDuration) {
            stream << "\t<duration>"
                   << toQStringAsStream(features[i].duration)
                   << "</duration>" << endl;    
        }
        if (features[i].values.size() > 0)
        {
            stream << "\t<values>";
            for (int j = 0; j < (int)features[i].values.size(); ++j) {
                if (j > 0) {
                    stream << " ";
                }
                if (output.binNames.size() > 0) {
                    stream << QString::fromStdString(output.binNames[j]) << ":";
                }
                stream << features[i].values[j];
            }
            stream << "</values>" << endl;
        }
        if (features[i].label.length() > 0)
            stream << "\t<label>"
                   << QString::fromStdString(features[i].label)
                   << "</label>" << endl;
        if (summaryType == "") {
            stream << "</feature>" << endl;
        } else {
            stream << "</summary>" << endl;
        }
    }
}
