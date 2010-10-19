/* the following include files must be present in all behavioral programs */
#include "spikecommon.h"
#include "spike_dio.h"

#include "stimcontrol_defines.h"
#include "spike_stimcontrol_defines.h"

/* Globals: */
DaqToUserInfo           daq_to_user_info;
SocketInfo              client_message; // the structure for the client messaging
NetworkInfo             netinfo;

fd_set  readfds;  // the set of readable fifo file descriptors 
int     maxfds;
int     outputfd; // the file descriptor for output to the spike behav program

u32 timestamp; // global timestamp tracking

int stimcontrolMode = DIO_RTMODE_DEFAULT;

PulseCommand rtStimPulseCmd;

double ratSpeed = 0.0; // global measure of rat speed

int realtimeProcessingEnabled = 0;

int         masterfd; // the file descriptor for commands to the master DSP

int commandCached = 0;

short unsigned int laserPort = 2; // the default laser port is 2

double cmPerPix = DIO_DEFAULT_CM_PER_PIX;

/* ---- */

int main(int argc, char **argv) 
{
  //unsigned int behavdata[3]; // what is that for  ?
  int         inputfd; // the file descriptor for output form the spike behav program
  int         datafd; // the file descriptor for data from the spike_daq module
  int         biggestfd;
  int         messagedata[MAX_MESSAGE_SIZE];
  int         messagedatalen;
  int         message;
  int         j;
  int         pending = 0;
  int         nPulses = 0;

  FILE        *outfile = NULL;

  unsigned short     *usptr;
  int         dspnum;
  DSPInfo     *dptr;

  int counter = 0;
  u32 tsdiff = 0;
  u32 oldts = 0;
  double avgtsdiff = 0.0;
  double stdtsdiff = 0.0;

  int pulseArraySize;

  inputfd = 0;
  outputfd = 0;
  datafd = 0;

  /* real initializations */
  InitPulseArray();

  /* start up the messaging */
  outputfd = GetClientSocket(USER_TO_DIO_MESSAGE);
  fprintf(stderr, "rt_user: client opened.\n");
  inputfd = GetServerSocket(DIO_TO_USER_MESSAGE);
  fprintf(stderr, "rt_user: server opened.\n");

  datafd = GetServerSocket(DAQ_TO_USER_DATA);
  fprintf(stderr, "rt_user: DAQ data server opened.\n");

  masterfd = GetUDPClientSocket("dsp0",DSP_MESSAGE_PORT);
  fprintf(stderr, "rt_user: UDP client to Master DSP opened.\n");

  biggestfd = MAX(datafd,inputfd);

  /* set up the initial list of file descriptors to be watched */
  FD_ZERO(&readfds);

  while (1) {
    FD_SET(inputfd, &readfds);
    FD_SET(datafd, &readfds);
    select(biggestfd+1, &readfds, NULL, NULL, NULL);

    /* Check data socket to see if data is available. */
    if (FD_ISSET(datafd, &readfds)) {
      /* read in the message */
      message = GetMessage(datafd, (char *) messagedata, &messagedatalen, 0);
      switch(message) {
        case DATA:
          usptr = (unsigned short *) messagedata;
          /* parse the read data */
          /* byte one contains the DSP number */
          memcpy(&timestamp, (usptr+1), 2 * sizeof(unsigned short));
          if (pending) {
            nextPulseCmd->start_samp_timestamp = timestamp + DELAY_TO_START_PULSE_FILE; // start
            fprintf(stderr,"Starting stim: %d, %d\n", timestamp,
                            nextPulseCmd->start_samp_timestamp);
            pending = 0;
          }
          timestamp = timestamp / SAMP_TO_TIMESTAMP;
          dspnum = ((int) usptr[0]);
          if (daq_to_user_info.dsps[dspnum]) {
            usptr += 3; /* move to the data */
            dptr = daq_to_user_info.dspinfo + dspnum; /* nchan and nsamp */
            ProcessData(usptr, dptr);
          }
          ProcessTimestamp();

          break;
        default:
          break;
      }
    }

    /* Check message socket to see if data is available. */
    if (FD_ISSET(inputfd, &readfds)) {
      /* read in the message */
      message = GetMessage(inputfd, (char *) messagedata, &messagedatalen, 0);
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
              InitRipple();
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
          break;
        case SETUP_DAQ_TO_USER:
          /* copy daq_to_user_dsps array into sysinfo structure */
          memcpy((char *)&daq_to_user_info, messagedata,
              sizeof(DaqToUserInfo));
          fprintf(stderr,"rt_user: DAQ to USER will be for dsps: ");
          for  (j = 0; j < MAX_DSPS; j++) {
            if (daq_to_user_info.dsps[j]) 
              fprintf(stderr, "%d", j);
              dptr = &daq_to_user_info.dspinfo[j];
          }
          fprintf(stderr,"\n");
          fprintf(stderr,"rt_user:             and for channels: ");
          for  (j = 0; j < MAX_CHANNELS; j++) 
            if (daq_to_user_info.channels[j])
              fprintf(stderr, "%d ", j);
          fprintf(stderr,"\n");
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
          close(outputfd);
          close(inputfd);
          close(datafd);
          unlink(DAQ_TO_USER_DATA);
          fclose(outfile);
          exit(0);
          break;
        default:
          break;
      }
    }
  }

  return 0;
}

