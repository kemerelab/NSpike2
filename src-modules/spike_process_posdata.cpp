/* 
* spike_process_posdata: Program for processing frames from position
* monitoring camera.
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
#include "spike_ffmpegEncoder.h"


void processposexit(int status);


int error;
int		bufferinfosize;	// the size of the buffer info array
char		tmpstring[200];


/* the global system information structure */
SysInfo 		sysinfo;

NetworkInfo		netinfo;
SocketInfo 		server_message[MAX_CONNECTIONS]; // the structure for receiving messages
SocketInfo 		client_message[MAX_CONNECTIONS]; // the structure for sending messages 
SocketInfo 		server_data[MAX_CONNECTIONS]; // the structure for receiving data
SocketInfo 		client_data[MAX_CONNECTIONS]; // the structure for sending data 


int main() {
  char		filename[200];
  u32 		nbufsread;     // the number of buffers read in
  u32 		bufferinfo[3];   // the information from the spike_posdaq buffer
  unsigned char	*inbufptr;  	// the input buffer pointer
  PosBuffer		posbuf;		// the current position buffer for the display program
  PosMPEGBuffer	posmpegbuf;		// the current position buffer for the ssave program
  int			*trackedpixptr;
  SysInfo		*systmp;
  int                 message;
  int			messagedata[MAX_BUFFER_SIZE]; // message data can contain a sysinfo or channelinfo structure
  int			messagedatalen; // the length of the data in the message
  int			inbufsize;    // the size of the input data buffer in bytes
  int			inbufpixels;     // the number of pixels in the input buffer;
  int			imagesize;	// the number of pixels in the image
  int			ratpos[2];  // temporary variables for computing the estimate of the animal's position
  int 		i, j, id, i2, id2;
  fd_set  readfds;  // the set of readable file descriptors
  int			maxfds = 0;

  int fd_isset;

  int			size = SAVE_BUF_SIZE;
  int 		length;

  // ffmpeg encoder variables
  int              iCodec = MPEG1_CODEC;  
  int              iGopSize; 
  int              iQual = 3;  // 1 to 31: 1=best, 31=terrible
  
  unsigned char   *YUYPbuf; // input buffer for the encoder
  unsigned char   *pYbuf; // Y plane pointer in the input buffer
  unsigned char   *pUbuf; // U plane pointer in the input buffer
  unsigned char   *pVbuf; // V plane pointer in the input buffer

  int              iWidth, iHeight;
  int              iEncInBufSize;

  int              iEncodeBufSize;
  unsigned char   *pEncodeBuf;
  int              iEncodedSize; 

  
  sysinfo.acq = 0;
  sysinfo.display = 1;
  sysinfo.diskon = 0;
  sysinfo.fileopen = 0;

  /* set the type of program we are in for messaging */
  sysinfo.program_type = SPIKE_PROCESS_POSDATA;

  sysinfo.statusfile == NULL;

  if (STATUSFILE == NULL) {
    /* open up the status file if it is not stderr*/
    gethostname(tmpstring,80);
    sprintf(filename, "spike_process_posdata_status_%s", tmpstring);
    if ((STATUSFILE = fopen(filename, "w")) == NULL) {
      fprintf(stderr, "spike_process_posdata: error opening status file\n");
      exit(-1);
    }
  }

  fprintf(STATUSFILE, "spike_process_posdata: starting messaging\n");
  if (StartNetworkMessaging(server_message, client_message, server_data, client_data) < 0) {
    fprintf(STATUSFILE, "spike_process_posdaq: Error starting network data messaging\n");
    processposexit(1);
  }


  while (1) { 
    SetupFDList(&readfds, &maxfds, server_message, server_data);
    select(maxfds, &readfds, NULL, NULL, NULL);
    j = 0;
    while ((id = netinfo.datainfd[j]) != -1) {
      /* At the moment data are received only from SPIKE_POSDAQ */
      if (FD_ISSET(server_data[id].fd, &readfds)) {
        /* get a data buffer from the spike_posdaq module */
        GetMessage(server_data[id].fd, (char *) bufferinfo, &messagedatalen, 1); 
        /* get the data  */
        GetMessage(server_data[id].fd, (char *) posbuf.image, &inbufsize, 1); 
        if (sysinfo.acq) {
          /* get the list of tracked pixels */
          inbufptr = posbuf.image;
          trackedpixptr = posbuf.trackedpixels;
          posbuf.ntracked = 0;
          for (i = 0; i < imagesize; i++) {
            if (sysinfo.trackdarkpixels) { // if we are tracking dark pixels
              if (*inbufptr <= sysinfo.posthresh) {
                *(trackedpixptr++) = i;
                posbuf.ntracked++;
              }
            } 
            else { // otherwise assume we are tracking bright pixels
              if (*inbufptr >= sysinfo.posthresh) {
                *(trackedpixptr++) = i;
                posbuf.ntracked++;
              }
            }
            inbufptr++;
          }

          /* set the animal's location. The incoming bufferinfo has
           * the location in bufferinfo[1] and bufferinfo[2] */
          ratpos[0] = bufferinfo[1];
          ratpos[1] = bufferinfo[2];
          posbuf.ratpos = ratpos[0] + sysinfo.posimagesize[0] * 
          ratpos[1];

          posbuf.timestamp = bufferinfo[0];
          /* if the disk is on, we need to encode the current frame */
          if (sysinfo.diskon) {
            posmpegbuf.timestamp = bufferinfo[0];
            // copy image to the encoder input buffer
            memcpy(YUYPbuf, posbuf.image, iEncInBufSize);
            // ffmpeg encode the frame
            posmpegbuf.size = encode_frame(); // returns ecdoded size
            if (SendPosMPEGBuffer(client_data[SPIKE_SAVE_DATA].fd, 
                                   &posmpegbuf) == -1) {
              sprintf(tmpstring, "spike_process_posdata: unable to send buffer info to spike_save_data.");
              fprintf(STATUSFILE, "%s\n", tmpstring);
              ErrorMessage(tmpstring, client_message);
            } 
          }

          /* Now send the data to the target programs. Skip the save
          * program, as we have already sent the mpeg encoded data
          * there */
          i2 = 0;
          while ((id2 = netinfo.dataoutfd[i2]) != -1) {
            if ((id2 != SPIKE_SAVE_DATA) && (SendPosBuffer(client_data[id2].fd, &posbuf) == -1)) {
              sprintf(tmpstring,"spike_process_posdata: unable to send buffer info to %d.",id2);
              fprintf(STATUSFILE, "%s\n", tmpstring);
              ErrorMessage(tmpstring, client_message);
            } 
            i2++;
          }
        }
      }
      j++;
    }
    /* check to see if we have have data to save, and if so
          process a message if there is one */
    i = 0;
    while ((id = netinfo.messageinfd[i]) != -1) {
      if (fd_isset = FD_ISSET(server_message[id].fd, &readfds)) {
        message = GetMessage(server_message[id].fd, (char *)messagedata, &messagedatalen, 0);
        //fprintf(STATUSFILE, "spike_process_posdata: message %d from %d [%d]\n", message, id, server_message[id].fd);
        switch(message) {
        case STOP_ACQUISITION:
          sysinfo.acq = 0;
          SendMessage(client_message[SPIKE_MAIN].fd, ACQUISITION_STOPPED, NULL, 0);
          break;
        case START_ACQUISITION:
          sysinfo.acq = 1;
          SendMessage(client_message[SPIKE_MAIN].fd, ACQUISITION_STARTED, NULL, 0);
          break;
        case STOP_PROCESS:
          sysinfo.process = 0;
          /* we need to clear out the data input buffer */
          //ClearData(server_message[SPIKE_POSDAQ].fd);
          SendMessage(client_message[SPIKE_MAIN].fd, SYSTEM_READY, NULL, 0);
          break;
        case START_PROCESS:
          sysinfo.process = 1;
          break;
        case STOP_SAVE:
          sysinfo.diskon = 0;
          SendMessage(client_message[SPIKE_MAIN].fd, SAVE_STOPPED, NULL, 0);
          break;
        case START_SAVE:
          if (sysinfo.fileopen) {
            sysinfo.diskon = 1;
            SendMessage(client_message[SPIKE_MAIN].fd, SAVE_STARTED, NULL, 0);
          }
          break;
        case STOP_DISPLAY:
          sysinfo.display = 0;
          break;
        case START_DISPLAY:
          sysinfo.display = 1;
          break;
        case OPEN_FILE:
          sysinfo.fileopen = 1;
          // Init encoder 
          break;
        case CLOSE_FILE:
          if (sysinfo.fileopen) {
            // ffmpeg finish encoding delayed frames. 
            // Not needed for MJPEG, but does not hurt, return size will be 0
            while( posmpegbuf.size = encode_delayed_frames() ) { // returns ecdoded size
              // send last data to save
              if (SendPosMPEGBuffer(client_data[SPIKE_SAVE_DATA].fd, &posmpegbuf) == -1) {
                sprintf(tmpstring,"spike_process_posdata: unable to send image to spike_save_data.");
                fprintf(STATUSFILE, "%s\n", tmpstring);
                ErrorMessage(tmpstring, client_message);
              } 
            }

            close_encoder();  // ffmpeg  close encoder
            free(YUYPbuf);
            sysinfo.fileopen = 0;
            
            /* send a confirmation message to the main
             * program */
            SendMessage(client_message[SPIKE_MAIN].fd, FILE_CLOSED, NULL, 0);
          }
          break;
        case POSITION_INFO:
          sysinfo.posthresh = (unsigned char) messagedata[0];
          break;
        case SYSTEM_INFO:
          /* read in the type of data, the buffersamp, the number of tetrodes
          * and the sampling rate. This message can only occur when acqusition
          * is stopped */
          systmp = (SysInfo *) messagedata;
          sysinfo.machinenum = systmp->machinenum;
          sysinfo.posimagesize[0] = systmp->posimagesize[0];
          sysinfo.posimagesize[1] = systmp->posimagesize[1];
          imagesize = sysinfo.posimagesize[0] * sysinfo.posimagesize[1];
          sysinfo.trackdarkpixels = systmp->trackdarkpixels;
          sysinfo.posthresh = systmp->posthresh;
          sysinfo.mpegquality = systmp->mpegquality;
          sysinfo.mpegslices = systmp->mpegslices;
          sysinfo.videocodec = systmp->videocodec;
          sysinfo.videogopsize = systmp->videogopsize;
          
          //  allocate buffers for ffmpeg encoder
          iWidth=sysinfo.posimagesize[0]; 
          iHeight=sysinfo.posimagesize[1];
          iEncInBufSize = iWidth * iHeight * BYTES_PER_PIXEL * sizeof(unsigned char);
          YUYPbuf = (unsigned char *) malloc(iEncInBufSize);

          pYbuf = YUYPbuf;
          pUbuf = pYbuf + (iWidth * iHeight);
          pVbuf = pUbuf + (iWidth * iHeight/2);

          // encoding  buffer info
          pEncodeBuf = posmpegbuf.frame;
          iEncodeBufSize = sizeof(posmpegbuf.frame);

          iCodec = sysinfo.videocodec;
          // map quality from 100-1 to 1-31
          iQual = ( -sysinfo.mpegquality * 30 + 3099)/99; 
          iGopSize = sysinfo.videogopsize;
          //  ffmpeg open, set encoding parameters
          // Note: if open_encoder() fails, exit(1) is called 
          open_encoder( iCodec, iWidth, iHeight, iQual, iGopSize, 
                        pYbuf, pUbuf, pVbuf,
                        pEncodeBuf, iEncodeBufSize);
          break;
        case EXIT:
          processposexit(0);        
          break;
        default:
          break;
        }
      }
      i++;
    }
  }
  return 0;
}

void processposexit(int status)
{
  int i;

  fprintf(stderr, "Exiting\n");

  SendMessage(client_message[SPIKE_MAIN].fd, EXITING, NULL, 0);
  /* sleep so that all of the other programs have a chance to get the message*/
  sleep(1);
  CloseSockets(server_message);
  CloseSockets(client_message);
  CloseSockets(server_data);
  CloseSockets(client_data);
  fclose(STATUSFILE);
  exit(status);
}
