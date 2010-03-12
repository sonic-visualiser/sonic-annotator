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

#ifndef _FEATURE_EXTRACTION_MANAGER_H_
#define _FEATURE_EXTRACTION_MANAGER_H_

#include <vector>
#include <set>
#include <string>

#include <vamp-hostsdk/Plugin.h>
#include <vamp-hostsdk/PluginSummarisingAdapter.h>
#include <transform/Transform.h>

using std::vector;
using std::set;
using std::string;
using std::pair;
using std::map;

class FeatureWriter;

class FeatureExtractionManager
{
public:
    FeatureExtractionManager();
    virtual ~FeatureExtractionManager();

    void setChannels(int channels);
    void setDefaultSampleRate(int sampleRate);

    bool setSummaryTypes(const set<string> &summaryTypes,
                         bool summariesOnly,
                         const Vamp::HostExt::PluginSummarisingAdapter::SegmentBoundaries &boundaries);

    bool addFeatureExtractor(Transform transform,
                             const vector<FeatureWriter*> &writers);

    bool addFeatureExtractorFromFile(QString transformXmlFile,
                                     const vector<FeatureWriter*> &writers);

    bool addDefaultFeatureExtractor(TransformId transformId,
                                    const vector<FeatureWriter*> &writers);

    void extractFeatures(QString audioSource);

private:
    // A plugin may have many outputs, so we can have more than one
    // transform requested for a single plugin.  The things we want to
    // run in our process loop are plugins rather than their outputs,
    // so we maintain a map from the plugins to the transforms desired
    // of them and then iterate through this map

    typedef map<Transform, vector<FeatureWriter *> > TransformWriterMap;
    typedef map<Vamp::Plugin *, TransformWriterMap> PluginMap;
    PluginMap m_plugins;
        
    // And a map back from transforms to their plugins.  Note that
    // this is keyed by transform, not transform ID -- two differently
    // configured transforms with the same ID must use different
    // plugin instances.

    typedef map<Transform, Vamp::Plugin *> TransformPluginMap;
    TransformPluginMap m_transformPluginMap;

    // Cache the plugin output descriptors, mapping from plugin to a
    // map from output ID to output descriptor.
    typedef map<string, Vamp::Plugin::OutputDescriptor> OutputMap;
    typedef map<Vamp::Plugin *, OutputMap> PluginOutputMap;
    PluginOutputMap m_pluginOutputs;

    // Map from plugin output identifier to plugin output index
    typedef map<string, int> OutputIndexMap;
    OutputIndexMap m_pluginOutputIndices;

    typedef set<std::string> SummaryNameSet;
    SummaryNameSet m_summaries;
    bool m_summariesOnly;
    Vamp::HostExt::PluginSummarisingAdapter::SegmentBoundaries m_boundaries;

    void writeSummaries(QString audioSource, Vamp::Plugin *);

    void writeFeatures(QString audioSource,
                       Vamp::Plugin *,
                       const Vamp::Plugin::FeatureSet &,
                       Transform::SummaryType summaryType =
                       Transform::NoSummary);

    void testOutputFiles(QString audioSource);
    void finish();

    int m_blockSize;
    int m_defaultSampleRate;
    int m_sampleRate;
    int m_channels;
    
    void print(Transform transform) const;
};

#endif
