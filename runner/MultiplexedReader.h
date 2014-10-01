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

#ifndef _MULTIPLEXED_READER_H_
#define _MULTIPLEXED_READER_H_

#include "data/fileio/AudioFileReader.h"

#include <QString>
#include <QList>

typedef std::vector<float> SampleBlock;

class MultiplexedReader : public QObject
{
    Q_OBJECT

public:
    MultiplexedReader(QList<AudioFileReader *> readers);
    virtual ~MultiplexedReader() { }

    //!!! the rest of this is currently still from AudioFileReader.h! Finish!




    bool isOK() const { return (m_channelCount > 0); }

    virtual QString getError() const { return ""; }

    int getFrameCount() const { return m_frameCount; }
    int getChannelCount() const { return m_channelCount; }
    int getSampleRate() const { return m_sampleRate; }

    virtual int getNativeRate() const { return m_sampleRate; } // if resampled

    /**
     * Return true if this file supports fast seek and random
     * access. Typically this will be true for uncompressed formats
     * and false for compressed ones.
     */
    virtual bool isQuicklySeekable() const = 0;

    /** 
     * Return interleaved samples for count frames from index start.
     * The resulting sample block will contain count *
     * getChannelCount() samples (or fewer if end of file is reached).
     *
     * The subclass implementations of this function must be
     * thread-safe -- that is, safe to call from multiple threads with
     * different arguments on the same object at the same time.
     */
    virtual void getInterleavedFrames(int start, int count,
				      SampleBlock &frames) const = 0;

    /**
     * Return de-interleaved samples for count frames from index
     * start.  Implemented in this class (it calls
     * getInterleavedFrames and de-interleaves).  The resulting vector
     * will contain getChannelCount() sample blocks of count samples
     * each (or fewer if end of file is reached).
     */
    virtual void getDeInterleavedFrames(int start, int count,
                                        std::vector<SampleBlock> &frames) const;

    // only subclasses that do not know exactly how long the audio
    // file is until it's been completely decoded should implement this
    virtual int getDecodeCompletion() const { return 100; } // %

    virtual bool isUpdating() const { return false; }

signals:
    void frameCountChanged();
    
protected:
    int m_frameCount;
    int m_channelCount;
    int m_sampleRate;
};

#endif
