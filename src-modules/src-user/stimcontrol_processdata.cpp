#include "spikecommon.h"
#include "spike_dio.h"

#include "stimcontrol_defines.h"
#include "spike_stimcontrol_defines.h"

extern SocketInfo client_data[MAX_CONNECTIONS]; // the structure for sending data


void ProcessData(int datatype, char *data, int datalen)
{

  UserDataContBuffer *cptr;
  
  short *dataptr;

  u32 stim_timestamp, curr_timestamp;
  int stim;
  int i, j;

  static int flag = 1;

  if (stimcontrolMode == DIO_RTMODE_OUTPUT_ONLY) // not processing data
    return;

  if (stimcontrolMode == DIO_RTMODE_DEFAULT) // not processing data
    return;

  //if (realtimeProcessingEnabled == 0) // start button not pushed
    //return;


  /* right now we only process continuous data */
  if (datatype == CONTINUOUS_DATA_TYPE) {
    cptr = (UserDataContBuffer *) data;
    dataptr = cptr->data;
    timestamp = cptr->timestamp;
    if (pending) {
      nextPulseCmd->start_samp_timestamp = timestamp * SAMP_TO_TIMESTAMP + 
	                                   DELAY_TO_START_PULSE_FILE; // start
      fprintf(stderr,"Starting stim: %d, %d\n", timestamp,
              nextPulseCmd->start_samp_timestamp);
      pending = 0;
    }

    /* go through each of the input channels */
    for (i = 0; i < cptr->nsamp; i++) {
      for (j = 0; j < cptr->nchan; j++, dataptr++) {
	switch (stimcontrolMode) {
	  case DIO_RTMODE_THETA:
	    curr_timestamp = timestamp + (i * DSP_BASE_SAMP_RATE) / 
			     cptr->samprate / SAMP_TO_TIMESTAMP;
	    stim_timestamp = ProcessThetaData((double) *dataptr, curr_timestamp);
	    if ((stim_timestamp > 0) && (realtimeProcessingEnabled)) {
	      thetaStimPulseCmd.start_samp_timestamp = stim_timestamp * 
		SAMP_TO_TIMESTAMP;
	      PulseLaserCommand(thetaStimPulseCmd);
	    }
	    break;
	  case DIO_RTMODE_RIPPLE_DISRUPT:
	    stim = ProcessRippleData((double) *dataptr);
	    if ((stim > 0) && (realtimeProcessingEnabled)) {
	      PulseLaserCommand(rippleStimPulseCmd, PULSE_IMMEDIATELY);
	    }
	    break;
	  case DIO_RTMODE_LATENCY_TEST:
	    stim = ProcessLatencyData(*dataptr);
	    if ((stim > 0) && (realtimeProcessingEnabled)) {
	      PulseLaserCommand(latencyTestPulseCmd, PULSE_IMMEDIATELY);
	    }
	    break;
	  default:
	    break;
	}
      }
    }
  }
  ProcessTimestamp();
}

void InitPulseArray( void)
{
  last_future_timestamp = 0;
  pulseArray[0].start_samp_timestamp = 0;
  nextPulseCmd = &pulseArray[0];
}

void ProcessTimestamp( void )
{

  int messageCode;

  if ((nextPulseCmd->pulse_width == DIO_PULSE_COMMAND_END ) || (nextPulseCmd->start_samp_timestamp == 0))
    return;

  if ( (timestamp * SAMP_TO_TIMESTAMP > last_future_timestamp) && 
       (timestamp > nextPulseCmd->start_samp_timestamp/3 - 500) ) { // consider sending another command
    fprintf(stderr,"\n\nrt_user: next command timestamp %d (%d) (%d)\n", nextPulseCmd->start_samp_timestamp/3, last_future_timestamp/3, timestamp);
    PulseLaserCommand(*nextPulseCmd); // send current next command
    SendMessage(client_data[SPIKE_MAIN].fd, DIO_PULSE_SEQ_STEP, (char *) &(nextPulseCmd->line),  sizeof(int));  // send info back to user program

    last_future_timestamp = nextPulseCmd->start_samp_timestamp;

    if (nextPulseCmd->n_repeats > 0) {
      nextPulseCmd->n_repeats--;
      nextPulseCmd->pre_delay = 0;
      nextPulseCmd->start_samp_timestamp = last_future_timestamp +
      nextPulseCmd->inter_frame_delay*3;
      //PulseCommandLength(*nextPulseCmd) +
      return;
    }
    else if (nextPulseCmd->n_repeats == -1) { // continuous
      nextPulseCmd->pre_delay = 0;
      nextPulseCmd->start_samp_timestamp = last_future_timestamp +
      nextPulseCmd->inter_frame_delay*3;
      //PulseCommandLength(*nextPulseCmd) +
      return;
    }

    nextPulseCmd++;

    if (nextPulseCmd->pulse_width == DIO_PULSE_COMMAND_REPEAT)
    {
      if (nextPulseCmd->n_repeats == 0) // done with repeats
        nextPulseCmd++;
      else if (nextPulseCmd->n_repeats == -1)  // continuous repeat mode
        nextPulseCmd = &pulseArray[nextPulseCmd->line];
      else {
        nextPulseCmd->n_repeats--;
        nextPulseCmd = &pulseArray[nextPulseCmd->line];
      }
    }

    if (nextPulseCmd->pulse_width == DIO_PULSE_COMMAND_END) { // end of file
      messageCode = 0;
      SendMessage(client_data[SPIKE_MAIN].fd, DIO_PULSE_SEQ_EXECUTED, (char *)&messageCode, sizeof(int)); 
      return;
    }
      
    nextPulseCmd->start_samp_timestamp = last_future_timestamp + 
      nextPulseCmd->pre_delay*3;
      //PulseCommandLength(*nextPulseCmd) +
  }

}

void InitTheta(void) 
{
  thetaStimParameters.pulse_length = DIO_RT_DEFAULT_PULSE_LEN;
  thetaStimParameters.vel_thresh = DIO_RT_DEFAULT_THETA_VEL;
  thetaStimParameters.filt_delay = DIO_RT_DEFAULT_THETA_FILTER_DELAY;
}

u32 ProcessThetaData(double d, u32 t) {return 0;}


void InitLatency(void)
{
  latencyTestParameters.pulse_length = DIO_RT_DEFAULT_PULSE_LEN;
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

void ResetUserStatus(void) {
    ResetRealtimeProcessing();
}

void ResetRealtimeProcessing(void) {
  switch (stimcontrolMode) {
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

  // if (posOutputFile != NULL)
    // fprintf(posOutputFile,"%f %f %f\n",lastx,lasty,smoothSpd);

  return smoothSpd;
}
