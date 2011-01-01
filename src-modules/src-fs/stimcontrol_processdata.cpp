#include "spikecommon.h"
#include "spike_dio.h"
#include "stimcontrol_defines.h"
#include "spike_stimcontrol_defines.h"

extern SocketInfo client_data[MAX_CONNECTIONS]; // the structure for sending data
extern SpeedFilterStatus speedFiltStat;
extern SpatialFilterStatus spatialFiltStat;
extern CommandTimeInfo ctinfo;
extern double speedFilt[NSPEED_FILT_POINTS];


void ProcessData(int datatype, char *data, int datalen)
{

  FSDataContBuffer *cptr;
  
  short *dataptr;
  short *electnumptr;

  u32 stim_timestamp, curr_timestamp;
  u32 *posdataptr;
  int stim;
  int i, j;

  static int flag = 1;

  if (stimcontrolMode == DIO_RTMODE_DEFAULT) {// not processing data
    return;
  }


  if (datatype == CONTINUOUS_DATA_TYPE) {
    cptr = (FSDataContBuffer *) data;
    dataptr = cptr->data;
    ctinfo.timestamp = cptr->timestamp;
    if (pending) {
      nextPulseCmd->start_samp_timestamp = ctinfo.timestamp * SAMP_TO_TIMESTAMP + 
	                                   DELAY_TO_START_PULSE_FILE; // start
      fprintf(stderr,"Starting stim: %d, %d\n", ctinfo.timestamp,
              nextPulseCmd->start_samp_timestamp);
      PulseOutputCommand(*nextPulseCmd);
      pending = 0;
    }

    /* go through each of the input channels */
    for (i = 0; i < cptr->nsamp; i++) {
      electnumptr = cptr->electnum;
      for (j = 0; j < cptr->nchan; j++, dataptr++, electnumptr++) {
	switch (stimcontrolMode) {
	  case DIO_RTMODE_THETA:
	    curr_timestamp = ctinfo.timestamp + (i * DSP_BASE_SAMP_RATE) / 
			     cptr->samprate / SAMP_TO_TIMESTAMP;
	    stim_timestamp = ProcessThetaData((double) *dataptr, curr_timestamp);
	    if ((stim_timestamp > 0) && (realtimeProcessingEnabled)) {
	      rtStimPulseCmd.start_samp_timestamp = stim_timestamp * 
		SAMP_TO_TIMESTAMP;
	      PrepareStimCommand(&rtStimPulseCmd, 1);
	      PulseOutputCommand(rtStimPulseCmd);
	    }
	    break;
	  case DIO_RTMODE_RIPPLE_DISRUPT:
	    stim = ProcessRippleData(*electnumptr, (double) *dataptr);
	    if ((stim > 0) && (realtimeProcessingEnabled)) {
	      PulseOutputCommand(rtStimPulseCmd);
	    }
	    break;
	  case DIO_RTMODE_LATENCY_TEST:
	    stim = ProcessLatencyData(*dataptr);
	    if ((stim > 0) && (realtimeProcessingEnabled)) {
	      PulseOutputCommand(rtStimPulseCmd);
	    }
	    break;
	  default:
	    break;
	}
      }
    }
    ProcessTimestamp();
  }
  else if (datatype == SPIKE_DATA_TYPE) {
  }
  if (datatype == POSITION_DATA_TYPE) {
    /* get the timestamp and the estimate of animal position */ 
    posdataptr = (u32 *) data;
    if (datalen != 3*sizeof(u32)) {
      fprintf(stderr,"rt_user: Misunderstood POS_DATA message received (wrong size: %d)\n",datalen);
    }
    else {
      ctinfo.timestamp = *posdataptr;
      ProcessTimestamp();
      if (pending) {
	nextPulseCmd->start_samp_timestamp = ctinfo.timestamp * SAMP_TO_TIMESTAMP + 
					     DELAY_TO_START_PULSE_FILE; // start
	fprintf(stderr,"Starting stim: %d, %d\n", ctinfo.timestamp,
		nextPulseCmd->start_samp_timestamp);
        PulseOutputCommand(*nextPulseCmd);
	pending = 0;
      }
      ratSpeed = filterPosSpeed(posdataptr[1],posdataptr[2]);
      if (stimcontrolMode == DIO_RTMODE_SPATIAL_STIM) {
	stim = ProcessSpatialData(posdataptr[1], posdataptr[2]);
	if (stim && !spatialFiltStat.stimOn) {
	  // the animal is in the box and stimulation is off, so turn it on
	  PulseOutputCommand(rtStimPulseCmd);
	  spatialFiltStat.stimOn = true;
	  spatialFiltStat.lastChange = ctinfo.timestamp;
	}
	else if (!stim && spatialFiltStat.stimOn) {
	  // the animal is not in the box and stimulation is on, so turn it off.
	  StopOutput(&rtStimPulseCmd);
	  spatialFiltStat.stimOn = false;
	  spatialFiltStat.lastChange = ctinfo.timestamp;
	  /* prepare the next command */
          PrepareStimCommand(&rtStimPulseCmd, 1);
	}
      }
    }
  }
}

void InitPulseArray( void)
{
  pulseArray[0].start_samp_timestamp = 0;
  nextPulseCmd = &pulseArray[0];

  ctinfo.command_time = 0;
  ctinfo.next_command_time = 0;
  ctinfo.command_cached = false;
  ctinfo.message_sent = false;
}

void ProcessTimestamp( void )
{

  int messageCode;

  if (ctinfo.timestamp > ctinfo.next_command_time) {
    /* prepare the next stimulation command if appropriate */
    if (!ctinfo.command_cached) {
      switch (stimcontrolMode) {
	case DIO_RTMODE_RIPPLE_DISRUPT:
	case DIO_RTMODE_LATENCY_TEST:
	  PrepareStimCommand(&rtStimPulseCmd, 1);
	  break;
      }
    }
    if (!ctinfo.message_sent) {
      fprintf(stderr, "sending executed message, mode = %d\n", nextPulseCmd->aout_mode);
      SendMessage(client_data[SPIKE_MAIN].fd, DIO_PULSE_SEQ_EXECUTED, (char *)&messageCode, sizeof(int)); 
      ctinfo.message_sent = true;
    }
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

void ResetFSStatus(void) {
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

  int i, ind;

  /* make sure this can't crash */
  double smoothSpd = 0.0;

  /* Calculate instantaneous speed and adjust to cm/sec */

  speedFiltStat.speed[speedFiltStat.ind] = ((x*cmPerPix-speedFiltStat.lastx)*
	                     (x*cmPerPix-speedFiltStat.lastx) + 
     			     (y*cmPerPix-speedFiltStat.lasty) * 
     			     (y*cmPerPix-speedFiltStat.lasty))* 29.97;

  /* make sure this can't crash */
  if (speedFiltStat.speed[speedFiltStat.ind] != 0) {
     speedFiltStat.speed[speedFiltStat.ind] = sqrt(speedFiltStat.speed[speedFiltStat.ind]);
  }


  speedFiltStat.lastx = x*cmPerPix; // save for next time
  speedFiltStat.lasty = y*cmPerPix;

  /* apply the filter to the speed points */
  for (i = 0; i < NSPEED_FILT_POINTS; i++) {
    ind = (speedFiltStat.ind + i) % NSPEED_FILT_POINTS;
    smoothSpd = smoothSpd + speedFiltStat.speed[ind] * speedFilt[i];
  }
  speedFiltStat.ind--;
  if (speedFiltStat.ind < 0) {
     speedFiltStat.ind = NSPEED_FILT_POINTS - 1;
  }

  // if (posOutputFile != NULL)
    // fprintf(posOutputFile,"%f %f %f\n",lastx,lasty,smoothSpd);

  return smoothSpd;
}
