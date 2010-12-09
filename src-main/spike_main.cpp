/*
 * spike_main.cpp: the main program for the nspike package.  
 *
 * Copyright 2005 Loren M. Frank
 *
 * This program is part of the nspike data acquisition package.
 * nspike is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * nspike is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with nspike; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* note that the spikecommon.h needs to be first */
#include "spikecommon.h"
#include "spikeMainWindow.h"
#include "spike_main.h"
#include "sqcompat.h"

#include <qapplication.h>
#include <qtimer.h>
#include <q3buttongroup.h>
#include <qlayout.h>
#include <qsizepolicy.h>
#include <qfont.h>
#include <string>
#include <sstream>


/* Global variables */
SpikeMainWindow *spikeMainWindow;
SysInfo         sysinfo;
NetworkInfo     netinfo;
DisplayInfo     dispinfo;
DigIOInfo       digioinfo;
CommonDSPInfo   cdspinfo;
FSDataInfo    fsdatainfo;

struct DisplayData *displaybuf;    // the buffer to be displayed next
struct DisplayData *databuf;    // the buffer to be filled with data
short  *tmpdatabuf;                     // a temporary buffer for incoming spike or continuous data;

struct PosDataRate *posdatarate; // the buffer containing the sizes of the last 30 buffers */

SocketInfo         server_message[MAX_CONNECTIONS]; // the structure for the server messaging
SocketInfo         client_message[MAX_CONNECTIONS]; // the structure for the client messaging
SocketInfo         server_data[MAX_CONNECTIONS]; // the structure for receiving data
SocketInfo         client_data[MAX_CONNECTIONS]; // the structure for sending data (currently unused)

ChannelInfo     lastchinfo[1];

struct timeval         timeout;


int            clock_input_fifo;
int            clock_output_fifo;

EventBuffer        event;

char            timestring[20];
char            tmpstring[200];
char            inputstr[200];
char            inputmessage[200];
int             inputstrlen;
char            inputmessagelen;
char            inputchar;

int             ratpos[2];

u32             synctimes[MAX_DSPS];

ChannelInfo     newchanprog[1000]; // a list of the channels to be programmed every half a second
int             nchanprog = 0;
int             tmpacq;

struct stat     filestat;
struct timeval  lastcommtime;
struct timeval  lastprogtime;
struct timeval  computertime;
struct timezone tz;


#include "spike_main_shared.cpp"

int main(int argc, char **argv) 
{    
  int         nxtarg = 0;
  int         i, j, ind;
  char        command[200];
  char        configname[200];
  char        netconfigname[200];
  int         lowfilt[NDSP_LOW_FILTERS] = DSP_LOW_FILTERS;
  int         highfilt[NDSP_HIGH_FILTERS] = DSP_HIGH_FILTERS;

  dispinfo.screen_width = dispinfo.default_screen_width = 
    DEFAULT_SCREEN_WIDTH;
  dispinfo.screen_height = dispinfo.default_screen_height = 
    DEFAULT_SCREEN_HEIGHT;
  dispinfo.errormessage[0] = '\0';
  dispinfo.fullscreen = 0;
  dispinfo.fullscreenelect = 0;
  //dispinfo.fullscreen = 1;
  dispinfo.totalxsize = X_MAX - X_MIN;
  dispinfo.totalysize = Y_MAX - Y_MIN;
  dispinfo.xunit = dispinfo.totalxsize / 200;
  dispinfo.yunit = dispinfo.totalysize / 200;
  dispinfo.contzoom = 4;
  dispinfo.displayratpos = 0;
  dispinfo.posdatarate = 0;
  dispinfo.eegtracestarttime = 0;

  sysinfo.rtmode = 0;
  sysinfo.acq = 0;
  sysinfo.dspacq = 0;
  sysinfo.posinputnum = -1;
  sysinfo.posthresh = 255;
  sysinfo.posimagesize[0] = PIXELS_PER_LINE;
  sysinfo.posimagesize[1] = LINES_PER_FIELD;
  sysinfo.mpegquality = 80;
  sysinfo.mpegslices = 2;
  sysinfo.videocodec = 1;  // video codec index. See MPEG1_CODEC MPEG2_CODEC MJPEG_CODEC definitions
  sysinfo.videogopsize = 0;  // number of frames between I frames. If 0 each frame is I-frame, like in MJPEG


  sysinfo.posxflip = 0;
  sysinfo.posyflip = 0;
  sysinfo.eegtracelength = 1.0;
  sysinfo.lastchslave = -1;
  sysinfo.origdatafilename[0] = '\0';
  sysinfo.datafilenumber = 1;
  sysinfo.sendalltrackedpixels = 0;
  sysinfo.use_compression = 1;
  sysinfo.compression_level = 6;
  sysinfo.allowsyncchanchange = 0;

  sysinfo.commonref = sysinfo.commonthresh = sysinfo.commonmdv = 
    sysinfo.commonfilt = FALSE;


  digioinfo.enable_DAQ_TO_FS = 0;

  /* Get the current time as for timing the packet output to the dsp network
   * switch */
  gettimeofday(&lastcommtime, &tz);

  /* copy the low and high filter information in to cdspinfo */
  cdspinfo.nLowFilters = NDSP_LOW_FILTERS;
  for (i = 0; i < cdspinfo.nLowFilters; i++) {
    cdspinfo.lowFilt[i] = lowfilt[i];
  }
  cdspinfo.nHighFilters = NDSP_HIGH_FILTERS;
  for (i = 0; i < cdspinfo.nHighFilters; i++) {
    cdspinfo.highFilt[i] = highfilt[i];
  } 
  /* set the gains and cutoffs to reasonable values. DAC 1 is usually sent to
   * the oscilloscope, so its cutoff is set to zero */
  for (i = 0; i < NDSP_AUDIO_CHANNELS; i++) {
    cdspinfo.audiogain[i] = DEFAULT_DSP_AUDIO_GAIN;
    cdspinfo.audiocutoff[i] = DEFAULT_DSP_AUDIO_CUTOFF;
    cdspinfo.audiodelay[i] = DEFAULT_DSP_AUDIO_DELAY;
  }


  inputchar = '\0';

  strcpy(configname, DEFAULT_CONFIG_FILE);
  strcpy(netconfigname, DEFAULT_NETCONFIG_FILE);
  strcpy(sysinfo.colorfilename, DEFAULT_COLOR_FILE);

  //sysinfo.statusfile == NULL;

  if (STATUSFILE == NULL) {
    /* open up the status file if it is not stderr*/
    gethostname(tmpstring,80);
    sprintf(command, "spike_main_status_%s", tmpstring);
    if ((STATUSFILE = fopen(command, "w")) == NULL) {
      fprintf(stderr, "spike_main: error opening status file\n");
      exit(-1);
    }
  }

  /* set the default configuration file name */
  strcpy(configname, DEFAULT_CONFIG_FILE);
  strcpy(netconfigname, DEFAULT_NETCONFIG_FILE);


  /* process the arguments */
  while (++nxtarg < argc) {
    if (strcmp(argv[nxtarg], "-config") == 0) {
      strcpy(configname, argv[++nxtarg]);
    }
    else if (strcmp(argv[nxtarg], "-netconfig") == 0) {
      strcpy(netconfigname, argv[++nxtarg]);
    }
    else if (strcmp(argv[nxtarg], "-v") == 0) {
      fprintf(stderr, "nspike version %s\n", VERSION);
      exit(1);
    }
    else {
      Usage();
      exit(-1);
    }
  }

  /* Read in the configuration file */
  fprintf(STATUSFILE, "spike_main: Reading config file\n");
  if (ReadConfigFile(configname, 0) < 0) {
    fprintf(STATUSFILE, "Error in configuration file, exiting.\n");
    exit(-1);
  }

  fflush(STATUSFILE);
  if (InitializeMasterSlaveNetworkMessaging() < 0) {
    fprintf(STATUSFILE, "spike_main: Error initializing master/slave network messaging\n");
    spikeexit(1);
  }

  /* set up the DSP info. We will send this out to spike_daq later */
  if (!SetDSPInfo()) {
    /* there was an error, so we should exit */ ;
    exit(-1);
  }
  sysinfo.program_type = SPIKE_MAIN;

  /* get the process ID for this process and store it for later renicing*/
  sysinfo.pid = getpid();

  /* initialize the diskfree and filesize elements */
  sysinfo.diskfree = 0;
  sysinfo.datafilesize = 0;

  sysinfo.diskon = 0;
  sysinfo.fileopen = 0;

  if (sysinfo.datatype[sysinfo.machinenum] & DIGITALIO) {
    /* assign the reward lengths */
    ind = 0;
    for (i = 0; i < digioinfo.nports; i++) {
      for (j = 0; j < BITS_PER_PORT; j++, ind++) {
        if (digioinfo.porttype[i]) {
          digioinfo.rewardlength[ind] = OUTPUT_PULSE_LENGTH;
          digioinfo.raised[ind] = 0;
        }
      }
    }
    /* set the pointers to the state machine memory locations. We start
     * one instruction into the buffer so that we can leave the first
     * instruction as a wait forever that we will jump back to once the 
     * state machine is done executing */
    digioinfo.statemachinebaseaddr[0] = DIO_STATE0_BASE_ADDR;
    digioinfo.statemachinebaseaddr[1] = DIO_STATE1_BASE_ADDR;
    digioinfo.statemachinebaseaddr[2] = DIO_STATE2_BASE_ADDR;
    digioinfo.statemachinebaseaddr[3] = DIO_STATE3_BASE_ADDR;
    digioinfo.statemachinebuffer[0] = DIO_STATE0_BUFFER_START + 1;
    digioinfo.statemachinebuffer[1] = DIO_STATE1_BUFFER_START + 1;
    digioinfo.statemachinebuffer[2] = DIO_STATE2_BUFFER_START + 1;
    digioinfo.statemachinebuffer[3] = DIO_STATE3_BUFFER_START + 1;
    digioinfo.statemachineptr[0] = DIO_STATE0_PTR;
    digioinfo.statemachineptr[1] = DIO_STATE1_PTR;
    digioinfo.statemachineptr[2] = DIO_STATE2_PTR;
    digioinfo.statemachineptr[3] = DIO_STATE3_PTR;
  }


  /* allocate space for the audio button groups */
  dispinfo.audioGroup = new QButtonGroup * [NAUDIO_OUTPUTS];
  dispinfo.fsaudioGroup = new QButtonGroup * [NAUDIO_OUTPUTS];

  cdspinfo.audiochan[0].dspchan  = -1;
  cdspinfo.audiochan[1].dspchan  = -1;

  /* set up the module messaging */
  SetupModuleMessaging();

  /* lauch the associated applications */
  fprintf(STATUSFILE, "Launching modules\n");
  if ((sysinfo.datatype[sysinfo.machinenum] & SPIKE) || (sysinfo.datatype[sysinfo.machinenum] & CONTINUOUS)) {
    sprintf(command, "spike_daq &");
    system(command); 
  }
  if (sysinfo.datatype[sysinfo.machinenum] & POSITION) {
    sprintf(command, "spike_posdaq &");
    system(command); 
    sprintf(command, "spike_process_posdata &");
    system(command); 
  }
  if (sysinfo.datatype[sysinfo.machinenum] & FSDATA) {
    sprintf(command, "spike_fsdata &");
    system(command); 
  }

  /*  Note that the position input, processing, and digital IO control will 
   *  be handled by spike_main_rt */
  sprintf(command, "spike_save_data &");
  system(command); 

  /* Now that we have all of the programs running, we establish the other
   * data and messaging ethernet sockets */
  if (StartNetworkMessaging(server_message, client_message, server_data, 
        client_data) < 0) {
    fprintf(STATUSFILE, "spike_main: Error starting network data messaging\n");
    spikeexit(1);
  }

  /* Send the initializing values for nelectrodes, datatype, buffersamp, and
   * samplingdivisor by sending out the sysinfo structure to each of the client programs*/
  /* Also initializes the global data variables for spike and continuous recording */
  fprintf(STATUSFILE, "Sending system config\n");
  SendSystemConfig();

  if (sysinfo.datatype[sysinfo.machinenum] & DIGITALIO) {
    /* set digioinfo.currentprogram to indicate that no program is being run */
    digioinfo.currentprogram = -1;
#ifndef NO_DSP_DEBUG
    /* add the client to the Master DSP as a source of incoming data */
    AddFD(client_message[DSP0].fd, server_data, netinfo.datainfd);
#endif
  }

  if ((sysinfo.fsdataoutput)) {
    fprintf(STATUSFILE, "Sending fsdata config\n");
    SendFSDataInfo();
    fprintf(STATUSFILE, "Sending digio config to spike_fsdata\n");
    SendDigIOInfo();
  }

  /* now set up the data buffers for display of spike or eeg traces */
  InitializeDataBuffer();


  if (sysinfo.datatype[sysinfo.machinenum] & SPIKE) {
    DefineTetrodeDrawInfo();
    DefineEEGDrawInfo();
  }
  if (sysinfo.datatype[sysinfo.machinenum] & CONTINUOUS) {
    DefineEEGDrawInfo();
  }
  if (sysinfo.datatype[sysinfo.machinenum] & POSITION) {
    DefinePosDrawInfo();
  }


  // open up the QT windows
  QApplication a(argc, argv); 
  /* use a Windows style to get square buttons (maximizes space for button
   * labels) */
  //a.setStyle("windows");

  /* create the main form that has all of the tabbed windows inside it */
  spikeMainWindow = new SpikeMainWindow();
  a.setMainWidget(spikeMainWindow);

  /* set it to have an expanding size policy so that it takes up the maximum 
   * possible space in the windows */
  QSizePolicy es(QSizePolicy::Expanding, QSizePolicy::Expanding, 1, 1);
  spikeMainWindow->resize(dispinfo.screen_width,
      dispinfo.screen_height);
  spikeMainWindow->setSizePolicy(es);


  a.connect( &a, SIGNAL( lastWindowClosed() ), spikeMainWindow, SLOT( masterQuit() ) );

  QTimer *t = new QTimer( SQCompat::instance() );
  t->connect( t, SIGNAL(timeout()), SQCompat::instance(), SLOT(spikeProcessMessages()) );
  t->start( 0, FALSE );


  //#ifndef NO_POS_DEBUG
  if (sysinfo.datatype[sysinfo.machinenum] & POSITION) {
    /* set up a timer to keep track of system and DSP time */
    QTimer *tstamptimer = new QTimer(SQCompat::instance());
    tstamptimer->connect(tstamptimer, SIGNAL(timeout()), 
        SQCompat::instance(), SLOT(getTimeCheck()));
    tstamptimer->start(10000, FALSE );
  }
  //#endif

  spikeMainWindow->show();

  /* program the dsps */
  StopLocalDSPAcquisition();
  if (sysinfo.system_type[sysinfo.machinenum] == MASTER)  {
    fprintf(STATUSFILE, "Programming DSPs\n");
    ProgramAllDSPS();
    fprintf(STATUSFILE, "Programed DSPs\n");
  }


  if (sysinfo.system_type[sysinfo.machinenum] == MASTER)  {
    ResetClock();
    fprintf(STATUSFILE, "Starting acquisition\n");
    StartAcquisition();
    StartLocalDSPAcquisition();
    fprintf(STATUSFILE, "Acquistion started\n");
  }


  if ((sysinfo.fsdataoutput)) {
    fprintf(STATUSFILE, "Starting Feedback / Stim. Data Transmission\n");
    MasterFSDataStart();
  }

  UpdateMenus();

  return a.exec();
}

/* Returns the global SysInfo object
*/
SysInfo& SQCompat::sysInfo() 
{
  return sysinfo;
}


SQCompat * SQCompat::_instance = NULL;

void SQCompat::spikeProcessMessages(void)
  /* the main display and processing loop */
{
  int         message, lastmessage;
  static int         messagedata[SAVE_BUF_SIZE];
  int         messagedatalen[1];
  unsigned short *usptr;
  u32                 *u32ptr;
  int            datareceived;
  int            newposimage;
  int            i, id;
  fd_set        readfds;
  int            maxfds;
  ChannelInfo   *ch, *chnew;
  DIOBuffer           diobuf;

  int tmpm = 0;

  int statemachine = -1;

  /* check for a message from the subsidiary programs */
  newposimage = 0;
  do {
    datareceived = 0;
    SetupFDList(&readfds, &maxfds, server_message, server_data);
    /* this delay will directly affect the frame rate of the display */
    timeout.tv_usec = 100;
    timeout.tv_sec = 0;
    select(maxfds, &readfds, NULL, NULL, &timeout);
    /* first check for data */
    id = 0;
    while ((i = netinfo.datainfd[id]) != -1) {
      if (FD_ISSET(server_data[i].fd, &readfds)) {
        if (i == SPIKE_FS_DATA) {
          /* this is a message from the user's digital IO program.
           * The message should contain a series of unsigned short
           * programming commands for the digital IO state machine
           * of the master DSP, so we pass these commands on to the
           * DSP */
          if ((message = GetMessage(server_data[i].fd, (char *) 
                  tmpdatabuf, messagedatalen, 1)) != -1) {
            //fprintf(stderr,"spike_main: message from spike user: %d\n", message);
            switch(message) {
              case DIO_COMMAND:
                //fprintf(stderr, "master DSP timestamp = %d\n", ReadDSPTimestamp(0));
                if (!WriteDSPDIOCommand((unsigned short *) tmpdatabuf, (int) *messagedatalen / sizeof(unsigned short), 0, 0)) { 
                  sprintf(tmpstring, "Error writing user program digital IO command to Master DSP\n");
                  DisplayErrorMessage(tmpstring);
                }
                break;
              case DIO_SPEC_STATEMACHINE:
                statemachine = *((int *)tmpdatabuf);
                break;
              case DIO_COMMAND_TO_STATEMACHINE:
		fprintf(stderr, "dio command to statemachine %d\n", statemachine);
                if (!WriteDSPDIOCommand((unsigned short *) tmpdatabuf, (int) *messagedatalen / sizeof(unsigned short),statemachine,0)) { 
                  sprintf(tmpstring, "Error writing user program digital IO command to Master DSP\n");
                  DisplayErrorMessage(tmpstring);
                }
                break;
              case DIO_EXPECT_DIO_RESPONSE:
                if (~LookForDSPDIOResponse() ) { 
                  // LookFor already throws an error
                }
                break;
              case DIO_RUN_STATEMACHINE:
                if (!WriteDSPDIORestartStateMachine( *((int *)tmpdatabuf) )) { 
                  sprintf(tmpstring, "Error restarting state machine %d\n", *((int *) tmpdatabuf));
                  DisplayErrorMessage(tmpstring);
                }
                break;
              default:
		if (dispinfo.fsguiptr) {
		  ((DIOInterface *)dispinfo.fsguiptr)->msgFromFS(message, (char *)tmpdatabuf);
		}
                break;
            }
          }
          else {
            fprintf(stderr, "Error getting spike_fsdata program output %d\n", i);
          }
        }
	else if (i == SPIKE_DAQ) {
          /* get the buffer information */
          if ((message = GetMessage(server_data[i].fd,
                  (char *) tmpdatabuf,
                  messagedatalen, 1)) != -1) {
            /* check to see what sort of data it is */
            if (message == SPIKE_DATA) {
              memcpy(&databuf->spikebuf, tmpdatabuf, 
                  *messagedatalen);
              /* calculate the number of spikes */
              databuf->nspikes = *messagedatalen / 
                sizeof(SpikeBuffer);
              databuf->datatype = SPIKE_DATA_TYPE;
              sysinfo.approxtime = databuf->spikebuf[0].timestamp;
              databuf = databuf->next;
            }
            else if (message == CONTINUOUS_DATA) {
              memcpy(&databuf->contbuf, tmpdatabuf, 
                  sizeof(ContBuffer));
              databuf->datatype = CONTINUOUS_DATA_TYPE;
              sysinfo.approxtime = databuf->contbuf.timestamp;
              databuf = databuf->next;
            }
            else if (message == DIGITALIO_EVENT) {
              /* copy the current status into the diobuf */
              memcpy(&diobuf, tmpdatabuf, DIO_BUF_STATIC_SIZE);
              /* if we have a user program running we send this
               * on to the user program so it can keep track for
               * the current digital IO state */
              if (digioinfo.outputfd > 0) {
                SendMessage(digioinfo.outputfd, DIGITALIO_EVENT,
                    (char *) &diobuf, DIO_BUF_STATIC_SIZE);
              }
              /* we also send the message on to the rewardControl
               * gui if it is running */
              if (spikeMainWindow->rewardCont) {
                spikeMainWindow->rewardCont->DIOInput(&diobuf);
              }

            }
          }
          else {
            fprintf(STATUSFILE, "Error getting data from spike_daq.\n");
          }
        }
        else if (i == SPIKE_POSDAQ) {
          /* read in the timestamp and the position data */
          if ((message = GetMessage(server_data[i].fd,
                  (char *) tmpdatabuf,
                  messagedatalen, 1)) != -1) {
            /* update the position timestamp */
            u32ptr = (u32 *) tmpdatabuf;
            u32ptr[0] += sysinfo.cpudsptimeoffset;
            if (digioinfo.outputfd > 0) {
              SendMessage(digioinfo.outputfd, POS_DATA, 
                  (char *) tmpdatabuf, *messagedatalen);
            }
            /* update the time if we don't get the time from the DSP data buffers */
            sysinfo.approxtime = *((u32 *) tmpdatabuf);
            if (!(sysinfo.datatype[sysinfo.machinenum] & SPIKE) && 
                !(sysinfo.datatype[sysinfo.machinenum] & CONTINUOUS)) {
              /* TO DO: use the time check to correct the approximate time from the position samples */
              sysinfo.approxtime = *((u32 *) tmpdatabuf);
            }
          }
          else {
            fprintf(STATUSFILE, "Error getting position data from spike_posdaq.\n");
          }
        }
        else if (i == SPIKE_PROCESS_POSDATA) {
          /* read in a position buffer from the processing routine */
          if (GetPosBuffer(server_data[SPIKE_PROCESS_POSDATA].fd,
                dispinfo.posbuf) != -1) {
            UpdatePosImage(dispinfo.posbuf);   
            newposimage = 1;
            /* set the approximate time */
            sysinfo.approxtime = dispinfo.posbuf->timestamp;
          }
          else {
            fprintf(stderr, "Error getting pos buffer\n");
          }
        }
        else if (server_data[i].fd == client_message[DSPDIO].fd) {
          /* this is a message about a digital output, so read it and
             send it on to SPIKE_SAVE_DATA. Note that in newer 
             versions of the DSP code this message comes from 
             SPIKE_DAQ */
          if (read(client_message[DSPDIO].fd, (char *) tmpdatabuf, 
                DIO_MESSAGE_SIZE) == DIO_MESSAGE_SIZE) {
            usptr = (unsigned short *) tmpdatabuf;
            ByteSwap(usptr, DIO_MESSAGE_SIZE / sizeof(unsigned short));
            /* put the data into a diobuffer */
            diobuf.timestamp = (usptr[1] + (usptr[2] << 16)) / 
              SAMP_TO_TIMESTAMP ;
            for (i = 0; i < MAX_DIO_PORTS; i++) {
              diobuf.status[i] = usptr[i+3];
            }
#ifdef DIO_DEBUG        
            fprintf(stderr, "dio packet at time %u, status:\n", 
                diobuf.timestamp);
            for (i = 0; i < MAX_DIO_PORTS; i++) {
              fprintf(stderr, "port %d: %x\n", i, diobuf.status[i]);
            }
#endif
            /* send the data on to be saved */
            SendMessage(client_data[SPIKE_SAVE_DATA].fd, DIGITALIO_EVENT, (char *) &diobuf, DIO_BUF_STATIC_SIZE);
            /* if there is a user program running we also send this
             * along to the user program */
            if (digioinfo.outputfd > 0) {
              SendMessage(digioinfo.outputfd, DIGITALIO_EVENT,
                  (char *) &diobuf, DIO_BUF_STATIC_SIZE);
            }
          }
          else {
            fprintf(STATUSFILE, "Error getting digital IO data from master DSP.\n");
          }
        }
        datareceived = 1;
      }
      id++;
    } 
    /* check for local messages */
    id = 0;
    while ((i = netinfo.messageinfd[id]) != -1) {
      if (FD_ISSET(server_message[i].fd, &readfds)) {
        message = GetMessage(server_message[i].fd, (char *) messagedata,
            messagedatalen, 0);
        lastmessage = message;
        switch(message) {
          case ERROR_MESSAGE:
            /* getting an error requires that we stop and restart processing and
             * saving*/
            DisplayErrorMessage((char *)messagedata);
            fprintf(STATUSFILE, "spike_main: error message from %d: %s/n", i, (char *) messagedata);
            ResetAll();
            break;
          case STATUS_MESSAGE:
            /* display the status message */
            DisplayStatusMessage((char *) messagedata);
            break;
          case DATA_READY:
            /* we no longer use this */
            break;
          case FILE_OPENED:
            /* update the file status button (common button 0)  
             * and the disk button (to show free space) */
            sysinfo.initialdiskfree = messagedata[0];
            sysinfo.fileopen = 1;
            spikeMainWindow->updateAllInfo();
            UpdateMenus();
            break;
          case FILE_CLOSED:
            /* update the file status button (common button 0)
             * and the disk * button (to show free space) */
            sysinfo.fileopen = 0;
            sysinfo.datafilesize = 0;
            spikeMainWindow->updateAllInfo();
            UpdateMenus();
            break;
          case SAVE_ERROR:
            /* we need to stop saving */
            StopSave();
            break;
          case DSP_TOGGLE_ACQ:
            /* we lost some data on the DSP, so stop and restart acquisition */
            StopLocalDSPAcquisition();
            StartLocalDSPAcquisition();
            break;
          case EXIT:
            /* one of the modules encountered a fatal error, 
             * so exit */
            StopLocalDSPAcquisition();
            StopAcquisition();
            spikeexit(-1);
            break;
          default:
            break;
        }
      }
      id++;
    }
    /* check for network messages from the master. At the moment this is
     * considered separately from the other types of messages, but in
     * theory the file descriptors could be incorporated into the
     * client_message array */
    if (sysinfo.system_type[sysinfo.machinenum] == SLAVE ) {
      if ((netinfo.masterfd[netinfo.myindex]) && 
          (FD_ISSET(netinfo.masterfd[netinfo.myindex], &readfds))) {
        message = GetMessage(netinfo.masterfd[netinfo.myindex], 
            (char *) messagedata, messagedatalen, 0);
        lastmessage = message;
        //fprintf(STATUSFILE, "slave message %d\n", message);
        switch(message) {
          case EXIT:
            if (!sysinfo.fileopen) {
              StopLocalDSPAcquisition();
              StopAcquisition();
              spikeexit(0);
            }
            break;
          case PROGRAM_DSPS:
            ProgramAllDSPS();
            break;
          case REPROGRAM_DSPS:
            if (!ReprogramLocalAuxDSPS((unsigned short *) 
                  messagedata)) {
              DisplayErrorMessage("Error programing local Aux DSPs");
            }
            SendMessage(netinfo.slavefd[netinfo.myindex], 
                DSPS_PROGRAMMED, NULL, 0);
            break;
          case GET_DSP_CODE_REV:
            GetAllDSPCodeRev();
            break;
          case START_ACQUISITION:
            sysinfo.acq = 0;
            StartAllAcquisition();
            break;
          case STOP_ACQUISITION:
            sysinfo.acq = 1;
            StopAllAcquisition();
            break;
          case RESET_CLOCK:
            /* acknowledge the message */
            SendMessage(netinfo.slavefd[netinfo.myindex], CLOCK_RESET, NULL, 0);
            break;
          case START_SAVE:
            if (sysinfo.fileopen) {
              StartSave();
              SendMessage(netinfo.slavefd[netinfo.myindex], 
                  SAVE_STARTED, NULL, 0);
            }
            break;
          case STOP_SAVE:
            if (sysinfo.fileopen) {
              StopSave();
              SendMessage(netinfo.slavefd[netinfo.myindex], 
                  SAVE_STOPPED, NULL, 0);
            }
            break;
          case FS_DATA_START:
            if (FSDataStart()) {
              SendMessage(netinfo.slavefd[netinfo.myindex], 
                  FS_DATA_STARTED, NULL, 0);
            }
            break;
          case FS_DATA_STOP:
            if (FSDataStart()) {
              SendMessage(netinfo.slavefd[netinfo.myindex], 
                  FS_DATA_STOPPED, NULL, 0);
            }
            break;
          case CLEAR_SCREEN: 
            ClearAll();
            SendMessage(netinfo.slavefd[netinfo.myindex], 
                SCREEN_CLEARED, NULL, 0);
            break;
          case OPEN_FILE: 
            {
              OpenFileMessage *ofm = reinterpret_cast<OpenFileMessage *>(messagedata);
              strcpy(sysinfo.datafilename, ofm->filename);
              if (OpenFile()) {
                SendMessage(netinfo.slavefd[netinfo.myindex], 
                    FILE_OPENED, NULL, 0);
              }
            }
            break;
          case CLOSE_FILE: 
            CloseFile();
            SendMessage(netinfo.slavefd[netinfo.myindex], 
                FILE_CLOSED, NULL, 0);
            break;
          case SET_AUDIO:
            /* update cdspinfo and then update the MainForm */
            ch = (ChannelInfo *) (messagedata + 1);
            SetAudio(messagedata[0], ch, 0);
            spikeMainWindow->updateAllInfo();
            break;
          case CHANNEL_INFO:
            /* copy the channel information to the appropriate
             * channel */
            chnew = (ChannelInfo *) messagedata;
            memcpy(newchanprog + nchanprog++, chnew, 
                sizeof(ChannelInfo));
            break;
        }
      }
    }
    else {
      /* this is a MASTER system, so check for messages from the slaves */
      for (i = 0; i < netinfo.nslaves; i++) { 
        if ((netinfo.masterfd[i]) && 
            (FD_ISSET(netinfo.masterfd[i], &readfds))) {
          message = GetMessage(netinfo.masterfd[i], 
              (char *) messagedata, messagedatalen, 0);
          lastmessage = message;
          switch(message) {
            case SET_AUDIO:
              ch = (ChannelInfo *) (messagedata + 1);
              SetAudio(messagedata[0], ch, 0);
              spikeMainWindow->updateAllInfo();
              break;
            case CHANNEL_INFO:
              /* copy the channel information to the appropriate
               * channel */
              chnew = (ChannelInfo *) messagedata;
              memcpy(newchanprog + nchanprog++, chnew, 
                  sizeof(ChannelInfo));
              /* Send out the data to the slaves */
              SendChannelInfo(chnew, i);
              break;
            case STATUS_MESSAGE:
              /* display the status message */
              DisplayStatusMessage((char *) messagedata);
              break;
          }
        }
      }
    }
    /* continue until we go through without getting data */
  } while (datareceived);
  tmpm = 0;

  /* display all of the buffers. This could be problematic if it takes too
   * long to display them and we run out of socket space, but on my machine
   * it is not a problem, even in continuous mode at 30 KHz across 48
   * channels */
  while (displaybuf != databuf) {
    if ((displaybuf->datatype == SPIKE_DATA_TYPE) && 
        (sysinfo.datatype[sysinfo.machinenum] & SPIKE)) {
      DisplayElectrodeData(displaybuf->spikebuf, displaybuf->nspikes);
    }
    else if ((displaybuf->datatype == CONTINUOUS_DATA_TYPE) && 
        (sysinfo.datatype[sysinfo.machinenum] & CONTINUOUS)) {
      DisplayContinuousData(&displaybuf->contbuf);

    }
    displaybuf->datatype = 0;
    displaybuf = displaybuf->next;
  }
  if (newposimage) {
    DisplayPosImage();
    if (posdatarate->display == 1) {
      DisplayPosDataRate();
    }
    if (dispinfo.displayratpos) {
      sprintf(tmpstring, "Center of mass of tracked pixels = %d, %d", 
          ratpos[0], ratpos[1]);
      DisplayStatusMessage(tmpstring);
    }

  }
  if (sysinfo.approxtime - sysinfo.lastdisplayedtime > 1000) {
    sysinfo.lastdisplayedtime = sysinfo.approxtime;
    DisplayTime();
  }
  if ((sysinfo.approxtime - sysinfo.lastfilesizetime > 20000) && 
      sysinfo.fileopen) {
    /* update the file size every two seconds */
    sysinfo.lastfilesizetime = sysinfo.approxtime;
    /* get the current file size */
    stat(sysinfo.datafilename, &filestat);
    sysinfo.datafilesize = (float) filestat.st_size / (float) 1048576;
    sysinfo.diskfree = sysinfo.initialdiskfree - sysinfo.datafilesize;
    /* check the file size to see if it is too large, 
     * and if so, open up a new file */
    if (sysinfo.datafilesize > MAX_FILE_SIZE) {
      sprintf(sysinfo.datafilename, "%s.%d", sysinfo.origdatafilename, 
          ++sysinfo.datafilenumber);
      sprintf(tmpstring, "Max file size reached, switching file to %s", 
          sysinfo.datafilename);
      DisplayStatusMessage(tmpstring);
      OpenFile();
    }
    if (sysinfo.diskfree < 1.0) {
      sprintf(tmpstring, "Disk is full");
      DisplayErrorMessage(tmpstring);
    }    
    spikeMainWindow->updateInfo();
  }
  gettimeofday(&computertime, &tz);

  /* program the channels every half a second. TO DO : move to QT timer
   * function*/
  if ((nchanprog > 0) && ((computertime.tv_sec - lastprogtime.tv_sec > 0) ||
        (computertime.tv_usec - lastprogtime.tv_usec > 500000))) {
    chnew = newchanprog;
    /* stop local acquisition if it is on and we are not saving to disk.
     * Note that this is safe because the slave cannot change references
     * while the disk is on, and none of the other changes require toggling
     * acquisition on the DSPs */
    tmpacq = 0;
    if (sysinfo.dspacq && !sysinfo.diskon) {
      tmpacq = 1;
      StopLocalAcq();
    }
    for (i = 0; i < nchanprog; i++, chnew++) {
      /* go to this channel */
      ch = sysinfo.channelinfo[chnew->machinenum] + 
        chnew->index;
      /* if this electrode is present on this system, set its
       * depth and reference */
      if (ch->depth != chnew->depth) {
        SetDepth(chnew->number, chnew->depth);
      }
      /* only set the reference if we do not have local references */
      if (!sysinfo.localref && ((ch->refelect != chnew->refelect) || 
            (ch->refchan != chnew->refchan))) {
        SetReference(chnew->number, chnew->refelect, 
            chnew->refchan);
      }
      memcpy(ch, chnew, sizeof(ChannelInfo));
    }
    if (tmpacq) {
      StartLocalAcq();
    }
    nchanprog = 0;
  }
  if (computertime.tv_sec - lastcommtime.tv_sec > 60) { 
    /* send out a packet once a minute to the fake dsp.  This packet will 
     * be discarded by the switch, but serves to tell the switch which 
     * port this machine is on */
    SendMessage(client_message[DSP0ECHO].fd, MESSAGE, NULL, 0); 
#ifndef DIO_ON_MASTER_DSP
    SendMessage(client_message[DSPDIOECHO].fd, MESSAGE, NULL, 0); 
#endif
    lastcommtime.tv_sec = computertime.tv_sec;
  }
  return;
}

void SQCompat::getTimeCheck(void)
  /* get the current system time and the Master DSP time and send the data to
   * spike_save_data */
{
  struct timeval      ctime;
  struct timezone     ctz;
  u32    timeinfo[3];


  /* get the current time */
  gettimeofday(&ctime, &ctz);
  timeinfo[0] = (u32) (ctime.tv_sec - sysinfo.computer_start_time.tv_sec) * 
    SEC_TO_TSTAMP + 
    (ctime.tv_usec - sysinfo.computer_start_time.tv_usec) / 
    TSTAMP_TO_USEC;
  /* now get the Master DSP timestamp */
  timeinfo[1] = ReadDSPTimestamp(0);
  /* repeat the computer time query to get the approximate time it took to 
   * read the timestamp */
  gettimeofday(&ctime, &ctz);
  sysinfo.cpudsptimeoffset = timeinfo[1] - timeinfo[0];
  timeinfo[2] = (u32) (ctime.tv_sec - sysinfo.computer_start_time.tv_sec) * 
    SEC_TO_TSTAMP + 
    (ctime.tv_usec - sysinfo.computer_start_time.tv_usec - 
     DSP_PAUSE_USEC) / 
    TSTAMP_TO_USEC;
  /* Send the time check to spike_save_data */
  SendMessage(client_data[SPIKE_SAVE_DATA].fd, TIME_CHECK, 
      (char *) timeinfo, TIME_CHECK_BUF_STATIC_SIZE);
  //sprintf(tmpstring, "timecheck %u %u %u, cdoffset = %u\n", timeinfo[0], timeinfo[1], timeinfo[2], sysinfo.cpudsptimeoffset);
  //DisplayStatusMessage(tmpstring);
}

inline unsigned char ClipByte( float fVal){
  fVal = fVal < 0 ? 0 : fVal;
  return (unsigned char) (fVal > 255 ? 255 : fVal);
}

void YUV422PtoRGB24( unsigned char * pYUV, GLubyte * pRGB, int nPixels, int iUseMask, char * pMask ){

  int i;

  float fC[5][255];
  unsigned char iY1, iY2, iU, iV;
  float fR, fG, fB;
  unsigned char * pY;
  unsigned char * pU;
  unsigned char * pV;

  for ( i = 0;  i<255; i++) {
    fC[0][i] = 1.1643 * (i - 16);
    fC[1][i] = 1.5958 * (i - 128);
    fC[2][i] = - 0.81290 * (i - 128);
    fC[3][i] = - 0.39173 * (i - 128);
    fC[4][i] = 2.017 * (i - 128);
  }

  pY = pYUV;
  pU = pY + nPixels;
  pV = pU + nPixels/2;


  for( i = 0; i<nPixels/2; i++) {
    iY1 = *pY++;
    iU = *pU++;
    iY2 = *pY++;
    iV = *pV++;

    fR = fC[0][iY1] + fC[1][iV];
    fG = fC[0][iY1] + fC[2][iV] + fC[3][iU];
    fB = fC[0][iY1] + fC[4][iU];

    if ( !iUseMask || !pMask[i*2] ) {
      *pRGB++ = ClipByte(fR);
      *pRGB++ = ClipByte(fG);
      *pRGB++ = ClipByte(fB);
    }
    else {
      pRGB +=3;
    }

    fR = fC[0][iY2]  + fC[1][iV];
    fG = fC[0][iY2]  + fC[2][iV] + fC[3][iU];
    fB = fC[0][iY2]  + fC[4][iU];

    if ( !iUseMask || !pMask[i*2 +1] ) {
      *pRGB++ = ClipByte(fR);
      *pRGB++ = ClipByte(fG);
      *pRGB++ = ClipByte(fB);
    }
    else {
      pRGB +=3;
    }
  }
}


void UpdatePosImage(PosBuffer *posbuf) 
  /* the elements of posbufferinfo as follows:
   * 1. the number of tracked int elements
   * 2. the number of new ushort pixels
   * 3. the number of new pixel colors
   * 4. the pixel number for the animal's location
   */
{
  int                 i;
  int                 *trackedptr;
  unsigned char       *newpixelptr;
  GLubyte             *tmpptr;

  YUV422PtoRGB24( posbuf->image, dispinfo.posimage, dispinfo.imagesize, 
      dispinfo.posoverlay, dispinfo.posoverlaypixels );


  /*    
  // go through all of the pixels and assign the RGB elements of dispinfo.posimage
  // accordingly 
  newpixelptr = posbuf->image;
  tmpptr = dispinfo.posimage;

  for (i = 0; i < dispinfo.imagesize; i++) {
  if ((!dispinfo.posoverlay) || (!dispinfo.posoverlaypixels[i])) {
  // assign the new color to the appropriate three elements 
   *(tmpptr++) = *newpixelptr;
   *(tmpptr++) = *newpixelptr;
   *(tmpptr++) = *(newpixelptr++);
   }
   else {
   tmpptr += 3;
   newpixelptr++;
   }
   }
   */
  // now gow through the list of tracked pixels and make them red 
  //fprintf(STATUSFILE, "%d tracked pixels\n", posbuf->ntracked);
  trackedptr = dispinfo.posbuf->trackedpixels;
  for (i = 0; i < (int) posbuf->ntracked; i++) {
    if ((!dispinfo.posoverlay) || (!dispinfo.posoverlaypixels[i])) {
      tmpptr = dispinfo.posimage + 3 * *trackedptr;
      // assign the new color to the appropriate three elements 
      *(tmpptr++) = 255;
      *(tmpptr++) = 0;
      *(tmpptr++) = 0;
      //          fprintf(STATUSFILE, "tracked pixel at %d, %d\n", *trackedptr % 320, 
      //                  *trackedptr / 240);
    }
    trackedptr++;
  }
  // now put a green pixel at the animal's position 
  tmpptr = dispinfo.posimage + 3 * posbuf->ratpos;
  *(tmpptr++) = 0;
  *(tmpptr++) = 255;
  *(tmpptr++) = 0;
  // calculate the x and y coordinates of the position 
  ratpos[0] = posbuf->ratpos % sysinfo.posimagesize[0];
  ratpos[1] = posbuf->ratpos / sysinfo.posimagesize[0];
  if (dispinfo.posoverlay) {
    // if we are in overlay mode, indicate that this pixel is not to be overwritten 
    dispinfo.posoverlaypixels[posbuf->ratpos] = 1;
  }

  // now update the approximate datarate 
  //    posdatarate->bytes = sysinfo.posbufstaticsize + 
  //                       posbuf->nnewpixels * sizeof(short) + 
  //                       posbuf->nnewcolors * sizeof(unsigned char);
  // move on the the next element of the circularly linked list 
  posdatarate = posdatarate->next;
  return;
}


void UpdatePosInfo(unsigned char newthresh)
{
  float yoffset;
  int   nt;

  nt = (int) newthresh;
  if (client_message[SPIKE_POSDAQ].fd) {
    SendMessage(client_message[SPIKE_POSDAQ].fd, POSITION_INFO, 
        (char *) &nt, sizeof(int));
  }
  if (client_message[SPIKE_PROCESS_POSDATA].fd) {
    SendMessage(client_message[SPIKE_PROCESS_POSDATA].fd, POSITION_INFO, 
        (char *) &nt, sizeof(int));
  }
  yoffset = sysinfo.posthresh * dispinfo.colorbaryscale;
  /* first erase the old threshold marker */
  glColor3f(0.0, 0.0, 0.0);
  glBegin(GL_TRIANGLES);
  glVertex2f(dispinfo.posthreshloc[0].x, dispinfo.posthreshloc[0].y + yoffset);
  glVertex2f(dispinfo.posthreshloc[1].x, dispinfo.posthreshloc[1].y + yoffset);
  glVertex2f(dispinfo.posthreshloc[2].x, dispinfo.posthreshloc[2].y + yoffset);
  glEnd();
  /* Now draw the new threshold marker */
  sysinfo.posthresh = newthresh;
  yoffset = sysinfo.posthresh * dispinfo.colorbaryscale;
  glColor3f(1.0, 1.0, 0.0);
  glBegin(GL_TRIANGLES);
  glVertex2f(dispinfo.posthreshloc[0].x, dispinfo.posthreshloc[0].y + yoffset);
  glVertex2f(dispinfo.posthreshloc[1].x, dispinfo.posthreshloc[1].y + yoffset);
  glVertex2f(dispinfo.posthreshloc[2].x, dispinfo.posthreshloc[2].y + yoffset);
  glEnd();
  glFlush();

  /* now update the display */
  dispinfo.spikePosInfo->update();
  return;
}

void DisplayPosDataRate(void)
{
  float       datarate = 0;
  int         i;

  /* calculate the new data rate. This will move all the way throught the list of data
   * rates */
  for (i = 0; i < FRAME_RATE; i++) {
    datarate += posdatarate->bytes;
    posdatarate = posdatarate->next;
  }
  /* convert datarate to MB/min */
  dispinfo.posdatarate /= 16666.6; 

  dispinfo.spikePosInfo->update();

  return;
}


void DrawInitialScreen(void)
{
  int i;
  /* clear the screen */
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
  /* Draw the initial screen */
  if (sysinfo.datatype[sysinfo.machinenum] & SPIKE) {
    /* if we are in spike mode, we need to reset all of the projection window pointers so
     * that we draw all of the projection points */
    for (i = 0; i < sysinfo.nelectrodes; i++) {
      dispinfo.projdata[i].lastdrawn = dispinfo.projdata[i].point;
    }
    /* deactivate the currently active button so that it is not drawn active in all
     * four tetrode windows */
    DisplayAllTetrodeWindows();
    DisplayAllTetrodeThresholds();
  } 
  else if (sysinfo.datatype[sysinfo.machinenum] & CONTINUOUS) {
    DrawInitialEEGScreen();
  } 
  if (sysinfo.datatype[sysinfo.machinenum] & POSITION) {
    DrawInitialPosScreen();
  } 
  return;
}



int abschannelnum(int electrode, int chan)
  /* returns the absolute number of the current channel given the channel and electrode */
{
  if (dispinfo.fullscreenelect == -1) {
    return (dispinfo.currentdispelect + electrode) * NCHAN_PER_ELECTRODE + 
      chan;
  }
  else {
    return (dispinfo.fullscreenelect * NCHAN_PER_ELECTRODE + chan);
  }
}

void UpdateSpikeProjScale(int electrode) 
  /* use each channel's maxdispval to update the scale of the trace */
{
  float       max;
  int         currentchan;
  int         chan;

  for (chan = 0; chan < NCHAN_PER_ELECTRODE; chan++) {
    currentchan = electrode * NCHAN_PER_ELECTRODE + chan;
    max = sysinfo.channelinfo[sysinfo.machinenum][currentchan].maxdispval;
    dispinfo.spikewinyscale[currentchan] = (float) (TET_SPIKE_WIN_HEIGHT - 
        TET_SPIKE_WIN_Y_ZERO) / max;
    dispinfo.projwinxscale[currentchan] = (float) (TET_PROJ_WIN_WIDTH - 
        dispinfo.xunit / 10) / max;
    dispinfo.projwinyscale[currentchan] = (float) (TET_PROJ_WIN_HEIGHT - 
        dispinfo.yunit / 10) / max;
  }
  return;
}

void UpdateContScale(int chan) 
  /* use the channel's maxdispval to update the scale of the trace */
{
  dispinfo.eegyscale[chan] = dispinfo.eegchanheight[chan] / 
    (float) sysinfo.channelinfo[sysinfo.machinenum][chan].maxdispval;
  return;
}


void DisplayAllTetrodeThresholds(void)
{
  int i, j, chan;

  for (i = 0; i < dispinfo.nelectperscreen; i++) {
    for (j = 0; j < NCHAN_PER_ELECTRODE; j++) {
      if (dispinfo.fullscreenelect == -1) {
        chan = abschannelnum(i, j);
      }
      else {
        chan = abschannelnum(dispinfo.fullscreenelect, j);
      }
      DrawChanThresh(i, chan);
    }
  }
  return;
}


void UpdateTetrodeMaxDispVal(int elect, short mdv) 
  /* Update the maximum displayed value for this tetrode */
{
  int i, chan;
  chan = elect * NCHAN_PER_ELECTRODE;
  for (i = 0; i < NCHAN_PER_ELECTRODE; i++, chan++) {
    UpdateChanMaxDispVal(elect, chan, mdv);
  }
  return;
}

void UpdateChanMaxDispVal(int elect, int chan, short maxdispval)
{
  short thresh;

  sysinfo.channelinfo[sysinfo.machinenum][chan].maxdispval = maxdispval;
  thresh = sysinfo.channelinfo[sysinfo.machinenum][chan].thresh;
  if (sysinfo.defaultdatatype & SPIKE) {
    UpdateSpikeProjScale(elect);
    UpdateChanThresh(elect, chan, thresh);
  }
  UpdateContScale(chan);
}


void UpdateTetrodeThresh(int elect, short thresh) 
  /* set the new threshold for either the specified channel (or electrode). Note that
   * the channel is the screen channel, not the absolute channel */
{
  int i, chan;

  chan = elect * NCHAN_PER_ELECTRODE;
  for (i = 0; i < NCHAN_PER_ELECTRODE; i++, chan++) {
    UpdateChanThresh(elect, chan, thresh);
  }
  return;
}

int UpdateChanThresh(int elect, int chan, short newval)
  /* erase the old threshold and draw the new one */
{
  ChannelInfo *ch;

  ch = sysinfo.channelinfo[sysinfo.machinenum] + chan;
  ch->thresh = newval;

  if ((sysinfo.datatype[sysinfo.machinenum] & SPIKE) && 
      (ElectrodeOnScreen(elect))) {
    DrawChanThresh(elect, chan);
  }

  /* update this channel on all machines */
  SendChannelInfo(ch, netinfo.myindex);

  /* update the channel information in spike_daq */
  SendMessage(client_message[SPIKE_DAQ].fd, CHANNEL_INFO, (char *) ch, 
      sizeof(ChannelInfo));
  return 1;
}

int DrawChanThresh(int elect, int chan)
  /* erase the old threshold and draw the new one */
{
  ChannelInfo *ch;
  Point *threshloc;
  Point *spikeorig;
  int   electchnum;

  ch = sysinfo.channelinfo[sysinfo.machinenum] + chan;

  /* get the number of this channel within the electrode */
  electchnum = chan % NCHAN_PER_ELECTRODE;

  if (dispinfo.fullscreenelect != -1) {
    PushTetrodeMatrix(2);
  }
  else {
    PushTetrodeMatrix(elect);
  }

  spikeorig = dispinfo.spikewinorig + electchnum;
  threshloc = dispinfo.threshloc + chan;


  glColor3f(0.0, 0.0, 0.0);
  glBegin(GL_LINES);
  glVertex2f(threshloc->x, threshloc->y);
  glVertex2f(threshloc->x + TET_THRESH_LENGTH, threshloc->y);
  glEnd();

  /* update the threshold */
  threshloc->y = spikeorig->y + dispinfo.spikewinyscale[chan] * 
    (float) ch->thresh;
  glColor3f(1.0, 1.0, 0.0);
  glBegin(GL_LINES);
  glVertex2f(threshloc->x, threshloc->y);
  glVertex2f(threshloc->x + TET_THRESH_LENGTH, threshloc->y);
  glEnd();

  glPopMatrix();
  glFlush();
  return 1;
}


void DisplayTime(void) 
  /* displays the time in hh:mm:ss.s format */
{
  FormatTS(&dispinfo.timeString, sysinfo.approxtime);
  dispinfo.timeLabel->setText(dispinfo.timeString);

  return;
}

void DisplayStatusMessage(const char *message)
{
  strcpy(dispinfo.statusmessage, message);
  dispinfo.errormessage[0] = '\0';
  /* Update spikeInfo */
  sysinfo.newmessage = 1;
  /* make sure the main form has been created */
  if (spikeMainWindow) {
    spikeMainWindow->updateInfo();
  }
  fprintf(stderr, "Status message: %s\n", message);
}

void DisplayStatusMessage(char *message)
{
  strcpy(dispinfo.statusmessage, message);
  dispinfo.errormessage[0] = '\0';
  /* Update spikeInfo */
  sysinfo.newmessage = 1;
  /* make sure the main form has been created */
  if (spikeMainWindow) {
    spikeMainWindow->updateInfo();
  }
  fprintf(stderr, "Status message: %s\n", message);
}

void DisplayErrorMessage(const char *message)
{

  strcpy(dispinfo.errormessage, message);
  sysinfo.newmessage = 1;
  /* Update spikeInfo */
  if (spikeMainWindow) {
    spikeMainWindow->updateInfo();
  }
  fprintf(stderr, "Error message: %s\n", message);
}

void DisplayErrorMessage(char *message)
{

  strcpy(dispinfo.errormessage, message);
  sysinfo.newmessage = 1;
  /* Update spikeInfo */
  if (spikeMainWindow) {
    spikeMainWindow->updateInfo();
  }
  fprintf(stderr, "Error message: %s\n", message);
}

int OpenFile(void) 
{

  if (!WriteConfigFile(sysinfo.datafilename, 1, 1)) {
    sprintf(tmpstring, "Error opening file %s for writing\n", sysinfo.datafilename);
    DisplayStatusMessage(tmpstring);
    return 0;
  }
  else {
    OpenFileMessage ofm;
    strncpy(ofm.filename, sysinfo.datafilename, sizeof(sysinfo.datafilename));
    ofm.type = OpenFileMessage::Default;
    ofm.compression_level = sysinfo.use_compression ? sysinfo.compression_level : 0;
    /* send a message to the saving program to open the specified file */
    SendMessage(client_message[SPIKE_SAVE_DATA].fd, OPEN_FILE, (char *)&ofm, sizeof(ofm));
    if (sysinfo.datatype[sysinfo.machinenum] & POSITION) {
      SendMessage(client_message[SPIKE_PROCESS_POSDATA].fd, OPEN_FILE, NULL, 0);
    }
    /* we will update the menus once the file open is confirmed */
    if (sysinfo.origdatafilename[0] == '\0') {
      /* this is a new file */
      strcpy(sysinfo.origdatafilename, sysinfo.datafilename);
      sysinfo.datafilenumber = 1;
    }
  }

  return 1;
}

void MasterOpenFiles(char *rootname)
{
  int i, error = 0;
  char tmpname[200];

  if (!(sysinfo.fileopen)) {
    /* send a message to the slaves to open their data files */
    for(i = 0; i < netinfo.nslaves; i++) {
      sprintf(tmpname, "%s.%s.dat", rootname, netinfo.slavename[i]);
      OpenFileMessage ofm;
      strncpy(ofm.filename, tmpname, sizeof(ofm.filename));
      ofm.type = OpenFileMessage::Default;
      ofm.compression_level = sysinfo.use_compression ? sysinfo.compression_level : 0;
      SendMessage(netinfo.slavefd[i], OPEN_FILE, (char *)&ofm, sizeof(ofm));
      /* Wait for the reply */
      if (!WaitForMessage(netinfo.masterfd[i], FILE_OPENED, 2)) {
        sprintf(tmpstring,"spike_main: Error opening data file on slave %s\n", netinfo.slavename[i]);
        DisplayStatusMessage(tmpstring);
        error = 1;
      }
    }
    if (!error) {
      sprintf(sysinfo.datafilename, "%s.%s.dat", rootname, 
          netinfo.myname);
      OpenFile();
    }
  }
  else if (sysinfo.fileopen) {
    sprintf(tmpstring,"Error: a file is already open"); 
    DisplayStatusMessage(tmpstring);
  }
}

void MasterCloseFiles(void)
{
  int i, error = 0;
  if ((sysinfo.fileopen) && !(sysinfo.diskon)){
    /* send a message to the slaves to close their data files */
    for(i = 0; i < netinfo.nslaves; i++) {
      SendMessage(netinfo.slavefd[i], CLOSE_FILE, NULL, 0);
      /* Wait for the reply */
      if (!WaitForMessage(netinfo.masterfd[i], FILE_CLOSED, 2)) {
        sprintf(tmpstring,"spike_main: Error closing data files on slave %s\n", netinfo.slavename[i]);
        DisplayStatusMessage(tmpstring);
        error = 1;
      }
    }
    if (!error) {
      /* if we stopped the save on all of the slaves, stop it here as well */ 
      CloseFile();
    }
  }
}

void CloseFile(void) 
{
  if ((sysinfo.fileopen) && !(sysinfo.diskon)){
    if (sysinfo.datatype[sysinfo.machinenum] & POSITION) {
      SendMessage(client_message[SPIKE_PROCESS_POSDATA].fd, CLOSE_FILE, 
          NULL,0);
      if (!WaitForMessage(server_message[SPIKE_PROCESS_POSDATA].fd, 
            FILE_CLOSED, 10)) {
        sprintf(tmpstring,"Error closing mpeg portion of file\n");
        DisplayStatusMessage(tmpstring);
      }
    }
    SendMessage(client_message[SPIKE_SAVE_DATA].fd, CLOSE_FILE, NULL, 0);
    /* we will update the menus once the file close is confirmed */
    sysinfo.origdatafilename[0] = '\0';
  }
  return;
}

void MasterStartSave(void) 
{
  int i, error = 0;
  if (sysinfo.fileopen) {
    /* send a message to the slaves to start saving */
    for(i = 0; i < netinfo.nslaves; i++) {
      SendMessage(netinfo.slavefd[i], START_SAVE, NULL, 0);
      /* Wait for the reply */
      if (!WaitForMessage(netinfo.masterfd[i], SAVE_STARTED, 2)) {
        sprintf(tmpstring,"spike_main: Error starting save on slave %s\n", netinfo.slavename[i]);
        DisplayStatusMessage(tmpstring);
        error = 1;
      }
    }
    if (!error) {
      /* if we started the save on all of the slaves, start it here as well */ 
      StartSave();
    }
  }
}

void StartSave(void) 
{
  if ((sysinfo.fileopen) && (!sysinfo.diskon)) {
    /* send a message to the saving program to start storing data to disk */
    SendMessage(client_message[SPIKE_SAVE_DATA].fd, START_SAVE, NULL, 0);
    if (!WaitForMessage(server_message[SPIKE_SAVE_DATA].fd, SAVE_STARTED, 
          2)) {
      sprintf(tmpstring,"spike_main: Error starting save, no response from spike_save_data \n");
      DisplayStatusMessage(tmpstring);
      return;
    }
    /* if we are on a position system we also need to tell
     * spike_process_posdata to start saving, as it has to begin the mpeg
     * encoding process */
    if (sysinfo.datatype[sysinfo.machinenum] & POSITION) {
      SendMessage(client_message[SPIKE_PROCESS_POSDATA].fd, START_SAVE, 
          NULL, 0);
      if (!WaitForMessage(server_message[SPIKE_PROCESS_POSDATA].fd, 
            SAVE_STARTED, 2)) {
        sprintf(tmpstring,"spike_main: Error starting save, no response from spike_process_posdata \n");
        DisplayStatusMessage(tmpstring);
        return;
      }
    }
    sysinfo.diskon = 1;
    /* output an event to the saving program */
    event.timestamp = sysinfo.approxtime;
    event.type = EVENT_SAVE_STARTED;
    event.descript[0] = '\0';
    SendMessage(client_data[SPIKE_SAVE_DATA].fd, EVENT, (char *) &event, sizeof(EventBuffer));
    sysinfo.disktime = sysinfo.approxtime;
    spikeMainWindow->updateInfo();
    UpdateMenus();
  }
  return;
} 

void MasterStopSave(void)
{
  int i, error = 0;
  if ((sysinfo.fileopen) && (sysinfo.diskon)){
    /* send a message to the slaves to stop saving */
    for(i = 0; i < netinfo.nslaves; i++) {
      SendMessage(netinfo.slavefd[i], STOP_SAVE, NULL, 0);
      /* Wait for the reply */
      if (!WaitForMessage(netinfo.masterfd[i], SAVE_STOPPED, 2)) {
        sprintf(tmpstring,"spike_main: Error stopping save on slave %s\n", netinfo.slavename[i]);
        DisplayStatusMessage(tmpstring);
        error = 1;
      }
    }
    if (!error) {
      /* if we stopped the save on all of the slaves, stop it here as well */ 
      StopSave();
    }
  }
}

void StopSave(void) 
{
  if (sysinfo.diskon) { 
    /* if we are on a position system we also need to tell
     * spike_process_posdata to stop saving, as it has to end the mpeg
     * encoding process */
    if (sysinfo.datatype[sysinfo.machinenum] & POSITION) {
      SendMessage(client_message[SPIKE_PROCESS_POSDATA].fd, STOP_SAVE, 
          NULL, 0);
      if (!WaitForMessage(server_message[SPIKE_PROCESS_POSDATA].fd, 
            SAVE_STOPPED, 5)) {
        sprintf(tmpstring,"spike_main: Error stopping save, no response from spike_process_posdata \n");
        DisplayStatusMessage(tmpstring);
      }
    }
    SendMessage(client_message[SPIKE_SAVE_DATA].fd, STOP_SAVE, NULL, 0);
    if (!WaitForMessage(server_message[SPIKE_SAVE_DATA].fd, SAVE_STOPPED, 2)) {
      sprintf(tmpstring,"spike_main: Error stopping save in spike_save_data\n");
      DisplayStatusMessage(tmpstring);
      return;
    }
    sysinfo.diskon = 0;
    /* output an event to the saving program */
    event.timestamp = sysinfo.approxtime;
    event.type = EVENT_SAVE_STOPPED;
    event.descript[0] = '\0';
    SendMessage(client_data[SPIKE_SAVE_DATA].fd, EVENT, (char *) &event, sizeof(EventBuffer));

    sysinfo.disktime = sysinfo.approxtime;
    spikeMainWindow->updateInfo();
    UpdateMenus();
  }
  return;
}

void UpdateMenus(void)
{
  /* Update the Master and File menus based on the current status of the
   * system */
  /* First enable acquisition.  This will be subsequently disabled if we're 
   * saving data */
  if (sysinfo.system_type[sysinfo.machinenum] == MASTER) {
    spikeMainWindow->masterAcqAction->setEnabled(true);
  }
  if (!sysinfo.fileopen) {
    /* if there is no file open, the only valid choices are to open a file*/
    spikeMainWindow->openFileAction->setEnabled(true);
    /* disable everything else */
    spikeMainWindow->closeFileAction->setEnabled(false);
    spikeMainWindow->startSaveAction->setEnabled(false);
    spikeMainWindow->stopSaveAction->setEnabled(false);
    if (sysinfo.system_type[sysinfo.machinenum] == MASTER) {
      spikeMainWindow->masterOpenFilesAction->setEnabled(true);
      spikeMainWindow->masterCloseFilesAction->setEnabled(false);
      spikeMainWindow->masterStartSaveAction->setEnabled(false);
      spikeMainWindow->masterStopSaveAction->setEnabled(false);
      /* allow clock resets and sync tests */
      spikeMainWindow->masterResetClockAction->setEnabled(true);
    }
  }
  else {
    /* the file is open, so we can't open it again */
    spikeMainWindow->openFileAction->setEnabled(false);
    if (sysinfo.system_type[sysinfo.machinenum] == MASTER) {
      spikeMainWindow->masterOpenFilesAction->setEnabled(false);
      /* We also need to disable the clock reset and test sync menu 
       * items */
      spikeMainWindow->masterResetClockAction->setEnabled(false);
    }
    if (!sysinfo.diskon) {
      /* if we are not saving, we can either start saving or close the
       * file */
      spikeMainWindow->closeFileAction->setEnabled(true);
      spikeMainWindow->startSaveAction->setEnabled(true);
      spikeMainWindow->stopSaveAction->setEnabled(false);
      if (sysinfo.system_type[sysinfo.machinenum] == MASTER) {
        spikeMainWindow->masterCloseFilesAction->setEnabled(true);
        spikeMainWindow->masterStartSaveAction->setEnabled(true);
        spikeMainWindow->masterStopSaveAction->setEnabled(false);
      }
    }
    else {
      /* the disk is on, so our only option is to stop saving. We also
       * need to disable acquisition */
      spikeMainWindow->closeFileAction->setEnabled(false);
      spikeMainWindow->startSaveAction->setEnabled(false);
      spikeMainWindow->stopSaveAction->setEnabled(true);
      if (sysinfo.system_type[sysinfo.machinenum] == MASTER) {
        spikeMainWindow->masterCloseFilesAction->setEnabled(false);
        spikeMainWindow->masterStartSaveAction->setEnabled(false);
        spikeMainWindow->masterStopSaveAction->setEnabled(true);
        spikeMainWindow->masterAcqAction->setEnabled(false);
      }
    }
  }
  /* update the FS menu if relevant */
  if (sysinfo.fsdataoutput) {
      spikeMainWindow->setFSMenuEnables();
  }
}


void ClearAll(void) 
{
  int tmp;
  if (sysinfo.datatype[sysinfo.machinenum] & SPIKE) {
    /* clear the all projection windows and spike windows */
    for (tmp = 0; tmp < sysinfo.nelectrodes; tmp++) {
      /* clear the current projection window by reseting the appropriate
       * pointers */
      /* get the real current electrode number */
      dispinfo.projdata[tmp].end = dispinfo.projdata[tmp].lastdrawn = dispinfo.projdata[tmp].point;
      dispinfo.projdata[tmp].endindex = 0;
    }
  }
  if (sysinfo.datatype[sysinfo.machinenum] & CONTINUOUS) {
    ResetEEGTraces(1);
  }
  if (sysinfo.datatype[sysinfo.machinenum] & POSITION) {
    /* clear all of the overlay pixels */
    memset(dispinfo.posoverlaypixels, 0, dispinfo.imagesize);
  }
  /* redraw the screen */
  DrawInitialScreen();
  return;
}

void StartDigIOProgram(int prognum)
{
  int error = 0;

  if ((prognum < digioinfo.nprograms)) {
    /* if we had an open digioioral program, we need to send it an exit
     * message and then close it */
    if (digioinfo.inputfd) {
      if (digioinfo.enable_DAQ_TO_FS) {
        SendMessage(client_message[SPIKE_DAQ].fd, CLOSE_DAQ_TO_FS, NULL, 0);
      }
      sleep(1);
      SendMessage(digioinfo.outputfd, EXIT, NULL, 0);
      /* pause to make sure the message is received */
      sleep(1);
      close(digioinfo.inputfd);
      close(digioinfo.outputfd);
      unlink(FS_TO_DIO_MESSAGE);
      unlink(DIO_TO_FS_MESSAGE);
      sprintf(tmpstring, "Stopped digital IO program %s", digioinfo.progname[digioinfo.currentprogram]);
      DisplayStatusMessage(tmpstring);
      /* remove the program from the file descriptor list
       * */
      RemoveFD(digioinfo.inputfd, server_data, netinfo.datainfd);
      digioinfo.inputfd = 0;
      digioinfo.outputfd = 0;
    }
    if (prognum >= 0) {
      /* now lauch the program and set up messaging with it */
      digioinfo.currentprogram = prognum;
      /* remiember that digio.progname includes a "&", meaning 
       * that the system call returns immeadiately. */
      error = system(digioinfo.progname[digioinfo.currentprogram]);
      if ((error == -1) || (error == 127)) {
        sprintf(tmpstring, "Error executing %s\n", 
            digioinfo.progname[digioinfo.currentprogram]);
        DisplayStatusMessage(tmpstring);
      }
      else {
        error = 0;
        /* open server socket (blocks until user program opens client) */
        if ((digioinfo.inputfd = GetServerSocket(FS_TO_DIO_MESSAGE, 
                1, 0)) == -1) {
          sprintf(tmpstring, "Error getting server socket %s for user program\n", 
              FS_TO_DIO_MESSAGE);
          DisplayStatusMessage(tmpstring);
          error = 1;
        }
        /* open client socket (blocks until user program opens server) */
        else if ((digioinfo.outputfd = GetClientSocket(DIO_TO_FS_MESSAGE, 
                1, 0)) == -1) {
          error = 1;
          /* close the server socket */
          close(digioinfo.inputfd);
          unlink(FS_TO_DIO_MESSAGE);
          sprintf(tmpstring, "Error getting server socket %s for user program\n", 
              FS_TO_DIO_MESSAGE);
          DisplayStatusMessage(tmpstring);
        }
        else if (digioinfo.enable_DAQ_TO_FS) {
          SendMessage(client_message[SPIKE_DAQ].fd, OPEN_DAQ_TO_FS, NULL, 0);
          memcpy(sysinfo.daq_to_user.dspinfo, sysinfo.dspinfo, MAX_DSPS*sizeof(DSPInfo));
        }

        if (!error) {
          sprintf(tmpstring, "Executed %s", digioinfo.progname[digioinfo.currentprogram]);
          DisplayStatusMessage(tmpstring);
          /* add the digio program to the list of file descriptors */
          AddFD(digioinfo.inputfd, server_data, netinfo.datainfd);
        }
        else {
          sprintf(tmpstring, "Error in messaging to/from %s", digioinfo.progname[digioinfo.currentprogram]);
          DisplayStatusMessage(tmpstring);
          digioinfo.inputfd = digioinfo.outputfd = 0;
        }
      }
    }
  }
}

void SendDigIOFSMessage(char *message, int len)
{

  if (digioinfo.outputfd) {
    SendMessage(digioinfo.outputfd, DIO_FS_MESSAGE, message,
        len * sizeof(char));
  }
  else {
    sprintf(tmpstring, "Error: user program has not been launched");
    DisplayStatusMessage(tmpstring);
  }
}

void SendFSDataMessage(int messagetype, char *message, int len)
{
  if (digioinfo.outputfd) {
      SendMessage(digioinfo.outputfd, messagetype, message,
	    len);
  }
  else {
      SendMessage(client_message[SPIKE_FS_DATA].fd, messagetype, message,
	    len);
  }
}





void SwitchDisplayModes(void)
{
  int                 i, acq;
  unsigned char       d;

  /* unset the fullscreenelect */
  dispinfo.fullscreenelect = -1;
  dispinfo.nelectperscreen = NELECT_PER_SCREEN;
  acq = 0;
  if (sysinfo.acq) {
    acq = 1;
    StopLocalAcq();
  }

  d = sysinfo.datatype[sysinfo.machinenum];
  if (d & SPIKE) {
    /* deactivate the current button */
    sysinfo.datatype[sysinfo.machinenum] = (d & ~SPIKE) | CONTINUOUS;
    for (i = 0; i < sysinfo.nchannels[sysinfo.machinenum]; i++) {
      UpdateContScale(i);
    }
  }
  else if (d & CONTINUOUS) {
    sysinfo.datatype[sysinfo.machinenum] = (d & ~CONTINUOUS) | SPIKE;
    /* deactivate the current button */
    for (i = 0; i < sysinfo.nelectrodes; i++) {
      UpdateSpikeProjScale(i);
    }
  }

  /* set the first channel of the first electrode to be active */
  dispinfo.currentchan = 0;
  dispinfo.currentelect = 0;
  dispinfo.currentdispelect = 0;

  DrawInitialScreen();

  /* send everyone the DATA_TYPE message to switch modes */
  SendDataType();

  /* move the displaybuf pointer to point to the next data buffer to be filled so that
   * we stay up to date */
  displaybuf = databuf;
  if (acq) {
    StartLocalAcq();
  }
  return;
}

void ResetAll(void)
  /* stop processing and saving and then restart */
{

  if ((sysinfo.program_type & SPIKE) || (sysinfo.program_type & CONTINUOUS)) {
    /* send a message to the processing routine to stop processing and a message to
     * the saving routine to stop saving */
    SendMessage(client_message[SPIKE_DAQ].fd, STOP_PROCESS, NULL, 0);
    SendMessage(client_message[SPIKE_SAVE_DATA].fd, STOP_SAVE, NULL, 0);
    /* Start those routines up again */
    SendMessage(client_message[SPIKE_DAQ].fd, START_PROCESS, NULL, 0);
    SendMessage(client_message[SPIKE_SAVE_DATA].fd, START_SAVE, NULL, 0);
  }
  sprintf(tmpstring, "spike_main: Reset occured at approximately %ud\n", sysinfo.approxtime);
  DisplayStatusMessage(tmpstring);
  return;
}


void SendDataType(void)
  /* send out the current sysinfo.datatype[sysinfo.machinenum] to all of the subprocesses */
{
  /* send messages to the acquisition routine  */
  SendMessage(client_message[SPIKE_DAQ].fd, DATA_TYPE, 
      (char *) &sysinfo.datatype[sysinfo.machinenum], 
      sizeof(sysinfo.datatype[sysinfo.machinenum])); 
  if (!WaitForMessage(server_message[SPIKE_DAQ].fd, SYSTEM_READY,5)) {
    sprintf(tmpstring, "Warning: did not receive SYSTEM_READY from spike_daq");
    fprintf(stderr, "Warning: did not receive SYSTEM_READY from spike_daq");
    DisplayStatusMessage(tmpstring);
  } 


  return;
}

void DrawInitialEEGScreen(void) 
  /* draw the tetrode windows */
{
  glCallList(EEG_CALL_LIST);
  return;
}

void DisplayAllTetrodeWindows(void) 
  /* draw the tetrode windows */
{
  int i;

  for (i = 0; i < dispinfo.nelectperscreen; i++) {
    DisplayTetrodeWindow(i);
  }
  return;
}

void DisplayTetrodeWindow(int elect) 
{
  if (dispinfo.fullscreenelect != -1) {
    PushTetrodeMatrix(2);
  }
  else {
    PushTetrodeMatrix(elect);
  }
  glCallList(TET_SPIKE_CALL_LIST);
  glCallList(TET_PROJ_CALL_LIST);
  glPopMatrix();
  glFlush();
}



void PushTetrodeMatrix(int elect)
  /* translate and scale to the appropriate tetrode matrix */
{
  glPushMatrix();
  /* translate to the electrode */
  if (dispinfo.fullscreenelect != -1) {
    /* scale the display to fill the screen */
    glTranslatef(TET_FULL_SCREEN_XSTART, TET_FULL_SCREEN_YSTART, 0.0f);
    glScalef(2.0, 2.0, 0);
  }
  else {
    glTranslatef(dispinfo.electloc[elect].x, dispinfo.electloc[elect].y, 0.0f);
  }
  return;
}

void SetEEGTraceLength(float len)
{
  if ((len > 0) && (len < 100)) {
    sysinfo.eegtracelength = len;
    SetEEGInc();
    ResetEEGTraces(1);
    DrawInitialEEGScreen();
  }
  else {
    sprintf(tmpstring, "Error: eeg trace length out of bounds (0 < length < 100)");
    DisplayStatusMessage(tmpstring);
  }
}


void DefineEEGDrawInfo(void)
{
  int         i;
  int         start;
  int         tmpchan;
  int         tmpr, tmpg, tmpb;
  float       tmpheight;
  char        tmpbuf[200];
  FILE        *colorfile;

  /* open and process the color file */
  if ((colorfile = fopen(sysinfo.colorfilename, "r")) == NULL) {
    fprintf(STATUSFILE, "Can not open color file %s, check config file for colorfile specification. exiting\n", sysinfo.colorfilename);
    spikeexit(-1);
  }
  /* parse the color file */
  dispinfo.ncolors = 0;
  dispinfo.color = NULL;
  while (fgets(tmpbuf, 200, colorfile)) {
    /* skip any white spaces */
    start = 0;
    while (isspace(tmpbuf[start]) && (tmpbuf[start] != '\0')) start++;
    if ((tmpbuf[start] != '%') && (tmpbuf[start] != '\0') && (tmpbuf[start] != '\n')) {
      /* allocate space for this color */
      dispinfo.color = (Color *) realloc(dispinfo.color, (dispinfo.ncolors+1) * sizeof(Color));
      sscanf(tmpbuf+start, "%d%d%d", &tmpr, &tmpb, &tmpg);
      dispinfo.color[dispinfo.ncolors].r = (float) tmpr / 255.0;
      dispinfo.color[dispinfo.ncolors].g = (float) tmpg / 255.0;
      dispinfo.color[dispinfo.ncolors].b = (float) tmpb / 255.0;
      dispinfo.ncolors++;
    }
  }

  /* get the indeces for the eeg channels */
  dispinfo.chanind = (int *) calloc(MAX_ELECTRODES, sizeof(int));
  for (i = 0; i < sysinfo.nchannels[sysinfo.machinenum]; i++) {
    /* we index the channels by their dsp channel number */
    dispinfo.chanind[sysinfo.channelinfo[sysinfo.machinenum][i].dspchan] = i;
  }

  /* Assuming two EEG windows, using the relative sizes of the two EEG windows, 
   * allocate an appropriate number of channels to each */
  dispinfo.neegchan1 = (int) ((float) sysinfo.nchannels[sysinfo.machinenum] * 
      ((EEG_WIN1_HEIGHT) / (EEG_WIN1_HEIGHT + EEG_WIN2_HEIGHT))); 
  dispinfo.neegchan2 =  sysinfo.nchannels[sysinfo.machinenum] - 
    dispinfo.neegchan1;

  dispinfo.eegtrace = (Point **) calloc(sysinfo.nchannels[sysinfo.machinenum], sizeof(Point *));
  dispinfo.neegpoints = (int *) calloc(sysinfo.nchannels[sysinfo.machinenum], sizeof(int));
  dispinfo.eegpointnum = (int *) calloc(sysinfo.nchannels[sysinfo.machinenum], sizeof(int));
  dispinfo.eegtraceorig = (Point *) malloc(sysinfo.nchannels[sysinfo.machinenum] * sizeof(Point));
  dispinfo.eegtraceoffset = (float *) calloc(sysinfo.nchannels[sysinfo.machinenum],sizeof(float));
  dispinfo.eegxinc = (float *) malloc(sysinfo.nchannels[sysinfo.machinenum] * sizeof(float));
  dispinfo.eegyscale = (float *) malloc(sysinfo.nchannels[sysinfo.machinenum] * sizeof(float));
  dispinfo.eegymin = (float *) malloc(sysinfo.nchannels[sysinfo.machinenum] * sizeof(float));
  dispinfo.eegymax = (float *) malloc(sysinfo.nchannels[sysinfo.machinenum] * sizeof(float));
  dispinfo.eegchanheight = (float *) malloc(sysinfo.nchannels[sysinfo.machinenum] * sizeof(float));

  /* set the increment for the EEG traces, This also allocates space for the
   * eeg traces */
  SetEEGInc();


  /* start with the left window */
  tmpheight = EEG_WIN1_HEIGHT / (dispinfo.neegchan1 + 1);
  tmpchan = 0;
  for(i = 0; i < dispinfo.neegchan1; i++) {
    dispinfo.eegchanheight[tmpchan] = tmpheight / 2;
    dispinfo.eegtraceorig[tmpchan].x = EEG_WIN1_X_START + EEG_WIN_WIDTH / 100.0;
    dispinfo.eegtraceorig[tmpchan].y = EEG_WIN1_Y_START + EEG_WIN1_HEIGHT - (i + 1) * tmpheight;
    /* set the y scale multiplier for each trace */
    /* for the x multiplier we need to figure out how many samples there
     * will be across the window  */
    UpdateContScale(tmpchan);
    /* set the minimum and maximum y values */
    dispinfo.eegymin[tmpchan] = EEG_WIN1_Y_START + EEG_WIN1_HEIGHT / 100.0;
    dispinfo.eegymax[tmpchan] = EEG_WIN1_Y_START + 99 * EEG_WIN1_HEIGHT / 100.0;
    tmpchan++;
  }

  /* now do the left window */
  tmpheight = EEG_WIN2_HEIGHT / (dispinfo.neegchan2 + 1);
  for(i = 0; i < dispinfo.neegchan2; i++) {
    dispinfo.eegchanheight[tmpchan] = tmpheight / 2;
    dispinfo.eegtraceorig[tmpchan].x = EEG_WIN2_X_START + EEG_WIN_WIDTH / 100.0;
    dispinfo.eegtraceorig[tmpchan].y = EEG_WIN2_Y_START + EEG_WIN2_HEIGHT - (i + 1) * tmpheight;
    /* set the x and y scale multiplier for each trace */
    UpdateContScale(tmpchan);
    /* set the minimum and maximum y values */
    dispinfo.eegymin[tmpchan] = EEG_WIN2_Y_START + EEG_WIN2_HEIGHT / 100.0;
    dispinfo.eegymax[tmpchan] = EEG_WIN2_Y_START + 99 * EEG_WIN2_HEIGHT / 100.0;
    tmpchan++;
  }


  /* allocate space for the eeg color lists, one waveform for each channel */
  dispinfo.eegcolor = (Color **) malloc(sysinfo.nchannels[sysinfo.machinenum] * sizeof(Color));
  for (i = 0; i < sysinfo.nchannels[sysinfo.machinenum]; i++) {
    if (sysinfo.channelinfo[sysinfo.machinenum][i].color < dispinfo.ncolors) {
      dispinfo.eegcolor[i] = dispinfo.color + sysinfo.channelinfo[sysinfo.machinenum][i].color;
    }
    else {
      fprintf(STATUSFILE, "Warning: color #%d out of bounds\n",  
          sysinfo.channelinfo[sysinfo.machinenum][i].color);
      dispinfo.eegcolor[i] = dispinfo.color;
    }
  }

  return;

}

void SetEEGInc(void)
  /* use the current eeg trace length to set the x increment for the EEG
   * traces and the number of points to skip during the drawing*/
{
  int i;
  /* assuming about half of the screen_width of pixels for the EEG trace, 
   * we want to skip drawing some of the data points so that we don't draw 
   * too many overlapping pixels. We're going to assume that we don't want
   * more than 10 pixels with the same y coordinate */
  for (i = 0; i < sysinfo.nchannels[sysinfo.machinenum]; i++) {
    /* get the eegskip based on the sampling rate for this channel */
    dispinfo.eegskip = (int) ((float) sysinfo.dspinfo[sysinfo.channelinfo[sysinfo.machinenum][i].dspnum].samprate * (float) sysinfo.eegtracelength 
        / (dispinfo.screen_width * 4));
    if (dispinfo.eegskip < 0) dispinfo.eegskip = 0;
    dispinfo.eegxinc[i] = EEG_WIN_WIDTH / (sysinfo.dspinfo[sysinfo.channelinfo[sysinfo.machinenum][i].dspnum].samprate * 
        sysinfo.eegtracelength) * (1 + dispinfo.eegskip); 
    /* we also need to reallocate space for the eeg point buffers so that 
     * we can be sure that we have enough points to store a complete trace*/
    dispinfo.neegpoints[i] = (int) (EEG_WIN_WIDTH / dispinfo.eegxinc[i]+10);
    dispinfo.eegtrace[i] = (Point *) realloc((void * ) dispinfo.eegtrace[i],
        dispinfo.neegpoints[i] * sizeof(Point));
    //fprintf(stderr, "%d: xinc = %f, %d points\n", i, dispinfo.eegxinc[i], 
    //      dispinfo.neegpoints[i]);
  }

  return;
}

void ResetEEGTraces(int clearlast)
{
  int         i;
  float       *eegoffsetptr;
  int         *npptr;
  int         *eegpointnumptr;
  Point       **eegtraceptr;

  eegoffsetptr = dispinfo.eegtraceoffset;
  eegpointnumptr = dispinfo.eegpointnum;
  for (i = 0; i < sysinfo.nchannels[sysinfo.machinenum]; i++, eegoffsetptr++, eegpointnumptr++) {
    *eegoffsetptr = 0;
    *eegpointnumptr = 0;
  }
  if (clearlast) {
    /* clear out the previously draw points */
    npptr = dispinfo.neegpoints;
    eegtraceptr = dispinfo.eegtrace;
    for (i = 0; i < sysinfo.nchannels[sysinfo.machinenum] ; i++, npptr++, eegtraceptr++) {
      memset((void *) *eegtraceptr, 0, *npptr * sizeof(Point));
    }
  }
  return;
}


void DefineTetrodeDrawInfo(void)
{
  float txinc, tyinc;
  float pxinc, pyinc;
  int i, j, tmpind;

  /* set the number of electrodes to display per screen */
  dispinfo.nelectperscreen = MIN(sysinfo.nelectrodes, NELECT_PER_SCREEN);
  /* allocate space for the list of electrode positions */
  dispinfo.electloc = (Point *) malloc(sizeof(Point) * sysinfo.nelectrodes);
  /* set up the electrode positions. Note that this is currently hard coded for four
   * tetrodes per screen and should be changed for another configuration */
  for (i = 0; i < (sysinfo.nelectrodes / dispinfo.nelectperscreen) + 1; i++) {
    /* we need to check to see if we have fewer electrodes than will fill
     * the screen */
    if (i*NELECT_PER_SCREEN < sysinfo.nelectrodes) {
      dispinfo.electloc[i*NELECT_PER_SCREEN].x = X_MIN;
      dispinfo.electloc[i*NELECT_PER_SCREEN].y = 5;
    }
    if (i*NELECT_PER_SCREEN+1 < sysinfo.nelectrodes) {
      dispinfo.electloc[i*NELECT_PER_SCREEN+1].x = 0;
      dispinfo.electloc[i*NELECT_PER_SCREEN+1].y = 5;
    }
    if (i*NELECT_PER_SCREEN+2 < sysinfo.nelectrodes) {
      dispinfo.electloc[i*NELECT_PER_SCREEN+2].x = X_MIN;
      dispinfo.electloc[i*NELECT_PER_SCREEN+2].y = Y_MIN + 10;
    }
    if (i*NELECT_PER_SCREEN+3 < sysinfo.nelectrodes) {
      dispinfo.electloc[i*NELECT_PER_SCREEN+3].x = 0;
      dispinfo.electloc[i*NELECT_PER_SCREEN+3].y = Y_MIN + 10;
    }
  }
  /* allocate space for the list of projection window points */
  dispinfo.projdata = (ProjectionWindowData *) malloc(sysinfo.nelectrodes * 
      sizeof(ProjectionWindowData));
  for (i = 0; i < sysinfo.nelectrodes;  i++) {
    dispinfo.projdata[i].maxpoints = NCHAN_PER_ELECTRODE * 
      MAX_PROJ_WIN_POINTS;
    dispinfo.projdata[i].endindex = 0;
    dispinfo.projdata[i].point = (short *) 
      malloc(dispinfo.projdata[i].maxpoints * sizeof(short));
    dispinfo.projdata[i].lastdrawn = dispinfo.projdata[i].point;
    dispinfo.projdata[i].end = dispinfo.projdata[i].point;
  }

  /* allocate space for the electrode numbers and set them according to the contents of
   * the sysinfo structure */
  dispinfo.electnum = (int *) malloc(sysinfo.nelectrodes * sizeof(int));
  dispinfo.electind = (int *) malloc(MAX_ELECTRODES* sizeof(int));
  for (i = 0; i < sysinfo.nelectrodes; i++) {
    dispinfo.electnum[i] = 
      sysinfo.channelinfo[sysinfo.machinenum][i*NCHAN_PER_ELECTRODE].number;
    dispinfo.electind[sysinfo.channelinfo[sysinfo.machinenum][i*NCHAN_PER_ELECTRODE].number] = 
      i;
  }
  /* set the initial pointer to indicate that the first four electrodes are to be
   * displayed */
  dispinfo.currentdispelect = 0;


  /* the box for each spike window (y,x) */
  txinc = TET_SPIKE_WIN_WIDTH;
  tyinc = TET_SPIKE_WIN_HEIGHT;

  pxinc = TET_PROJ_WIN_WIDTH;
  pyinc = TET_PROJ_WIN_HEIGHT;

  dispinfo.nelectbuttons = NCHAN_PER_ELECTRODE + 1;
  dispinfo.currentchan = 0;
  dispinfo.currentelect = 0;
  /* set up the button positions for the spike windows and the projection window */

  /* set the distance between adjacent points of the spike waveform */
  dispinfo.spikewinxinc = (float) txinc / (float) (NPOINTS_PER_SPIKE + 1);


  /* allocate space for the threshold y locations */
  dispinfo.threshloc = (Point *) malloc(sizeof(Point) * 
      sysinfo.nchannels[sysinfo.machinenum]);

  /* define the origin point for the four spike windows and the location for the
   * thresholds */
  /* we're going to assume that the origin is 1/3 of the way up the window */
  for (i = 0; i < NCHAN_PER_ELECTRODE; i++) {
    dispinfo.spikewinorig[i].x = TET_SPIKE_WIN_X_START + i * TET_SPIKE_WIN_WIDTH;
    dispinfo.spikewinorig[i].y = TET_SPIKE_WIN_Y_START + TET_SPIKE_WIN_Y_ZERO;
    for (j = 0; j < sysinfo.nelectrodes; j++) {
      dispinfo.threshloc[j*NCHAN_PER_ELECTRODE + i].x = 
        TET_SPIKE_WIN_X_START + .25 * 
        dispinfo.xunit  + i * TET_SPIKE_WIN_WIDTH;
      dispinfo.threshloc[j*NCHAN_PER_ELECTRODE + i].y = 
        TET_SPIKE_WIN_Y_START + dispinfo.yunit;

    }
  }

  dispinfo.spikewinymin = TET_SPIKE_WIN_Y_START + TET_SPIKE_WIN_HEIGHT/50;
  dispinfo.spikewinymax = TET_SPIKE_WIN_Y_START + 49 * TET_SPIKE_WIN_HEIGHT/50;
  /* initialize the overlay */
  dispinfo.overlay = (int *) calloc(sysinfo.nelectrodes, sizeof(int));
  dispinfo.fullscreenelect = -1;

  /* allocate space for the spikewaveform lists, one waveform for each electrode */
  dispinfo.spikewaveform = (Point **) malloc(sysinfo.nelectrodes * sizeof(Point *));
  for (i = 0; i < sysinfo.nelectrodes; i++) {
    dispinfo.spikewaveform[i] = (Point *) calloc(NPOINTS_PER_SPIKE * NCHAN_PER_ELECTRODE, sizeof(Point));
  }

  /* set the multiplier for the translation from the data values to screen coordinates*/

  /* allocate space for the projection window scales and the spike window
   * scales */
  dispinfo.spikewinyscale = (float *) calloc(
      sysinfo.nchannels[sysinfo.machinenum], sizeof(float));
  dispinfo.projwinxscale = (float *) calloc(
      sysinfo.nchannels[sysinfo.machinenum], sizeof(float));
  dispinfo.projwinyscale = (float *) calloc(
      sysinfo.nchannels[sysinfo.machinenum], sizeof(float));

  /* set the multiplier for the translation from the data values to screen coordinates. */
  for (i = 0; i < sysinfo.nelectrodes; i++) {
    UpdateSpikeProjScale(i);
  }

  /* allocate space for the projection window origins and set them */
  dispinfo.nprojwin = (NCHAN_PER_ELECTRODE-1) * (NCHAN_PER_ELECTRODE-2);
  dispinfo.projwinorig = (Point *) calloc(dispinfo.nprojwin, sizeof(Point));


  tmpind = 0;
  for (i = 0; i < NCHAN_PER_ELECTRODE-2; i++) {
    for (j = 0; j < NCHAN_PER_ELECTRODE-1; j++) {
      dispinfo.projwinorig[tmpind].x = TET_PROJ_WIN_X_START + 
        i * TET_PROJ_WIN_WIDTH + 
        dispinfo.xunit / 10; 
      dispinfo.projwinorig[tmpind++].y = TET_PROJ_WIN_Y_START + 
        (2-j) * TET_PROJ_WIN_HEIGHT + 
        dispinfo.yunit / 10; 
    }
  }

  dispinfo.projwinxmax = TET_PROJ_WIN_WIDTH - dispinfo.xunit;
  dispinfo.projwinymax = TET_PROJ_WIN_HEIGHT - dispinfo.yunit;

  /* set the location for the tetrode number */
  dispinfo.electnumloc1.x = TET_SPIKE_WIN_X_START;
  dispinfo.electnumloc1.y = TET_SPIKE_WIN_Y_START + tyinc + 2 * dispinfo.yunit;
  dispinfo.electnumloc2.x = TET_SPIKE_WIN_X_START + 10 * dispinfo.xunit;
  dispinfo.electnumloc2.y = dispinfo.electnumloc1.y + 8 * dispinfo.yunit;

  /* set the location for the tetrode depth */
  dispinfo.depthloc1.x = TET_SPIKE_WIN_X_START + txinc + 5 * dispinfo.xunit;
  dispinfo.depthloc1.y = TET_SPIKE_WIN_Y_START + tyinc + 2 * dispinfo.yunit;
  dispinfo.depthloc2.x = TET_SPIKE_WIN_X_START + txinc + 25 * dispinfo.xunit;
  dispinfo.depthloc2.y = dispinfo.depthloc1.y + 8 * dispinfo.yunit;


  return;
}

void DefinePosDrawInfo(void)
{
  int         i;
  int         ncolors = MAX_POS_THRESH;
  struct PosDataRate *current, *last;

  dispinfo.imagesize = sysinfo.posimagesize[0] * sysinfo.posimagesize[1];
  /* allocate space for the image variables */
  dispinfo.posimage = (GLubyte *) calloc(dispinfo.imagesize * 3, sizeof(GLubyte));
  dispinfo.posoverlaypixels = (char *) calloc(dispinfo.imagesize, sizeof(char));

  dispinfo.posoverlay = 0;

  SetPosLoc();

  /* set up the color bar for the threshold */
  dispinfo.poscolorbarloc[0].x = POS_COLORBAR_X_START;
  dispinfo.poscolorbarloc[0].y = POS_COLORBAR_Y_START;
  dispinfo.poscolorbarloc[1].x = POS_COLORBAR_X_START + POS_COLORBAR_WIDTH;
  dispinfo.poscolorbarloc[1].y = POS_COLORBAR_Y_START + POS_COLORBAR_HEIGHT;
  dispinfo.colorbaryscale = (dispinfo.poscolorbarloc[1].y - dispinfo.poscolorbarloc[0].y) / ncolors;


  /* setup the coordinates for the position threshold triangle. This triangle is drawn
   * shifted along the y dimension to put it in the correct position */
  dispinfo.posthreshloc[0].x = POS_COLORBAR_THRESH_X_START + POS_COLORBAR_THRESH_WIDTH;
  dispinfo.posthreshloc[0].y = POS_COLORBAR_Y_START;
  dispinfo.posthreshloc[1].x = POS_COLORBAR_THRESH_X_START;
  dispinfo.posthreshloc[1].y = POS_COLORBAR_Y_START - POS_COLORBAR_THRESH_HALF_HEIGHT;
  dispinfo.posthreshloc[2].x = POS_COLORBAR_THRESH_X_START;
  dispinfo.posthreshloc[2].y = POS_COLORBAR_Y_START + POS_COLORBAR_THRESH_HALF_HEIGHT;

  /* set up the location for the position MB / min string */
  dispinfo.posdataratestrloc[0].x = POS_WIN_X_START;
  dispinfo.posdataratestrloc[0].y = POS_WIN_Y_START + POS_WIN_HEIGHT + dispinfo.yunit;
  dispinfo.posdataratestrloc[1].x = POS_WIN_X_START + POS_WIN_WIDTH / 2;
  dispinfo.posdataratestrloc[1].y = POS_WIN_Y_START + POS_WIN_HEIGHT + 5 * dispinfo.yunit;

  /* set up the linked list for posdatarate */
  posdatarate = (struct PosDataRate *) malloc(sizeof(struct PosDataRate));
  current = posdatarate;
  for (i = 0; i < FRAME_RATE - 1; i++) {
    current->display = 0;
    current->bytes = 0;
    last = (struct PosDataRate *) malloc(sizeof(struct PosDataRate));
    current->next = last;
    current = current->next;
  }
  current->display = 1;
  current->next = posdatarate;


  /* set up the location for the posdiffthresh and posdiffthresh divisor text */
  dispinfo.posdiffstrloc[0].x = POS_WIN_X_START + POS_WIN_WIDTH / 2;
  dispinfo.posdiffstrloc[0].y = POS_WIN_Y_START + POS_WIN_HEIGHT + dispinfo.yunit;
  dispinfo.posdiffstrloc[1].x = POS_WIN_X_START + POS_WIN_WIDTH;
  dispinfo.posdiffstrloc[1].y = POS_WIN_Y_START + POS_WIN_HEIGHT + 5 * dispinfo.yunit;

  return;
}

void SetPosLoc(void)
{ 
  static int first = 1;
  /* set the image zoom */
  dispinfo.poszoom.x = ((POS_WIN_WIDTH / dispinfo.totalxsize) * 
      dispinfo.screen_width) / sysinfo.posimagesize[0];
  dispinfo.poszoom.y = ((POS_WIN_HEIGHT / dispinfo.totalysize) * 
      dispinfo.screen_height) / sysinfo.posimagesize[1];
  if (first == 1) {
    fprintf(stderr,"poszoom x = %f y = %f\n", dispinfo.poszoom.x,
        dispinfo.poszoom.y);
    first = 0;
  }
  if (sysinfo.posxflip) {
    dispinfo.posloc.x = POS_WIN_X_START + POS_WIN_WIDTH;
    dispinfo.poszoom.x = -dispinfo.poszoom.x;
  }
  else  {
    dispinfo.posloc.x = POS_WIN_X_START;
  }
  /* we want to flip the y axis by default */
  if (sysinfo.posyflip) {
    dispinfo.posloc.y = POS_WIN_Y_START + POS_WIN_HEIGHT;
    dispinfo.poszoom.y = -dispinfo.poszoom.y;
  }
  else  {
    dispinfo.posloc.y = POS_WIN_Y_START;
  }
}


void DrawInitialPosScreen(void) 
{
  /* do the position call list */
  glCallList(POSITION_CALL_LIST);
  glFlush();
  /* draw the threshold marker by updating the position info */
  UpdatePosInfo(sysinfo.posthresh);
  return;
}



void DisplayElectrodeData(SpikeBuffer *spikebuf, int nspikes)
  /* update the projection window data elements and, if this electrode is to be
   * displayed, display it in the correct window */
{
  int                         j, chan, samp, chan2;
  int                         spikeind;
  int                         onscreen;
  int                         electnum;
  int                         firstchan;
  int                         offset1, offset2;
  static int                  totalspikelen = NPOINTS_PER_SPIKE * NCHAN_PER_ELECTRODE;
  short                       tmpmax[NCHAN_PER_ELECTRODE];
  short                       *maxptr;
  short                       *dataptr;
  short                       *data;
  float                       xinc;
  Point                       *orig;          // a pointer to the origin for the windows
  Point                       projpnt;
  Point                       *spike;
  Point                       *spikeptr;
  ProjectionWindowData        *projdataptr;
  SpikeBuffer                 *sptr;

  sptr = spikebuf;
  spikeind = 0;
  while (spikeind++ < nspikes) {
    electnum = dispinfo.electind[sptr->electnum];
    data = sptr->data;
    firstchan = electnum * NCHAN_PER_ELECTRODE;
    //fprintf(stderr, "spike_main: displaying data at position %d\n", electnum);
    onscreen = ElectrodeOnScreen(electnum);

    /* the data for this electrode is in the electind element of projdata */
    projdataptr = dispinfo.projdata + electnum;

    /* check to see if we have maxed out this projection window */
    if (projdataptr->endindex + MAX_SPIKES_PER_BUF >= 
        projdataptr->maxpoints) {
      /* if so, we copy the last 3/4 of the data to the first part of the
       * buffer and then reset the endindex and lastdrawn indeces */
      /* Note that this is only 3/4 for tetrodes, and this should probably be
       * changed for other electrode types */
      offset1 = projdataptr->maxpoints / NCHAN_PER_ELECTRODE;
      offset2 = projdataptr->maxpoints * (NCHAN_PER_ELECTRODE - 1) / NCHAN_PER_ELECTRODE;
      memmove(projdataptr->point, projdataptr->point + offset1, 
          offset2 * sizeof(short));
      projdataptr->end = projdataptr->point + offset2;
      projdataptr->endindex = offset2; 
      projdataptr->lastdrawn = projdataptr->point;
    }

    if (projdataptr->maxpoints > projdataptr->endindex) {
      /* go through all of the points in the new buffer and find the maximum 
       * on each channel */
      for (j = 0; j < NCHAN_PER_ELECTRODE; j++) {
        tmpmax[j] = MIN_DATAVAL;
      }
      // find the maximum on each channel for each this spike
      dataptr = data;
      for (samp = 0; samp < NPOINTS_PER_SPIKE; samp++) {
        maxptr = tmpmax;
        for (chan = 0; chan < NCHAN_PER_ELECTRODE; chan++) {
          if (*dataptr > *maxptr) {
            *maxptr = *dataptr;
          }
          dataptr++;
          maxptr++;
        }
      }
      /* update the peak amplitude list  */
      maxptr = tmpmax;
      for (chan = 0; chan < NCHAN_PER_ELECTRODE; chan++) {
        *(projdataptr->end) = MAX(*maxptr, 0);
        projdataptr->end++;
        maxptr++;
      }
      projdataptr->endindex += NCHAN_PER_ELECTRODE;
    }

    /* if this electrode is on screen, draw the spikes and the projection windows */
    if (onscreen) {
      if (dispinfo.fullscreenelect != -1) {
        PushTetrodeMatrix(2);
      }
      else {
        PushTetrodeMatrix(electnum);
      }
      spike = dispinfo.spikewaveform[electnum];
      glColor3f(1.0, 1.0, 1.0);
      glPointSize(1.0f);
      glBegin(GL_POINTS);
      /* draw the projection windows */
      while (projdataptr->lastdrawn < projdataptr->end) {
        orig = dispinfo.projwinorig;
        for (chan = 0; chan < NCHAN_PER_ELECTRODE; chan++) {
          for (chan2 = chan + 1; chan2 < NCHAN_PER_ELECTRODE; chan2++) {
            projpnt.x = *(projdataptr->lastdrawn+chan) * 
              dispinfo.projwinxscale[firstchan + chan];
            projpnt.y = *(projdataptr->lastdrawn+chan2) * 
              dispinfo.projwinyscale[firstchan + chan2];
            /* clip the points */
            if (projpnt.x > dispinfo.projwinxmax) {
              projpnt.x = dispinfo.projwinxmax;
            }
            if (projpnt.y > dispinfo.projwinymax) {
              projpnt.y = dispinfo.projwinymax;
            }
            glVertex2f(orig->x + projpnt.x, orig->y + projpnt.y);
            orig++;
          }
        }
        projdataptr->lastdrawn += NCHAN_PER_ELECTRODE;
      }
      glEnd();
      glFlush();
      /* now, if overlay is off, erase the previously drawn spike points */
      spikeptr = spike;
      if (!dispinfo.overlay[electnum]) {
        glPointSize(1.2f);
        glBegin(GL_POINTS);
        glColor3f(0.0, 0.0, 0.0);
        spikeptr = spike;
        for (samp = 0; samp < totalspikelen; samp++) {
          glVertex2f(spikeptr->x, spikeptr->y);
          spikeptr++;
        }                       
        glEnd();
        glFlush();
      }                   
      dataptr = data;
      spikeptr = spike;
      xinc = dispinfo.spikewinxinc;
      glPointSize(1.2f);
      glBegin(GL_POINTS);
      glColor3f(1.0, 1.0, 1.0);
      /* draw the spike */
      for (samp = 0; samp < NPOINTS_PER_SPIKE; samp++) {
        orig = dispinfo.spikewinorig;
        for (chan = 0; chan < NCHAN_PER_ELECTRODE; chan++) {
          /* save the x and y values so that we can erase them later */
          spikeptr->y = orig->y + (*(dataptr++)) * 
            dispinfo.spikewinyscale[firstchan + chan];
          spikeptr->x = orig->x + xinc;
          /* clip the y value */
          if (spikeptr->y < dispinfo.spikewinymin) {
            spikeptr->y = dispinfo.spikewinymin;
          }
          else if (spikeptr->y > dispinfo.spikewinymax) {
            spikeptr->y = dispinfo.spikewinymax;
          }
          glVertex2f(spikeptr->x, spikeptr->y);
          spikeptr++;
          orig++;
        }
        xinc += dispinfo.spikewinxinc;
      }
      glEnd();
      glFinish();
      /* return to the normal matrix */
      glPopMatrix();
    }
    sptr++;
  }
  return;
}

int ElectrodeOnScreen(int electind)
  /* check to see if a particular electrode index refers to an electrode on
   * the screen */
{
  if ((electind == dispinfo.fullscreenelect) || 
      ((dispinfo.fullscreenelect == -1) && (electind >= dispinfo.currentdispelect) && 
       (electind < dispinfo.currentdispelect + dispinfo.nelectperscreen))) {
    return 1;
  }
  return 0;
}


void DisplayContinuousData(ContBuffer *contbuf)
  /* draw all of the eeg traces in the buffer */
{
  int                         i, chan, samp;
  int                         nsamples;
  int                         nerase;
  int                         nchan;
  int                         dataskip;
  int                         
    short                       chanind[MAX_CHANNELS];
  DSPInfo                     *dptr;
  short                       *chindptr;
  short                       *data;
  short                       *dataptr;
  float                       *yscaleptr;
  float                       *xincptr;
  float                       *yminptr;
  float                       *ymaxptr;
  float                       *eegoffsetptr;
  Point                       *traceptr;
  int                         *eegpointnumptr;
  Point                       *origptr;
  Color                       **colorptr;

  /* set the total number of channels in this buffer and get the numbers for
   * each of the channels */
  dptr = sysinfo.dspinfo + contbuf->dspnum;
  nchan = dptr->nchan;
  nsamples = dptr->nsampout;

  /* get the and channel indeces which indicate where each trace should be
   * displayed */
  for (i = 0; i < nchan; i++) {
    chanind[i] = dispinfo.chanind[dptr->dspchan[i]];
  }

  data = contbuf->data;

  dataptr = data; 

  dataskip = dispinfo.eegskip * nchan;

  glBegin(GL_POINTS);
  /* we first need to check to see if we're at point 0 and the
   * eegtracestartime is set, in which case we need to erase points until we
   * get to the start time of the current data */
  eegpointnumptr = dispinfo.eegpointnum + *chanind;
  if ((*eegpointnumptr == 0) && (dispinfo.eegtracestarttime)) {
    nerase = round((float) contbuf->timestamp / SEC_TO_TSTAMP - 
        dispinfo.eegtracestarttime * dptr->samprate);
    for (samp = 0; samp < nerase; samp++) {
      /* reset the channel number pointer */
      chindptr = chanind;
      for (chan = 0; chan < nchan; chan++) {
        eegpointnumptr = dispinfo.eegpointnum + *chindptr;
        traceptr = dispinfo.eegtrace[*chindptr] + *eegpointnumptr;
        colorptr = dispinfo.eegcolor + *chindptr;
        origptr = dispinfo.eegtraceorig + *chindptr;
        yscaleptr = dispinfo.eegyscale + *chindptr;
        xincptr = dispinfo.eegxinc + *chindptr;
        yminptr = dispinfo.eegymin + *chindptr;
        ymaxptr = dispinfo.eegymax + *chindptr;
        eegoffsetptr = dispinfo.eegtraceoffset + *chindptr;
        /* erase the old point */
        glColor3f(0.0f, 0.0f, 0.0f);
        glVertex2f(traceptr->x, traceptr->y);
      }
      (*eegpointnumptr)++;
      /* increment the pointers */
      chindptr++;
    }
  }

  /* draw the new points */
  for (samp = 0; samp < nsamples; samp++) {
    /* reset the channel number pointer */
    chindptr = chanind;
    for (chan = 0; chan < nchan; chan++) {
      eegpointnumptr = dispinfo.eegpointnum + *chindptr;
      traceptr = dispinfo.eegtrace[*chindptr] + *eegpointnumptr;
      colorptr = dispinfo.eegcolor + *chindptr;
      origptr = dispinfo.eegtraceorig + *chindptr;
      yscaleptr = dispinfo.eegyscale + *chindptr;
      xincptr = dispinfo.eegxinc + *chindptr;
      yminptr = dispinfo.eegymin + *chindptr;
      ymaxptr = dispinfo.eegymax + *chindptr;
      eegoffsetptr = dispinfo.eegtraceoffset + *chindptr;
      /* first erase the old point */
      glColor3f(0.0f, 0.0f, 0.0f);
      glVertex2f(traceptr->x, traceptr->y);
      /* now draw the new point */
      glColor3f((*colorptr)->r, (*colorptr)->g, (*colorptr)->b);
      traceptr->x = origptr->x + *eegoffsetptr;
      traceptr->y = origptr->y + *dataptr * *yscaleptr * 
        dispinfo.contzoom;
      /* clip the y value to the window */
      if (traceptr->y > *ymaxptr) {
        traceptr->y = *ymaxptr;
      }
      else if (traceptr->y < *yminptr) {
        traceptr->y = *yminptr;
      }
      glVertex2f(traceptr->x, traceptr->y);
      /* increment the offset for this channel */
      *eegoffsetptr += *xincptr;
      /* check to see if we are at the edge of the window, and if so,
       * reset all of the eegxtraceoffsets */
      if (*eegoffsetptr >= EEG_WIN_WIDTH * .98) {
        ResetEEGTraces(0);
        /* we also need to skip the rest of the channels for this
         * sample and set the time for the first sample of this 
         * window */
        dataptr += nchan - chan;
        dispinfo.eegtracestarttime = contbuf->timestamp / SEC_TO_TSTAMP
          + (float) samp / (float) dptr->samprate;
        break;
      }
      (*eegpointnumptr)++;
      /* increment the pointers */
      chindptr++;
      dataptr++;
    }
    /* skip a few points so that we are not redrawing the same pixel */
    dataptr += dataskip;
    samp += dispinfo.eegskip;
  }
  glEnd();
  glFlush();
  return;
}


void DisplayPosImage(void)
{
  /* set the current raster position and display the image */
  glPixelZoom(dispinfo.poszoom.x, dispinfo.poszoom.y);
  glRasterPos2f(dispinfo.posloc.x, dispinfo.posloc.y);
  glDrawPixels(sysinfo.posimagesize[0], sysinfo.posimagesize[1], GL_RGB, 
      GL_UNSIGNED_BYTE, dispinfo.posimage);
  glFlush();

}


void InitializeDataBuffer(void)
  /* set up the circularly linked list of data buffers */
{
  int i;
  int tmpsize;
  struct DisplayData *tmpbuf1, *tmpbuf2;


  tmpsize = MAX_PACKET_SIZE;

  /* create the first data buffer in the linked list */
  databuf = (struct DisplayData *) malloc(sizeof(struct DisplayData));
  databuf->datatype = -1;
  tmpbuf1 = databuf;
  /* create the rest of the buffers */
  for (i = 1; i < NDISPLAY_DATA_BUFS; i++) {
    tmpbuf2 = (struct DisplayData *) malloc(sizeof(struct DisplayData));
    tmpbuf2->datatype = -1;
    tmpbuf1->next = tmpbuf2;
    tmpbuf1 = tmpbuf1->next;
  }
  /* link the next element to the first buffer */
  tmpbuf1->next = databuf;
  displaybuf = databuf;

  /* allocate space for the temporary buffer */
  tmpdatabuf = (short *) malloc(MAX_SPIKES_PER_BUF * sizeof(SpikeBuffer));

  /* if this is a position machine we need to create space for the position
   * buffer */
  if (sysinfo.datatype[sysinfo.machinenum] & POSITION) {
    dispinfo.posbuf = (PosBuffer *) malloc(sizeof(PosBuffer));
  }


  return; 
}




/* Master commands */

void StartAllAcquisition(void) 
{
  if (!sysinfo.acq) {
    ResetEEGTraces(1);
    usleep(100000);
    StartAcquisition();
    StartLocalDSPAcquisition();
  }
}

void StopAllAcquisition(void) 
{
  if (sysinfo.acq) {
    StopLocalDSPAcquisition();
    StopAcquisition();
  }
}

void ToggleAcq(void)
{
  if (sysinfo.acq) {
    StopAllAcquisition();
  }
  else {
    StartAllAcquisition();
  }
}

void StopLocalAcq(void)
{
  if (sysinfo.dspacq) {
    sysinfo.dspacq = 0;
    /* Stop acquisition on local DSPs and spike_daq modules only */
    StopLocalDSPAcquisition();
    StopModuleAcquisition(SPIKE_DAQ);
  }
}

void StartLocalAcq(void)
{
  if (!sysinfo.dspacq) {
    //      usleep(10000);
    sysinfo.dspacq = 1;
    /* Start acquisition on local DSPs and modules only */
    StartModuleAcquisition(SPIKE_DAQ);
    StartLocalDSPAcquisition();
  }
}

void MasterClearAll(void) 
{
  int i;
  /* send a message to the slaves to clear their screens */
  for(i = 0; i < netinfo.nslaves; i++) {
    SendMessage(netinfo.slavefd[i], CLEAR_SCREEN, NULL, 0);
    /* Wait for the reply */
    if (!WaitForMessage(netinfo.masterfd[i], SCREEN_CLEARED, 0.5)) {
      sprintf(tmpstring,"spike_main: Error clearing screen on slave %s\n", netinfo.slavename[i]);
      DisplayStatusMessage(tmpstring);
    }
  }
  ClearAll();
}


void Quit (int status)
{
  if (!sysinfo.fileopen) {
    StopLocalDSPAcquisition();
    StopAcquisition();
    spikeexit(status);
  }
  else {
    sprintf(tmpstring, "Error: cannot quit with files open");
    DisplayStatusMessage(tmpstring);
  }
}

void SetDepth(int electnum, int depth)
  /* set the depth of the selected electrode. */
{
  int i, j, updated = 0;

  /* find all of the channels with this electnum and set their depths */
  if (sysinfo.system_type[sysinfo.machinenum] == MASTER)  {
    for (i = 0; i < netinfo.nmachines; i++) {
      for (j = 0; j < sysinfo.nchannels[i]; j++) {
        if (sysinfo.channelinfo[i][j].number == electnum) {
          if (sysinfo.channelinfo[i][j].depth != depth) {
            sysinfo.channelinfo[i][j].depth = depth;
            updated = 1;
          }
        }
      }
    }
  }
  else {
    /* just update the local data */
    for (j = 0; j < sysinfo.nchannels[sysinfo.machinenum]; j++) {
      if (sysinfo.channelinfo[sysinfo.machinenum][j].number == electnum) {
        if (sysinfo.channelinfo[sysinfo.machinenum][j].depth != depth) {
          sysinfo.channelinfo[sysinfo.machinenum][j].depth = depth;
          updated = 1;
        }
      }
    }
  }
  if (updated) {
    spikeMainWindow->updateAllInfo();
  }
  return;
}

void SetReference(int electnum, int refelect, int refchan)
  /* set the reference of the selected electrode.  */
{
  int i, updated = 0, localacq = 0;
  ChannelInfo *ch;

  if (sysinfo.dspacq) {
    StopLocalAcq();
    localacq = 1;
  }
  /* find all of the channels with this electnum and set their reference */
  for (i = 0; i < sysinfo.nchannels[sysinfo.machinenum]; i++) {
    ch = sysinfo.channelinfo[sysinfo.machinenum] + i;
    if ((ch->number == electnum) && ((ch->refelect != refelect) || 
          (ch->refchan != refchan))) {
      /* update this channel */
      ch = sysinfo.channelinfo[sysinfo.machinenum] + i;
      ch->refelect = refelect;
      ch->refchan = refchan;
      /* Update the local DSP settings */
      UpdateChannel(ch, 1);
      updated = 1;
    }
  }
  if (localacq) {
    StartLocalAcq();
  }
  if (updated) {
    spikeMainWindow->updateAllInfo();
  }
  return;
}



bool MasterFSDataStart(void) 
{
  int i, error = 0;
  if (FSDataSelected()) {
    /* send a message to the slaves to start saving */
    for(i = 0; i < netinfo.nslaves; i++) {
      SendMessage(netinfo.slavefd[i], FS_DATA_START, NULL, 0);
    }
    if (!error) {
      /* if we started the save on all of the slaves, start it here as well */ 
      FSDataStart();
    }
    /* disable the settings menu item */
    //spikeMainWindow->fsDataSettingsAction->setEnabled(false);
    /* disable the acquisition toggle menu item */
    return true;
  }
  else {
      sysinfo.fsdataon = 0;
      DisplayErrorMessage("no feedback / stimulation data selected\n");
      return false;
  }
}


void MasterFSDataStop(void)
{
  int i, error = 0;
  if (sysinfo.fsdataon) {
    /* send a message to the slaves to stop saving */
    for(i = 0; i < netinfo.nslaves; i++) {
      SendMessage(netinfo.slavefd[i], FS_DATA_STOP, NULL, 0);
    }
    if (!error) {
      /* if we stopped the save on all of the slaves, stop it here as well */ 
      FSDataStop();
    }
    /* enable the acquisition toggle menu item */
    spikeMainWindow->masterAcqAction->setEnabled(true); 
  }
}

bool FSDataSelected(void)
{
    /* return 1 if any data are being sent */
    return (fsdatainfo.sendcont | fsdatainfo.sendspike | fsdatainfo.sendpos |
	    fsdatainfo.senddigio);
}


int AllowChanChange(int chanind)
{
  /* return 1 if we can change this channel and 0 otherwise */
  ChannelInfo *ch;

  if (sysinfo.allowsyncchanchange == 1) {
    return 1;
  }
  ch = sysinfo.channelinfo[sysinfo.machinenum] + chanind;
  if (ch->dspchan == DSP_POS_SYNC_CHAN) {
    return 0;
  }
  return 1;
}


void GLToWindow(Point glpoint, QRect &geom, int *windowx, int *windowy, float xscale, float yscale)
{
  /* convert the gl coordinates into the window reference frame */
  *windowx = round(((glpoint.x - X_MIN)  / (X_MAX - X_MIN)) * xscale *
      geom.width());
  *windowy = geom.height()- round(((glpoint.y - Y_MIN)  / 
        (Y_MAX - Y_MIN)) * yscale * geom.height());
}


void FormatTS(QString *s, u32 time)
{
  int t[3], msec;
  int i;
  t[0] = time/36000000L;
  t[1] = (time/600000L)%60;
  t[2] = (time/10000L)%60;
  msec = time%10000L;
  s->truncate(0);
  //s->append("Time: ");
  for (i = 0; i < 3; i++) {
    if (t[i] < 10) {
      s->append('0');
    }
    s->append(QString().setNum(t[i]));
    if (i < 2) {
      s->append(':');
    }
  }
  s->append('.');
  if (msec < 1000)
    s->append('0');
  if (msec < 100)
    s->append('0');
  if (msec < 10)
    s->append('0');
  s->append(QString().setNum(msec));
}

