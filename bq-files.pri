
BQ_HEADERS += \
	bqvec/bqvec/Allocators.h \
	bqvec/bqvec/Barrier.h \
	bqvec/bqvec/ComplexTypes.h \
	bqvec/bqvec/Restrict.h \
	bqvec/bqvec/RingBuffer.h \
	bqvec/bqvec/VectorOpsComplex.h \
	bqvec/bqvec/VectorOps.h \
	bqvec/pommier/neon_mathfun.h \
	bqvec/pommier/sse_mathfun.h \
        bqfft/bqfft/FFT.h \
	bqresample/bqresample/Resampler.h \
	bqresample/speex/speex_resampler.h

BQ_SOURCES += \
	bqvec/src/Allocators.cpp \
	bqvec/src/Barrier.cpp \
	bqvec/src/VectorOpsComplex.cpp \
        bqfft/src/FFT.cpp \
	bqresample/src/Resampler.cpp

