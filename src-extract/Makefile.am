bin_PROGRAMS = nspike_extract nspike_postimestamp

nspike_extract_SOURCES = spike_extract.cpp \
          @top_srcdir@/src-modules/spike_config.cpp \
          @top_srcdir@/src-modules/spike_message.cpp \
          @top_srcdir@/src-modules/spike_util.cpp 
nspike_extract_LDADD = -lz

nspike_postimestamp_SOURCES = spike_postimestamp.cpp @top_srcdir@/src-modules/spike_util.cpp 

INCLUDES = -I@top_srcdir@/include
