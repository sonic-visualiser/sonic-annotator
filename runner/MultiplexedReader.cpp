/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Annotator
    A utility for batch feature extraction from audio files.
    Mark Levy, Chris Sutton and Chris Cannam, Queen Mary, University of London.
    Copyright 2007-2014 QMUL.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "MultiplexedReader.h"

MultiplexedReader::MultiplexedReader(QList<AudioFileReader *> readers) : 
    m_readers(readers)
{
    m_channelCount = readers.size();
    m_sampleRate = readers[0]->getSampleRate();
    
    m_frameCount = 0;
    m_quicklySeekable = true;
    
    foreach (AudioFileReader *r, m_readers) {
	if (!r->isOK()) {
	    m_channelCount = 0;
	    m_error = r->getError();
	} else {
	    if (r->getFrameCount() > m_frameCount) {
		m_frameCount = r->getFrameCount();
	    }
	    if (!r->isQuicklySeekable()) {
		m_quicklySeekable = false;
	    }
	}
    }
}

MultiplexedReader::~MultiplexedReader()
{
    foreach (AudioFileReader *r, m_readers) {
	delete r;
    }
}

void
MultiplexedReader::getInterleavedFrames(int start, int count,
					SampleBlock &frames) const
{
    int nr = m_readers.size();

    frames = SampleBlock(count * nr);

    for (int ri = 0; ri < nr; ++ri) {

	AudioFileReader *reader = m_readers[ri];
	SampleBlock rs(count * reader->getChannelCount());

	reader->getInterleavedFrames(start, count, rs);

	int nc = reader->getChannelCount();
	for (int i = 0; i < count; ++i) {
	    for (int c = 0; c < nc; ++c) {
		frames[i * nr + ri] += rs[i * nc + c];
	    }
	}
    }
}

int
MultiplexedReader::getDecodeCompletion() const
{
    int completion = 100;
    foreach (AudioFileReader *r, m_readers) {
	int c = r->getDecodeCompletion();
	if (c < 100) {
	    completion = c;
	}
    }
    return completion;
}

bool
MultiplexedReader::isUpdating() const
{
    foreach (AudioFileReader *r, m_readers) {
	if (r->isUpdating()) return true;
    }
    return false;
}


    
