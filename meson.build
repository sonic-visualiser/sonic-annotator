
project(
  'Sonic Annotator',
  'c', 'cpp',
  version: '2.0',
  license: 'GPL-2.0-or-later',
  default_options: [
    'cpp_std=c++17',
    'warning_level=2',
    'buildtype=release',
    'b_ndebug=if-release',
  ],
  meson_version: '>= 0.57.0'
)

conf = configuration_data()
conf.set('version', meson.project_version())
configure_file(input: 'version.h.in', output: 'version.h', configuration: conf)

system = host_machine.system()
architecture = host_machine.cpu_family()
compiler = meson.get_compiler('cpp').get_id()
buildtype = get_option('buildtype')

qt = import('qt6')

qt_dep = dependency(
  'qt6',
  modules: [
    'Core',
    'Xml',
    'Network',
    'Test',
  ],
  main: true
)

general_defines = []
general_link_args = []

config_summary = {
  'Compiler': compiler,
  'Architecture': architecture,
  'System': system,
  'Build type': buildtype
}

if buildtype.startswith('release')
  general_defines += [
    '-DNO_TIMING',
    '-DNO_HIT_COUNTS',
  ]
  if compiler == 'clang'
    general_defines += [
      '-Wno-deprecated-copy', # Qt currently hits lots of these internally
    ]
  elif compiler == 'gcc'
    general_defines += [
    ]
  elif compiler == 'msvc'
    general_defines += [
      '-wd4068', # unknown pragma (we have lots of pragma 'GCC')
      '-wd4267', # conversion from size_t to smaller integral type
      '-wd4996', # deprecated functions, including annoying warnings about POSIX
      '-Zc:twoPhase-', # work around incompatibility with some SDKs (CI only)
    ]
  endif # compiler
elif buildtype.startswith('debug')
  general_defines += [
    '-DWANT_TIMING',
    '-DWANT_HIT_COUNTS',
  ]
endif # get_option('buildtype')

if architecture == 'x86'
  general_defines += [
    # See note in vamp-plugin-sdk/src/vamp-hostsdk/Files.cpp
    '-DDO_NOT_FREE_LIBRARIES=1'
  ]
endif

svcore_moc_args = []
feature_additional_libpaths = []
feature_additional_libs = []
feature_include_dirs = []

dl_dep = meson.get_compiler('c').find_library('dl', required: false)
vamphostsdk_dep = dependency('vamp-hostsdk', version: '>= 2.10', required: false)

unwind_dep = []
with_unwind = false

if system == 'linux'

  rc = []
  
  bzip2_dep = dependency('bzip2', required: false)

  if not bzip2_dep.found()
    message('Failed to find bzip2 dependency. On many systems this exists but is not discoverable through pkg-config; proceeding on this assumption')
    feature_additional_libs = [ '-lbz2' ]
  endif # bzip2_dep

  fftw3_dep = dependency('fftw3', version: '>= 3.0.0')
  sndfile_dep = dependency('sndfile', version: '>= 1.0.16')
  samplerate_dep = dependency('samplerate', version: '>= 0.1.2')
  sord_dep = dependency('sord-0', version: '>= 0.5')
  serd_dep = dependency('serd-0', version: '>= 0.5')
  oggz_dep = dependency('oggz', version: '>= 1.0.0')
  fishsound_dep = dependency('fishsound', version: '>= 1.0.0')
  mad_dep = dependency('mad', version: '>= 0.15.0')
  id3tag_dep = dependency('id3tag', version: '>= 0.15.0')
  opus_dep = dependency('opusfile')

  feature_dependencies = [
    vamphostsdk_dep,
    bzip2_dep,
    fftw3_dep,
    sndfile_dep,
    samplerate_dep,
    sord_dep,
    serd_dep,
    oggz_dep,
    fishsound_dep,
    mad_dep,
    id3tag_dep,
    opus_dep,
  ]
  
  feature_defines = [
    '-DHAVE_BZ2',
    '-DHAVE_FFTW3',
    '-DFFTW_DOUBLE_ONLY',
    '-DHAVE_SNDFILE',
    '-DHAVE_LIBSAMPLERATE',
    '-DHAVE_SORD', '-DUSE_SORD',
    '-DHAVE_SERD',
    '-DHAVE_OGGZ',
    '-DHAVE_FISHSOUND',
    '-DHAVE_MAD',
    '-DHAVE_ID3TAG',
    '-DHAVE_OPUS',
    '-DHAVE_OPUS_READ_ONLY',
  ]
  svcore_moc_args += [
    '-DHAVE_MAD',
  ]
  
elif system == 'darwin'

  svdeps_dir = meson.current_source_dir() / 'sv-dependency-builds/osx'

  if architecture == 'aarch64'
    svdeps_libdir = svdeps_dir / 'lib-arm64'
  else
    svdeps_libdir = svdeps_dir / 'lib'
  endif
  
  rc = []

  general_defines += [
    '-mmacosx-version-min=10.15'
  ]
  general_link_args += [
    '-mmacosx-version-min=10.15'
  ]
  
  feature_defines = [
    '-DHAVE_BZ2',
    '-DHAVE_SNDFILE',
    '-DHAVE_LIBSAMPLERATE',
    '-DHAVE_SORD', '-DUSE_SORD',
    '-DHAVE_SERD',
    '-DHAVE_OPUS',
    '-DHAVE_OPUS_READ_ONLY',
    '-DHAVE_COREAUDIO',
    '-DHAVE_VDSP',
  ]
    
  # vDSP will be used in preference to FFTW most of the time, but
  # FFTW supports more fft lengths - without it we can end up with a
  # very slow implementation for unusual lengths.
  feature_defines += [
    '-DHAVE_FFTW3',
    '-DFFTW_DOUBLE_ONLY',
  ]

  if architecture != 'aarch64'

    # We don't actually need MAD on this platform at all - CoreAudio
    # can read mp3 files no problem - but we have been using it up to
    # now, so for consistency let's stick with it. No need to bring it
    # along to aarch64 with us though.
    feature_defines += [
      '-DHAVE_MAD',
      '-DHAVE_ID3TAG',
    ]
    svcore_moc_args += [
      '-DHAVE_MAD',
    ]
    
  endif
    
  feature_dependencies = [
    vamphostsdk_dep,
  ]
  
  feature_include_dirs = [
    # These must be relative, so not using svdeps_dir
    'sv-dependency-builds/osx/include-arm64',
    'sv-dependency-builds/osx/include',
    'sv-dependency-builds/osx/include/opus',
  ]

  feature_additional_libs = [
    '-L' + svdeps_libdir,
    '-lbz2',
    '-lsndfile',
    '-lFLAC',
    '-logg',
    '-lvorbis',
    '-lvorbisenc',
    '-lvorbisfile',
    '-lopusfile',
    '-lopus',
    '-lsamplerate',
    '-lfftw3',
    '-lsord-0',
    '-lserd-0',
    '-lz',
    '-framework', 'CoreAudio',
    '-framework', 'CoreMidi',
    '-framework', 'AudioUnit',
    '-framework', 'AudioToolbox',
    '-framework', 'CoreFoundation',
    '-framework', 'CoreServices',
    '-framework', 'Accelerate',
  ]
  
  if architecture != 'aarch64'
    # See comments when setting up feature_defines above
    feature_additional_libs += [
      '-lmad',
      '-lid3tag',
    ]
  endif
  
elif system == 'windows'

  if architecture == 'x86'
    svdeps_dir = meson.current_source_dir() / 'sv-dependency-builds/win32-msvc'
  else 
    svdeps_dir = meson.current_source_dir() / 'sv-dependency-builds/win64-msvc'
  endif # architecture

  windows = import('windows')
  rc = windows.compile_resources('icons/sv.rc')

  general_defines += [
    '-DWIN32',
    '-DNOMINMAX',
    '-DUNICODE',
    '-D_USE_MATH_DEFINES',
    '-D_HAS_STD_BYTE=1',
  ]
  
  feature_defines = [
    '-DHAVE_BZ2',
    '-DHAVE_FFTW3',
    '-DFFTW_DOUBLE_ONLY',
    '-DHAVE_SNDFILE',
    '-DHAVE_LIBSAMPLERATE',
    '-DHAVE_SORD', '-DUSE_SORD', '-DSORD_STATIC',
    '-DHAVE_SERD', '-DSERD_STATIC',
    '-DZIX_STATIC',
    '-DHAVE_MAD',
    '-DHAVE_ID3TAG',
    '-DHAVE_OPUS',
    '-DHAVE_OPUS_READ_ONLY',
  ]

  feature_dependencies = [
    vamphostsdk_dep,
  ]
  
  if architecture == 'x86'
    feature_include_dirs = [
      # These must be relative, so not using svdeps_dir
      'sv-dependency-builds/win32-msvc/include',
      'sv-dependency-builds/win32-msvc/include/opus',
    ]
  else
    feature_defines += [
      '-DHAVE_MEDIAFOUNDATION',
    ]
    feature_include_dirs = [
      # These must be relative, so not using svdeps_dir
      'sv-dependency-builds/win64-msvc/include',
      'sv-dependency-builds/win64-msvc/include/opus',
    ]
  endif # architecture

  if buildtype.startswith('release')
    feature_additional_libpaths =  [
      '-L' + svdeps_dir / 'lib',
    ]
  else
    feature_additional_libpaths =  [
      '-L' + svdeps_dir / 'lib/debug',
      '-L' + svdeps_dir / 'lib',
    ]
  endif
  
  feature_additional_libs = [
    feature_additional_libpaths,
    '-lbz2',
    '-lfftw3',
    '-lsndfile',
    '-logg',
    '-lopusfile',
    '-lopus',
    '-lmad',
    '-lid3tag',
    '-lsamplerate',
    '-lz',
  ]

  if architecture == 'x86'
    feature_additional_libs += [
      '-lFLAC',
      '-lvorbis',
      '-lvorbisenc',
      '-lvorbisfile',
      '-lsord-0',
      '-lserd-0',
      # (We don't have MediaFoundation support with this build sadly)
      '-lwinmm',
      '-lws2_32',
    ]
  elif architecture == 'x86_64'
    feature_additional_libs += [
#      '-NODEFAULTLIB:LIBCMT',
      '-lsord',
      '-lserd',
      '-lmfplat',
      '-lmfreadwrite',
      '-lmfuuid',
      '-lpropsys',
      '-ladvapi32',
      '-lwinmm',
      '-lws2_32',
    ]
  else 
    error('Build for architecture ' + architecture + ' is not supported on this platform')
  endif # architecture
  
  svcore_moc_args += [
    '-DHAVE_MAD',
    '-DQ_OS_WIN',
  ]

else
    error('This operating system ("' + system + '") is not supported by this build file')
endif # system
    
all_include_dirs = include_directories(
  [
    'bqvec',
    'bqvec/bqvec',
    'bqfft',
    'bqresample',
    'bqaudiostream',
    'bqaudiostream/bqaudiostream',
    'bqthingfactory',
    'piper-vamp-cpp',
    'checker',
    'dataquay',
    'dataquay/dataquay',
    'svcore',
    'svcore/data',
    'svcore/plugin/api/alsa',
    'svcore',
    'vamp-plugin-sdk',
  ]
)

bq_files = [
  'bqvec/src/Allocators.cpp',
  'bqvec/src/Barrier.cpp',
  'bqvec/src/VectorOpsComplex.cpp',
  'bqfft/src/FFT.cpp',
  'bqresample/src/Resampler.cpp',
  'bqaudiostream/src/AudioReadStream.cpp',
  'bqaudiostream/src/AudioReadStreamFactory.cpp',
  'bqaudiostream/src/AudioWriteStreamFactory.cpp',
  'bqaudiostream/src/AudioStreamExceptions.cpp',
]

if vamphostsdk_dep.found()
  config_summary += { 'Vamp SDK': 'system' }
  vamphostsdk_files = []
else
  config_summary += { 'Vamp SDK': 'bundled' }
  vamphostsdk_files = [
    'vamp-plugin-sdk/src/vamp-hostsdk/PluginBufferingAdapter.cpp',
    'vamp-plugin-sdk/src/vamp-hostsdk/PluginChannelAdapter.cpp',
    'vamp-plugin-sdk/src/vamp-hostsdk/PluginHostAdapter.cpp',
    'vamp-plugin-sdk/src/vamp-hostsdk/PluginInputDomainAdapter.cpp',
    'vamp-plugin-sdk/src/vamp-hostsdk/PluginLoader.cpp',
    'vamp-plugin-sdk/src/vamp-hostsdk/PluginSummarisingAdapter.cpp',
    'vamp-plugin-sdk/src/vamp-hostsdk/PluginWrapper.cpp',
    'vamp-plugin-sdk/src/vamp-hostsdk/RealTime.cpp',
    'vamp-plugin-sdk/src/vamp-hostsdk/Files.cpp',
  ]
endif

dataquay_files = [
  'dataquay/src/Connection.cpp',
  'dataquay/src/Node.cpp',
  'dataquay/src/PropertyObject.cpp',
  'dataquay/src/RDFException.cpp',
  'dataquay/src/Store.cpp',
  'dataquay/src/Transaction.cpp',
  'dataquay/src/TransactionalStore.cpp',
  'dataquay/src/Triple.cpp',
  'dataquay/src/Uri.cpp',
  'dataquay/src/backend/BasicStoreRedland.cpp',
  'dataquay/src/backend/BasicStoreSord.cpp',
  'dataquay/src/backend/define-check.cpp',
  'dataquay/src/objectmapper/ContainerBuilder.cpp',
  'dataquay/src/objectmapper/ObjectBuilder.cpp',
  'dataquay/src/objectmapper/ObjectLoader.cpp',
  'dataquay/src/objectmapper/ObjectMapper.cpp',
  'dataquay/src/objectmapper/ObjectMapperForwarder.cpp',
  'dataquay/src/objectmapper/ObjectStorer.cpp',
  'dataquay/src/objectmapper/TypeMapping.cpp',
]

dataquay_moc_files = qt.preprocess(
  moc_headers: [
    'dataquay/dataquay/Connection.h',
    'dataquay/dataquay/TransactionalStore.h',
    'dataquay/dataquay/objectmapper/ObjectMapper.h',
    'dataquay/dataquay/objectmapper/ObjectMapperForwarder.h',
  ])

svcore_files = [
  'svcore/base/AudioLevel.cpp',
  'svcore/base/ById.cpp',
  'svcore/base/Clipboard.cpp',
  'svcore/base/ColumnOp.cpp',
  'svcore/base/Command.cpp',
  'svcore/base/Debug.cpp',
  'svcore/base/EventSeries.cpp',
  'svcore/base/Exceptions.cpp',
  'svcore/base/HelperExecPath.cpp',
  'svcore/base/HitCount.cpp',
  'svcore/base/LogRange.cpp',
  'svcore/base/Pitch.cpp',
  'svcore/base/PlayParameterRepository.cpp',
  'svcore/base/PlayParameters.cpp',
  'svcore/base/Preferences.cpp',
  'svcore/base/Profiler.cpp',
  'svcore/base/ProgressPrinter.cpp',
  'svcore/base/ProgressReporter.cpp',
  'svcore/base/PropertyContainer.cpp',
  'svcore/base/RangeMapper.cpp',
  'svcore/base/RealTimeSV.cpp',
  'svcore/base/RecentFiles.cpp',
  'svcore/base/RecordDirectory.cpp',
  'svcore/base/ResourceFinder.cpp',
  'svcore/base/Selection.cpp',
  'svcore/base/Serialiser.cpp',
  'svcore/base/StorageAdviser.cpp',
  'svcore/base/StringBits.cpp',
  'svcore/base/Strings.cpp',
  'svcore/base/TempDirectory.cpp',
  'svcore/base/TempWriteFile.cpp',
  'svcore/base/TextMatcher.cpp',
  'svcore/base/Thread.cpp',
  'svcore/base/UnitDatabase.cpp',
  'svcore/base/ViewManagerBase.cpp',
  'svcore/base/XmlExportable.cpp',
  'svcore/base/ZoomLevel.cpp',
  'svcore/data/fileio/AudioFileReader.cpp',
  'svcore/data/fileio/AudioFileReaderFactory.cpp',
  'svcore/data/fileio/AudioFileSizeEstimator.cpp',
  'svcore/data/fileio/BQAFileReader.cpp',
  'svcore/data/fileio/BZipFileDevice.cpp',
  'svcore/data/fileio/CachedFile.cpp',
  'svcore/data/fileio/CodedAudioFileReader.cpp',
  'svcore/data/fileio/CSVFileReader.cpp',
  'svcore/data/fileio/CSVFileWriter.cpp',
  'svcore/data/fileio/CSVFormat.cpp',
  'svcore/data/fileio/DataFileReaderFactory.cpp',
  'svcore/data/fileio/DecodingWavFileReader.cpp',
  'svcore/data/fileio/FileReadThread.cpp',
  'svcore/data/fileio/FileSource.cpp',
  'svcore/data/fileio/MIDIFileReader.cpp',
  'svcore/data/fileio/MIDIFileWriter.cpp',
  'svcore/data/fileio/MP3FileReader.cpp',
  'svcore/data/fileio/PlaylistFileReader.cpp',
  'svcore/data/fileio/TextTest.cpp',
  'svcore/data/fileio/WavFileReader.cpp',
  'svcore/data/fileio/WavFileWriter.cpp',
  'svcore/data/midi/MIDIInput.cpp',
  'svcore/data/midi/rtmidi/RtMidi.cpp',
  'svcore/data/model/AggregateWaveModel.cpp',
  'svcore/data/model/AlignmentModel.cpp',
  'svcore/data/model/BasicCompressedDenseThreeDimensionalModel.cpp',
  'svcore/data/model/Dense3DModelPeakCache.cpp',
  'svcore/data/model/DenseTimeValueModel.cpp',
  'svcore/data/model/EditableDenseThreeDimensionalModel.cpp',
  'svcore/data/model/FFTModel.cpp',
  'svcore/data/model/Model.cpp',
  'svcore/data/model/ModelDataTableModel.cpp',
  'svcore/data/model/PowerOfSqrtTwoZoomConstraint.cpp',
  'svcore/data/model/PowerOfTwoZoomConstraint.cpp',
  'svcore/data/model/RangeSummarisableTimeValueModel.cpp',
  'svcore/data/model/RelativelyFineZoomConstraint.cpp',
  'svcore/data/model/WaveformOversampler.cpp',
  'svcore/data/model/WaveFileModel.cpp',
  'svcore/data/model/ReadOnlyWaveFileModel.cpp',
  'svcore/data/model/WritableWaveFileModel.cpp',
  'svcore/data/osc/OSCMessage.cpp',
  'svcore/data/osc/OSCQueue.cpp',
  'svcore/plugin/PluginScan.cpp',
  'svcore/plugin/DSSIPluginFactory.cpp',
  'svcore/plugin/DSSIPluginInstance.cpp',
  'svcore/plugin/FeatureExtractionPluginFactory.cpp',
  'svcore/plugin/LADSPAPluginFactory.cpp',
  'svcore/plugin/LADSPAPluginInstance.cpp',
  'svcore/plugin/NativeVampPluginFactory.cpp',
  'svcore/plugin/PiperVampPluginFactory.cpp',
  'svcore/plugin/PluginIdentifier.cpp',
  'svcore/plugin/PluginPathSetter.cpp',
  'svcore/plugin/PluginXml.cpp',
  'svcore/plugin/RealTimePluginFactory.cpp',
  'svcore/plugin/RealTimePluginInstance.cpp',
  'svcore/plugin/plugins/SamplePlayer.cpp',
  'svcore/rdf/PluginRDFIndexer.cpp',
  'svcore/rdf/PluginRDFDescription.cpp',
  'svcore/rdf/RDFExporter.cpp',
  'svcore/rdf/RDFFeatureWriter.cpp',
  'svcore/rdf/RDFImporter.cpp',
  'svcore/rdf/RDFTransformFactory.cpp',
  'svcore/system/Init.cpp',
  'svcore/system/System.cpp',
  'svcore/transform/CSVFeatureWriter.cpp',
  'svcore/transform/FeatureExtractionModelTransformer.cpp',
  'svcore/transform/FileFeatureWriter.cpp',
  'svcore/transform/RealTimeEffectModelTransformer.cpp',
  'svcore/transform/Transform.cpp',
  'svcore/transform/TransformFactory.cpp',
  'svcore/transform/ModelTransformer.cpp',
  'svcore/transform/ModelTransformerFactory.cpp',
]

if compiler != 'msvc' # i.e. build is not the 64-bit Windows one
  svcore_files += [
    'svcore/system/os-other.cpp',
  ]
endif

svcore_moc_files = qt.preprocess(
  moc_headers: [
    'svcore/base/Command.h',
    'svcore/base/PlayParameterRepository.h',
    'svcore/base/PlayParameters.h',
    'svcore/base/Preferences.h',
    'svcore/base/ProgressPrinter.h',
    'svcore/base/ProgressReporter.h',
    'svcore/base/PropertyContainer.h',
    'svcore/base/RecentFiles.h',
    'svcore/base/Thread.h',
    'svcore/base/UnitDatabase.h',
    'svcore/base/ViewManagerBase.h',
    'svcore/data/fileio/AudioFileReader.h',
    'svcore/data/fileio/BQAFileReader.h',
    'svcore/data/fileio/BZipFileDevice.h',
    'svcore/data/fileio/CodedAudioFileReader.h',
    'svcore/data/fileio/CSVFileWriter.h',
    'svcore/data/fileio/DecodingWavFileReader.h',
    'svcore/data/fileio/FileReadThread.h',
    'svcore/data/fileio/FileSource.h',
    'svcore/data/fileio/MIDIFileReader.h',
    'svcore/data/fileio/MP3FileReader.h',
    'svcore/data/midi/MIDIInput.h',
    'svcore/data/model/AggregateWaveModel.h',
    'svcore/data/model/AlignmentModel.h',
    'svcore/data/model/BasicCompressedDenseThreeDimensionalModel.h',
    'svcore/data/model/Dense3DModelPeakCache.h',
    'svcore/data/model/DenseThreeDimensionalModel.h',
    'svcore/data/model/DenseTimeValueModel.h',
    'svcore/data/model/EditableDenseThreeDimensionalModel.h',
    'svcore/data/model/FFTModel.h',
    'svcore/data/model/ImageModel.h',
    'svcore/data/model/Labeller.h',
    'svcore/data/model/Model.h',
    'svcore/data/model/ModelDataTableModel.h',
    'svcore/data/model/NoteModel.h',
    'svcore/data/model/RangeSummarisableTimeValueModel.h',
    'svcore/data/model/RegionModel.h',
    'svcore/data/model/SparseOneDimensionalModel.h',
    'svcore/data/model/SparseTimeValueModel.h',
    'svcore/data/model/TextModel.h',
    'svcore/data/model/BoxModel.h',
    'svcore/data/model/WaveFileModel.h',
    'svcore/data/model/ReadOnlyWaveFileModel.h',
    'svcore/data/model/WritableWaveFileModel.h',
    'svcore/data/osc/OSCQueue.h',
    'svcore/rdf/RDFImporter.h',
    'svcore/rdf/RDFTransformFactory.h',
    'svcore/transform/FeatureExtractionModelTransformer.h',
    'svcore/transform/TransformFactory.h',
    'svcore/transform/ModelTransformerFactory.h',
  ],
  moc_extra_arguments: svcore_moc_args,
)

svcore_lib = static_library(
  'svcore',
  bq_files,
  vamphostsdk_files,
  dataquay_moc_files,
  svcore_moc_files,
  dataquay_files,
  svcore_files,
  include_directories: [
    all_include_dirs,
    feature_include_dirs,
  ],
  dependencies: [
    qt_dep,
    feature_dependencies,
    dl_dep,
  ],
  cpp_args: [
    feature_defines,
    general_defines,
  ],
)

svcore_dep = declare_dependency(
  link_with: svcore_lib,
  dependencies: [
  ],
  include_directories: [
    all_include_dirs,
    feature_include_dirs,
  ],
)

svcore_base_test_moc_files = qt.preprocess(
  moc_headers: [
    'svcore/base/test/TestById.h',
    'svcore/base/test/TestColumnOp.h',
    'svcore/base/test/TestLogRange.h',
    'svcore/base/test/TestMovingMedian.h',
    'svcore/base/test/TestOurRealTime.h',
    'svcore/base/test/TestPitch.h',
    'svcore/base/test/TestEventSeries.h',
    'svcore/base/test/TestRangeMapper.h',
    'svcore/base/test/TestScaleTickIntervals.h',
    'svcore/base/test/TestStringBits.h',
    'svcore/base/test/TestVampRealTime.h',
    'svcore/base/test/StressEventSeries.h',
  ],
  moc_extra_arguments: svcore_moc_args,
)

svcore_system_test_moc_files = qt.preprocess(
  moc_headers: [
    'svcore/system/test/TestEnv.h',
  ])

svcore_data_model_test_moc_files = qt.preprocess(
  moc_headers: [
    'svcore/data/model/test/MockWaveModel.h',
    'svcore/data/model/test/TestFFTModel.h',
    'svcore/data/model/test/TestSparseModels.h',
    'svcore/data/model/test/TestWaveformOversampler.h',
    'svcore/data/model/test/TestZoomConstraints.h',
  ])

svcore_data_fileio_test_moc_files = qt.preprocess(
  moc_headers: [
    'svcore/data/model/test/MockWaveModel.h',
    'svcore/data/fileio/test/AudioFileReaderTest.h',
    'svcore/data/fileio/test/BogusAudioFileReaderTest.h',
    'svcore/data/fileio/test/AudioFileWriterTest.h',
    'svcore/data/fileio/test/EncodingTest.h',
    'svcore/data/fileio/test/MIDIFileReaderTest.h',
    'svcore/data/fileio/test/CSVFormatTest.h',
    'svcore/data/fileio/test/CSVReaderTest.h',
    'svcore/data/fileio/test/CSVStreamWriterTest.h',
  ])

runner_moc_files = qt.preprocess(
  moc_headers: [
    'runner/MultiplexedReader.h',
  ]
)

runner_main_files = [
  'runner/main.cpp',
  'runner/DefaultFeatureWriter.cpp',
  'runner/FeatureExtractionManager.cpp',
  'runner/AudioDBFeatureWriter.cpp',
  'runner/FeatureWriterFactory.cpp',
  'runner/JAMSFeatureWriter.cpp',
  'runner/LabFeatureWriter.cpp',
  'runner/MIDIFeatureWriter.cpp',
  'runner/MultiplexedReader.cpp',
]

runner_main_name = 'sonic-annotator'

executable(
  runner_main_name,
  runner_main_files,
  runner_moc_files,
  rc,
  dependencies: [
    svcore_dep,
    qt_dep,
    feature_dependencies,
    dl_dep,
    unwind_dep
  ],
  cpp_args: [
    feature_defines,
    general_defines,
  ],
  link_args: [
    feature_additional_libs,
    general_link_args,
  ],
  win_subsystem: 'windows',
  install: true,
)

svcore_base_test_exe = executable(
  'test-svcore-base',
  svcore_base_test_moc_files,
  'svcore/base/test/svcore-base-test.cpp',
  dependencies: [
    svcore_dep,
    qt_dep,
    feature_dependencies,
    dl_dep,
  ],
  cpp_args: [
    feature_defines,
    general_defines,
  ],
  link_args: [
    feature_additional_libs,
    general_link_args,
  ],
  win_subsystem: 'console'
)

svcore_system_test_exe = executable(
  'test-svcore-system',
  svcore_system_test_moc_files,
  'svcore/system/test/svcore-system-test.cpp',
  dependencies: [
    svcore_dep,
    qt_dep,
    feature_dependencies,
    dl_dep,
  ],
  cpp_args: [
    feature_defines,
    general_defines,
  ],
  link_args: [
    feature_additional_libs,
    general_link_args,
  ],
  win_subsystem: 'console'
)

svcore_data_model_test_exe = executable(
  'test-svcore-data-model',
  svcore_data_model_test_moc_files,
  'svcore/data/model/test/MockWaveModel.cpp',
  'svcore/data/model/test/svcore-data-model-test.cpp',
  dependencies: [
    svcore_dep,
    qt_dep,
    feature_dependencies,
    dl_dep,
  ],
  cpp_args: [
    feature_defines,
    general_defines,
  ],
  link_args: [
    feature_additional_libs,
    general_link_args,
  ],
  win_subsystem: 'console'
)

svcore_data_fileio_test_exe = executable(
  'test-svcore-data-fileio',
  svcore_data_fileio_test_moc_files,
  'svcore/data/model/test/MockWaveModel.cpp',
  'svcore/data/fileio/test/UnsupportedFormat.cpp',
  'svcore/data/fileio/test/svcore-data-fileio-test.cpp',
  dependencies: [
    svcore_dep,
    qt_dep,
    feature_dependencies,
    dl_dep,
  ],
  cpp_args: [
    feature_defines,
    general_defines,
  ],
  link_args: [
    feature_additional_libs,
    general_link_args,
  ],
  win_subsystem: 'console'
)

test('svcore-base', svcore_base_test_exe)
test('svcore-system', svcore_system_test_exe)
test('svcore-data-model', svcore_data_model_test_exe)
test('svcore-data-fileio', svcore_data_fileio_test_exe,
     args: [
       '--testdir', meson.current_source_dir() / 'svcore/data/fileio/test'
     ])

summary({'prefix': get_option('prefix'),
         'bindir': get_option('bindir'),
         'libdir': get_option('libdir'),
         'datadir': get_option('datadir'),
        }, section: 'Directories')

summary(config_summary, section: 'Configuration', bool_yn: true)
