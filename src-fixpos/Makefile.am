if MAKE_NSPIKE_FIXPOS
  FIXPOS = nspike_fixpos
else !MAKE_NSPIKE_FIXPOS
  FIXPOS =
endif

bin_PROGRAMS = $(FIXPOS)

nspike_fixpos_SOURCES = spike_fixpos.cpp spike_fixposutil.cpp \
                        spike_fixpos_defines.h \
                        @top_srcdir@/src-modules/spike_config.cpp \
                        @top_srcdir@/src-modules/spike_message.cpp \
                        @top_srcdir@/src-modules/spike_util.cpp 
nspike_fixpos_LDADD =  -lGL -lglut -lX11 -lmpeg2 -lmpeg2convert -lm -lz 

INCLUDES = -I/usr/include/mpeg2dec -I@top_srcdir@/include
