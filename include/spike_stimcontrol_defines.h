#ifndef __SPIKE_STIMCONTROL_DEFINES_H__
#define __SPIKE_STIMCONTROL_DEFINES_H__

#define DEFAULT_MODE -1
#define OUTPUT_ONLY_MODE 0
#define REALTIME_FEEDBACK_MODE 1

#define DIO_STIMCONTROL_MODE 100

#define DIO_RTMODE_OUTPUT_ONLY 2000
#define DIO_RTMODE_DEFAULT 2001
#define DIO_RTMODE_LATENCY_TEST 2002
#define DIO_RTMODE_THETA 2003
#define DIO_RTMODE_RIPPLE_DISRUPT 2004
#define DIO_RTMODE_SPATIAL_STIM 2005

#define DIO_SET_RT_STIM_PARAMS 101
#define DIO_SET_RT_STIM_PULSE_PARAMS 102
#define DIO_SET_RIPPLE_STIM_PARAMS 103
#define DIO_SET_RT_FEEDBACK_PARAMS 104
#define DIO_SET_CM_PER_PIX 105
#define DIO_SET_SPATIAL_STIM_PARAMS 106
#define DIO_SET_SPATIAL_STIM_PULSE_PARAMS 107

#define DIO_STOP_RT_FEEDBACK 2200
#define DIO_START_RT_FEEDBACK 2201
#define DIO_RESET_RT_FEEDBACK 2202

#define DIO_RT_ENABLE 2300
#define DIO_RT_DISABLE 2301

#define DIO_PULSE_SEQ		300    // the message contains text with 
					//a series of pulse sequences
#define DIO_PULSE_SEQ_START	301
#define DIO_PULSE_SEQ_STOP	302

#define MAX_PULSE_SEQS 		500

#define DIO_QUERY_RT_FEEDBACK_STATUS 400
#define DIO_RT_STATUS_RIPPLE_DISRUPT 404
#define DIO_RT_STATUS_SPATIAL_STIM 408

#define DIO_PULSE_SEQ_STEP	411  // a single command has been exec'd
#define DIO_PULSE_SEQ_EXECUTED	412  // the complete file has been exec'd

#define DIO_PULSE_SEQ_RESTART	305
					// has been executed

#define DIO_RT_DEFAULT_PULSE_LEN 10

#define DIO_RT_DEFAULT_THETA_VEL 2
#define DIO_RT_DEFAULT_THETA_FILTER_DELAY 730

#define DIO_RT_DEFAULT_SAMP_DIVISOR  100000
#define DIO_RT_DEFAULT_RIPPLE_COEFF1 1.2 
#define DIO_RT_DEFAULT_RIPPLE_COEFF2 0.2
#define DIO_RT_DEFAULT_MUA_COEFF1 1.2 
#define DIO_RT_DEFAULT_MUA_COEFF2 0.2
#define DIO_RT_DEFAULT_RIPPLE_TIME_DELAY 0
#define DIO_RT_DEFAULT_RIPPLE_JITTER 0
#define DIO_RT_DEFAULT_RIPPLE_THRESHOLD 3.0 
#define DIO_RT_DEFAULT_RIPPLE_N_ABOVE_THRESH 1 
#define DIO_RT_DEFAULT_MUA_THRESHOLD 3.0 
#define DIO_RT_DEFAULT_RIPPLE_LOCKOUT 2500 
#define DIO_RT_DEFAULT_RIPPLE_SPEED_THRESH 1000.0 

#define DIO_RT_DEFAULT_LATENCY_TEST_THRESHOLD 2000

#define DIO_DEFAULT_CM_PER_PIX 0.87

#define DIO_AO_MODE_CONTINUOUS 	1
#define DIO_AO_MODE_PULSE	2
#define DIO_AO_MODE_RAMP	3
#define DIO_AO_MODE_SINE	4

#include <stdint.h>

typedef struct _PulseCommand {
    uint32_t start_samp_timestamp;
    bool digital_only;  // true if we should do only digital IO; false for analog IO 
    int statemachine;

    /* digital defines */
    unsigned short pin1, pin2;
    int is_biphasic;

    /* analog defines */
    int aout;  // the number of the analog output to use
    int aout_mode; // the mode for the analog out
    float minv, maxv; // the minimum and maximum voltages

    int cont_percent; // the continuous mode level (1-100% of max)
    int pulse_percent; // the pulse mode level (1-100% of max)


    int line;
    int pre_delay; // in ticks; automatically zeroed after first
    int pulse_width; // in ticks (10 kHz)
    int n_pulses;
    int inter_pulse_delay; // in ticks
    int n_repeats; // note that this is decremented to zero (not preserved) by code
                   // except for -1 which is the special case of continuous
    int inter_frame_delay; // in ticks;

} PulseCommand;

// special codes - embedded in the "pulse_width" field
#define DIO_PULSE_COMMAND_END  -1    //  end of command sequence
#define DIO_PULSE_COMMAND_REPEAT -10 //  repeat command sequence
          // jumping to command in line field, repeating n_repeats


typedef struct _ThetaStimParameters {
    int pulse_length;
    int vel_thresh; // in ms
    int filt_delay;
    int stim_phase;
} ThetaStimParameters;

typedef struct _RippleStimParameters {
    int pulse_length;
    int sampDivisor;
    double ripCoeff1, ripCoeff2;
    int time_delay;
    int jitter;
    double ripple_threshold;
    int n_above_thresh;
    int lockout;
    double speed_threshold;
} RippleStimParameters;

typedef struct _RippleStatusMsg {
  double ripMean;
  // think about using a qsignalmapper 
  double ripStd;
  int sincelast;
  int isRunning;
  double ratSpeed;
} RippleStatusMsg;


typedef struct _LatencyTestParameters {
    int pulse_length;
    int thresh;
} LatencyTestParameters;

typedef struct _SpatialStimParameters {
    short xpos, ypos;
    int lowerLeftX;
    int lowerLeftY;
    int upperRightX;
    int upperRightY;
    double minSpeed;
    double maxSpeed;
} SpatialStimParameters;
#endif
