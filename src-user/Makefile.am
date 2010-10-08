bin_PROGRAMS = user_program 

user_program_SOURCES = stimcontrol_main.cpp \
                    generate_pulse_functions.cpp stimcontrol_process.cpp \
                    send_dsp_commands.cpp ripple_process.cpp \
                    filtercoeffs.h stimcontrol_defines.h \
                    @top_srcdir@/src-modules/spike_message.cpp
user_program_LDADD =  -lm

INCLUDES = -I@top_srcdir@/include
