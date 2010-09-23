bin_PROGRAMS = user_program 

user_program_SOURCES = laser.cpp \
                    pulseFunctions.cpp laser_process.cpp \
                    send_dsp_commands.cpp ripple_process.cpp \
                    filtercoeffs.h laser_defines.h \
                    @top_srcdir@/src-modules/spike_message.cpp
user_program_LDADD =  -lm

INCLUDES = -I@top_srcdir@/include
