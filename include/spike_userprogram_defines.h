#ifndef __SPIKE_USERPROGRAM_DEFINES_H__
#define __SPIKE_USERPROGRAM_DEFINES_H__

// Laser control commands
#define DIO_PULSE_SEQ		300    // the message contains text with 
					//a series of pulse sequences
#define DIO_PULSE_SEQ_START	301
#define DIO_PULSE_SEQ_STOP	302
#define DIO_PULSE_SEQ_STEP	303  // a single command has been exec'd
#define DIO_PULSE_SEQ_EXECUTED	304  // the complete file has been exec'd

#define DIO_PULSE_SEQ_RESTART	304
					// has been executed

#define DIO_SET_SINGLE_STIM_PIN 900
#define DIO_SET_BIPHASIC_STIM_PINS 901

#define DIO_SET_CM_PER_PIX 902

#define DIO_REQUEST_SIMPLE_STIMS 330
#define DIO_REQUEST_PULSE_FILE 340

#define DIO_REQUEST_THETA_STIM 350
#define DIO_SET_RT_THETA_PARAMS 351
#define DIO_THETA_STIM_START	352	// start phase extraction / processing
#define DIO_THETA_STIM_STOP	353	// stop phase extraction / processing

#define DIO_REQUEST_RIPPLE_DISRUPT 360
#define DIO_SET_RT_RIPPLE_PARAMS 361
#define DIO_RIPPLE_STIM_START	362	// start extraction / processing
#define DIO_RIPPLE_STIM_STOP	363	// stop extraction / processing
#define DIO_QUERY_RT_RIPPLE_STATUS 364
#define DIO_RT_RIPPLE_STATUS 365

#define DIO_REQUEST_LATENCY_TEST 370
#define DIO_SET_RT_LATENCY_TEST_PARAMS 371
#define DIO_LATENCY_TEST_START	372	// start phase extraction / processing
#define DIO_LATENCY_TEST_STOP	373	// start phase extraction / processing

#define DIO_RT_ENABLE		320	// start continuous data transfer to user program
#define DIO_RT_DISABLE		321	// stop continuous data transfer to user program

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
