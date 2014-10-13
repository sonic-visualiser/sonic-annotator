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

string
DefaultFeatureWriter::getDescription() const
{
    return "Write features in a generic XML format, with <feature> or <summary> elements containing output name and some or all of timestamp, duration, values, and label.";
}

void DefaultFeatureWriter::write(QString,
                                 const Transform &,
                                 const Vamp::Plugin::OutputDescriptor& output,
                                 const Vamp::Plugin::FeatureList& featureList,
                                 std::string summaryType)
{
    // generic XML output
    
    /*
     
     <feature>
        <name>output.name</name>
        <timestamp>feature.timestamp</timestamp>    
        <values>output.binName[0]:feature.value[0]...</values>
        <label>feature.label</label>
     </feature>
     
     */
    
    for (int i = 0; i < (int)featureList.size(); ++i) {

        if (summaryType == "") {
            cout << "<feature>" << endl;
        } else {
            cout << "<summary type=\"" << summaryType << "\">" << endl;
        }
        cout << "\t<name>" << output.name << "</name>" << endl;
        if (featureList[i].hasTimestamp) {
            cout << "\t<timestamp>" << featureList[i].timestamp << "</timestamp>" << endl;    
        }
        if (featureList[i].hasDuration) {
            cout << "\t<duration>" << featureList[i].duration << "</duration>" << endl;    
        }
        if (featureList[i].values.size() > 0)
        {
            cout << "\t<values>";
            for (int j = 0; j < (int)featureList[i].values.size(); ++j) {
                if (j > 0)
                    cout << " ";
                if (output.binNames.size() > 0)
                    cout << output.binNames[j] << ":";
                cout << featureList[i].values[j];
            }
            cout << "</values>" << endl;
        }
        if (featureList[i].label.length() > 0)
            cout << "\t<label>" << featureList[i].label << "</label>" << endl;            
        if (summaryType == "") {
            cout << "</feature>" << endl;
        } else {
            cout << "</summary>" << endl;
        }
    }
}
