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

#define DIO_SET_RT_STIM_PARAMS 101
#define DIO_SET_RT_FEEDBACK_PARAMS 102
#define DIO_SET_CM_PER_PIX 103

#define DIO_STOP_RT_FEEDBACK 2200
#define DIO_START_RT_FEEDBACK 2201

#define DIO_RT_ENABLE 2300
#define DIO_RT_DISABLE 2301

#define DIO_PULSE_SEQ		300    // the message contains text with 
					//a series of pulse sequences
#define DIO_PULSE_SEQ_START	301
#define DIO_PULSE_SEQ_STOP	302

#define MAX_PULSE_SEQS 50

#define DIO_QUERY_RT_FEEDBACK_STATUS 400
#define DIO_RT_STATUS_RIPPLE_DISRUPT 404

#define DIO_PULSE_SEQ_STEP	411  // a single command has been exec'd
#define DIO_PULSE_SEQ_EXECUTED	412  // the complete file has been exec'd

#define DIO_PULSE_SEQ_RESTART	305
					// has been executed

#define DIO_RT_DEFAULT_PULSE_LEN 10

#define DIO_RT_DEFAULT_THETA_VEL 2
#define DIO_RT_DEFAULT_THETA_FILTER_DELAY 730

#define DIO_RT_DEFAULT_RIPPLE_COEFF1 1.2 
#define DIO_RT_DEFAULT_RIPPLE_COEFF2 0.2
#define DIO_RT_DEFAULT_RIPPLE_TIME_DELAY 0
#define DIO_RT_DEFAULT_RIPPLE_JITTER 0
#define DIO_RT_DEFAULT_RIPPLE_THRESHOLD 3.0 
#define DIO_RT_DEFAULT_RIPPLE_LOCKOUT 150 
#define DIO_RT_DEFAULT_RIPPLE_SPEED_THRESH 1000.0 

#define DIO_RT_DEFAULT_LATENCY_TEST_THRESHOLD 2000

#define DIO_DEFAULT_CM_PER_PIX 0.87

#include <stdint.h>

typedef struct _PulseCommand {
    uint32_t start_samp_timestamp;
    int line;
    int pre_delay; // in ticks; automatically zeroed after first
    int pulse_width; // in ticks (10 kHz)
    int n_pulses;
    int inter_pulse_delay; // in ticks
    int is_biphasic;
    uint64_t pin1, pin2;
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
    double ripCoeff1, ripCoeff2;
    int time_delay;
    int jitter;
    double ripple_threshold;
    int lockout;
    double speed_threshold;

} RippleStimParameters;

typedef struct _RippleStatusMsg {
  double mean;
  double std;
  int sincelast;
  int isRunning;
  double ratSpeed;
} RippleStatusMsg;


typedef struct _LatencyTestParameters {
    int pulse_length;
    int thresh;
} LatencyTestParameters;

#endif
