bin_PROGRAMS = spike_daq spike_posdaq \
spike_process_posdata spike_save_data \
spike_fsdata 
#nspike_fsdata

spike_daq_SOURCES = spike_daq.cpp spike_message.cpp 
spike_daq_LDADD = -lm 
spike_daq_CPPFLAGS = -Wall

spike_posdaq_SOURCES = spike_posdaq.cpp spike_message.cpp \
                       spike_V4Lcapture.h spike_V4Lcapture.cpp

spike_process_posdata_SOURCES = spike_process_posdata.cpp \
    spike_message.cpp spike_ffmpegEncoder.h spike_ffmpegEncoder.cpp
spike_process_posdata_CPPFLAGS = -D__STDC_CONSTANT_MACROS
                     # needed for FFMPEG
spike_process_posdata_LDADD = -lavcodec -lavutil -lm -lz 
spike_process_posdata_LDFLAGS = -D_GNU_SOURCE -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64

spike_save_data_SOURCES = spike_save_data.cpp spike_message.cpp 
spike_save_data_LDADD = -lz 

# add your fs functions in src-fs and to the two fsdata definitions
#nspike_fsdata_SOURCES = spike_main_fsdata.cpp spike_message.cpp \
#		spike_config.cpp src-fs/stimcontrol_processmessage.cpp \
#		src-fs/stimcontrol_processdata.cpp \
#		src-fs/generate_pulse_functions.cpp \
#		src-fs/ripple_process.cpp 
#
#nspike_fsdata_LDADD = -lm -lz

spike_fsdata_SOURCES = spike_fsdata.cpp spike_message.cpp \
		src-fs/stimcontrol_processmessage.cpp \
		src-fs/stimcontrol_processdata.cpp \
		src-fs/generate_pulse_functions.cpp \
		src-fs/ripple_process.cpp \
		src-fs/spatial_process.cpp 

INCLUDES = -I@top_srcdir@/include -I@top_srcdir@/src-modules/src-fs
