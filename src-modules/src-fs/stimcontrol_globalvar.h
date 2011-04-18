#ifndef __STIMCONTROL_GLOBAL_VAR_H__
#define __STIMCONTROL_GLOBAL_VAR_H__
extern SocketInfo	client_data[MAX_CONNECTIONS];  // data output

ThetaStimParameters thetaStimParameters;
RippleStimParameters rippleStimParameters;
LatencyTestParameters latencyTestParameters;
SpatialStimParameters spatialStimParameters;

PulseCommand thetaStimPulseCmd;
PulseCommand rippleStimPulseCmd;
PulseCommand latencyTestPulseCmd;
PulseCommand rtStimPulseCmd;
PulseCommand *nextPulseCmd;
PulseCommand pulseArray[MAX_PULSE_SEQS+1];

RippleFilterStatus ripFiltStat[MAX_ELECTRODES];
SpeedFilterStatus speedFiltStat;
SpatialFilterStatus spatialFiltStat;

CommandTimeInfo ctinfo;

/* the time to send the next command */
u32 next_command_time = 0;
u32 timestamp; // global timestamp tracking

int stimcontrolMode = DIO_RTMODE_DEFAULT; // global program mode determinator

int realtimeProcessingEnabled = 0;


unsigned short laserPort = 2;  // the default laser port is 2

int commandCached;
int pending;

double cmPerPix = DIO_DEFAULT_CM_PER_PIX; // global mapping between video and reality
double ratSpeed = 0; // global measure of rat speed

double fNumerator[NFILT] = {
   2.435723358568172431e-02,
  -1.229133831328424326e-01,
   2.832924715801946602e-01,
  -4.629092463232863941e-01,
   6.834398182647745124e-01,
  -8.526143367711925825e-01,
   8.137704425816699727e-01,
  -6.516133270563613245e-01,
   4.138371933419512372e-01,
   2.165520280363200556e-14,
  -4.138371933419890403e-01,
   6.516133270563868596e-01,
  -8.137704425816841836e-01,
   8.526143367711996879e-01,
  -6.834398182647782871e-01,
   4.629092463232882815e-01,
  -2.832924715801954929e-01,
   1.229133831328426407e-01,
  -2.435723358568174512e-02};


double fDenominator[NFILT] = {
   1.000000000000000000e+00,
  -7.449887056735371438e+00,
   2.866742370538527496e+01,
  -7.644272470167831557e+01,
   1.585893197862293391e+02,
  -2.703338821178639932e+02,
   3.898186201116285474e+02,
  -4.840217978093359079e+02,
   5.230782138295531922e+02,
  -4.945387299274730140e+02,
   4.094389697124813665e+02,
  -2.960738943482194827e+02,
   1.857150345772943751e+02,
  -9.980204002570326338e+01,
   4.505294594295533273e+01,
  -1.655156422615593215e+01,
   4.683913633549676270e+00,
  -9.165841559639211766e-01,
   9.461443242601841330e-02};

double speedFilt[NSPEED_FILT_POINTS] = {
   0.0779,
   0.0775,
   0.0768,
   0.0758,
   0.0745,
   0.0728,
   0.0709,
   0.0688,
   0.0663,
   0.0637,
   0.0610,
   0.0581,
   0.0551,
   0.0520,
   0.0488};


#endif
