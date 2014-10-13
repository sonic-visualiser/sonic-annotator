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

#include <fstream>

#include <QFileInfo>

#include "AudioDBFeatureWriter.h"

using namespace std;
using namespace Vamp;

string
AudioDBFeatureWriter::catalogueIdParam = "catid";

string
AudioDBFeatureWriter::baseDirParam = "basedir";

struct AudioDBFeatureWriter::TrackStream
{
    QString trackid;
    ofstream* ofs;
};

AudioDBFeatureWriter::AudioDBFeatureWriter() : 
    catalogueId("catalog"), baseDir("audiodb")
{
    
}

AudioDBFeatureWriter::~AudioDBFeatureWriter()
{
    // close all open files
    for (map<string, TrackStream>::iterator iter = dbfiles.begin(); iter != dbfiles.end(); ++iter)
    {
        if (iter->second.ofs) {
            iter->second.ofs->close();
            delete iter->second.ofs;
        }
    }
    
    // TODO: error handling on close
}

AudioDBFeatureWriter::ParameterList
AudioDBFeatureWriter::getSupportedParameters() const
{
    ParameterList pl;
    Parameter p;

    p.name = catalogueIdParam;
    p.description = "Catalogue ID";
    p.hasArg = true;
    pl.push_back(p);

    p.name = baseDirParam;
    p.description = "Base output directory path";
    p.hasArg = true;
    pl.push_back(p);

    return pl;
}

void
AudioDBFeatureWriter::setParameters(map<string, string> &params)
{
    if (params.find(catalogueIdParam) != params.end()) {
        setCatalogueId(params[catalogueIdParam]);
        params.erase(catalogueIdParam);
    }
    if (params.find(baseDirParam) != params.end()) {
        setBaseDirectory(params[baseDirParam]);
        params.erase(baseDirParam);
    }
}

void
AudioDBFeatureWriter::setCatalogueId(const string &catid)
{
    catalogueId = catid;
}

void
AudioDBFeatureWriter::setBaseDirectory(const string &base)
{
    baseDir = base;
}

void AudioDBFeatureWriter::write(QString trackid,
                                 const Transform &,
                                 const Vamp::Plugin::OutputDescriptor& output, 
                                 const Vamp::Plugin::FeatureList& featureList,
                                 std::string summaryType)
{
    //!!! use summaryType
    if (summaryType != "") {
        //!!! IMPLEMENT
        cerr << "ERROR: AudioDBFeatureWriter::write: Writing summaries is not yet implemented!" << endl;
        exit(1);
    }


    // binary output for FeatureSet
    
    // feature-dimension feature-1 feature-2 ...
    // timestamp-1 timestamp-2 ...
    
    // audioDB has to write each feature to a different file
    // assume a simple naming convention of
    // <catalog-id>/<track-id>.<feature-id>
    // with timestamps in a corresponding <catalog-id>/<track-id>.<feature-id>.timestamp file
    // (start and end times in seconds for each frame -- somewhat optional)
    
    // the feature writer holds a map of open file descriptors
    // the catalog-id is passed in to the feature writer's constructor    
    
    // NB -- all "floats" in the file should in fact be doubles

    // TODO:
    // - write feature end rather than start times, once end time is available in vamp
    // - write a power file, probably by wrapping plugin in a PluginPowerAdapter :)
    
    if (output.binCount == 0)    // this kind of feature just outputs timestamps and labels, assume of no interest to audioDB
        return;    
        
    for (int i = 0; i < (int)featureList.size(); ++i) {

        // replace output files if necessary
        if (replaceDBFile(trackid, output.identifier))
        {                
            // write the feature length for the next track feature record
            // binCount has to be set
            // - it can be zero, i.e. if the output is really a set of labels + timestamps
            *dbfiles[output.identifier].ofs /*<< ios::binary*/ << output.binCount;
            
            cerr << "writing bin count " << output.binCount << " for " << output.identifier << endl;
        }
        
        if (replaceDBFile(trackid, output.identifier + ".timestamp"))
        {
            // write the start time to the timestamp file
            // as we want it for the first feature in the file
            *dbfiles[output.identifier + ".timestamp"].ofs << featureList[i].timestamp.toString() << endl;
        }

        if (dbfiles[output.identifier].ofs) {
            for (int j = 0; j < (int) featureList[i].values.size(); ++j)
                *dbfiles[output.identifier].ofs /*<< ios::binary*/ << featureList[i].values[j];
        
            // write the *end* time of each feature to the timestamp file
            // NOT IMPLEMENTED YET
//            *dbfiles[output.identifier + ".timestamp"].ofs << featureList[i].timestamp.toString() << endl;
        }
    }
}

bool AudioDBFeatureWriter::openDBFile(QString trackid, const string& identifier)
{
    QString trackBase = QFileInfo(trackid).fileName();
    string filepath = baseDir + "/" + catalogueId  + "/"
        + trackBase.toStdString() + "." + identifier;
    cerr << "AudioDBFeatureWriter::openDBFile: filepath is \"" << filepath << "\"" << endl;
    ofstream* ofs = new ofstream(filepath.c_str());
    if (!*ofs)
    {    
        cerr << "ERROR AudioDBFeatureWriter::openDBFile(): can't open file " << filepath << endl;
        return false;
    }
    TrackStream ts;
    ts.trackid = trackid;
    ts.ofs = ofs;
    dbfiles[identifier] = ts;
    return true;
}

// replace file if no file open for this track, else return false
bool AudioDBFeatureWriter::replaceDBFile(QString trackid, 
                                         const string& identifier)
{
    if (dbfiles.find(identifier) != dbfiles.end() && dbfiles[identifier].trackid == trackid)
        return false;    // have an open file for this track
    
    if (dbfiles.find(identifier) != dbfiles.end() && dbfiles[identifier].trackid != trackid)
    {
        // close the current file
        if (dbfiles[identifier].ofs) {
            dbfiles[identifier].ofs->close();
            delete dbfiles[identifier].ofs;
            dbfiles[identifier].ofs = 0;
        }
    }
    
    // open a new file
    if (!openDBFile(trackid, identifier)) {
        dbfiles[identifier].ofs = 0;
        return false; //!!! should throw an exception, otherwise we'll try to open the file again and again every time we want to write to it
    }
    
    return true;
}
    
    
