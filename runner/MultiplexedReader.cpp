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
    
    SVDEBUG << "MultiplexedReader: channel count: " << m_channelCount
            << " (i.e. " << m_channelCount << " reader(s) to multiplex)" << endl;
    SVDEBUG << "MultiplexedReader: sample rate from first reader: "
            << m_sampleRate << endl;
    
    m_frameCount = 0;
    m_quicklySeekable = true;
    
    foreach (AudioFileReader *r, m_readers) {
	if (!r->isOK()) {
	    m_channelCount = 0;
	    m_error = r->getError();
        } else if (r->getSampleRate() != m_sampleRate) {
            m_channelCount = 0;
            m_error = "Readers provided to MultiplexedReader must have the same sample rate";
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

floatvec_t
MultiplexedReader::getInterleavedFrames(sv_frame_t start, sv_frame_t frameCount) const
{
    int out_chans = m_readers.size();

    // Allocate and zero
    floatvec_t block(frameCount * out_chans, 0.f);

    for (int out_chan = 0; out_chan < out_chans; ++out_chan) {

	AudioFileReader *reader = m_readers[out_chan];
	auto readerBlock = reader->getInterleavedFrames(start, frameCount);

	int in_chans = reader->getChannelCount();

	for (int frame = 0; frame < frameCount; ++frame) {

            int out_index = frame * out_chans + out_chan;

	    for (int in_chan = 0; in_chan < in_chans; ++in_chan) {
                int in_index = frame * in_chans + in_chan;
                if (in_index >= (int)readerBlock.size()) break;
		block[out_index] += readerBlock[in_index];
	    }

            if (in_chans > 1) {
                block[out_index] /= float(in_chans);
            }
	}
    }

    return block;
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


    
