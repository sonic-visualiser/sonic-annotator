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

#ifndef _DEFAULT_FEATURE_WRITER_H_
#define _DEFAULT_FEATURE_WRITER_H_


#include "transform/FeatureWriter.h"

class DefaultFeatureWriter : public FeatureWriter
{
public:
    virtual ~DefaultFeatureWriter() { }
    virtual string getDescription() const;
    virtual void write(QString trackid,
                       const Transform &transform,
                       const Vamp::Plugin::OutputDescriptor &output,
                       const Vamp::Plugin::FeatureList &features,
                       std::string summaryType = "");
    virtual void finish() { }
    virtual QString getWriterTag() const { return "default"; }
};

#endif
