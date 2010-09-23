bin_PROGRAMS = spike_daq spike_posdaq \
spike_process_posdata spike_save_data \
nspike_matlab spike_matlab 

spike_daq_SOURCES = spike_daq.cpp spike_message.cpp 
spike_daq_LDADD = -lm 
spike_daq_CPPFLAGS = -Wall

spike_posdaq_SOURCES = spike_posdaq.cpp spike_message.cpp \
                       spike_V4Lcapture.h spike_V4Lcapture.cpp

spike_process_posdata_SOURCES = spike_process_posdata.cpp \
    spike_message.cpp spike_ffmpegEncoder.h spike_ffmpegEncoder.cpp
# spike_process_posdata_CPPFLAGS = @FFMPEG_CFLAGS@
spike_process_posdata_LDADD = -lavcodec -lavutil -lm -lz 
spike_process_posdata_LDFLAGS = -D_GNU_SOURCE -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64

spike_save_data_SOURCES = spike_save_data.cpp spike_message.cpp 
spike_save_data_LDADD = -lz 

nspike_matlab_SOURCES = spike_main_matlab.cpp spike_message.cpp spike_config.cpp
nspike_matlab_LDADD = -lm -lz

spike_matlab_SOURCES = spike_matlab.cpp spike_message.cpp 

INCLUDES = -I@top_srcdir@/include
