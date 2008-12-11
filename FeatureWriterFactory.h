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


#ifndef _FEATURE_WRITER_FACTORY_H_
#define _FEATURE_WRITER_FACTORY_H_

#include <set>
#include <string>

using std::set;
using std::string;

class FeatureWriter;

class FeatureWriterFactory
{
public:
    static set<string> getWriterTags();
    static FeatureWriter *createWriter(string tag);
};


#endif
