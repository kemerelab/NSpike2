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

struct UserDataBuffer {
  char 	*data;
  char 	*dataptr;
  char 	*endptr;
  int		offset;
  bool	send;	// TRUE if we should send this buffer to UserData
  UserDataBufferInfo bufferinfo;
  struct UserDataBuffer *next;
};

void UpdateUserDataInfo(struct UserDataBuffer *buf);
void ClearUserDataInfo(struct UserDataBuffer *buf);
void userdataexit(int status);
void Usage(void);


int main(int argc, char **argv) 
{
  UserDataBuffer	*buf, *nextbuf, *bufptr, *sendbuf;
  // we save the buffer index each time we increment the size of the buffer
  int 		bufsize =  USER_DATA_BUFFER_SIZE;
  int			nbuffers = USER_DATA_NUM_BUFFERS;
  int			retrysec;
  int			socketsec;
  int			socketusec;
  char		savebuf[SAVE_BUF_SIZE];
  int			savebufsize;
  short		datatype;
  int			nspikes;

  int			maxfds;
  fd_set	   	readfds; 

  char 		tmpstring[100];	// a temporary buffer 
  char 		filename[100];	
  int         message;	// a temporary message variable
  int			messagedata[MAX_BUFFER_SIZE]; // message data can contain a sysinfo or channelinfo structure
  int			messagedatalen; // the length of the data in the message
  int 		i, id, j, nxtarg;

  struct timeval	tval, lasttval;
  struct timezone	tzone;
  int			fd;
  int 		datalen;
  int 		nwritten;
  int 		totalwritten;
  int 		writesize;

  SysInfo		*systmp;
  SpikeBuffer		*stmp;
  UserDataContBuffer	*mptr;

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
  nxtarg = 0;
  while (++nxtarg < argc) {
	if (strcmp(argv[nxtarg], "-bufsize") == 0) {
	  bufsize = atoi(argv[++nxtarg]);
    }
	else if (strcmp(argv[nxtarg], "-nbuffers") == 0) {
	  nbuffers = atoi(argv[++nxtarg]);
    }
    else {
	  fprintf(stderr, "spike_userdata: unknown command line option %s, ignoring\n", argv[nxtarg]);
	  Usage();
    }
  }
  /* check to make sure the buffer sizes are set reasonably */
  if ((bufsize < 100000) || (nbuffers < 3)) {
	Usage();
	fprintf(stderr, "Using default values\n");
  	bufsize =  USER_DATA_BUFFER_SIZE;
  	nbuffers = USER_DATA_NUM_BUFFERS;
  }


  sysinfo.acq = 0;
  sysinfo.userdataon = 0;

  if (StartNetworkMessaging(server_message, client_message, server_data, 
		client_data) < 0) {
    fprintf(STATUSFILE, "spike_userdata: Error starting network data messaging\n");
	userdataexit(1);
  }

  /* set the retry times and the timeouts for checking for an open socket */
  retrysec = 5;
  socketusec = 200000;
  socketsec = 0;

  /* set the type of program we are in for messaging */
  sysinfo.program_type = SPIKE_USER_DATA;

  /* allocate the circularly linked list of data buffers */
  buf = (struct UserDataBuffer *) malloc(sizeof(struct UserDataBuffer));
  buf->data = (char *) calloc(bufsize, sizeof(char));
  buf->dataptr = buf->data;
  buf->endptr = buf->data + bufsize - 1;
  buf->offset = 0;
  buf->send = 1;
  ClearUserDataInfo(buf);
  bufptr = buf;
  /* create the rest of the buffers */
  for (i = 1; i < nbuffers; i++) {
    nextbuf = (struct UserDataBuffer *) 
		    malloc(sizeof(struct UserDataBuffer));
    nextbuf->data = (char *) calloc(bufsize, sizeof(char));
    nextbuf->dataptr = nextbuf->data;
    nextbuf->endptr = nextbuf->data + bufsize - 1;
    nextbuf->offset = 0;
    nextbuf->send = 0;
    ClearUserDataInfo(nextbuf);
    bufptr->next = nextbuf;
    bufptr = bufptr->next;
  }
  /* link the next element to the first buffer */
  bufptr->next = buf;

  /* start at the first buffer */
  bufptr = sendbuf = buf;
  
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
	  /* check to see if adding the message on to the end of the
	   * data buffer would push it past it's maximum size */
	  if (bufptr->offset + savebufsize + sizeof(short) > 
	      bufsize) {
	    /* we are off the end, so move on to the next
	     * buffer in the linked list */
	    bufptr = bufptr->next;
	    /* check to see if this buffer is the same as the first
	     * buffer to send, and if so, increment the first
	     * buffer to send. This will erase a buffers worth of
	     * data */
	    if (bufptr == sendbuf) {
	      sendbuf = sendbuf->next;
	      StatusMessage("Warning: filled all userdata buffers",
		      client_message);
	    }
	    bufptr->send = 1;
	    bufptr->offset = 0;
	    bufptr->dataptr = bufptr->data;
	    UpdateUserDataInfo(bufptr);
	  }
	  switch(i) {
	    case SPIKE_DAQ:
	      if (message == SPIKE_DATA) {
		datatype = SPIKE_DATA_TYPE;
		/* get the number of each electrode and increment
		 * the number of spikes on them */
		nspikes = savebufsize / sizeof(SpikeBuffer);
		stmp = (SpikeBuffer *) savebuf;
		for (j = 0; j < nspikes; j++, stmp++) {
		   bufptr->bufferinfo.nspikes[stmp->electnum]++;
		}
	      }
	      else if (message == CONTINUOUS_DATA) {
		datatype = CONTINUOUS_DATA_TYPE;
		mptr = (UserDataContBuffer *) savebuf;
		for (j = 0; j < mptr->nchan; j++) {
		   bufptr->bufferinfo.ncontsamp[mptr->electnum[j]] += 
		     mptr->nsamp;
		}
	      }
	      break;
	    case SPIKE_POSDAQ:
	      datatype = POSITION_DATA_TYPE;
	      /* update the number of position records */
	      bufptr->bufferinfo.nposbuf++;
	      break;
	    case SPIKE_MAIN: 
	      datatype = DIGITALIO_DATA_TYPE;
	      bufptr->bufferinfo.ndigiobuf++;
	      u32ptr =  (u32 *) savebuf;
	      break;
	  }
	  /* put the datatype, the size of the data and the data 
	   * itself in the buffer */
	  memcpy(bufptr->dataptr, &datatype, sizeof(short));
	  bufptr->dataptr += sizeof(short);
	  /* write out the size of the data buffer */
	  memcpy(bufptr->dataptr, &savebufsize, sizeof(int));
	  bufptr->dataptr += sizeof(int);
	  /* write out the data */
	  memcpy(bufptr->dataptr, &savebuf, savebufsize);
	  bufptr->dataptr += savebufsize;
	  /* adjust the offset */
	  bufptr->offset += sizeof(short) + sizeof(int) + savebufsize;
	  bufptr->send = 1;
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
	     /* we now update the userdatainfo of the current buffer */
	     UpdateUserDataInfo(bufptr);
	     break;
	  case USER_DATA_START:
	     sysinfo.userdataon = 1;
	     break;
	  case USER_DATA_STOP:
	     sysinfo.userdataon = 0;
	     /* move on to the next buffer so that we can start
	     * fresh */
	     bufptr = bufptr->next;
	     /* check to see if this buffer is the same as the first
	     * buffer to send, and if so, increment the first
	     * buffer to send. This will erase a buffers worth of
	     * data */
	     if (bufptr == sendbuf) {
		sendbuf = sendbuf->next;
	     }
	     bufptr->send = 1;
	     bufptr->offset = 0;
	     bufptr->dataptr = bufptr->data;
	     UpdateUserDataInfo(bufptr);
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
    /* if the specified length of time has passed, try to open a client to
     * send data to userdata */
    gettimeofday(&tval, &tzone);
    if (tval.tv_sec - lasttval.tv_sec > retrysec) {
      /* close the socket if it was open */
      if (fd > 0) {
	    close(fd);
      }
      /* we should try to send a buffer */
      //fprintf(stderr, "spike_userdata: trying to open UserData socket\n");
      if ((fd = GetClientSocket(USER_DATA_SOCKET_NAME, socketsec, 
		  socketusec)) > 0) {
	/* we have sucessfully opened the socket, so we send the
	 * data from the current buffer through and then close the
	 * socket */
	fprintf(stderr, "spike_userdata: opened UserData socket, sending data\n");
	/* we first get the information on what sort of data is in the
	 * buffers and the total number of each type of buffer and send
	 * that out first */
	while (bufptr->send) {
	  /* we first send the buffer information */
	  if ((nwritten = write(fd, &bufptr->bufferinfo, 
		  sizeof(UserDataBufferInfo))) != 
		  sizeof(UserDataBufferInfo)) {
	    fprintf(STATUSFILE, "Error: unable to write buffer information to UserData\n");
	  }
	  fprintf(stderr, "wrote UserDataBufferInfo, datalen to write = %d\n", bufptr->offset);

	  /* now write out the buffer */
	  datalen = bufptr->offset;
	  /* reset the dataptr */
	  bufptr->dataptr = bufptr->data;
	  nwritten = 0;
	  totalwritten = 0;
	  /* send the data*/
	  while (datalen > 0) {
	    writesize = datalen > MAX_SOCKET_WRITE_SIZE ? 
			    MAX_SOCKET_WRITE_SIZE : datalen;
	    nwritten = write(fd, bufptr->dataptr, writesize);
	    bufptr->dataptr += nwritten;
	    if (nwritten == -1) {
	       fprintf(STATUSFILE, "Error: unable to write complete buffer to UserData\n");
	       break;
	    }
	    else {
	      datalen -= nwritten;
	    } 
	  }
	  /* reset the buffer */
	  bufptr->send = 0;
	  bufptr->offset = 0;
	  bufptr->dataptr = bufptr->data;
	  ClearUserDataInfo(bufptr);
	  /* move on to the next buffer */
	  bufptr = bufptr->next;
	  UpdateUserDataInfo(bufptr);
	}
	fprintf(stderr, "spike_userdata: done writing data \n");
	/* we have sent all of the data, so we reset the timer */
	lasttval.tv_sec = tval.tv_sec;
      }
      else {
	/* we can't send data, so we reset the timer */
	lasttval.tv_sec = tval.tv_sec;
      }
    }
  }
  return 0;
}

void ClearUserDataInfo(struct UserDataBuffer *buf)
{
  UserDataBufferInfo *iptr;
  int i;

  /* set all the bytes in the structure to 0 to clear it out */
  memset(&buf->bufferinfo, 0, sizeof(UserDataBufferInfo));

  /* set the starttime to be very large so that we can set it by looking for
   * a smaller time */
  buf->bufferinfo.conttimes[0] = UINT_MAX;
  return;
}

void UpdateUserDataInfo(struct UserDataBuffer *buf)
  /* combine the current userdata information with that already stored to
   * produce a complete list of the types of data and channels in the current
   * buffer */
{
  UserDataBufferInfo *iptr;
  int i;

  iptr = &buf->bufferinfo;

  /* copy the DSPInfo from sysinfo */
  iptr->userdatainfo.sendcont |= userdatainfo.sendcont;
  iptr->userdatainfo.sendspike |= userdatainfo.sendspike;
  iptr->userdatainfo.sendpos |= userdatainfo.sendpos;
  iptr->userdatainfo.senddigio |= userdatainfo.senddigio;

  for (i = 0; i < MAX_ELECTRODE_NUMBER; i++) {
	iptr->userdatainfo.contelect[i] |= userdatainfo.contelect[i];
	if (iptr->userdatainfo.contelect[i] && 
		(i > iptr->maxconttetnum)) {
	  iptr->maxconttetnum = i;
	}
	iptr->userdatainfo.spikeelect[i] |= userdatainfo.spikeelect[i]; 
	if (iptr->userdatainfo.spikeelect[i] && 
		(i > iptr->maxspiketetnum)) {
	  iptr->maxspiketetnum = i;
	}
  }
  return;
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
