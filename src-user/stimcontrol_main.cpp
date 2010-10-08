/* the following include files must be present in all behavioral programs */
#include "spikecommon.h"
#include "spike_dio.h"

#include "stimcontrol_defines.h"
#include "spike_userprogram_defines.h"

/* Globals: */
DaqToUserInfo           daq_to_user_info;
SocketInfo              client_message; // the structure for the client messaging
NetworkInfo             netinfo;

fd_set  readfds;  // the set of readable fifo file descriptors 
int     maxfds;
int     outputfd; // the file descriptor for output to the spike behav program

u32 timestamp; // global timestamp tracking

int state = STATE_SINGLE_PULSE_COMMANDS;

unsigned short pin1 = DEFAULT_LASER_PIN;
unsigned short pin2 = DEFAULT_LASER_PIN_2;
int biphasicStim = 0;

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

  FILE        *outfile = NULL;

  unsigned short     *usptr;
  int         dspnum;
  DSPInfo     *dptr;

  int counter = 0;
  u32 tsdiff = 0;
  u32 oldts = 0;
  double avgtsdiff = 0.0;
  double stdtsdiff = 0.0;

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

          /*
          // Check timing of timestamp packets 
          // answer - by default they come roughly every 193 samples, 
          // note this is adjustable with the config file option [dspsampperpacket  1 2] 
          tsdiff = timestamp - oldts;
          avgtsdiff = avgtsdiff + (tsdiff - avgtsdiff)/100;
          stdtsdiff = stdtsdiff + ( abs(tsdiff - avgtsdiff) - stdtsdiff )/100;
          oldts = timestamp;
          if (counter++ > 10) {
            counter = 0;
            fprintf(stderr,"Moving average timestamp diff: %f (%f); Last: %d\n",
                avgtsdiff, stdtsdiff, tsdiff);
          }
          */

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
          /* a behavioral event contains three unsigned in. The first
           * is a timestamp indicating when the output should be set (0 for
           * immediately) and the second is the output to set */
          break;
        case DIO_USER_MESSAGE:
          break;
        case DIO_SET_CM_PER_PIX:
          cmPerPix = ((double*)messagedata)[0];
          fprintf(stderr,"rt_user: Setting cm/pix = %f.\n",cmPerPix);
          break;
        case DIO_SET_STIM_PARAMETERS:
          break;
        case DIO_SET_SINGLE_STIM_PIN:
          biphasicStim = 0;
          laserPort = ((int*)messagedata)[0] / 16;
          pin1 = (unsigned short) (0x01 << (((int*)messagedata)[0] % 16));
          fprintf(stderr,"rt_user: Setting  stim pin = %d [PORT %d].\n",pin1,laserPort);
          break;
        case DIO_SET_BIPHASIC_STIM_PINS:
          biphasicStim = 1;
          laserPort = ((int*)messagedata)[0] / 16;
          pin1 = (unsigned short) (0x01 << (((int*)messagedata)[0] % 16));
          pin2 = (unsigned short) (0x01 << (((int*)messagedata)[1] % 16));
          fprintf(stderr,"rt_user: Setting biphasic stim A = %d, B = %d [PORT %d].\n",pin1, pin2, laserPort);
          break;
        case DIO_REQUEST_PULSE_FILE:
        case DIO_REQUEST_SIMPLE_STIMS:
          fprintf(stderr,"rt_user: Received request to do pulse file stimulation.\n");
          state = STATE_PULSE_FILE;
          break;
        case DIO_REQUEST_THETA_STIM:
          InitTheta();
          fprintf(stderr,"rt_user: Received request to do theta stimulation.\n");
          state = STATE_THETA_STIM;
          break;
        case DIO_REQUEST_RIPPLE_DISRUPT:
          InitRipple();
          fprintf(stderr,"rt_user: Received request to do ripple disruption.\n");
          state = STATE_RIPPLE_STIM;
          break;
        case DIO_REQUEST_LATENCY_TEST:
          InitLatency();
          fprintf(stderr,"rt_user: Received request to do latency test.\n");
          state = STATE_LATENCY_TEST;
          break;
        case DIO_SET_RT_THETA_PARAMS:
          memcpy((char *)&thetaStimParameters, messagedata,
              sizeof(ThetaStimParameters));
          fprintf(stderr,"rt_user: Received theta stim parameters.\n");
          thetaStimPulseCmd = GenerateSimplePulseCmd(thetaStimParameters.pulse_length);
          PrepareStimCommand(thetaStimPulseCmd);
          break;
        case DIO_SET_RT_RIPPLE_PARAMS:
          memcpy((char *)&rippleStimParameters, messagedata,
              sizeof(RippleStimParameters));
          fprintf(stderr,"rt_user: Received ripple stim parameters.\n");
          fprintf(stderr,"rt_user: Lockout: %d\n", rippleStimParameters.lockout);
          fprintf(stderr,"rt_user: Speed threshold: %f\n", rippleStimParameters.speed_threshold);
          rippleStimPulseCmd = GenerateSimplePulseCmd(rippleStimParameters.pulse_length);
          PrepareStimCommand(rippleStimPulseCmd);
          break;
        case DIO_SET_RT_LATENCY_TEST_PARAMS:
          memcpy((char *)&latencyTestParameters, messagedata,
              sizeof(LatencyTestParameters));
          fprintf(stderr,"rt_user: Received latency test parameters.\n");
          fprintf(stderr,"rt_user: Latency threshold set to %d\n", latencyTestParameters.thresh);
          fprintf(stderr,"rt_user: Latency pulsewidth set to %d\n", latencyTestParameters.pulse_length);
          latencyTestPulseCmd = GenerateSimplePulseCmd(latencyTestParameters.pulse_length);
          PrepareStimCommand(latencyTestPulseCmd);
          break;
        case DIO_QUERY_RT_RIPPLE_STATUS:
          sendRippleStatusUpdate();
          fprintf(stderr,"rt_user: Recieved ripple status query.\n");
          break;
        case DIO_THETA_STIM_START:
        case DIO_RIPPLE_STIM_START:
        case DIO_LATENCY_TEST_START:
          realtimeProcessingEnabled = 1;
          fprintf(stderr,"rt_user: Received Realtime START command (state = %d)......\n", state);
          break;
        case DIO_THETA_STIM_STOP:
        case DIO_RIPPLE_STIM_STOP:
        case DIO_LATENCY_TEST_STOP:
          realtimeProcessingEnabled = 0;
          fprintf(stderr,"rt_user: Received Realtime STOP command......\n");
          break;
        case DIO_PULSE_SEQ:
          ParsePulseFile((char *)messagedata, pulseArray);
          nextPulseCmd = pulseArray;
          nextPulseCmd->start_samp_timestamp = 0; // wait for start
          break;
        case DIO_PULSE_SEQ_START:
          pending = 1;
          fprintf(stderr,"rt_user: Received START command......\n");
          break;
        case DIO_PULSE_SEQ_STOP:
          fprintf(stderr,"rt_user: Received STOP command\n");
          if (nextPulseCmd->type == 0) { // end of file
                  nextPulseCmd = pulseArray;
          }
          else {
             nextPulseCmd->start_samp_timestamp = 0;
             // skip through end of pulse sequence
             if (nextPulseCmd->is_part_of_sequence == 1) {
               while (nextPulseCmd->is_part_of_sequence == 1)
                 nextPulseCmd++;
               nextPulseCmd->start_samp_timestamp = 0;
             }
          }
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

