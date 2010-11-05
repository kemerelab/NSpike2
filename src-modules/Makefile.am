bin_PROGRAMS = spike_daq spike_posdaq \
spike_process_posdata spike_save_data \
spike_userdata 
#nspike_userdata

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

# add your user functions in src-user and to the two userdata definitions
#nspike_userdata_SOURCES = spike_main_userdata.cpp spike_message.cpp \
#		spike_config.cpp src-user/stimcontrol_processmessage.cpp \
#		src-user/stimcontrol_processdata.cpp \
#		src-user/generate_pulse_functions.cpp \
#		src-user/ripple_process.cpp 
#
#nspike_userdata_LDADD = -lm -lz

spike_userdata_SOURCES = spike_userdata.cpp spike_message.cpp \
		src-user/stimcontrol_processmessage.cpp \
		src-user/stimcontrol_processdata.cpp \
		src-user/generate_pulse_functions.cpp \
		src-user/ripple_process.cpp 

INCLUDES = -I@top_srcdir@/include -I@top_srcdir@/src-modules/src-user
