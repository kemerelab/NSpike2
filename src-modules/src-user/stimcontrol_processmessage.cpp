/* the following include files must be present in all behavioral programs */
#include "spikecommon.h"
#include "spike_dio.h"
#include "spike_user_defines.h"
#include "stimcontrol_globalvar.h"

void ProcessMessage(int message, char *messagedata, int messagedatalen)
{
  int         j;

  int nPulses;

  int messageCode;

  int counter = 0;
/*  u32 tsdiff = 0;
  u32 oldts = 0;
  double avgtsdiff = 0.0;
  double stdtsdiff = 0.0; */

  int pulseArraySize;

  /* real initializations */
  InitPulseArray();

  switch(message) {
    case DIO_EVENT:
      /* a behavioral event contains three unsigned ints. The first
       * is a timestamp indicating when the output should be set (0 for
       * immediately) and the second is the output to set */
      break;
    case DIO_USER_MESSAGE:
      break;
    case DIO_SET_CM_PER_PIX:
      cmPerPix = ((double*)messagedata)[0];
      fprintf(stderr,"rt_user: Setting cm/pix = %f.\n",cmPerPix);
      break;
    case DIO_STIMCONTROL_MODE:
      memcpy((char *)&stimcontrolMode, messagedata, sizeof(int));
      switch (stimcontrolMode) {
	case DIO_RTMODE_OUTPUT_ONLY:
	  fprintf(stderr,"rt_user: Received request to do output only stimulation.\n");
	  break;
	case DIO_RTMODE_THETA:
	  InitTheta();
	  fprintf(stderr,"rt_user: Received request to do theta stimulation.\n");
	  break;
	case DIO_RTMODE_RIPPLE_DISRUPT:
	  InitRipple();
	  fprintf(stderr,"rt_user: Received request to do ripple disruption.\n");
	  break;
	case DIO_RTMODE_LATENCY_TEST:
	  InitLatency();
	  fprintf(stderr,"rt_user: Received request to do latency test.\n");
	  break;
	case DIO_RTMODE_DEFAULT:
	default:
	  fprintf(stderr,"rt_user: Entering realtime feedback mode with no operation.\n");
	  break;
      }
      break;
    case DIO_SET_RT_STIM_PARAMS:
      memcpy((char *)&rtStimPulseCmd, messagedata,
	  sizeof(PulseCommand));
      PrepareStimCommand(rtStimPulseCmd);
      break;
    case DIO_SET_RT_FEEDBACK_PARAMS:
      //memcpy((char *)&rtStimParameters, messagedata,
	  //sizeof(rtStimParameters));
      break;
    case DIO_QUERY_RT_FEEDBACK_STATUS:
      switch (stimcontrolMode) {
	case DIO_RTMODE_RIPPLE_DISRUPT:
	  sendRippleStatusUpdate();
	  break;
	case DIO_RTMODE_OUTPUT_ONLY:
	case DIO_RTMODE_THETA:
	case DIO_RTMODE_LATENCY_TEST:
	case DIO_RTMODE_DEFAULT:
	default:
	  fprintf(stderr,"rt_user: Recieved status query.\n");
	  break;
      }
      break;
    case DIO_START_RT_FEEDBACK:
      realtimeProcessingEnabled = 1;
      fprintf(stderr,"rt_user: Received Realtime START command (state = %d)......\n", stimcontrolMode);
      break;
    case DIO_STOP_RT_FEEDBACK:
      realtimeProcessingEnabled = 0;
      fprintf(stderr,"rt_user: Received Realtime STOP command......\n");
      break;
    case DIO_PULSE_SEQ:
      nPulses = messagedatalen / sizeof(PulseCommand);
      if ((nPulses > MAX_PULSE_SEQS) || (nPulses < 1)) {
	pulseArray[0].pulse_width = DIO_PULSE_COMMAND_END;
	fprintf(stderr,"rt_user: Pulse command more than MAX_PULSE_SEQS (%d)\n", MAX_PULSE_SEQS);
      }
      else {
	memcpy(pulseArray,(PulseCommand *)messagedata, nPulses * sizeof(PulseCommand));
	fprintf(stderr,"rt_user: Received pulse command of length %d.\n", nPulses);
      }
      nextPulseCmd = pulseArray;
      nextPulseCmd->start_samp_timestamp = 0; // wait for start
      break;
    case DIO_PULSE_SEQ_START:
      pending = 1;
      fprintf(stderr,"rt_user: Received START command......\n");
      break;
    case DIO_PULSE_SEQ_STOP:
      fprintf(stderr,"rt_user: Received STOP command\n");
      nextPulseCmd->start_samp_timestamp = 0;
      messageCode = -1;
      SendMessage(client_data[SPIKE_MAIN].fd, DIO_PULSE_SEQ_EXECUTED, (char *)&messageCode, sizeof(int)); 
      break;
      // reset real time processing for new tetrode
      ResetRealtimeProcessing();
      break;
    case POS_DATA:
      if (messagedatalen != 3*sizeof(u32))
	fprintf(stderr,"rt_user: Misunderstood POS_DATA message received (wrong size: %d)\n",messagedatalen);
      else
	ratSpeed = filterPosSpeed(*(((u32*)messagedata)+1),*(((u32*)messagedata)+2));
      break;
    case EXIT:
      fprintf(stderr, "rt_user: exiting\n");
      unlink(DAQ_TO_USER_DATA);
      exit(0);
      break;
    default:
      break;
  }
}

