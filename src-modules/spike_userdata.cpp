/*
 * spike_userdata.cpp: Program for storing data from acquisition modules for
 * tranmission to UserData.  
 *
 * Eventually it will be possible to run the userdata interface in two ways, but
 * currently only the second option below is supported:
 *
 * 1.  As a module on a separate (non-data acquisition) computer. In this case 
 * the user must start spike2_userdata which will start this program. The
 * computer this runs on must have USERDATA as its datatype
 *
 * 2.  As a module on the master or on a slave machine.  In this case the
 * program will be launched automatically on the system it is supposed to run 
 * on. In this case the datatype line in the main configuration file must have
 * USERDATA as one of the datatypes for the machine on which this program is to
 * run. No other machine should have userdata in the datatype line
 *
 * The program stores the incoming data in a cicularly
 * linked list of data buffers and sends out data whenever it can open a socket
 * to the UserData mex file that connects to it.  Note that if more data come in
 * than can be stored in the list of buffers, the oldest data is erased 
 *
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

#include <spikecommon.h>
/* function definitions that allow us to use spike_dsp_shared.h */
void DisplayErrorMessage(char *);
void DisplayStatusMessage(char *);
void StartLocalAcq(void) { return; };
void StopLocalAcq(void) { return; };

char tmpstring[200];



/* Global variables */
SysInfo 		sysinfo;
NetworkInfo		netinfo;
UserDataInfo		userdatainfo;
DSPInfo			*dptr;
DigIOInfo		digioinfo;

SocketInfo 		server_message[MAX_CONNECTIONS]; // the structure for the server messaging
SocketInfo 		client_message[MAX_CONNECTIONS]; // the structure for the client messaging
SocketInfo 		server_data[MAX_CONNECTIONS]; // the structure for receiving data
SocketInfo 		client_data[MAX_CONNECTIONS]; // the structure for sendin data (currently unused)

#include "spike_dsp_shared.h"
#include "../src-main/spike_dsp_shared.cpp"

void userdataexit(int status);
void Usage(void);


int main(int argc, char **argv) 
{
  char		savebuf[SAVE_BUF_SIZE];
  int		savebufsize;
  short		datatype;
  int		nspikes;

  int		maxfds;
  fd_set	readfds; 

  char 		tmpstring[100];	// a temporary buffer 
  char 		filename[100];	
  int         	message;	// a temporary message variable
  int		messagedata[MAX_BUFFER_SIZE]; // message data can contain a sysinfo or channelinfo structure
  int		messagedatalen; // the length of the data in the message
  int 		i, id, j, nxtarg;

  struct timeval	tval, lasttval;
  struct timezone	tzone;
  int			fd;
  int 		datalen;
  int 		nwritten;
  int 		totalwritten;
  int 		writesize;

  SysInfo		*systmp;
  SpikeBuffer		*sptr;
  UserDataContBuffer	*cptr;

  u32			*u32ptr;


  sysinfo.program_type = SPIKE_USER_DATA;

  fprintf(STATUSFILE, "spike_userdata: starting\n");

  sysinfo.statusfile == NULL;
  if (STATUSFILE == NULL) {
	/* open up the status file if it is not stderr*/
	gethostname(tmpstring,80);
	sprintf(filename, "spike_userdata_status_%s", tmpstring);
	if ((STATUSFILE = fopen(filename, "w")) == NULL) {
	  fprintf(stderr, "spike_userdata: error opening status file\n");
	  exit(-1);
	}
  }

  /* process the arguments */
  sysinfo.acq = 0;
  sysinfo.userdataon = 0;

  if (StartNetworkMessaging(server_message, client_message, server_data, 
		client_data) < 0) {
    fprintf(STATUSFILE, "spike_userdata: Error starting network data messaging\n");
	userdataexit(1);
  }

  /* get the current time */
  gettimeofday(&lasttval, &tzone);
  
  while (1) { 
    /* set up the initial list of file descriptors to watch */
    SetupFDList(&readfds, &maxfds, server_message, server_data);
    select(maxfds, &readfds, NULL, NULL, NULL);
    id = 0;
    /* check for incoming data. Note that we save all of the incoming data
     * to the data buffer, as the individual programs that send us data
     * have already filtered it */
    while ((i = netinfo.datainfd[id]) != -1) {
      if (FD_ISSET(server_data[i].fd, &readfds)) {
	/* check to see if the disk is on, and if not, get the message
	 * and discard it. Note that this works for everyone except
	 * spike_process_posdata, as it only sends data out when the
	 * disk is on.  */
	message = GetMessage(server_data[i].fd, savebuf, &savebufsize, 
		1);
	if (sysinfo.userdataon) {
	  switch(i) {
	    case SPIKE_DAQ:
	      if (message == SPIKE_DATA) {
		datatype = SPIKE_DATA_TYPE;
		/* get the number of each electrode and increment
		 * the number of spikes on them */
		nspikes = savebufsize / sizeof(SpikeBuffer);
		sptr = (SpikeBuffer *) savebuf;
	      }
	      else if (message == CONTINUOUS_DATA) {
		datatype = CONTINUOUS_DATA_TYPE;
		cptr = (UserDataContBuffer *) savebuf;
	      }
	      break;
	    case SPIKE_POSDAQ:
	      datatype = POSITION_DATA_TYPE;
	      break;
	    case SPIKE_MAIN: 
	      datatype = DIGITALIO_DATA_TYPE;
	      break;
	  }
	}
      }
      id++;
    }
    id = 0;
    /* check for messages */
    while ((i = netinfo.messageinfd[id]) != -1) {
      if (FD_ISSET(server_message[i].fd, &readfds)) {
	 message = GetMessage(server_message[i].fd,
				(char *) messagedata, &messagedatalen, 0);
	switch(message) {
	  case STOP_ACQUISITION:
	     sysinfo.acq = 0;
	     SendMessage(client_message[SPIKE_MAIN].fd, ACQUISITION_STOPPED,
		 NULL, 0);
	     break;
	  case START_ACQUISITION:
	     sysinfo.acq = 1;
	     SendMessage(client_message[SPIKE_MAIN].fd, ACQUISITION_STARTED, 			     NULL, 0);
	     break;
	  case USER_DATA_INFO:
	     /* get the userdatainfo structure */
	     memcpy((char *) &userdatainfo, messagedata,
		    sizeof(UserDataInfo));
	     break;
	  case USER_DATA_START:
	     sysinfo.userdataon = 1;
	     break;
	  case USER_DATA_STOP:
	     sysinfo.userdataon = 0;
	     /* move on to the next buffer so that we can start
	     * fresh */
	     break;
	  case SYSTEM_INFO:
	     systmp = (SysInfo *) messagedata;
	     /* copy the DSPInfo structure */
	     memcpy(sysinfo.dspinfo, systmp->dspinfo, 
		    sizeof(DSPInfo) * MAX_DSPS);
	     dptr = sysinfo.dspinfo;
	     break;
	  case DIGIO_INFO:
	     /* copy the DSPInfo structure */
	     memcpy(messagedata, &digioinfo, sizeof(DigIOInfo));
	     break;
	  case EXIT:
	     userdataexit(0);		    
	     break;
	  default:
	     break;
	}
      }
      id++;
    }
  }
  return 0;
}

void userdataexit(int status)
{
   SendMessage(client_message[SPIKE_MAIN].fd, EXITING, NULL, 0);
   /* sleep so that all of the other programs have a chance to get the message*/
   sleep(1);
   CloseSockets(server_message);
   CloseSockets(client_message);
   CloseSockets(server_data);
   fclose(STATUSFILE);
   exit(status);
}

void Usage(void) 
{
  fprintf(stderr, "Usage: spike_userdata -config configfile -netconfig networkconfigfile [-bufsize #] [-nbuffers #]\n\bufsize must be > 100000\n\t nbufs must be >= 3\n");
}

void DisplayErrorMessage(char *message)
{
  fprintf(stderr, "spike_userdata error: %s\n", message);
}

void DisplayStatusMessage(char *message)
{
  fprintf(stderr, "spike_userdata status: %s\n", message);
}