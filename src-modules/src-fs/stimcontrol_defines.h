#ifndef __STIMCONTROL_DEFINES_H__
#define __STIMCONTROL_DEFINES_H__
/* the following include files must be present in all behavioral programs */
#include "spikecommon.h"
#include "spike_dio.h"
#include "spike_stimcontrol_defines.h"

#define ONTIME  3500  // all messages are an on message followed by an off message 3500 time steps (0.35 sec) later

#define INPUT_BUF_SIZE 2000
#define OUTPUT_BUF_SIZE 2000

//#define DEBUG_FS

//#define LASER_PORT 2
#define DEFAULT_LASER_PIN 0x01
#define DEFAULT_LASER_PIN_2 0x02 

#define DELAY_TO_START_PULSE_FILE 5000 // 500 ms delay from receiving pulse file to running

#define MIN_LATENCY 300 // about 9 Hz
#define MAX_LATENCY 900 // about 6 Hz

#define STATE_SINGLE_PULSE_COMMANDS 100
#define STATE_PULSE_FILE 200
#define STATE_THETA_STIM 300
#define STATE_RIPPLE_STIM 400
#define STATE_LATENCY_TEST 500

#define PULSE_IMMEDIATELY 1

#define NLAST_VALS 20
#define NFILT 20
#define NSPEED_FILT_POINTS 15

/* Globals */

extern SocketInfo	client_data[MAX_CONNECTIONS];  // data output

extern ThetaStimParameters thetaStimParameters;
extern RippleStimParameters rippleStimParameters;
extern LatencyTestParameters latencyTestParameters;

extern PulseCommand thetaStimPulseCmd;
extern PulseCommand rippleStimPulseCmd;
extern PulseCommand latencyTestPulseCmd;
extern PulseCommand spatialPulseCmd;
extern PulseCommand rtStimPulseCmd;
extern PulseCommand *nextPulseCmd;
extern PulseCommand pulseArray[MAX_PULSE_SEQS+1];

extern u32 last_future_timestamp;

extern int stimcontrolMode ; // global program mode determinator

extern int realtimeProcessingEnabled ;

extern u32 timestamp; // global timestamp tracking

extern unsigned short laserPort;  // the default laser port is 2

extern int commandCached;
extern int pending;

extern double cmPerPix; // global mapping between video and reality
extern double ratSpeed; // global measure of rat speed

typedef struct {
    double rippleMean;
    double rippleSd;
    double fX[NFILT];
    double fY[NFILT];
    int	   filtind;
    double lastval[NLAST_VALS];
    int	   lvind;
    double currentVal;
    double currentThresh;
    double posgain;
} RippleFilterStatus;

typedef struct {
    double speed[NSPEED_FILT_POINTS];
    double lastx;
    double lasty;
    int	   ind;
} SpeedFilterStatus;

typedef struct {
    double speed[NSPEED_FILT_POINTS];
    double lastx;
    double lasty;
    int	   ind;
} SpeedFilterStatus;

typedef struct {
    bool stimOn;
} SpatialFilterStatus;

typedef struct {
  double w1,w2;
  double b0, b1, b2;
  double a1, a2;
} filterStruct;

double BiquadFilter(const double inData, filterStruct * const filtStruct);
double DoubleBiquadFilter(const double inData,  filterStruct * const f1, filterStruct * const f2);
void InitFilters (filterStruct *lp1, filterStruct *lp2, filterStruct *hp1, filterStruct *hp2);
int FindPeak(const double inData, double *filterData);


double filterPosSpeed(u32 x, u32 y);

void InitTheta(void);
void InitRipple(void);
void InitSpatial(void);
void InitPulseArray(void);

void ProcessTimestamp(void);
void ResetRealtimeProcessing(void);

void ParsePulseFile (char *inData, PulseCommand *pulseArray);
void PulseOutput (int pulseWidth);
void PulseOutputCommand(PulseCommand pulseCmd, int ignoreTimestamp = 0); 
u32 PulseCommandLength(PulseCommand pulseCmd); 

void PrepareStimCommand(PulseCommand pulseCmd);
void StopAOut(PulseCommand *pulseCmd);

void InitTheta(void);
u32 ProcessThetaData(double d, u32 t);

void InitRipple(void);
int ProcessRippleData(short electnum, double d);
void sendRippleStatusUpdate (void);
void ResetRippleData(void);
void ResetRippleCounters(void);
int nAboveRippleThresh(RippleFilterStatus *rptr);

void ResetSpeedData(void);

void ProcessSpatialData(u32 xpos, u32 ypos);
void sendSpatialStatusUpdate(void);

void InitLatency(void);
int ProcessLatencyData(short d);

#endif
