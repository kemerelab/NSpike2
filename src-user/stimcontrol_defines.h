#ifndef __STIMCONTROL_DEFINES_H__
#define __STIMCONTROL_DEFINES_H__
/* the following include files must be present in all behavioral programs */
#include "spikecommon.h"
#include "spike_dio.h"
#include "spike_userprogram_defines.h"

#define ONTIME  3500  // all messages are an on message followed by an off message 3500 time steps (0.35 sec) later

#define INPUT_BUF_SIZE 2000
#define OUTPUT_BUF_SIZE 2000

//#define DEBUG_USER

//#define LASER_PORT 2
#define DEFAULT_LASER_PIN 0x01
#define DEFAULT_LASER_PIN_2 0x02 

#define DELAY_TO_START_PULSE_FILE 5000 // 500 ms delay from receiving pulse file to running
#define MAX_PULSES 500

#define MIN_LATENCY 300 // about 9 Hz
#define MAX_LATENCY 900 // about 6 Hz

#define STATE_SINGLE_PULSE_COMMANDS 100
#define STATE_PULSE_FILE 200
#define STATE_THETA_STIM 300
#define STATE_RIPPLE_STIM 400
#define STATE_LATENCY_TEST 500

#define PULSE_IMMEADIATELY 1


typedef struct _PulseCommand {
    char type;
    int line;
    int pulse_width; // in 100 us?
    float frequency; // in Hz
    int repetitions;
    int delay; // in ms
    int is_part_of_sequence;
    u32 start_samp_timestamp;
} PulseCommand;

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
void InitPulseArray(void);

void ProcessData(unsigned short *usptr, DSPInfo *dptr);
void ProcessTimestamp(void);
void ResetRealtimeProcessing(void);


void ParsePulseFile (char *inData, PulseCommand *pulseArray);
void PulseLaser (int pulseWidth);
void PulseLaserCommand(PulseCommand pulseCmd, int ignoreTimestamp = 0); 
u32 PulseCommandLength(PulseCommand pulseCmd); 

/* Globals */
extern DaqToUserInfo           daq_to_user_info;
extern SocketInfo              client_message; // the structure for the client messaging
extern NetworkInfo             netinfo;

extern fd_set                  readfds;  // the set of readable fifo file descriptors 
extern int                     maxfds;
extern int         outputfd; // the file descriptor for output to the spike behav program

extern int state; // global program mode determinator

extern u32 timestamp; // global timestamp tracking

extern unsigned short pin1;
extern unsigned short pin2;
extern int biphasicStim;
extern unsigned short laserPort;

extern int commandCached;

extern double cmPerPix; // global mapping between video and reality
extern double ratSpeed; // global measure of rat speed

/* Globals for stimulation */
extern PulseCommand pulseArray[MAX_PULSES + 1];
extern PulseCommand *nextPulseCmd;

void PrepareStimCommand(PulseCommand pulseCmd);

extern ThetaStimParameters thetaStimParameters;
extern RippleStimParameters rippleStimParameters;
extern LatencyTestParameters latencyTestParameters;

extern PulseCommand thetaStimPulseCmd;
extern PulseCommand rippleStimPulseCmd;
extern PulseCommand latencyTestPulseCmd;

PulseCommand GenerateSimplePulseCmd(int pulse_length);

void InitTheta(void);
u32 ProcessThetaData(double d, u32 t);

void InitRipple(void);
int ProcessRippleData(double d);
void sendRippleStatusUpdate (void);
void ResetRippleData(void);

void InitLatency(void);
int ProcessLatencyData(short d);

extern int realtimeProcessingEnabled;

extern int SendStartDIOCommand(void);
extern int masterfd; // the file descriptor for commands to the master DSP

extern FILE *posOutputFile;

#endif
