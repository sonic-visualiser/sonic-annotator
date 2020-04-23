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
#include "MultiplexedReader.h"

#include <vamp-hostsdk/PluginChannelAdapter.h>
#include <vamp-hostsdk/PluginBufferingAdapter.h>
#include <vamp-hostsdk/PluginInputDomainAdapter.h>
#include <vamp-hostsdk/PluginSummarisingAdapter.h>
#include <vamp-hostsdk/PluginWrapper.h>
#include <vamp-hostsdk/PluginLoader.h>

#include "base/Debug.h"
#include "base/Exceptions.h"

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
#include "base/TempDirectory.h"
#include "base/ProgressPrinter.h"
#include "transform/TransformFactory.h"
#include "rdf/RDFTransformFactory.h"
#include "transform/FeatureWriter.h"

#include <QTextStream>
#include <QFile>
#include <QFileInfo>

FeatureExtractionManager::FeatureExtractionManager(bool verbose) :
    m_verbose(verbose),
    m_summariesOnly(false),
    // We can read using an arbitrary fixed block size --
    // PluginBufferingAdapter handles this for us. But while this
    // doesn't affect the step and block size actually passed to the
    // plugin, it does affect the overall time range of the audio
    // input (which gets rounded to the nearest block boundary). So
    // although a larger blocksize will normally run faster, and we
    // used a blocksize of 16384 in earlier releases of Sonic
    // Annotator for that reason, a smaller blocksize produces
    // "better" results and this is particularly relevant now we
    // support the start and duration flags for a transform.
    m_blockSize(1024),
    m_defaultSampleRate(0),
    m_sampleRate(0),
    m_channels(0),
    m_normalise(false)
{
}

FeatureExtractionManager::~FeatureExtractionManager()
{
    SVDEBUG << "FeatureExtractionManager::~FeatureExtractionManager: cleaning up"
            << endl;
    
    foreach (AudioFileReader *r, m_readyReaders) {
        delete r;
    }

    // We need to ensure m_allLoadedPlugins outlives anything that
    // holds a shared_ptr to a plugin adapter built from one of the
    // raw plugin pointers. So clear these explicitly, in this order,
    // instead of allowing it to happen automatically

    m_pluginOutputs.clear();
    m_transformPluginMap.clear();
    m_orderedPlugins.clear();
    m_plugins.clear();

    // and last
    
    m_allAdapters.clear();
    m_allLoadedPlugins.clear();
    
    SVDEBUG << "FeatureExtractionManager::~FeatureExtractionManager: done" << endl;
}

void FeatureExtractionManager::setChannels(int channels)
{
    m_channels = channels;
}

void FeatureExtractionManager::setDefaultSampleRate(sv_samplerate_t sampleRate)
{
    m_defaultSampleRate = sampleRate;
}

void FeatureExtractionManager::setNormalise(bool normalise)
{
    m_normalise = normalise;
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

bool
FeatureExtractionManager::setSummaryTypes(const set<string> &names,
                                          const PluginSummarisingAdapter::SegmentBoundaries &boundaries)
{
    for (SummaryNameSet::const_iterator i = names.begin();
         i != names.end(); ++i) {
        if (getSummaryType(*i) == PluginSummarisingAdapter::UnknownSummaryType) {
            SVCERR << "ERROR: Unknown summary type \"" << *i << "\"" << endl;
            return false;
        }
    }
    m_summaries = names;
    m_boundaries = boundaries;
    return true;
}

void
FeatureExtractionManager::setSummariesOnly(bool summariesOnly)
{
    m_summariesOnly = summariesOnly;
}

static PluginInputDomainAdapter::WindowType
convertWindowType(WindowType t)
{
    switch (t) {
    case RectangularWindow:
        return PluginInputDomainAdapter::RectangularWindow;
    case BartlettWindow:
        return PluginInputDomainAdapter::BartlettWindow;
    case HammingWindow:
        return PluginInputDomainAdapter::HammingWindow;
    case HanningWindow:
        return PluginInputDomainAdapter::HanningWindow;
    case BlackmanWindow:
        return PluginInputDomainAdapter::BlackmanWindow;
    case NuttallWindow:
        return PluginInputDomainAdapter::NuttallWindow;
    case BlackmanHarrisWindow:
        return PluginInputDomainAdapter::BlackmanHarrisWindow;
    case GaussianWindow:
    case ParzenWindow:
        // Not supported in Vamp SDK, fall through
    default:
        SVCERR << "ERROR: Unknown or unsupported window type \"" << t << "\", using Hann (\"" << HanningWindow << "\")" << endl;
        return PluginInputDomainAdapter::HanningWindow;
    }
}

bool FeatureExtractionManager::addFeatureExtractor
(Transform transform, const vector<FeatureWriter*> &writers)
{
    //!!! exceptions rather than return values?

    if (transform.getSampleRate() == 0) {
        if (m_sampleRate == 0) {
            SVCERR << "NOTE: Transform does not specify a sample rate, using default rate of " << m_defaultSampleRate << endl;
            transform.setSampleRate(m_defaultSampleRate);
            m_sampleRate = m_defaultSampleRate;
        } else {
            SVCERR << "NOTE: Transform does not specify a sample rate, using previous transform's rate of " << m_sampleRate << endl;
            transform.setSampleRate(m_sampleRate);
        }
    }

    if (m_sampleRate == 0) {
        m_sampleRate = transform.getSampleRate();
    }

    if (transform.getSampleRate() != m_sampleRate) {
        SVCERR << "WARNING: Transform sample rate " << transform.getSampleRate() << " does not match previously specified transform rate of " << m_sampleRate << " -- only a single rate is supported for each run" << endl;
        SVCERR << "WARNING: Using previous rate of " << m_sampleRate << " for this transform as well" << endl;
        transform.setSampleRate(m_sampleRate);
    }

    shared_ptr<Plugin> plugin = nullptr;

    // Remember what the original transform looked like, and index
    // based on this -- because we may be about to fill in the zeros
    // for step and block size, but we want any further copies with
    // the same zeros to match this one
    Transform originalTransform = transform;

    // In a few cases here, after loading the plugin, we create an
    // adapter to wrap it. We always give a raw pointer to the adapter
    // (that's what the API requires). It is safe to use the raw
    // pointer obtained from .get() on the originally loaded
    // shared_ptr, so long as we also stash the shared_ptr somewhere
    // so that it doesn't go out of scope before the adapter is
    // deleted, and we call disownPlugin() on the adapter to prevent
    // the adapter from trying to delete it. We have
    // m_allLoadedPlugins and m_allAdapters as our stashes of
    // shared_ptrs, which share the lifetime of this manager object.
    
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
                SVCERR << "NOTE: Already have transform identical to this one (for \""
                     << transform.getIdentifier().toStdString()
                     << "\") in every detail except output identifier and/or "
                     << "summary type; sharing its plugin instance" << endl;
                plugin = i->second;
                if (transform.getSummaryType() != Transform::NoSummary &&
                    !std::dynamic_pointer_cast<PluginSummarisingAdapter>(plugin)) {
                    // See comment above about safety of raw pointer here
                    auto psa =
                        make_shared<PluginSummarisingAdapter>(plugin.get());
                    psa->disownPlugin();
                    psa->setSummarySegmentBoundaries(m_boundaries);
                    m_allAdapters.insert(psa);
                    plugin = psa;
                    i->second = plugin;
                }
                break;
            }
        }

        if (!plugin) {

            TransformFactory *tf = TransformFactory::getInstance();

            shared_ptr<PluginBase> pb = tf->instantiatePluginFor(transform);
            plugin = dynamic_pointer_cast<Vamp::Plugin>(pb);
                
            if (!plugin) {
                //!!! todo: handle non-Vamp plugins too, or make the main --list
                // option print out only Vamp transforms
                SVCERR << "ERROR: Failed to load plugin for transform \""
                     << transform.getIdentifier().toStdString() << "\"" << endl;
                if (pb) {
                    SVCERR << "NOTE: (A plugin was loaded, but apparently not a Vamp plugin)" << endl;
                }
                return false;
            }

            m_allLoadedPlugins.insert(pb);
            
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

            shared_ptr<PluginInputDomainAdapter> pida = nullptr;

            // adapt the plugin for buffering, channels, etc.
            if (plugin->getInputDomain() == Plugin::FrequencyDomain) {

                // See comment up top about safety of raw pointer here
                pida = make_shared<PluginInputDomainAdapter>(plugin.get());
                pida->disownPlugin();
                pida->setProcessTimestampMethod
                    (PluginInputDomainAdapter::ShiftData);

                PluginInputDomainAdapter::WindowType wtype =
                    convertWindowType(transform.getWindowType());
                pida->setWindowType(wtype);

                m_allAdapters.insert(pida);
                plugin = pida;
            }

            auto pba = make_shared<PluginBufferingAdapter>(plugin.get());
            pba->disownPlugin();

            m_allAdapters.insert(pba);
            plugin = pba;

            if (transform.getStepSize() != 0) {
                pba->setPluginStepSize(transform.getStepSize());
            } else {
                transform.setStepSize(int(pluginStepSize));
            }

            if (transform.getBlockSize() != 0) {
                pba->setPluginBlockSize(transform.getBlockSize());
            } else {
                transform.setBlockSize(int(pluginBlockSize));
            }

            auto pca = make_shared<PluginChannelAdapter>(plugin.get());
            pca->disownPlugin();

            m_allAdapters.insert(pca);
            plugin = pca;

            if (!m_summaries.empty() ||
                transform.getSummaryType() != Transform::NoSummary) {
                auto psa = make_shared<PluginSummarisingAdapter>(plugin.get());
                psa->disownPlugin();
                psa->setSummarySegmentBoundaries(m_boundaries);
                m_allAdapters.insert(psa);
                plugin = psa;
            }

            if (!plugin->initialise(m_channels, m_blockSize, m_blockSize)) {
                SVCERR << "ERROR: Plugin initialise (channels = " << m_channels << ", stepSize = " << m_blockSize << ", blockSize = " << m_blockSize << ") failed." << endl;    
                return false;
            }

            SVDEBUG << "Initialised plugin" << endl;

            size_t actualStepSize = 0;
            size_t actualBlockSize = 0;
            pba->getActualStepAndBlockSizes(actualStepSize, actualBlockSize);
            transform.setStepSize(int(actualStepSize));
            transform.setBlockSize(int(actualBlockSize));

            Plugin::OutputList outputs = plugin->getOutputDescriptors();
            for (int i = 0; i < (int)outputs.size(); ++i) {

                SVDEBUG << "Newly initialised plugin output " << i << " has bin count " << outputs[i].binCount << endl;

                m_pluginOutputs[plugin][outputs[i].identifier] = outputs[i];
                m_pluginOutputIndices[outputs[i].identifier] = i;
            }

            SVCERR << "NOTE: Loaded and initialised plugin for transform \""
                 << transform.getIdentifier().toStdString()
                 << "\" with plugin step size " << actualStepSize
                 << " and block size " << actualBlockSize
                 << " (adapter step and block size " << m_blockSize << ")"
                 << endl;

            SVDEBUG << "NOTE: That transform is: " << transform.toXmlString() << endl;
            
            if (pida) {
                SVCERR << "NOTE: PluginInputDomainAdapter timestamp adjustment is "
                     << pida->getTimestampAdjustment() << endl;
            }

        } else {

            if (transform.getStepSize() == 0 || transform.getBlockSize() == 0) {

                auto pw = dynamic_pointer_cast<PluginWrapper>(plugin);
                if (pw) {
                    PluginBufferingAdapter *pba =
                        pw->getWrapper<PluginBufferingAdapter>();
                    if (pba) {
                        size_t actualStepSize = 0;
                        size_t actualBlockSize = 0;
                        pba->getActualStepAndBlockSizes(actualStepSize,
                                                        actualBlockSize);
                        if (transform.getStepSize() == 0) {
                            transform.setStepSize(int(actualStepSize));
                        }
                        if (transform.getBlockSize() == 0) {
                            transform.setBlockSize(int(actualBlockSize));
                        }
                    }
                }
            }
        }

        if (transform.getPluginVersion() != "") {
            if (QString("%1").arg(plugin->getPluginVersion())
                != transform.getPluginVersion()) {
                SVCERR << "ERROR: Transform specifies version "
                     << transform.getPluginVersion()
                     << " of plugin \"" << plugin->getIdentifier()
                     << "\", but installed plugin is version "
                     << plugin->getPluginVersion()
                     << endl;
                return false;
            }
        }

        if (transform.getOutput() == "") {
            transform.setOutput
                (plugin->getOutputDescriptors()[0].identifier.c_str());
        } else {
            if (m_pluginOutputs[plugin].find
                (transform.getOutput().toLocal8Bit().data()) ==
                m_pluginOutputs[plugin].end()) {
                SVCERR << "ERROR: Transform requests nonexistent plugin output \""
                     << transform.getOutput()
                     << "\"" << endl;
                return false;
            }
        }

        m_transformPluginMap[transform] = plugin;

        SVDEBUG << "NOTE: Assigned plugin " << plugin << " for transform: " << transform.toXmlString() << endl;

        if (!(originalTransform == transform)) {
            m_transformPluginMap[originalTransform] = plugin;
            SVDEBUG << "NOTE: Also assigned plugin " << plugin << " for original transform: " << originalTransform.toXmlString() << endl;
        }

    } else {
        
        plugin = m_transformPluginMap[transform];
    }

    if (m_plugins.find(plugin) == m_plugins.end()) {
        m_orderedPlugins.push_back(plugin);
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
            SVCERR << "ERROR: Default transform requested, but no default sample rate available" << endl;
            return false;
        } else {
            SVCERR << "NOTE: Using default sample rate of " << m_defaultSampleRate << " for default transform" << endl;
            m_sampleRate = m_defaultSampleRate;
        }
    }

    Transform transform = tf->getDefaultTransformFor(transformId, m_sampleRate);

    bool result = addFeatureExtractor(transform, writers);
    if (!result) {
        if (transform.getType() == Transform::UnknownType) {
            SVCERR << "(Maybe mixed up filename with transform, or --transform with --default?)" << endl;
        }
    }
    return result;
}

bool FeatureExtractionManager::addFeatureExtractorFromFile
(QString transformFile, const vector<FeatureWriter*> &writers)
{
    // We support two formats for transform description files, XML (in
    // a format specific to Sonic Annotator) and RDF/Turtle. The RDF
    // format can describe multiple transforms in a single file, the
    // XML only one.
    
    // Possible errors we should report:
    //
    // 1. File does not exist or cannot be opened
    // 2. File is ostensibly XML, but is not parseable
    // 3. File is ostensibly Turtle, but is not parseable
    // 4. File is XML, but contains no valid transform (e.g. is unrelated XML)
    // 5. File is Turtle, but contains no valid transform(s)
    // 6. File is Turtle and contains both valid and invalid transform(s)

    {
        // We don't actually need to open this here yet, we just hoist
        // it to the top for error reporting purposes
        QFile file(transformFile);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            // Error case 1. File does not exist or cannot be opened
            SVCERR << "ERROR: Failed to open transform file \"" << transformFile
                 << "\" for reading" << endl;
            return false;
        }
    }
    
    bool tryRdf = true;
    if (transformFile.endsWith(".xml") || transformFile.endsWith(".XML")) {
        // We don't support RDF-XML (and nor does the underlying
        // parser library) so skip the RDF parse if the filename
        // suggests XML, to avoid puking out a load of errors from
        // feeding XML to a Turtle parser
        tryRdf = false;
    }

    bool tryXml = true;
    if (transformFile.endsWith(".ttl") || transformFile.endsWith(".TTL") ||
        transformFile.endsWith(".ntriples") || transformFile.endsWith(".NTRIPLES") ||
        transformFile.endsWith(".n3") || transformFile.endsWith(".N3")) {
        tryXml = false;
    }

    QString rdfError, xmlError;
    
    if (tryRdf) {

        RDFTransformFactory factory
            (QUrl::fromLocalFile(QFileInfo(transformFile).absoluteFilePath())
             .toString());
        ProgressPrinter printer("Parsing transforms RDF file");
        std::vector<Transform> transforms = factory.getTransforms
            (m_verbose ? &printer : 0);

        if (factory.isOK()) {
            if (transforms.empty()) {
                SVCERR << "ERROR: Transform file \"" << transformFile
                     << "\" is valid RDF but defines no transforms" << endl;
                return false;
            } else {
                bool success = true;
                for (int i = 0; i < (int)transforms.size(); ++i) {
                    if (!addFeatureExtractor(transforms[i], writers)) {
                        success = false;
                    }
                }
                return success;
            }
        } else { // !factory.isOK()
            if (factory.isRDF()) {
                SVCERR << "ERROR: Invalid transform RDF file \"" << transformFile
                     << "\": " << factory.getErrorString() << endl;
                return false;
            }

            // the not-RDF case: fall through without reporting an
            // error, so we try the file as XML, and if that fails, we
            // print a general unparseable-file error
            rdfError = factory.getErrorString();
        }
    }

    if (tryXml) {
        
        QFile file(transformFile);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            SVCERR << "ERROR: Failed to open transform file \""
                 << transformFile.toStdString() << "\" for reading" << endl;
            return false;
        }
        
        QTextStream *qts = new QTextStream(&file);
        QString qs = qts->readAll();
        delete qts;
        file.close();
    
        Transform transform(qs);
        xmlError = transform.getErrorString();

        if (xmlError == "") {

            if (transform.getIdentifier() == "") {
                SVCERR << "ERROR: Transform file \"" << transformFile
                     << "\" is valid XML but defines no transform" << endl;
                return false;
            }

            return addFeatureExtractor(transform, writers);
        }
    }

    SVCERR << "ERROR: Transform file \"" << transformFile
         << "\" could not be parsed" << endl;
    if (rdfError != "") {
        SVCERR << "ERROR: RDF parser reported: " << rdfError << endl;
    }
    if (xmlError != "") {
        SVCERR << "ERROR: XML parser reported: " << xmlError << endl;
    }

    return false;
}

void FeatureExtractionManager::addSource(QString audioSource, bool willMultiplex)
{
    SVCERR << "Have audio source: \"" << audioSource.toStdString() << "\"" << endl;

    // We don't actually do anything with it here, unless it's the
    // first audio source and we need it to establish default channel
    // count and sample rate

    if (m_channels == 0 || m_defaultSampleRate == 0) {

        ProgressPrinter retrievalProgress("Retrieving first input file to determine default rate and channel count...");

        FileSource source(audioSource, m_verbose ? &retrievalProgress : 0);
        if (!source.isAvailable()) {
            SVCERR << "ERROR: File or URL \"" << audioSource.toStdString()
                 << "\" could not be located";
            if (source.getErrorString() != "") {
                SVCERR << ": " << source.getErrorString();
            }
            SVCERR << endl;
            throw FileNotFound(audioSource);
        }
    
        source.waitForData();

        // Open to determine validity, channel count, sample rate only
        // (then close, and open again later with actual desired rate &c)

        AudioFileReaderFactory::Parameters params;
        params.normalisation = (m_normalise ?
                                AudioFileReaderFactory::Normalisation::Peak :
                                AudioFileReaderFactory::Normalisation::None);
        
        AudioFileReader *reader =
            AudioFileReaderFactory::createReader
            (source, params, m_verbose ? &retrievalProgress : 0);
    
        if (!reader) {
            throw FailedToOpenFile(audioSource);
        }

        if (m_verbose) retrievalProgress.done();

        SVCERR << "File or URL \"" << audioSource.toStdString() << "\" opened successfully" << endl;

        if (!willMultiplex) {
            if (m_channels == 0) {
                m_channels = reader->getChannelCount();
                SVCERR << "Taking default channel count of "
                     << reader->getChannelCount() << " from audio file" << endl;
            }
        }

        if (m_defaultSampleRate == 0) {
            m_defaultSampleRate = reader->getNativeRate();
            SVCERR << "Taking default sample rate of "
                 << reader->getNativeRate() << "Hz from audio file" << endl;
            SVCERR << "(Note: Default may be overridden by transforms)" << endl;
        }

        m_readyReaders[audioSource] = reader;
    }

    if (willMultiplex) {
        ++m_channels; // channel count is simply number of sources
        SVCERR << "Multiplexing, incremented target channel count to " 
             << m_channels << endl;
    }
}

void FeatureExtractionManager::extractFeatures(QString audioSource)
{
    if (m_plugins.empty()) return;

    testOutputFiles(audioSource);

    if (m_sampleRate == 0) {
        throw FileOperationFailed
            (audioSource, "internal error: have sources and plugins, but no sample rate");
    }
    if (m_channels == 0) {
        throw FileOperationFailed
            (audioSource, "internal error: have sources and plugins, but no channel count");
    }

    AudioFileReader *reader = prepareReader(audioSource);
    extractFeaturesFor(reader, audioSource); // Note this also deletes reader
}

void FeatureExtractionManager::extractFeaturesMultiplexed(QStringList sources)
{
    if (m_plugins.empty() || sources.empty()) return;

    QString nominalSource = sources[0];

    testOutputFiles(nominalSource);

    if (m_sampleRate == 0) {
        throw FileOperationFailed
            (nominalSource, "internal error: have sources and plugins, but no sample rate");
    }
    if (m_channels == 0) {
        throw FileOperationFailed
            (nominalSource, "internal error: have sources and plugins, but no channel count");
    }

    QList<AudioFileReader *> readers;
    foreach (QString source, sources) {
        AudioFileReader *reader = prepareReader(source);
        readers.push_back(reader);
    }

    AudioFileReader *reader = new MultiplexedReader(readers);
    extractFeaturesFor(reader, nominalSource); // Note this also deletes reader
}

AudioFileReader *
FeatureExtractionManager::prepareReader(QString source)
{
    AudioFileReader *reader = 0;
    if (m_readyReaders.contains(source)) {
        reader = m_readyReaders[source];
        m_readyReaders.remove(source);
        if (reader->getSampleRate() != m_sampleRate) {
            // can't use this; open it again
            delete reader;
            reader = 0;
        }
    }

    if (!reader) {
        ProgressPrinter retrievalProgress("Retrieving audio data...");
        FileSource fs(source, m_verbose ? &retrievalProgress : 0);
        fs.waitForData();

        AudioFileReaderFactory::Parameters params;
        params.targetRate = m_sampleRate;
        params.normalisation = (m_normalise ?
                                AudioFileReaderFactory::Normalisation::Peak :
                                AudioFileReaderFactory::Normalisation::None);
        
        reader = AudioFileReaderFactory::createReader
            (fs, params, m_verbose ? &retrievalProgress : 0);
        if (m_verbose) retrievalProgress.done();
    }
    
    if (!reader) {
        throw FailedToOpenFile(source);
    }
    if (reader->getChannelCount() != m_channels ||
        reader->getNativeRate() != m_sampleRate) {
        SVCERR << "NOTE: File will be mixed or resampled for processing, to: "
             << m_channels << "ch at " 
             << m_sampleRate << "Hz" << endl;
    }
    return reader;
}

void
FeatureExtractionManager::extractFeaturesFor(AudioFileReader *reader,
                                             QString audioSource)
{
    // Note: This also deletes reader

    SVCERR << "Audio file \"" << audioSource.toStdString() << "\": "
         << reader->getChannelCount() << "ch at " 
         << reader->getNativeRate() << "Hz" << endl;

    // allocate audio buffers
    float **data = new float *[m_channels];
    for (int c = 0; c < m_channels; ++c) {
        data[c] = new float[m_blockSize];
    }
    
    struct LifespanMgr { // unintrusive hack introduced to ensure
                         // destruction on exceptions
        AudioFileReader *m_r;
        int m_c;
        float **m_d;
        LifespanMgr(AudioFileReader *r, int c, float **d) :
            m_r(r), m_c(c), m_d(d) { }
        ~LifespanMgr() { destroy(); }
        void destroy() {
            if (!m_r) return;
            delete m_r;
            for (int i = 0; i < m_c; ++i) delete[] m_d[i];
            delete[] m_d;
            m_r = 0;
        }
    };
    LifespanMgr lifemgr(reader, m_channels, data);

    sv_frame_t frameCount = reader->getFrameCount();
    
    SVDEBUG << "FeatureExtractionManager: file has " << frameCount << " frames" << endl;

    sv_frame_t earliestStartFrame = 0;
    sv_frame_t latestEndFrame = frameCount;
    bool haveExtents = false;

    for (auto plugin: m_orderedPlugins) {

        PluginMap::iterator pi = m_plugins.find(plugin);

        SVDEBUG << "FeatureExtractionManager: Calling reset on " << plugin << endl;
        plugin->reset();

        for (TransformWriterMap::iterator ti = pi->second.begin();
             ti != pi->second.end(); ++ti) {

            const Transform &transform = ti->first;

            sv_frame_t startFrame = RealTime::realTime2Frame
                (transform.getStartTime(), m_sampleRate);
            sv_frame_t duration = RealTime::realTime2Frame
                (transform.getDuration(), m_sampleRate);
            if (duration == 0) {
                duration = frameCount - startFrame;
            }

            if (!haveExtents || startFrame < earliestStartFrame) {
                earliestStartFrame = startFrame;
            }
            if (!haveExtents || startFrame + duration > latestEndFrame) {
                latestEndFrame = startFrame + duration;
            }

/*
            SVDEBUG << "startFrame for transform " << startFrame << endl;
            SVDEBUG << "duration for transform " << duration << endl;
            SVDEBUG << "earliestStartFrame becomes " << earliestStartFrame << endl;
            SVDEBUG << "latestEndFrame becomes " << latestEndFrame << endl;
*/
            haveExtents = true;

            string outputId = transform.getOutput().toStdString();
            if (m_pluginOutputs[plugin].find(outputId) ==
                m_pluginOutputs[plugin].end()) {
                // We shouldn't actually reach this point:
                // addFeatureExtractor tests whether the output exists
                SVCERR << "ERROR: Nonexistent plugin output \"" << outputId << "\" requested for transform \""
                     << transform.getIdentifier().toStdString() << "\", ignoring this transform"
                     << endl;

                SVDEBUG << "Known outputs for all plugins are as follows:" << endl;
                for (PluginOutputMap::const_iterator k = m_pluginOutputs.begin();
                     k != m_pluginOutputs.end(); ++k) {
                    SVDEBUG << "Plugin " << k->first << ": ";
                    if (k->second.empty()) {
                        SVDEBUG << "(none)";
                    }
                    for (OutputMap::const_iterator i = k->second.begin();
                         i != k->second.end(); ++i) {
                        SVDEBUG << "\"" << i->first << "\" ";
                    }
                    SVDEBUG << endl;
                }
            }
        }
    }
    
    sv_frame_t startFrame = earliestStartFrame;
    sv_frame_t endFrame = latestEndFrame;
    
    for (auto plugin: m_orderedPlugins) {

        PluginMap::iterator pi = m_plugins.find(plugin);

        for (TransformWriterMap::const_iterator ti = pi->second.begin();
             ti != pi->second.end(); ++ti) {
        
            const vector<FeatureWriter *> &writers = ti->second;
            
            for (int j = 0; j < (int)writers.size(); ++j) {
                FeatureWriter::TrackMetadata m;
                m.title = reader->getTitle();
                m.maker = reader->getMaker();
                m.duration = RealTime::frame2RealTime(reader->getFrameCount(),
                                                      reader->getSampleRate());
                writers[j]->setTrackMetadata(audioSource, m);
            }
        }
    }

    ProgressPrinter extractionProgress("Extracting and writing features...");
    int progress = 0;

    for (sv_frame_t i = startFrame; i < endFrame; i += m_blockSize) {
        
        //!!! inefficient, although much of the inefficiency may be
        // susceptible to compiler optimisation
        
        auto frames = reader->getInterleavedFrames(i, m_blockSize);
        
        // We have to do our own channel handling here; we can't just
        // leave it to the plugin adapter because the same plugin
        // adapter may have to serve for input files with various
        // numbers of channels (so the adapter is simply configured
        // with a fixed channel count).

        int rc = reader->getChannelCount();

        // m_channels is the number of channels we need for the plugin

        int index;
        int fc = (int)frames.size();

        if (m_channels == 1) { // only case in which we can sensibly mix down
            for (int j = 0; j < m_blockSize; ++j) {
                data[0][j] = 0.f;
            }
            for (int c = 0; c < rc; ++c) {
                for (int j = 0; j < m_blockSize; ++j) {
                    index = j * rc + c;
                    if (index < fc) data[0][j] += frames[index];
                }
            }
            for (int j = 0; j < m_blockSize; ++j) {
                data[0][j] /= float(rc);
            }
        } else {                
            for (int c = 0; c < m_channels; ++c) {
                for (int j = 0; j < m_blockSize; ++j) {
                    data[c][j] = 0.f;
                }
                if (c < rc) {
                    for (int j = 0; j < m_blockSize; ++j) {
                        index = j * rc + c;
                        if (index < fc) data[c][j] += frames[index];
                    }
                }
            }
        }                

        RealTime timestamp = RealTime::frame2RealTime(i, m_sampleRate);
        
        for (auto plugin: m_orderedPlugins) {

            PluginMap::iterator pi = m_plugins.find(plugin);

            // Skip any plugin none of whose transforms have come
            // around yet. (Though actually, all transforms for a
            // given plugin must have the same start time -- they can
            // only differ in output and summary type.)
            bool inRange = false;
            for (TransformWriterMap::const_iterator ti = pi->second.begin();
                 ti != pi->second.end(); ++ti) {
                sv_frame_t startFrame = RealTime::realTime2Frame
                    (ti->first.getStartTime(), m_sampleRate);
                if (i >= startFrame || i + m_blockSize > startFrame) {
                    inRange = true;
                    break;
                }
            }
            if (!inRange) {
                continue;
            }

            Plugin::FeatureSet featureSet =
                plugin->process(data, timestamp.toVampRealTime());

            if (!m_summariesOnly) {
                writeFeatures(audioSource, plugin, featureSet);
            }
        }

        int pp = progress;
        progress = int((double(i - startFrame) * 100.0) /
                       double(endFrame - startFrame) + 0.1);
        if (progress > pp && m_verbose) extractionProgress.setProgress(progress);
    }

    SVDEBUG << "FeatureExtractionManager: deleting audio file reader" << endl;

    lifemgr.destroy(); // deletes reader, data
        
    for (auto plugin: m_orderedPlugins) {

        Plugin::FeatureSet featureSet = plugin->getRemainingFeatures();

        if (!m_summariesOnly) {
            writeFeatures(audioSource, plugin, featureSet);
        }

        if (!m_summaries.empty()) {
            // Summaries requested on the command line, for all transforms
            auto adapter =
                dynamic_pointer_cast<PluginSummarisingAdapter>(plugin);
            if (!adapter) {
                SVCERR << "WARNING: Summaries requested, but plugin is not a summarising adapter" << endl;
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
                    writeFeatures(audioSource, plugin, featureSet,
                                  Transform::stringToSummaryType(sni->c_str()));
                }
            }
        }

        // Summaries specified in transform definitions themselves
        writeSummaries(audioSource, plugin);
    }

    if (m_verbose) extractionProgress.done();

    finish();
    
    TempDirectory::getInstance()->cleanup();
}

void
FeatureExtractionManager::writeSummaries(QString audioSource,
                                         shared_ptr<Plugin> plugin)
{
    // caller should have ensured plugin is in m_plugins
    PluginMap::iterator pi = m_plugins.find(plugin);

    for (TransformWriterMap::const_iterator ti = pi->second.begin();
         ti != pi->second.end(); ++ti) {
        
        const Transform &transform = ti->first;

        SVDEBUG << "FeatureExtractionManager::writeSummaries: plugin is " << plugin
                << ", found transform: " << transform.toXmlString() << endl;
        
        Transform::SummaryType summaryType = transform.getSummaryType();
        PluginSummarisingAdapter::SummaryType pType =
            (PluginSummarisingAdapter::SummaryType)summaryType;

        if (transform.getSummaryType() == Transform::NoSummary) {
            SVDEBUG << "FeatureExtractionManager::writeSummaries: no summary for this transform" << endl;
            continue;
        }

        auto adapter = dynamic_pointer_cast<PluginSummarisingAdapter>(plugin);
        if (!adapter) {
            SVCERR << "FeatureExtractionManager::writeSummaries: INTERNAL ERROR: Summary requested for transform, but plugin is not a summarising adapter" << endl;
            continue;
        }

        Plugin::FeatureSet featureSet = adapter->getSummaryForAllOutputs
            (pType, PluginSummarisingAdapter::ContinuousTimeAverage);

        SVDEBUG << "summary type " << int(pType) << " for transform:" << endl << transform.toXmlString().toStdString()<< endl << "... feature set with " << featureSet.size() << " elts" << endl;

        writeFeatures(audioSource, plugin, featureSet, summaryType);
    }
}

void FeatureExtractionManager::writeFeatures(QString audioSource,
                                             shared_ptr<Plugin> plugin,
                                             const Plugin::FeatureSet &features,
                                             Transform::SummaryType summaryType)
{
    // caller should have ensured plugin is in m_plugins
    PluginMap::iterator pi = m_plugins.find(plugin);

    // Write features from the feature set passed in, according to the
    // transforms listed for the given plugin with the given summary type
    
    for (TransformWriterMap::const_iterator ti = pi->second.begin();
         ti != pi->second.end(); ++ti) {
        
        const Transform &transform = ti->first;
        const vector<FeatureWriter *> &writers = ti->second;

//        SVDEBUG << "writeFeatures: plugin " << plugin << " has transform: " << transform.toXmlString() << endl;

        if (transform.getSummaryType() == Transform::NoSummary &&
            !m_summaries.empty()) {
            SVDEBUG << "writeFeatures: transform has no summary, but summaries requested on command line, so going for it anyway" << endl;
        } else if (transform.getSummaryType() != summaryType) {
            // Either we're not writing a summary and the transform
            // has one, or we're writing a summary but the transform
            // has none or a different one; either way, skip it
            SVDEBUG << "writeFeatures: transform summary type " << transform.getSummaryType() << " differs from passed-in one " << summaryType << ", skipping" << endl;
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

//        SVDEBUG << "this transform has " << writers.size() << " writer(s)" << endl;
        
        for (int j = 0; j < (int)writers.size(); ++j) {
            writers[j]->write
                (audioSource, transform, desc, fsi->second,
                 Transform::summaryTypeToString(summaryType).toStdString());
        }
    }
}

void FeatureExtractionManager::testOutputFiles(QString audioSource)
{
    for (PluginMap::iterator pi = m_plugins.begin();
         pi != m_plugins.end(); ++pi) {

        for (TransformWriterMap::iterator ti = pi->second.begin();
             ti != pi->second.end(); ++ti) {
        
            vector<FeatureWriter *> &writers = ti->second;

            for (int i = 0; i < (int)writers.size(); ++i) {
                writers[i]->testOutputFile(audioSource, ti->first.getIdentifier());
            }
        }
    }
}

void FeatureExtractionManager::finish()
{
    for (auto plugin: m_orderedPlugins) {

        PluginMap::iterator pi = m_plugins.find(plugin);

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
