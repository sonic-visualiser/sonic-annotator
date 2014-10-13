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


#include "FeatureWriterFactory.h"

#include "DefaultFeatureWriter.h"
#include "rdf/RDFFeatureWriter.h"
#include "AudioDBFeatureWriter.h"
#include "MIDIFeatureWriter.h"
#include "transform/CSVFeatureWriter.h"

set<string>
FeatureWriterFactory::getWriterTags()
{
    set<string> tags;
    tags.insert("default");
    tags.insert("rdf");
    tags.insert("audiodb");
    tags.insert("csv");
    tags.insert("midi");
    return tags;
}

FeatureWriter *
FeatureWriterFactory::createWriter(string tag)
{
    if (tag == "default") {
        return new DefaultFeatureWriter();
    } else if (tag == "rdf") {
        return new RDFFeatureWriter();
    } else if (tag == "audiodb") {
        return new AudioDBFeatureWriter();
    } else if (tag == "csv") {
        return new CSVFeatureWriter();
    } else if (tag == "midi") {
        return new MIDIFeatureWriter();
    }

    return 0;
}
