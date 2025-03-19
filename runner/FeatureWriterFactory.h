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


#ifndef FEATURE_WRITER_FACTORY_H
#define FEATURE_WRITER_FACTORY_H

#include <set>
#include <string>

namespace sv {

class FeatureWriter;

class FeatureWriterFactory
{
public:
    static std::set<std::string> getWriterTags();
    static FeatureWriter *createWriter(std::string tag);
};

}

#endif
