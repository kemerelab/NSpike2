#ifndef __STIMCONTROL_GLOBAL_VAR_H__
#define __STIMCONTROL_GLOBAL_VAR_H__
extern SocketInfo	client_data[MAX_CONNECTIONS];  // data output

ThetaStimParameters thetaStimParameters;
RippleStimParameters rippleStimParameters;
LatencyTestParameters latencyTestParameters;

PulseCommand thetaStimPulseCmd;
PulseCommand rippleStimPulseCmd;
PulseCommand latencyTestPulseCmd;
PulseCommand rtStimPulseCmd;
PulseCommand *nextPulseCmd;
PulseCommand pulseArray[MAX_PULSE_SEQS+1];

u32 last_future_timestamp;

int stimcontrolMode = DIO_RTMODE_DEFAULT; // global program mode determinator

int realtimeProcessingEnabled = 0;

u32 timestamp; // global timestamp tracking

unsigned short laserPort = 2;  // the default laser port is 2

int commandCached;
int pending;

double cmPerPix = DIO_DEFAULT_CM_PER_PIX; // global mapping between video and reality
double ratSpeed = 0; // global measure of rat speed

#endif
