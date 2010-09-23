#include "spikecommon.h"
#include "spike_dio.h"

#include "laser_defines.h"
#include "spike_userprogram_defines.h"

extern DaqToUserInfo           daq_to_user_info;
extern u32 timestamp; // global timestamp tracking

ThetaStimParameters thetaStimParameters;
RippleStimParameters rippleStimParameters;
LatencyTestParameters latencyTestParameters;

PulseCommand thetaStimPulseCmd;
PulseCommand rippleStimPulseCmd;
PulseCommand latencyTestPulseCmd;

PulseCommand *nextPulseCmd;
PulseCommand pulseArray[MAX_PULSES+1];
u32 last_future_timestamp;

void ProcessData(unsigned short *usptr, DSPInfo *dptr) 
{

  u32 stim_timestamp, curr_timestamp;
  int stim;
  int i, j;

  static int flag = 1;

  if (state < 300) // not processing data
    return;

  //if (realtimeProcessingEnabled == 0) // start button not pushed
    //return;

  if (flag==1) {
    fprintf(stderr,"rt_user: number of samples per packet: %d\n",dptr->nsamp);
    flag = 0;
  }

  for (i = 0; i < dptr->nsamp; i++) {
    for (j = 0; j < dptr->nchan; j++, usptr++) {
      if (daq_to_user_info.channels[dptr->dspchan[j]]) {
        switch (state) {
          case STATE_THETA_STIM:
            curr_timestamp = timestamp + (i * DSP_BASE_SAMP_RATE) / dptr->samprate / SAMP_TO_TIMESTAMP;
            stim_timestamp = ProcessThetaData((double) ((short) *usptr), curr_timestamp);
            if ((stim_timestamp > 0) && (realtimeProcessingEnabled)) {
              thetaStimPulseCmd.start_samp_timestamp = stim_timestamp * SAMP_TO_TIMESTAMP;
              PulseLaserCommand(thetaStimPulseCmd);
            }
            break;
          case STATE_RIPPLE_STIM:
            stim = ProcessRippleData((double) ((short) *usptr));
            // if ((stim > 0) && (realtimeProcessingEnabled)) {
            // // push realtimeProcessing into ProcessRippleData
            if ((stim > 0) && (realtimeProcessingEnabled)) {
              PulseLaserCommand(rippleStimPulseCmd, PULSE_IMMEADIATELY);
            }
            break;
          case STATE_LATENCY_TEST:
            stim = ProcessLatencyData((short) *usptr);
            if ((stim > 0) && (realtimeProcessingEnabled)) {
              PulseLaserCommand(latencyTestPulseCmd, PULSE_IMMEADIATELY);
            }
            break;
          default:
            break;
        }
      }
    }
  }

}

void InitPulseArray( void)
{
  last_future_timestamp = 0;
  pulseArray[0].type = 0;
  pulseArray[0].start_samp_timestamp = 0;
  nextPulseCmd = &pulseArray[0];
}

void ProcessTimestamp( void )
{

  u32 next_samp_timestamp;

  if ((nextPulseCmd->type == 0) || (nextPulseCmd->start_samp_timestamp == 0))
    return;


  if ( (timestamp * SAMP_TO_TIMESTAMP > last_future_timestamp) && 
       (timestamp > nextPulseCmd->start_samp_timestamp/3 - 500) ){ // consider sending another command
    fprintf(stderr,"\n\nrt_user: next command timestamp %d (%d) (%d)\n", 
       nextPulseCmd->start_samp_timestamp/3, last_future_timestamp/3, timestamp);
    PulseLaserCommand(*nextPulseCmd); // send current next command
    SendMessage(outputfd, DIO_PULSE_SEQ_STEP, (char *) &(nextPulseCmd->line),  sizeof(int)); 
    // fprintf(stderr,"rt_user: sending DIO_PULSE_SEQ_STEP line %d\n", nextPulseCmd->line);

    last_future_timestamp = nextPulseCmd->start_samp_timestamp;
    next_samp_timestamp = last_future_timestamp + PulseCommandLength(*nextPulseCmd);
    nextPulseCmd++;

    while (nextPulseCmd->type == 'd') {
      next_samp_timestamp += nextPulseCmd->delay * SAMP_TO_TIMESTAMP;
      nextPulseCmd++;
      // fprintf(stderr,"rt_user: deleted delay\n");
    }

    if (nextPulseCmd->type == 'r') {
      nextPulseCmd = pulseArray;
      // fprintf(stderr,"rt_user: should be wrapping around.\n");
    }


    if (nextPulseCmd->type == 0) { // end of file
      SendMessage(outputfd, DIO_PULSE_SEQ_EXECUTED, NULL, 0); 
      // fprintf(stderr,"rt_user: sending DIO_PULSE_SEQ_EXECUTED \n");
      return;
    }
    nextPulseCmd->start_samp_timestamp = next_samp_timestamp;

  }

}

PulseCommand GenerateSimplePulseCmd(int pulse_length) {
  PulseCommand pCmd;

  pCmd.type = 'p';
  pCmd.repetitions = 1;
  pCmd.frequency = 100;
  pCmd.pulse_width = pulse_length;
  pCmd.start_samp_timestamp = 0;

  return pCmd;
}

void InitTheta(void) 
{
  thetaStimParameters.pulse_length = DIO_RT_DEFAULT_PULSE_LEN;
  thetaStimParameters.vel_thresh = DIO_RT_DEFAULT_THETA_VEL;
  thetaStimParameters.filt_delay = DIO_RT_DEFAULT_THETA_FILTER_DELAY;

  thetaStimPulseCmd = GenerateSimplePulseCmd(thetaStimParameters.pulse_length);
}

u32 ProcessThetaData(double d, u32 t) {return 0;}


void InitLatency(void)
{
  latencyTestParameters.pulse_length = DIO_RT_DEFAULT_PULSE_LEN;
  latencyTestPulseCmd = GenerateSimplePulseCmd(latencyTestParameters.pulse_length);
  PrepareStimCommand(latencyTestPulseCmd);
}

int ProcessLatencyData(short d)
{
  int stim = 0;
  static int counter = 0;

  if (counter > 0) {
    counter--;
    return 0;
  }

  if ((d > latencyTestParameters.thresh)) {
    stim = 1;
    counter = 500;
  }
  return stim;

  //return (int) (d > latencyTestParameters.thresh);
}

void ResetRealtimeProcessing(void) {
  switch (state) {
    case STATE_THETA_STIM:
      break;
    case STATE_RIPPLE_STIM:
      ResetRippleData();
      break;
    case STATE_LATENCY_TEST:
      break;
    default:
      break;
  }
}


double filterPosSpeed (u32 x, u32 y) {
  static double lastx = 0.0;
  static double lasty = 0.0;
  static double smoothSpd = 0.0;

  /* Calculate instantaneous speed... */
  double spd = sqrt( (x*cmPerPix-lastx)*(x*cmPerPix-lastx) + 
     (y*cmPerPix-lasty)*(y*cmPerPix-lasty) );

  spd = spd * 29.97; // adjust to cm/second

  lastx = x*cmPerPix; // save for next time
  lasty = y*cmPerPix;

  /* Smooth instantaneous speed. */
  smoothSpd = 0.5*spd + 0.5*smoothSpd;

  if (posOutputFile != NULL)
    fprintf(posOutputFile,"%f %f %f\n",lastx,lasty,smoothSpd);

  return smoothSpd;
}
