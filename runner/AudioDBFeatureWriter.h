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

#ifndef _AUDIO_DB_FEATURE_WRITER_H_
#define _AUDIO_DB_FEATURE_WRITER_H_

#include <string>
#include <map>

using std::string;
using std::map;

#include "transform/FeatureWriter.h"

class AudioDBFeatureWriter : public FeatureWriter
{
public:
    AudioDBFeatureWriter();
    virtual ~AudioDBFeatureWriter();

    virtual string getDescription() const;
    
    virtual ParameterList getSupportedParameters() const;
    virtual void setParameters(map<string, string> &params);

    virtual void setCatalogueId(const string &);
    virtual void setBaseDirectory(const string &);

    virtual void write(QString trackid,
                       const Transform &transform,
                       const Vamp::Plugin::OutputDescriptor &output,
                       const Vamp::Plugin::FeatureList &features,
                       std::string summaryType = "");
    
    virtual void finish() { }

    virtual QString getWriterTag() const { return "audiodb"; }

private:
    string catalogueId;
    string baseDir;

    static string catalogueIdParam;
    static string baseDirParam;
    
    struct TrackStream;
    map<string, TrackStream> dbfiles;
    
    bool openDBFile(QString trackid, const string& identifier);
    bool replaceDBFile(QString trackid, const string& identifier);
};

#endif
