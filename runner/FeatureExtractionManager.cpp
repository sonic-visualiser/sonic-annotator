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

#include "FeatureExtractionManager.h"

#include <vamp-hostsdk/PluginChannelAdapter.h>
#include <vamp-hostsdk/PluginBufferingAdapter.h>
#include <vamp-hostsdk/PluginInputDomainAdapter.h>
#include <vamp-hostsdk/PluginSummarisingAdapter.h>
#include <vamp-hostsdk/PluginWrapper.h>
#include <vamp-hostsdk/PluginLoader.h>

#include <iostream>

using namespace std;

using Vamp::Plugin;
using Vamp::PluginBase;
using Vamp::HostExt::PluginLoader;
using Vamp::HostExt::PluginChannelAdapter;
using Vamp::HostExt::PluginBufferingAdapter;
using Vamp::HostExt::PluginInputDomainAdapter;
using Vamp::HostExt::PluginSummarisingAdapter;
using Vamp::HostExt::PluginWrapper;

#include "data/fileio/FileSource.h"
#include "data/fileio/AudioFileReader.h"
#include "data/fileio/AudioFileReaderFactory.h"
#include "data/fileio/PlaylistFileReader.h"
#include "base/TempDirectory.h"
#include "base/ProgressPrinter.h"
#include "transform/TransformFactory.h"
#include "rdf/RDFTransformFactory.h"
#include "transform/FeatureWriter.h"

#include <QTextStream>
#include <QFile>
#include <QFileInfo>

FeatureExtractionManager::FeatureExtractionManager() :
    m_summariesOnly(false),
    // We can read using an arbitrary fixed block size --
    // PluginBufferingAdapter handles this for us.  It's likely to be
    // quicker to use larger sizes than smallish ones like 1024
    m_blockSize(16384),
    m_defaultSampleRate(0),
    m_sampleRate(0),
    m_channels(1)
{
}

FeatureExtractionManager::~FeatureExtractionManager()
{
    for (PluginMap::iterator pi = m_plugins.begin();
         pi != m_plugins.end(); ++pi) {
        delete pi->first;
    }
}

void FeatureExtractionManager::setChannels(int channels)
{
    m_channels = channels;
}

void FeatureExtractionManager::setDefaultSampleRate(int sampleRate)
{
    m_defaultSampleRate = sampleRate;
}

static PluginSummarisingAdapter::SummaryType
getSummaryType(string name)
{
    if (name == "min")      return PluginSummarisingAdapter::Minimum;
    if (name == "max")      return PluginSummarisingAdapter::Maximum;
    if (name == "mean")     return PluginSummarisingAdapter::Mean;
    if (name == "median")   return PluginSummarisingAdapter::Median;
    if (name == "mode")     return PluginSummarisingAdapter::Mode;
    if (name == "sum")      return PluginSummarisingAdapter::Sum;
    if (name == "variance") return PluginSummarisingAdapter::Variance;
    if (name == "sd")       return PluginSummarisingAdapter::StandardDeviation;
    if (name == "count")    return PluginSummarisingAdapter::Count;
    return PluginSummarisingAdapter::UnknownSummaryType;
}

bool FeatureExtractionManager::setSummaryTypes(const set<string> &names,
                                               bool summariesOnly,
                                               const PluginSummarisingAdapter::SegmentBoundaries &boundaries)
{
    for (SummaryNameSet::const_iterator i = names.begin();
         i != names.end(); ++i) {
        if (getSummaryType(*i) == PluginSummarisingAdapter::UnknownSummaryType) {
            cerr << "ERROR: Unknown summary type \"" << *i << "\"" << endl;
            return false;
        }
    }
    m_summaries = names;
    m_summariesOnly = summariesOnly;
    m_boundaries = boundaries;
    return true;
}

bool FeatureExtractionManager::addFeatureExtractor
(Transform transform, const vector<FeatureWriter*> &writers)
{
    //!!! exceptions rather than return values?

    if (transform.getSampleRate() == 0) {
        if (m_sampleRate == 0) {
            cerr << "NOTE: Transform does not specify a sample rate, using default rate of " << m_defaultSampleRate << endl;
            transform.setSampleRate(m_defaultSampleRate);
            m_sampleRate = m_defaultSampleRate;
        } else {
            cerr << "NOTE: Transform does not specify a sample rate, using previous transform's rate of " << m_sampleRate << endl;
            transform.setSampleRate(m_sampleRate);
        }
    }

    if (m_sampleRate == 0) {
        m_sampleRate = transform.getSampleRate();
    }

    if (transform.getSampleRate() != m_sampleRate) {
        cerr << "WARNING: Transform sample rate " << transform.getSampleRate() << " does not match previously specified transform rate of " << m_sampleRate << " -- only a single rate is supported for each run" << endl;
        cerr << "WARNING: Using previous rate of " << m_sampleRate << " for this transform as well" << endl;
        transform.setSampleRate(m_sampleRate);
    }

    Plugin *plugin = 0;

    // Remember what the original transform looked like, and index
    // based on this -- because we may be about to fill in the zeros
    // for step and block size, but we want any further copies with
    // the same zeros to match this one
    Transform originalTransform = transform;
    
    if (m_transformPluginMap.find(transform) == m_transformPluginMap.end()) {

        // Test whether we already have a transform that is identical
        // to this, except for the output requested and/or the summary
        // type -- if so, they should share plugin instances (a vital
        // optimisation)

        for (TransformPluginMap::iterator i = m_transformPluginMap.begin();
             i != m_transformPluginMap.end(); ++i) {
            Transform test = i->first;
            test.setOutput(transform.getOutput());
            test.setSummaryType(transform.getSummaryType());
            if (transform == test) {
                cerr << "NOTE: Already have transform identical to this one (for \""
                     << transform.getIdentifier().toStdString()
                     << "\") in every detail except output identifier and/or "
                     << "summary type; sharing its plugin instance" << endl;
                plugin = i->second;
                if (transform.getSummaryType() != Transform::NoSummary &&
                    !dynamic_cast<PluginSummarisingAdapter *>(plugin)) {
                    plugin = new PluginSummarisingAdapter(plugin);
                    i->second = plugin;
                }
                break;
            }
        }

        if (!plugin) {

            TransformFactory *tf = TransformFactory::getInstance();

            PluginBase *pb = tf->instantiatePluginFor(transform);
            plugin = tf->downcastVampPlugin(pb);
            if (!plugin) {
                //!!! todo: handle non-Vamp plugins too, or make the main --list
                // option print out only Vamp transforms
                cerr << "ERROR: Failed to load plugin for transform \""
                     << transform.getIdentifier().toStdString() << "\"" << endl;
                delete pb;
                return false;
            }
            
            // We will provide the plugin with arbitrary step and
            // block sizes (so that we can use the same read/write
            // block size for all transforms), and to that end we use
            // a PluginBufferingAdapter.  However, we need to know the
            // underlying step size so that we can provide the right
            // context for dense outputs.  (Although, don't forget
            // that the PluginBufferingAdapter rewrites
            // OneSamplePerStep outputs so as to use FixedSampleRate
            // -- so it supplies the sample rate in the output
            // feature.  I'm not sure whether we can easily use that.)

            size_t pluginStepSize = plugin->getPreferredStepSize();
            size_t pluginBlockSize = plugin->getPreferredBlockSize();

            // adapt the plugin for buffering, channels, etc.
            if (plugin->getInputDomain() == Plugin::FrequencyDomain) {
                plugin = new PluginInputDomainAdapter(plugin);
            }

            PluginBufferingAdapter *pba = new PluginBufferingAdapter(plugin);
            plugin = pba;

            if (transform.getStepSize() != 0) {
                pba->setPluginStepSize(transform.getStepSize());
            } else {
                transform.setStepSize(pluginStepSize);
            }

            if (transform.getBlockSize() != 0) {
                pba->setPluginBlockSize(transform.getBlockSize());
            } else {
                transform.setBlockSize(pluginBlockSize);
            }

            plugin = new PluginChannelAdapter(plugin);

            if (!m_summaries.empty() ||
                transform.getSummaryType() != Transform::NoSummary) {
                PluginSummarisingAdapter *adapter =
                    new PluginSummarisingAdapter(plugin);
                adapter->setSummarySegmentBoundaries(m_boundaries);
                plugin = adapter;
            }

            if (!plugin->initialise(m_channels, m_blockSize, m_blockSize)) {
                cerr << "ERROR: Plugin initialise (channels = " << m_channels << ", stepSize = " << m_blockSize << ", blockSize = " << m_blockSize << ") failed." << endl;    
                delete plugin;
                return false;
            }

//            cerr << "Initialised plugin" << endl;

            size_t actualStepSize = 0;
            size_t actualBlockSize = 0;
            pba->getActualStepAndBlockSizes(actualStepSize, actualBlockSize);
            transform.setStepSize(actualStepSize);
            transform.setBlockSize(actualBlockSize);

            Plugin::OutputList outputs = plugin->getOutputDescriptors();
            for (int i = 0; i < (int)outputs.size(); ++i) {

//                cerr << "Newly initialised plugin output " << i << " has bin count " << outputs[i].binCount << endl;

                m_pluginOutputs[plugin][outputs[i].identifier] = outputs[i];
                m_pluginOutputIndices[outputs[i].identifier] = i;
            }

            cerr << "NOTE: Loaded and initialised plugin for transform \""
                 << transform.getIdentifier().toStdString() << "\"" << endl;

        } else {

            if (transform.getStepSize() == 0 || transform.getBlockSize() == 0) {

                PluginWrapper *pw = dynamic_cast<PluginWrapper *>(plugin);
                if (pw) {
                    PluginBufferingAdapter *pba =
                        pw->getWrapper<PluginBufferingAdapter>();
                    if (pba) {
                        size_t actualStepSize = 0;
                        size_t actualBlockSize = 0;
                        pba->getActualStepAndBlockSizes(actualStepSize,
                                                        actualBlockSize);
                        if (transform.getStepSize() == 0) {
                            transform.setStepSize(actualStepSize);
                        }
                        if (transform.getBlockSize() == 0) {
                            transform.setBlockSize(actualBlockSize);
                        }
                    }
                }
            }
        }

        if (transform.getOutput() == "") {
            transform.setOutput
                (plugin->getOutputDescriptors()[0].identifier.c_str());
        }

        m_transformPluginMap[transform] = plugin;

        if (!(originalTransform == transform)) {
            m_transformPluginMap[originalTransform] = plugin;
        }

    } else {
        
        plugin = m_transformPluginMap[transform];
    }

    m_plugins[plugin][transform] = writers;

    return true;
}

bool FeatureExtractionManager::addDefaultFeatureExtractor
(TransformId transformId, const vector<FeatureWriter*> &writers)
{
    TransformFactory *tf = TransformFactory::getInstance();

    if (m_sampleRate == 0) {
        if (m_defaultSampleRate == 0) {
            cerr << "ERROR: Default transform requested, but no default sample rate available" << endl;
            return false;
        } else {
            cerr << "NOTE: Using default sample rate of " << m_defaultSampleRate << " for default transform" << endl;
            m_sampleRate = m_defaultSampleRate;
        }
    }

    Transform transform = tf->getDefaultTransformFor(transformId, m_sampleRate);

    return addFeatureExtractor(transform, writers);
}

bool FeatureExtractionManager::addFeatureExtractorFromFile
(QString transformXmlFile, const vector<FeatureWriter*> &writers)
{
    RDFTransformFactory factory
        (QUrl::fromLocalFile(QFileInfo(transformXmlFile).absoluteFilePath())
         .toString());
    ProgressPrinter printer("Parsing transforms RDF file");
    std::vector<Transform> transforms = factory.getTransforms(&printer);
    if (!factory.isOK()) {
        cerr << "WARNING: FeatureExtractionManager::addFeatureExtractorFromFile: Failed to parse transforms file: " << factory.getErrorString().toStdString() << endl;
        if (factory.isRDF()) {
            return false; // no point trying it as XML
        }
    }
    if (!transforms.empty()) {
        bool success = true;
        for (int i = 0; i < (int)transforms.size(); ++i) {
            if (!addFeatureExtractor(transforms[i], writers)) {
                success = false;
            }
        }
        return success;
    }

    QFile file(transformXmlFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        cerr << "ERROR: Failed to open transform XML file \""
             << transformXmlFile.toStdString() << "\" for reading" << endl;
        return false;
    }

    QTextStream *qts = new QTextStream(&file);
    QString qs = qts->readAll();
    delete qts;
    file.close();

    Transform transform(qs);

    return addFeatureExtractor(transform, writers);
}

void FeatureExtractionManager::extractFeatures(QString audioSource)
{
    if (m_plugins.empty()) return;

    ProgressPrinter retrievalProgress("Retrieving audio data...");

    FileSource source(audioSource, &retrievalProgress);
    if (!source.isAvailable()) {
        cerr << "ERROR: File or URL \"" << audioSource.toStdString()
             << "\" could not be located" << endl;
        exit(1);
    }
    
    source.waitForData();
    
    if (QFileInfo(audioSource).suffix().toLower() == "m3u") {
        PlaylistFileReader reader(source);
        if (reader.isOK()) {
            vector<QString> files = reader.load();
            for (int i = 0; i < (int)files.size(); ++i) {
                extractFeatures(files[i]);
            }
            return;
        } else {
            cerr << "ERROR: Playlist \"" << audioSource.toStdString()
                 << "\" could not be opened" << endl;
            exit(1);
        }
    }

    if (m_sampleRate == 0) {
        cerr << "ERROR: Internal error in FeatureExtractionManager::extractFeatures: Plugin list is non-empty, but no sample rate set" << endl;
        exit(1);
    }

    AudioFileReader *reader =
        AudioFileReaderFactory::createReader(source, m_sampleRate, &retrievalProgress);
    
    if (!reader) {
        cerr << "ERROR: File or URL \"" << audioSource.toStdString()
             << "\" could not be opened" << endl;
        exit(1);
    }

    size_t channels = reader->getChannelCount();

    retrievalProgress.done();

    cerr << "Opened " << channels << "-channel file or URL \"" << audioSource.toStdString() << "\"" << endl;

    // reject file if it has too few channels, plugin will handle if it has too many
    if ((int)channels < m_channels) {
        //!!! should not be terminating here!
        cerr << "ERROR: File or URL \"" << audioSource.toStdString() << "\" has less than " << m_channels << " channels" << endl;
        exit(1);
    }
    
    // allocate audio buffers
    float **data = new float *[m_channels];
    for (int c = 0; c < m_channels; ++c) {
        data[c] = new float[m_blockSize];
    }

    size_t frameCount = reader->getFrameCount();
    
//    cerr << "file has " << frameCount << " frames" << endl;

    for (PluginMap::iterator pi = m_plugins.begin();
         pi != m_plugins.end(); ++pi) {

        Plugin *plugin = pi->first;

//        std::cerr << "Calling reset on " << plugin << std::endl;
        plugin->reset();

        for (TransformWriterMap::iterator ti = pi->second.begin();
             ti != pi->second.end(); ++ti) {

            const Transform &transform = ti->first;

            //!!! we may want to set the start and duration times for extraction
            // in the transform record (defaults of zero indicate extraction
            // from the whole file)
//            transform.setStartTime(RealTime::zeroTime);
//            transform.setDuration
//                (RealTime::frame2RealTime(reader->getFrameCount(), m_sampleRate));

            string outputId = transform.getOutput().toStdString();
            if (m_pluginOutputs[plugin].find(outputId) ==
                m_pluginOutputs[plugin].end()) {
                //!!! throw?
                cerr << "WARNING: Nonexistent plugin output \"" << outputId << "\" requested for transform \""
                     << transform.getIdentifier().toStdString() << "\", ignoring this transform"
                     << endl;
/*
                cerr << "Known outputs for all plugins are as follows:" << endl;
                for (PluginOutputMap::const_iterator k = m_pluginOutputs.begin();
                     k != m_pluginOutputs.end(); ++k) {
                    cerr << "Plugin " << k->first << ": ";
                    if (k->second.empty()) {
                        cerr << "(none)";
                    }
                    for (OutputMap::const_iterator i = k->second.begin();
                         i != k->second.end(); ++i) {
                        cerr << "\"" << i->first << "\" ";
                    }
                    cerr << endl;
                }
*/
            }
        }
    }
    
    long startFrame = 0;
    long endFrame = frameCount;

/*!!! No -- there is no single transform to pull this stuff from --
 * the transforms may have various start and end times, need to be far
 * cleverer about this if we're going to support them

    RealTime trStartRT = transform.getStartTime();
    RealTime trDurationRT = transform.getDuration();

    long trStart = RealTime::realTime2Frame(trStartRT, m_sampleRate);
    long trDuration = RealTime::realTime2Frame(trDurationRT, m_sampleRate);

    if (trStart == 0 || trStart < startFrame) {
        trStart = startFrame;
    }

    if (trDuration == 0) {
        trDuration = endFrame - trStart;
    }
    if (trStart + trDuration > endFrame) {
        trDuration = endFrame - trStart;
    }

    startFrame = trStart;
    endFrame = trStart + trDuration;
*/
    
    for (PluginMap::iterator pi = m_plugins.begin();
         pi != m_plugins.end(); ++pi) { 

        for (TransformWriterMap::const_iterator ti = pi->second.begin();
             ti != pi->second.end(); ++ti) {
        
            const vector<FeatureWriter *> &writers = ti->second;
            
            for (int j = 0; j < (int)writers.size(); ++j) {
                FeatureWriter::TrackMetadata m;
                m.title = reader->getTitle();
                m.maker = reader->getMaker();
                writers[j]->setTrackMetadata(audioSource, m);
            }
        }
    }

    ProgressPrinter extractionProgress("Extracting and writing features...");
    int progress = 0;

    for (long i = startFrame; i < endFrame; i += m_blockSize) {
        
        //!!! inefficient, although much of the inefficiency may be
        // susceptible to optimisation
        
        SampleBlock frames;
        reader->getInterleavedFrames(i, m_blockSize, frames);
        
        // We have to do our own channel handling here; we can't just
        // leave it to the plugin adapter because the same plugin
        // adapter may have to serve for input files with various
        // numbers of channels (so the adapter is simply configured
        // with a fixed channel count, generally 1).

        int rc = reader->getChannelCount();

        for (int j = 0; j < m_blockSize; ++j) {
            for (int c = 0; c < m_channels; ++c) {
                int index;
                if (c < rc) {
                    index = j * rc + c;
                    data[c][j] = 0.f;
                } else {
                    index = j * rc + (c % rc);
                }
                if (index < (int)frames.size()) {
                    data[c][j] += frames[index];
                }
            }
        }    

        Vamp::RealTime timestamp = Vamp::RealTime::frame2RealTime
            (i, m_sampleRate);
        
        for (PluginMap::iterator pi = m_plugins.begin();
             pi != m_plugins.end(); ++pi) {

            Plugin *plugin = pi->first;
            Plugin::FeatureSet featureSet = plugin->process(data, timestamp);

            if (!m_summariesOnly) {
                writeFeatures(audioSource, plugin, featureSet);
            }
        }

        int pp = progress;
        progress = int(((i - startFrame) * 100.0) / (endFrame - startFrame) + 0.1);
        if (progress > pp) extractionProgress.setProgress(progress);
    }

    delete reader;
    
    for (PluginMap::iterator pi = m_plugins.begin();
         pi != m_plugins.end(); ++pi) { 

        Plugin *plugin = pi->first;
        Plugin::FeatureSet featureSet = plugin->getRemainingFeatures();

        if (!m_summariesOnly) {
            writeFeatures(audioSource, plugin, featureSet);
        }

        if (!m_summaries.empty()) {
            PluginSummarisingAdapter *adapter =
                dynamic_cast<PluginSummarisingAdapter *>(plugin);
            if (!adapter) {
                cerr << "WARNING: Summaries requested, but plugin is not a summarising adapter" << endl;
            } else {
                for (SummaryNameSet::const_iterator sni = m_summaries.begin();
                     sni != m_summaries.end(); ++sni) {
                    featureSet.clear();
                    //!!! problem here -- we are requesting summaries
                    //!!! for all outputs, but they in principle have
                    //!!! different averaging requirements depending
                    //!!! on whether their features have duration or
                    //!!! not
                    featureSet = adapter->getSummaryForAllOutputs
                        (getSummaryType(*sni),
                         PluginSummarisingAdapter::ContinuousTimeAverage);
                    writeFeatures(audioSource, plugin, featureSet,//!!! *sni);
                                  Transform::stringToSummaryType(sni->c_str()));
                }
            }
        }

        writeSummaries(audioSource, plugin);
    }

    extractionProgress.done();

    finish();
    
    TempDirectory::getInstance()->cleanup();
}

void
FeatureExtractionManager::writeSummaries(QString audioSource, Plugin *plugin)
{
    // caller should have ensured plugin is in m_plugins
    PluginMap::iterator pi = m_plugins.find(plugin);

    for (TransformWriterMap::const_iterator ti = pi->second.begin();
         ti != pi->second.end(); ++ti) {
        
        const Transform &transform = ti->first;
        const vector<FeatureWriter *> &writers = ti->second;

        Transform::SummaryType summaryType = transform.getSummaryType();
        PluginSummarisingAdapter::SummaryType pType =
            (PluginSummarisingAdapter::SummaryType)summaryType;

        if (transform.getSummaryType() == Transform::NoSummary) {
            continue;
        }

        PluginSummarisingAdapter *adapter =
            dynamic_cast<PluginSummarisingAdapter *>(plugin);
        if (!adapter) {
            cerr << "FeatureExtractionManager::writeSummaries: INTERNAL ERROR: Summary requested for transform, but plugin is not a summarising adapter" << endl;
            continue;
        }

        Plugin::FeatureSet featureSet = adapter->getSummaryForAllOutputs
            (pType, PluginSummarisingAdapter::ContinuousTimeAverage);

//        cout << "summary type " << int(pType) << " for transform:" << endl << transform.toXmlString().toStdString()<< endl << "... feature set with " << featureSet.size() << " elts" << endl;

        writeFeatures(audioSource, plugin, featureSet, summaryType);
    }
}

void FeatureExtractionManager::writeFeatures(QString audioSource,
                                             Plugin *plugin,
                                             const Plugin::FeatureSet &features,
                                             Transform::SummaryType summaryType)
{
    // caller should have ensured plugin is in m_plugins
    PluginMap::iterator pi = m_plugins.find(plugin);

    for (TransformWriterMap::const_iterator ti = pi->second.begin();
         ti != pi->second.end(); ++ti) {
        
        const Transform &transform = ti->first;
        const vector<FeatureWriter *> &writers = ti->second;
        
        if (transform.getSummaryType() != Transform::NoSummary &&
            m_summaries.empty() &&
            summaryType == Transform::NoSummary) {
            continue;
        }

        if (transform.getSummaryType() != Transform::NoSummary &&
            summaryType != Transform::NoSummary &&
            transform.getSummaryType() != summaryType) {
            continue;
        }

        string outputId = transform.getOutput().toStdString();

        if (m_pluginOutputs[plugin].find(outputId) ==
            m_pluginOutputs[plugin].end()) {
            continue;
        }
        
        const Plugin::OutputDescriptor &desc =
            m_pluginOutputs[plugin][outputId];
        
        int outputIndex = m_pluginOutputIndices[outputId];
        Plugin::FeatureSet::const_iterator fsi = features.find(outputIndex);
        if (fsi == features.end()) continue;

        for (int j = 0; j < (int)writers.size(); ++j) {
            writers[j]->write
                (audioSource, transform, desc, fsi->second,
                 Transform::summaryTypeToString(summaryType).toStdString());
        }
    }
}

void FeatureExtractionManager::finish()
{
    for (PluginMap::iterator pi = m_plugins.begin();
         pi != m_plugins.end(); ++pi) {

        for (TransformWriterMap::iterator ti = pi->second.begin();
             ti != pi->second.end(); ++ti) {
        
            vector<FeatureWriter *> &writers = ti->second;

            for (int i = 0; i < (int)writers.size(); ++i) {
                writers[i]->flush();
                writers[i]->finish();
            }
        }
    }
}

void FeatureExtractionManager::print(Transform transform) const
{
    QString qs;
    QTextStream qts(&qs);
    transform.toXml(qts);
    cerr << qs.toStdString() << endl;
}
