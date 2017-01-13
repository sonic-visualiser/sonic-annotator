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

class MultiplexedReader : public AudioFileReader
{
    Q_OBJECT

public:
    // I take ownership of readers
    MultiplexedReader(QList<AudioFileReader *> readers);
    virtual ~MultiplexedReader();

    virtual QString getError() const override { return m_error; }
    virtual bool isQuicklySeekable() const override { return m_quicklySeekable; }

    virtual floatvec_t getInterleavedFrames
    (sv_frame_t start, sv_frame_t count) const override;

    virtual int getDecodeCompletion() const override;

    virtual bool isUpdating() const override;

protected:
    QString m_error;
    bool m_quicklySeekable;
    QList<AudioFileReader *> m_readers;
};

#endif
