/*
 * spike_posdaq: Interface code for reading buffers from the bt878 frame
 * grabber board  
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


#include "spikecommon.h"
#include <assert.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>    /* for videodev2.h */
#include <linux/videodev2.h>
#define CLEAR(x) memset (&(x), 0, sizeof (x))


#define VIDEO_DEV_NAME   "/dev/video"
#include "spike_V4Lcapture.h"

struct buffer {
  void *      data;
  size_t      length;
};

static int      	video_fd      = -1;
struct buffer *   	video_buffer   = NULL;
static unsigned int   nvideo_buffers     = 0;
unsigned char 		*outbuf; // image capture dest. buffer
unsigned char 		*yuvbuf;  // image converted to YUV422P
unsigned char 		*outbufptr; // a pointer to the output buffer
int			imagesize;



void posdaqexit(int status);
int GetRatPos(unsigned char *buffer, u32 *ratx, u32 *raty, short *trackedx, short *trackedy, int imagesize);

/* the global system information structure */
SysInfo 		sysinfo;

NetworkInfo		netinfo;
UserDataInfo		userdatainfo;
SocketInfo 		server_message[MAX_CONNECTIONS]; // the structure for receiving messages
SocketInfo 		server_data[MAX_CONNECTIONS]; // the structure for receiving messages
SocketInfo 		client_message[MAX_CONNECTIONS]; // the structure for sending messages
SocketInfo 		client_data[MAX_CONNECTIONS]; // the structure for sending data;
struct timeval	starttime;  // the time at which the clock was reset

int main() 
{
  struct v4l2_buffer	v4l2buf;  // the current buffer from the card
  struct buffer 	*vbufptr; // the pointer to the current video buffer
  
  u32 	nbufsread;   // the number of position buffers read in
  SysInfo		*systmp;
  int         message;
  int			messagedata[MAX_BUFFER_SIZE]; // message data can contain a sysinfo struct
  int			messagedatalen; // the length of the data in the message
  int			userdatafd; // the file descriptor for the userdata program
  u32			bufferinfo[3];	/* information about the buffer to be 
					 *  sent
					 * the bufferinfo array contains the 
					 * 1) the ulong timestamp 
					 * 2) the estimated x position of the
					 * animal
					 * 3) the estimated y position of the
					 * animal */
  struct timezone	tz;  
  u32	starttimestamp; // the beginning timestamp for the current buffer for each card
  u32	timestamp, tdiff;
  long int time_sec, time_usec;
  u32	bufferduration;
  u32	dmasamp;	// the size of each DMA transfer 
  int			setfirsttimestamp; 	// 1 for each card if the next timestamp should be saved as the timestamp 
                    // for the beginning of the next buffer

  int			bufferinfosize;


  fd_set		readfds;  // the set of readable fifo file descriptors 
  struct timeval	timeout;
  int			maxfds;

  char		tmpstring[100];
  char		filename[200];
  int 		done;
  int			error;
  int 		i, j, k, id;
  int			tmpint;

  /* tracked pixels buffer for userdata output */
  short		*userposdata;
  short		*trackedx;
  short		*trackedy;
  u32			ntracked;

#ifdef NO_VIDEO_DEBUG
  int			currentx, currenty, currenttraj;
  Point		arm0, arm1, arm2;
#endif

  float 		tmp1, tmp2;
  u32	lasttimestamp, nexttimestamp;
	int 		setfaketime = 0;

  sysinfo.acq = 0;
  sysinfo.userdataon = 0;


  /* set the type of program we are in for messaging */
  sysinfo.program_type = SPIKE_POSDAQ;

  sysinfo.statusfile == NULL;

#ifdef NO_VIDEO_DEBUG
  fprintf(stderr, "spike_posdaq: NO_VIDEO_DEBUG defined\n");
#else
  fprintf(stderr, "spike_posdaq: NO_VIDEO_DEBUG not defined\n");
#endif

  if (STATUSFILE == NULL) {
    /* open up the status file if it is not stderr*/
    gethostname(tmpstring,80);
    sprintf(filename, "spike_posdaq_status_%s", tmpstring);
    if ((STATUSFILE = fopen(filename, "w")) == NULL) {
      fprintf(stderr, "spike_main: error opening status file\n");
      exit(-1);
    }
  }

  fprintf(STATUSFILE, "spike_posdaq: starting messaging\n");
  if (StartNetworkMessaging(server_message, client_message, server_data, 
		client_data) < 0) {
    fprintf(STATUSFILE, "spike_posdaq: Error starting network data messaging\n");
	posdaqexit(1);
  }

  bufferinfosize = 3 * sizeof(u32);

  done = 0;

  sysinfo.acq = 0;
  sysinfo.process = 1;

  nbufsread = 0;

  setfirsttimestamp = 1;
  /* set up the initial list of file descriptors to be watched */
  tmpint = 0;
  lasttimestamp = nexttimestamp = 0; 
	timestamp = 0;

  while (1) {
    error = 0;
    SetupFDList(&readfds, &maxfds, server_message, server_data);
    /* check for incoming data or a message */
#ifndef NO_VIDEO_DEBUG
    select(maxfds, &readfds, NULL, NULL, NULL);
    j = 0;
    /* clear out any readable data on the video */
    while ((id = netinfo.datainfd[j]) != -1) {
      /*fprintf(stderr, "spike_posdaq: checking data %d, fd %d\n", id, 
	       server_data[id].fd); */
      /* check for data from the frame grabber card */
      if (FD_ISSET(server_data[id].fd, &readfds) && 
	(server_data[id].fd == video_fd) && sysinfo.acq) {
	/* read in the message */
	read_frame(outbuf, &time_sec, &time_usec);
	/* calculate the time, in timestamp units since the starttime*/
	*bufferinfo = (time_sec - starttime.tv_sec) *
		SEC_TO_TSTAMP + (time_usec - starttime.tv_usec) 
		/ 100;;
      
	YUY422to422P(outbuf, yuvbuf, yuvbuf+imagesize, yuvbuf+imagesize+(imagesize/2));        
	/* check the timestamp */
	tdiff = *bufferinfo - lasttimestamp;
	if ((lasttimestamp != 0) && (tdiff >= MAX_POS_TIMESTAMP_DIFF)) {
	  fprintf(stderr, "Position timestamp error: last = %u, current = %u, diff = %u\n", lasttimestamp, *bufferinfo, tdiff);
	}
	lasttimestamp = *bufferinfo;
	error = 0;

	/* calculate the center of the tracked pixels */
	ntracked = GetRatPos(yuvbuf, bufferinfo+1, bufferinfo+2, 
			    trackedx, trackedy, imagesize);

	if (sysinfo.acq) {
	  /* send the data out */
	  /* if we are sending only the position of the center of
	   * mass out, send it on to spike_userdata and spike_main */
	  if (!sysinfo.sendalltrackedpixels) {
	    if (sysinfo.userdataon) {
	      if (SendMessage(userdatafd, POS_DATA, (char *) bufferinfo, 
		      bufferinfosize) == -1) {
		    sprintf(tmpstring, "spike_posdaq: Error sending data from spike_posdaq to spike_userdata: Resetting");
		    fprintf(STATUSFILE, "%s\n", tmpstring);
		    ErrorMessage(tmpstring, client_message);
		    break;
	      }
	    }
	    if (SendMessage(client_data[SPIKE_MAIN].fd, 
		    POS_DATA, (char *) bufferinfo, 
		    bufferinfosize) == -1) {
	      sprintf(tmpstring, "spike_posdaq: Error sending data from spike_posdaq to spike_main: Resetting");
	      fprintf(STATUSFILE, "%s\n", tmpstring);
	      ErrorMessage(tmpstring, client_message);
	      break;
	    }
	  } 
	  else {
	    /* create the userdata data array 
	     * put the timestamp in the beginning of the userposdata
	     * array */
	    memcpy(userposdata, bufferinfo, sizeof(u32));
	    memcpy(userposdata + 2, &ntracked, sizeof(u32));
	    /* we need to create a single data buffer, so we copy
	     * the y tracked pixel coordinates to the end of the x
	     * array */
	    memcpy(trackedx + ntracked, trackedy, ntracked * 
		    sizeof(short));
	    if (sysinfo.userdataon) {
	      if (SendMessage(userdatafd, POS_DATA,
			(char *) userposdata, 2 * 
			(ntracked + 2) * sizeof(short)) == -1) {
		sprintf(tmpstring, "spike_posdaq: Error sending data from spike_posdaq to spike_userdata: Resetting");
		fprintf(STATUSFILE, "%s\n", tmpstring);
		ErrorMessage(tmpstring, client_message);
		break;
	      }
	    }
	    if (SendMessage(client_data[SPIKE_MAIN].fd, POS_DATA, 
		      (char *) userposdata, 2 * 
		      (ntracked + 2) * sizeof(short)) == -1) {
	      sprintf(tmpstring, "spike_posdaq: Error sending data from spike_posdaq to spike_main: Resetting");
	      fprintf(STATUSFILE, "%s\n", tmpstring);
	      ErrorMessage(tmpstring, client_message);
	    }
	  }
	  /* send the initial message to the processing module */
	  if (SendMessage(client_data + SPIKE_PROCESS_POSDATA, 
			POS_DATA, (char *) bufferinfo, 
			bufferinfosize) == -1) {
	     error = 1;
	  } 
	  /* send the data */
	  if (SendMessage(client_data + SPIKE_PROCESS_POSDATA, 
			POS_DATA, (char *) yuvbuf, 
			imagesize * BYTES_PER_PIXEL) == -1) {
		error = 1;
	  } 
	  if (error) {
	    /* the only reason we would get an error is if the buffer is
	     * full, so we need to send an error message so that we can
	     * restart */
	    sprintf(tmpstring, "spike_posdaq: Error sending data from spike_posdaq to spike_process_posdata: Resetting");
	    fprintf(STATUSFILE, "%s\n", tmpstring);
	    ErrorMessage(tmpstring, client_message);
	    sysinfo.acq = 0;
	    break;
	  }
	  //fprintf(STATUSFILE, "spike_posdaq: sent data out\n");
	}
      }
      j++;
    }
#else
    /* set for 30Hz sampling */
    timeout.tv_sec = 0;
    timeout.tv_usec = 33333;
    //timeout.tv_usec =  0;
    select(maxfds+1, &readfds, NULL, NULL, &timeout);
    if (sysinfo.acq) {
      outbufptr = yuvbuf;
      /* send out a buffer every 33.3 ms */
      timestamp += 333;
      bufferinfo[0] = timestamp;
      //for (i = 0; i < 320*240; i++) {
      //  yuvbuf[i] += 2;
      //  yuvbuf[i] %= 190;
      //}
      for (i = 0; i < 320*240; i++, outbufptr++) {
	*outbufptr = 100;
      }
      /* simulate the animal running in a triangle between goal locations */
      for (i = -5; i < 5; i++) {
	    yuvbuf[currentx + i + currenty * sysinfo.posimagesize[0]] = 127;
      }
      for (i = -5; i < 5; i++) {
	    yuvbuf[currentx + (currenty + i) * sysinfo.posimagesize[0]] = 127;
      }
      switch(currenttraj) {
      case 0:
	    currentx--;
	    currenty++;
	    if (currentx == arm1.x) {
	      currenttraj = 1;
	    }
	    break;
      case 1:
	    currentx++;
	    currenty--;
	    if (currentx == arm0.x) {
	      currenttraj = 2;
	    }
	    break;
      case 2:
	    currentx++;
	    currenty++;
	    if (currentx == arm2.x) {
	      currenttraj = 3;
	    }
	    break;
      case 3:
	    currentx--;
	    currenty--;
	    if (currentx == arm0.x) {
	      currenttraj = 0;
	    }
	    break;
      }
      /* now put a plus  at the rat's position */
      for (i = -5; i < 5; i++) {
	    yuvbuf[currentx + i + currenty * sysinfo.posimagesize[0]] = 255;
      }
      for (i = -5; i < 5; i++) {
	    yuvbuf[currentx + (currenty + i) * sysinfo.posimagesize[0]] = 255;
      }
      ntracked = GetRatPos(yuvbuf, bufferinfo+1, bufferinfo+2, trackedx, 
	      trackedy, imagesize);
      //fprintf(stderr, "in spike_posdaq: rat pos: %ld %ld\n", bufferinfo[1], bufferinfo[2]);
      j = 0;
      //fprintf(stderr, "sending buffer, time %d, size %d, ntracked = %d\n", bufferinfo[0], imagesize, j);
      /* first send the position information to spike_main */
      if (!sysinfo.sendalltrackedpixels) {
	    if (client_data[SPIKE_MAIN].fd) {
	      if (SendMessage(client_data[SPIKE_MAIN].fd, 
		      POS_DATA, (char *) bufferinfo, 
		      bufferinfosize) == -1) {
		    sprintf(tmpstring, "spike_posdaq: Error sending data from spike_posdaq to spike_main: Resetting");
		    fprintf(STATUSFILE, "%s\n", tmpstring);
		    ErrorMessage(tmpstring, client_message);
		    break;
	      }
	    } 
      } 
      else {
	    /* create the userdata data array to send to the main
	     * program */
	    /* put the timestamp in the beginning of the userposdata
	     * array */
	    memcpy(userposdata, bufferinfo, sizeof(u32));
	    memcpy(userposdata + 2, &ntracked, sizeof(u32));
	    /* we need to create a single data buffer, so we copy
	     * the y tracked pixel coordinates to the end of the x
	     * array */
	    memcpy(trackedx + ntracked, trackedy, ntracked * 
		    sizeof(short));
	    if (SendMessage(client_data[SPIKE_MAIN].fd, POS_DATA, 
		      (char *) userposdata, 2 * 
		      (ntracked + 2) * sizeof(short)) == -1) {
	      error = 1;
	    }
	    if (error) {
	      fprintf(STATUSFILE, "spike_posdaq: ERROR sending data to spike_main\n");
	    }
      }
      if (SendMessage(client_data + SPIKE_PROCESS_POSDATA, POS_DATA, 
		    (char *) bufferinfo, bufferinfosize) == -1) {
	    error = 1;
      }
      /* send the data */
      if (SendMessage(client_data + SPIKE_PROCESS_POSDATA, POS_DATA, 
		      (char *) yuvbuf, imagesize * BYTES_PER_PIXEL) == -1) {
	    error = 1;
      }
      if (error) {
	    fprintf(STATUSFILE, "spike_posdaq: ERROR sending data to spike_process_posdata\n");
      }

      /* if userdata saving is on, put the data in a buffer and send it 
       * to spike_userdata */
      if (sysinfo.userdataon) {
	    /* put the timestamp in the beginning of the userposdata
	     * array */
	    memcpy(userposdata, bufferinfo, sizeof(u32));
	    memcpy(userposdata + 2, &ntracked, sizeof(u32));
	    /* we need to create a single data buffer, so we copy
	     * the y tracked pixel coordinates to the end of the x
	     * array */
	    memcpy(trackedx + ntracked, trackedy, ntracked * 
		    sizeof(short));
	    if (SendMessage(userdatafd, POS_DATA, 
		      (char *) userposdata, 2 * (ntracked + 2) * 
		      sizeof(short)) == -1) {
	      error = 1;
	    }
	    if (error) {
	      fprintf(STATUSFILE, "spike_posdaq: ERROR sending data to spike_userdata\n");
	    }
      }
    }
#endif
    j = 0;
    while ((id = netinfo.messageinfd[j]) != -1) {
      /* process a message if there is one */
      if (FD_ISSET(server_message[id].fd, &readfds)) {
	    message = GetMessage(server_message[id].fd, 
		  (char *) messagedata, &messagedatalen, 0);
       switch(message) {
	  case STOP_ACQUISITION:
	    sysinfo.acq = 0;
	    /* send out a STOP_ACQUISITION message on the module input fifos */
#ifndef NO_VIDEO_DEBUG
	    stop_capturing();
#endif
	    SendMessage(client_message[SPIKE_MAIN].fd, ACQUISITION_STOPPED, NULL, 0);

	    break;
	  case START_ACQUISITION:
	    sysinfo.acq = 1;
#ifndef NO_VIDEO_DEBUG
	    /* start the capture process and get the system time */
	    start_capturing();
#endif
	    SendMessage(client_message[SPIKE_MAIN].fd, ACQUISITION_STARTED, NULL, 0);
	    lasttimestamp = 0;
	    break;
	  case START_TIME:
	    memcpy(&starttime, messagedata, sizeof(struct timeval));
	    break;
	  case DATA_TYPE:
	    break;
	  case SYSTEM_INFO:
	    /* read in the type of data, the buffersamp, the number of 
	     * channels and the sampling rate. This message can only occur 
	     * when acqusition is stopped */
	    systmp = (SysInfo *) messagedata;
	    sysinfo.machinenum = systmp->machinenum;
	    sysinfo.posinputnum = systmp->posinputnum;
	    sysinfo.posimagesize[0] = systmp->posimagesize[0];
	    sysinfo.posimagesize[1] = systmp->posimagesize[1];
	    /* Also read in whether we are tracking dark pixels, 
	    * and what the tracking luminance threshold is */
	    sysinfo.trackdarkpixels = systmp->trackdarkpixels;
	    sysinfo.sendalltrackedpixels = 
	    systmp->sendalltrackedpixels;
	    sysinfo.posthresh = systmp->posthresh;
	    /* currently the image size is fixed. It would be nice to fix 
	     * this in a future version */
	    imagesize = sysinfo.posimagesize[0] * sysinfo.posimagesize[1] * sizeof(char);
	    /* set the last element of buffer_info to be this size */
	    /* compute the expected time between buffers */
	    bufferduration = (u32) ((1.0 / FRAME_RATE) * SEC_TO_TSTAMP); 
	    /* allocate space for the output buffers */
	    outbuf = (unsigned char *) malloc(imagesize * BYTES_PER_PIXEL);
	    yuvbuf = (unsigned char *) malloc(imagesize * BYTES_PER_PIXEL);
#ifdef NO_VIDEO_DEBUG
	    for (i = 0 ; i < imagesize; i++) {
	      //yuvbuf[i] = (i / sysinfo.posimagesize[0]);
	      yuvbuf[i] = 30;
	    }
	      
	    arm0.x = 160;
	    arm0.y = 80;
	    arm1.x = 80;
	    arm1.y = 80;
	    arm2.x = 240;
	    arm2.y = 80;
	    currentx = (int) arm0.x;
	    currenty = (int) arm0.y;
	    currenttraj = 0;
#else 
	    /* initialize the video device */
	    open_device (VIDEO_DEV_NAME, &video_fd);
	    init_device ( 0, // input index
	    sysinfo.posimagesize[0],  // width
	    sysinfo.posimagesize[1] );  // height 
	    /* add the file descriptor for the camera */
	    AddFD(video_fd, server_data, netinfo.datainfd);
#endif
	    /* allocate space for the tracked pixels that could be
	     * sent to userdata including two elements for the timestamp. */
	    userposdata = (short *) calloc(2*imagesize+4, sizeof(short));
	    /* userposdata's first four elements are the timestamp
	     * and the number of pixels */
	    trackedx = userposdata + 4;
	    trackedy = trackedx + imagesize;
	    break;
	  case POSITION_INFO:
	    sysinfo.posthresh = (unsigned char) messagedata[0];
	    break;
	  case USER_DATA_INFO:
	    /* get the userdatainfo structure */
	    memcpy(messagedata, (char *) &userdatainfo, sizeof(UserDataInfo));
	    /* we also need to find the userdata file descriptor */
	    k = 0;
	    while ((id = netinfo.dataoutfd[k++]) != -1) {
	        if (client_data[id].toid == SPIKE_USER_DATA) {
                    userdatafd = client_data[id].fd;
		    break;
		}
	    }
	    break;
	  case USER_DATA_START:
	    sysinfo.userdataon = 1;
	    break;
	  case USER_DATA_STOP:
	     sysinfo.userdataon = 0;
	     break;
	  case EXIT:
	    posdaqexit(0);		    
	    break;
	  default:
	    break;
	}
      }
      j++;
    }
  }
  return 0;
}

int GetRatPos(unsigned char *buffer, u32 *ratx, u32 *raty, short *trackedx, short *trackedy, int imagesize)
  /* go through the buffer and calculate the center of mass of the tracked 
   * pixels. Return the number of tracked pixels and their x and y coordinates
   * in trackedx and trackedy*/
{
  int i, n;
  unsigned char *bufptr;
  short	*xptr, *yptr;
  
  bufptr = buffer;
  *ratx = *raty = 0;
  xptr = trackedx;
  yptr = trackedy;
  n = 0;
  //fprintf(stderr, "sysinfo.trackdarkpixels set to %d\n", sysinfo.trackdarkpixels);
  /* get the list of tracked pixels */
  if (sysinfo.trackdarkpixels) { // are we tracking dark pixels?...
    for (i = 0; i < imagesize; i++, bufptr++) {
      if (*bufptr <= sysinfo.posthresh) {
        n++;
        /* collect up the x and y locations of the pixels to
         * determine the animal's current location */
        *xptr = i % sysinfo.posimagesize[0];
        *yptr = i / sysinfo.posimagesize[0];
        // *ratx += *(xptr++);
        *ratx += *xptr;
        *raty += *yptr;
      }
    }
  } else { // ...or are we tracking bright pixels?
    for (i = 0; i < imagesize; i++, bufptr++) {
      if (*bufptr >= sysinfo.posthresh) {
        n++;
        /* collect up the x and y locations of the pixels to
         * determine the animal's current location */
        *xptr = i % sysinfo.posimagesize[0];
        *yptr = i / sysinfo.posimagesize[0];
        *ratx += *xptr;
        *raty += *yptr;
      }
    }
  }
    
  if (n) {
    *ratx /= n;
    *raty /= n;
  }
  return n;
}



void posdaqexit(int status)
{
   int i;
   SendMessage(client_message[SPIKE_MAIN].fd, EXITING, NULL, 0);
   // sleep so that all of the other programs have a chance to get the message
   sleep(1);
#ifndef NO_VIDEO_DEBUG
   uninit_device();
   close_device();
#endif

   CloseSockets(server_message);
   CloseSockets(client_message);
   CloseSockets(client_data);
   CloseSockets(server_data);
   fclose(STATUSFILE);
   exit(status);
}

