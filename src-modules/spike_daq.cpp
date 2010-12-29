/* 
 * spike_daq.cpp: Interface code for getting data from one or more DSP modules
 * 
 * This code reads the packets from the DSP modules.
 * In spike mode we process the data for spikes and send them out.
 * In continuous mode we relay the data
 *
 * Data are sent to the spike_save_data (for saving to disk) 
 * and nspike (for display)
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
#include "spike_dio.h"
#include "spike_stimcontrol_defines.h"

void daqexit(int status);
void getspike(short *data);
int getcont(ContBuffer *contbuf);
int MakeFSDataBuf(ContBuffer *cptr, FSDataContBuffer *udptr);

int tempnum = 0;

/* the global system information structure */
SysInfo         sysinfo;

NetworkInfo    	netinfo;
FSDataInfo    fsdatainfo; // data that will be sent to spike_fsdata for real time processing if this is enabled
SocketInfo     	server_message[MAX_CONNECTIONS]; // the structure for receiving messages
SocketInfo     	client_message[MAX_CONNECTIONS]; // the structure for sending messages
SocketInfo     	client_data[MAX_CONNECTIONS]; // the structure for sending data;
SocketInfo     	server_data[MAX_CONNECTIONS]; // the structure for sending data;

/* the data buffer for temporary storage of spike data from each dsp for spike
 * processing*/
typedef struct _DataBuffer {
  u32     timestamp; 
  short   data[SAVE_BUF_SIZE];
  short   *endptr;
  int     nsampperchan;
  int     npoints;    
  int     settimestamp;
  int     startindex[MAX_ELECT_PER_DSP];
} DataBuffer;

DataBuffer databuf[MAX_DSPS];

void ExtractSpikes(DataBuffer *databuf, int dspnum);

int	      fsdatafd; // the file descriptor for the user data 

int main() 
{

  /* data buffers */
  ContBuffer     *cptr;
  ContBuffer     contbuf[MAX_DSPS];
  u32        	timestamp;
  short        	**contbufdataptr;
  int        	contbufsize[MAX_DSPS];
  int        	nsamp[MAX_DSPS];
  int        	datainc[MAX_DSPS];
  FSDataContBuffer   fscontdata;
  DataBuffer   *databufptr;
  DIOBuffer    diobuf;
  DSPInfo      *dptr;

  SysInfo      *systmp;
  EventBuffer  event;
  int        	message;
  int        	messagedatalen;
  unsigned short   *usptr;
  char         	messagedata[MAX_BUFFER_SIZE];
  int        	dspnum;
  int        	size;
  short        	tmpsize;

  int         nspikepoints = 0;  // gets reset when the system info is received
  fd_set      readfds;  // the set of readable fifo file descriptors 
  struct timeval  timeout;
  int         maxfds;

  char        tmpstring[100];
  char        filename[200];
  int         done;
  int         error;
  int         i, j, k, id, id2;
  int         msize;

  ChannelInfo    *chnew, *ch;
  u32        ts[MAX_DSPS];
  int        np = 0;
  int        npvalid = 0, npbadtime = 0;
  u32        firsttime = 0;
  u32        lasttime = 0;
  u32        tsinc[MAX_DSPS];
  int        nskipped;
  short      dspnulldata[100000];


  sysinfo.acq = 0;
  sysinfo.fsdataon = 0;

  /* set the type of program we are in for messaging */
  sysinfo.program_type = SPIKE_DAQ;

  sysinfo.statusfile = NULL;

  if (STATUSFILE == NULL) {
    /* open up the status file if it is not stderr*/
    gethostname(tmpstring,80);
    sprintf(filename, "spike_daq_status_%s", tmpstring);
    if ((STATUSFILE = fopen(filename, "w")) == NULL) {
      fprintf(stderr, "spike_main: error opening status file\n");
      exit(-1);
    }
  }
  fprintf(STATUSFILE, "spike_daq: starting\n");

  fprintf(STATUSFILE, "spike_daq: starting messaging\n");
  if (StartNetworkMessaging(server_message, client_message, server_data, 
        client_data) < 0) {
    fprintf(STATUSFILE, "spike_daq: Error starting network data messaging\n");
    daqexit(1);
  }
  
  memset(dspnulldata, 0, 10000);

  done = 0;
  error = 0;

  sysinfo.acq = 0;
  sysinfo.fsdataon = 0;

  sysinfo.daq_to_user.is_enabled = 0;

  timeout.tv_sec = 0;

  for (i = 0; i < MAX_DSPS; i++) {
    contbuf[i].timestamp = 0;
  }


  while (1) {
#ifdef NO_DSP_DEBUG
    SetupFDList(&readfds, &maxfds, server_message, server_data);
    if (sysinfo.acq) {
      /* Send out packets from the first DSP with data */
      timeout.tv_sec = 0; 
      timeout.tv_usec = (__suseconds_t) ((float) sysinfo.dspinfo[1].nsampout / ((float) sysinfo.dspinfo[1].samprate) * 1e6);
      select(maxfds+1, &readfds, NULL, NULL, &timeout);
      /* we  have to send out a different format depending on the data type */
      if (timeout.tv_usec == 0) {
        for (j = 1; j < sysinfo.ndsps; j++) {
          if ((contbuf[j].dspnum > 0) && 
            (sysinfo.dspinfo[j].nchan > 0) &&
            (sysinfo.dspinfo[j].machinenum == sysinfo.machinenum)) {
            getcont(contbuf+j);
            if (sysinfo.datatype[sysinfo.machinenum] & SPIKE) {
              //fprintf(stderr, "timestamp = %u\n", contbuf.timestamp);
              /* copy the data to the databuffer  */
              dspnum = contbuf[j].dspnum;
              dptr = sysinfo.dspinfo + dspnum;
              databufptr = databuf + dspnum;
              if (databufptr->settimestamp) {
                databufptr->timestamp = contbuf[j].timestamp;
                databufptr->settimestamp = 0;
              }

              tmpsize = dptr->nsampout * dptr->nchan;
              memcpy(databufptr->endptr, contbuf[j].data, 
                  tmpsize * sizeof(short));
              databufptr->endptr += tmpsize;
              databufptr->nsampperchan += dptr->nsampout;
              databufptr->npoints += tmpsize;
              if ((databufptr->nsampperchan) > nspikepoints) {
                ExtractSpikes(databufptr, dspnum);
                databufptr->settimestamp = 1;
              }
            }
            else if (sysinfo.datatype[sysinfo.machinenum] & CONTINUOUS){
              i = 0;
	      if (sysinfo.fsdataon) {
		msize = MakeFSDataBuf(contbuf + j, &fscontdata);
		if ((msize > 0) && SendMessage(fsdatafd, 
		      CONTINUOUS_DATA, (char *) &fscontdata, msize)==-1) {
		   error = 1;
		} 
	      }
              while ((id = netinfo.dataoutfd[i++]) != -1) {
                if (client_data[id].toid != SPIKE_FS_DATA) {
                  if (SendMessage(client_data[id].fd, 
                       CONTINUOUS_DATA, (char *) 
                       (contbuf + j), contbufsize[j])==-1) {
                    error = 1;
                  } 
                }
              }
            }
          }
        }
      }
    }
#else
    /* check for data or a message */
    SetupFDList(&readfds, &maxfds, server_message, server_data);
    select(maxfds, &readfds, NULL, NULL, NULL);
    i = 0;
    while ((id = netinfo.datainfd[i]) != -1) {
      if (FD_ISSET(server_data[id].fd, &readfds)) {
        size = read(server_data[id].fd, messagedata, MAX_BUFFER_SIZE);
        if (sysinfo.acq) {
          np++;
        }
        else {
          break;
        }
        if (size == -1) break;
        npvalid++;
        usptr = (unsigned short *) messagedata;
        

        /* parse the read data */
        ByteSwap(usptr, 1);
        dspnum = ((int) usptr[0]);
        dptr = sysinfo.dspinfo + dspnum;

        /* read in the data */
        usptr++;

        /* move to the timestamp field */
        ByteSwap(usptr, dptr->packetshorts-1);

        memcpy(&timestamp, usptr, 2 * sizeof(unsigned short));
        /* check to see if this is from the master dsp, in which case
         * we put the data into a diobuf and send it out to
         * spike_save_data */
        if (dspnum == 0) {
          /* put the data into a diobuffer */
          diobuf.timestamp = timestamp / SAMP_TO_TIMESTAMP ;
          for (i = 0; i < MAX_DIO_PORTS; i++) {
            diobuf.status[i] = usptr[i+2];
          }
	  if (sysinfo.fsdataon) {
	     SendMessage(fsdatafd, DIGITALIO_EVENT, 
		     	(char *) &diobuf, DIO_BUF_STATIC_SIZE);
	  }
          /* send the digital IO event on to be saved */
          SendMessage(client_data[SPIKE_SAVE_DATA].fd, 
              DIGITALIO_EVENT, (char *) &diobuf, 
              DIO_BUF_STATIC_SIZE);
          /* send the digital IO event on to the main program*/
          SendMessage(client_data[SPIKE_MAIN].fd, 
              DIGITALIO_EVENT, (char *) &diobuf, 
              DIO_BUF_STATIC_SIZE);
#ifdef DIO_DEBUG    
          fprintf(stderr, "dio packet at time %u, size %d, status:\n",
			  diobuf.timestamp, size);
          fprintf(stderr, "%04x", diobuf.status[0]);
          for (i = 1; i < MAX_DIO_PORTS; i++) {
            fprintf(stderr, " : %04x", diobuf.status[i]);
          }
          fprintf(stderr,"\n");

#endif
        }
        else {
          if (firsttime == 0) {
              firsttime = timestamp;
          }
          lasttime = timestamp;
          /* check for / notify about lost packets... */
          if ((ts[dspnum] > 0) && (timestamp - ts[dspnum] > 
                tsinc[dspnum] + 2)) {
            nskipped = (timestamp - ts[dspnum]) / tsinc[dspnum] - 1;
            npbadtime+=nskipped;
            event.timestamp = timestamp + tsinc[dspnum];
            event.type = EVENT_PACKET_LOSS;
            sprintf(event.descript, "%d %d (packets lost, dspnum)", 
                nskipped, dspnum);
            SendMessage(client_data[SPIKE_SAVE_DATA].fd, EVENT, 
                (char *) &event, sizeof(EventBuffer));
            sprintf(tmpstring, "lost packets: %d of %d, %f %%\n", 
                npbadtime, np, (float) npbadtime / (float) np 
                * 100.0);
            StatusMessage(tmpstring, client_message);
            if (nskipped > 100) {
              /* This is a catastrophic loss, so we reset the data
               * pointer to the beginning of the buffer and carry
               * on.  */
              if (sysinfo.datatype[sysinfo.machinenum] & SPIKE) {
                databufptr = databuf + dspnum;
                databufptr->settimestamp = 1;
                /* zero out the beginning of the databuffer */
                memset(databufptr->data, 0, databuf->npoints * 
                    sizeof(short));
                /* reset the end pointer */
                databufptr->endptr = databufptr->data + 
                    databufptr->npoints;
              }
              else {
                cptr = contbuf + dspnum;
                /* reset the data pointer to the beginning of 
                 * the buffer */
                contbufdataptr[dspnum] = cptr->data;
              } 
            }
            /* Fill in the missing buffers with 0 */
            else {
              for (j = 0; j < nskipped; j++) {
                ts[dspnum] += (j+ 1) *
                  (dptr->nsamp * 30000 / dptr->samprate);
                ts[dspnum] /= SAMP_TO_TIMESTAMP;
                if (sysinfo.datatype[sysinfo.machinenum] & 
                    SPIKE) {
                  /* copy the data into the dsp data buffer */
                  databufptr = databuf + dspnum;
                  if (databufptr->settimestamp) {
                    databufptr->timestamp = ts[dspnum];
                    databufptr->settimestamp = 0;
                  }
                  tmpsize = dptr->nsamp * dptr->nchan; 
                  memcpy(databufptr->endptr, dspnulldata, 
                      dptr->datasize);
                  databufptr->endptr += dptr->ntotalsamp;
                  databufptr->nsampperchan += dptr->nsamp;
                  databufptr->npoints += tmpsize;
                  /* if we have > nspikepoints points per channel, process the
                   * data buffer for spikes. If you change this make sure
                   * that the number of spikes in a buffer cannot exceed
                   * MAX_SPIKES_PER_BUF */
                  if ((databufptr->nsampperchan) > nspikepoints) {
                    ExtractSpikes(databufptr, dspnum);
                    databufptr->settimestamp = 1;
                  }
                }
                else {
                  cptr = contbuf + dspnum;
                  if (contbufdataptr[dspnum] == cptr->data) {
                    /* if we're at the beginning of the buffer we
                     * need to set the timestamp */
                    cptr->timestamp = ts[dspnum];
                  }
                  /* append the data to the correspondign contbuf */
                  memcpy(contbufdataptr[dspnum], dspnulldata, 
                      dptr->datasize);
                  nsamp[dspnum] += dptr->nsamp;
                  contbufdataptr[dspnum] += datainc[dspnum];
                  if (nsamp[dspnum] >= dptr->nsampout) {
                    /* send the data on */
                    j = 0;
                    while ((id2 = netinfo.dataoutfd[j++]) != -1) {
                      if (client_data[id].toid != SPIKE_FS_DATA) {
                        if (SendMessage(client_data[id2].fd, 
                              CONTINUOUS_DATA, (char *) cptr,
                              contbufsize[dspnum])== -1) {
                          error = 1;
                        } 
                      }
                      else if (sysinfo.fsdataon) {
                        msize = MakeFSDataBuf(cptr, &fscontdata);
                        if (SendMessage(client_data[id2].fd, 
                              CONTINUOUS_DATA, (char *) 
                              &fscontdata,
                              msize)== -1) {
                          error = 1;
                        } 
                      }
                    }
                    if (error) {
                      fprintf(STATUSFILE, "Error sending data from spike_daq\n");
                    }
                    /* reset the continuous buffer and the data
                     * pointer */
                    contbufdataptr[dspnum] = cptr->data;
                    nsamp[dspnum] = 0;
                  }
                }
              }
            }
          }  
/* timestamp / packet errors have been corrected */
          ts[dspnum] = timestamp;
          /* adjust the timestamp to be in 100 usec units */
          timestamp /= SAMP_TO_TIMESTAMP;

          /* move to the data */
          usptr += 2;
          //fprintf(stderr, "timestamp = %u\n", timestamp);
          /* check to see if we should process this data for
           * spikes (which machine are we running on?) */
          /* Doesn't this mean that a machine processes either
           * SPIKE or other stuff, but not both? Doesn't that
           * defeat the purpose of using a mask? (BUG?) */
          if (sysinfo.datatype[sysinfo.machinenum] & SPIKE) {
            /* copy the data into the dsp data buffer */
            databufptr = databuf + dspnum;
            if (databufptr->settimestamp) {
              databufptr->timestamp = timestamp;
              databufptr->settimestamp = 0;
            }
            tmpsize = dptr->nsamp * dptr->nchan; 
            memcpy(databufptr->endptr, usptr, dptr->datasize);
            databufptr->endptr += dptr->ntotalsamp;
            databufptr->nsampperchan += dptr->nsamp;
            databufptr->npoints += tmpsize;
            /* if we have > nspikepoints points per channel, process the
             * data buffer for spikes. If you change this make sure
             * that the number of spikes in a buffer cannot exceed
             * MAX_SPIKES_PER_BUF */
            if ((databufptr->nsampperchan) > nspikepoints) {
              ExtractSpikes(databufptr, dspnum);
              databufptr->settimestamp = 1;
            }
          }
          else {
            cptr = contbuf + dspnum;
            if (contbufdataptr[dspnum] == cptr->data) {
              /* if we're at the beginning of the buffer we
               * need to set the timestamp */
              cptr->timestamp = timestamp;
            }
            /* append the data to the corresponding contbuf */
            memcpy(contbufdataptr[dspnum], usptr, dptr->datasize);
            nsamp[dspnum] += dptr->nsamp;
            contbufdataptr[dspnum] += datainc[dspnum];
            /* if we've collect the appropriate number of
             * samples we send the data out */
            if (nsamp[dspnum] >= dptr->nsampout) {
              /* send the data on */
              j = 0;
	      if (sysinfo.fsdataon) {
		msize = MakeFSDataBuf(cptr, &fscontdata);
		if (SendMessage(client_data[SPIKE_FS_DATA].fd, 
		      CONTINUOUS_DATA, (char *) 
		      &fscontdata,
		      msize)== -1) {
		  error = 1;
		} 
	      }
              while ((id2 = netinfo.dataoutfd[j++]) != -1) {
                if (client_data[id2].toid != SPIKE_FS_DATA) {
                  if (SendMessage(client_data[id2].fd, 
                        CONTINUOUS_DATA, (char *) cptr,
                        contbufsize[dspnum])== -1) {
                    error = 1;
                  } 
                }
              }
              if (error) {
                fprintf(STATUSFILE, "Error sending data from spike_daq\n");
              }
              /* reset the continuous buffer and the data
               * pointer */
              contbufdataptr[dspnum] = cptr->data;
              nsamp[dspnum] = 0;
            }
          }
        }
      }
      i++;
    }
#endif
    i = 0;
    while ((id = netinfo.messageinfd[i++]) != -1) {
      /* check for messages */
      if (FD_ISSET(server_message[id].fd, &readfds)) {
        message = GetMessage(server_message[id].fd, 
            (char *) messagedata, &messagedatalen, 0);
        switch(message) {
          case STOP_ACQUISITION:
            sysinfo.acq = 0;
            SendMessage(client_message[SPIKE_MAIN].fd, 
                  ACQUISITION_STOPPED, NULL, 0);
            break;
          case START_ACQUISITION:
            sysinfo.acq = 1;
            /* initialize the spike data buffers */
            for (j = 0; j < MAX_DSPS; j++) {
              if (sysinfo.dspinfo[j].nchan) {
                databufptr = databuf+j;
                databufptr->settimestamp = 1;
                databufptr->nsampperchan = NPOINTS_PER_SPIKE;
                databufptr->npoints = NPOINTS_PER_SPIKE * 
                  sysinfo.dspinfo[j].nchan;
                /* start one spike in to the buffer so that spike
                 * extraction works correctly */
                databufptr->endptr = databufptr->data + 
                    databufptr->npoints;
                 /* set the initial start index.  This starts us at
                  * the first sample where we could go back
                  * NPRE_THRESH_POINTS and still be within the
                  * buffer*/
                for (k = 0; k < sysinfo.dspinfo[j].nelectrodes; 
                    k++) {
                  databufptr->startindex[k]  = 
                    NPRE_THRESH_POINTS * 
                    sysinfo.dspinfo[j].nchan;
                }
                contbufdataptr[j] = contbuf[j].data;
                ts[j] = 0;
              }
            }
#ifndef NO_DSP_DEBUG
            /* clear out the data from the DSPs */
            for (j = 0; j < MAX_DSPS; j++) {
              if (sysinfo.dspinfo[j].nchan) {
                ClearData(server_data[j].fd);
              }
            }
#endif  
            SendMessage(client_message[SPIKE_MAIN].fd, 
                  ACQUISITION_STARTED, NULL, 0);
            break;
          case DATA_TYPE:
            sysinfo.datatype[sysinfo.machinenum] = messagedata[0];
            /* initialize the spike data buffers */
            for (j = 0; j < MAX_DSPS; j++) {
              if (sysinfo.dspinfo[j].nchan) {
                databufptr = databuf+j;
                databufptr->settimestamp = 1;
                databufptr->nsampperchan = NPOINTS_PER_SPIKE;
                databufptr->npoints = NPOINTS_PER_SPIKE * 
                  sysinfo.dspinfo[j].nchan;
                /* start one spike in to the buffer so that 
                 * spike extraction works correctly */
                databufptr->endptr = databufptr->data + 
                    databufptr->npoints;
                 /* set the initial start index.  This starts 
                  * us at * the first sample where we could go 
                  * back NPRE_THRESH_POINTS and still be 
                  * within the buffer*/
                for (k = 0; k < sysinfo.dspinfo[k].nelectrodes; 
                    k++) {
                  databufptr->startindex[k]  = 
                    NPRE_THRESH_POINTS * 
                    sysinfo.dspinfo[j].nchan;
                }
                contbufdataptr[i] = contbuf[i].data;
              }
            }
            SendMessage(client_message[SPIKE_MAIN].fd, 
                  SYSTEM_READY, NULL, 0);
            break;
          case SYSTEM_INFO:
            /* get the relevant parts of the system information 
             * structure */
            systmp = (SysInfo *) messagedata;
			sysinfo.rtmode = systmp->rtmode;
			if (sysinfo.rtmode) {
			  /* process each buffer for spikes when */
			  nspikepoints =  2 * NPOINTS_PER_SPIKE;
			}
			else {

			  /* if we have > nspikepoints points per channel, 
			   * process the data buffer for spikes. If you 
			   * change this make sure that the number of spikes 
			   * in a buffer cannot exceed MAX_SPIKES_PER_BUF 
			   * (e.g. 500 points, a spike every 24 points,
			   * 4 * tetrodes per dsp = 500 / 24 * 4 spikes per 
			   * buf max (~83)) */
			  nspikepoints = 500;
			}
			sysinfo.ndsps = systmp->ndsps;
			sysinfo.machinenum = systmp->machinenum;
            sysinfo.datatype[sysinfo.machinenum] = 
              systmp->datatype[sysinfo.machinenum];
            sysinfo.nchannels[sysinfo.machinenum] = 
              systmp->nchannels[sysinfo.machinenum];
            /* copy the relevant section of the channelinfo
             * structure */
            memcpy(sysinfo.channelinfo[sysinfo.machinenum], 
                systmp->channelinfo[sysinfo.machinenum], 
                sizeof(ChannelInfo) * MAX_CHANNELS);
            /* copy the DSPInfo structure */
            memcpy(sysinfo.dspinfo, systmp->dspinfo, 
                sizeof(DSPInfo) * MAX_DSPS);
            /* allocate space for the data pointers */
            contbufdataptr = (short **) malloc(sysinfo.ndsps * 
                sizeof(short *));
            /* calculate the size of the continuous data from each
             * dsp */
            for (j = 0; j < sysinfo.ndsps; j++) {
              dptr  = sysinfo.dspinfo+j;
              datainc[j] = sysinfo.dspinfo[j].nchan * 
                      sysinfo.dspinfo[j].nsamp;
              contbuf[j].dspnum = j;
              contbufsize[j] = CONT_BUF_STATIC_SIZE + 
                       sysinfo.dspinfo[j].nchan *
                       sysinfo.dspinfo[j].nsampout *
                       sizeof(short);
              nsamp[j] = 0;
              /* set the expected increment for the timestamp */
              tsinc[j] = dptr->nsamp * (30000 / dptr->samprate); 
            }
            break;
          case CHANNEL_INFO:
            /* copy the channel information to the appropriate
             * channel */
            chnew = (ChannelInfo *) messagedata;
            /* go to this channel */
            ch = sysinfo.channelinfo[chnew->machinenum] + 
               chnew->index;
            memcpy(ch, chnew, sizeof(ChannelInfo));
            break;
          case FS_DATA_INFO:
            /* get the fsdatainfo structure */
            memcpy((char *) &fsdatainfo, messagedata,
                sizeof(FSDataInfo));
	    /* we also need to find the fsdata file descriptor */
	    j = 0;
	    while ((id = netinfo.dataoutfd[j++]) != -1) {
	        if (client_data[id].toid == SPIKE_FS_DATA) {
                    fsdatafd = client_data[id].fd;
		    break;
		}
	    }
            break;
          case FS_DATA_START:
            sysinfo.fsdataon = 1;
            break;
          case FS_DATA_STOP:
            sysinfo.fsdataon = 0;
            break;
          case EXIT:
            daqexit(0);            
            break;
          default:
            break;
        }
      }
    }
  }
  return 0;
}


void getspike(short *data)
  /* put a spike of one of three sizes into data */
{
  const short spike[40]={0,0,0,0,100,400,900,1600,2500,3600,7200,14400,20000,25000,27000,25000,20000,14400,7000,-500,-7200,-8000,-9000,-8000,-6000,-3500,-2000,-1500,-1100,-700,-400,-100,0,0,0,0,0,0,0,0};
  int r, i, j, k;
  float mult[4];

  for (i = 0; i < 4; i++) {
    r = rand();
    if (r < (RAND_MAX / 3)) {
      mult[i] = 1 + (float) (rand() - RAND_MAX) / 
        ((float) RAND_MAX * 5);
    }
    else if (r < (2.0/3.0 * RAND_MAX)) {
      mult[i] = .66 + (float) (rand() - RAND_MAX) / 
        ((float) RAND_MAX * 6);
    }
    else {
      mult[i] = .33 + (float) (rand() - RAND_MAX) / 
        ((float) RAND_MAX * 6);
    }
  }

  for (i = 0; i < 4; i++) {
    k = 0;
    for (j = 0; j < 40; j++) {
      data[i + k] = (short) (spike[j] * mult[i] / 120.0);
      k += 4;
    }
  }

  return;
}

int getcont(ContBuffer *contbuf)
  /* put an eight hz sine wave into data */
{
  int i, j, nchan;
  short *dataptr;
  float    time, freq, ampfreq, mod;
  short    stmp;
  DSPInfo *dptr;

  freq = 200;
  ampfreq = .5;
  dataptr = contbuf->data;
  dptr = sysinfo.dspinfo + contbuf->dspnum;
  time = (float) contbuf->timestamp / 10000.0;
  nchan = dptr->nchan;
  for (i = 0; i < dptr->nsampout; i++) {
    for (j = 0; j < nchan; j++) {
	mod = sin(TWOPI * ampfreq * (time + ((float) i) / 
			       ((float) dptr->samprate)));  
	mod =  (mod > .98) ? 50 * (mod - .98): 0;
        stmp = (short) (250.0 * mod * sin(TWOPI * freq * (time + 
			       ((float) i) / ((float) dptr->samprate))) + 
			25 * (float) rand() / (float) RAND_MAX);
      *(dataptr++) =  stmp;

//                
//        *(dataptr++) = (short) (250.0 * sin(TWOPI * 40 * (time + 
//                 ((float) i) / ((float) dptr->samprate))));
//                
//                //50.0 * (float) rand() / RAND_MAX);
//      else
//        *(dataptr++) = 0;
    }
  }
  contbuf->timestamp += (u32) (((float) dptr->nsampout) / ((float) dptr->samprate) 
                    * 10000.0);
  return CONT_BUF_STATIC_SIZE + nchan * dptr->nsamp * sizeof(short);
}

int MakeFSDataBuf(ContBuffer *cptr, FSDataContBuffer *udptr) 
  /* create a continuous buffer with only the data that are supposed to be
   * sent to fsdata and return the size of the buffer */
{
  short     *dataptr, *outdataptr;
  DSPInfo   *dptr;
  ChannelInfo   *chptr;

  bool    savechan[MAX_CHANNELS];
  short     i, j;


  dptr = sysinfo.dspinfo + cptr->dspnum;
  chptr = sysinfo.channelinfo[sysinfo.machinenum];

  udptr->timestamp = cptr->timestamp;
  udptr->samprate = dptr->samprate;

  udptr->nchan = 0;
  /* zero out the list of channels to save */
  memset((void *) savechan, 0, MAX_CHANNELS * sizeof(bool));
  for (j = 0; j < dptr->nchan; j++, chptr++) {
    if (fsdatainfo.contelect[dptr->electinfo[j].number]) {
      /* save this channel */
      savechan[j] = 1;
      /* note that the dsp channel is only correct when only one channel
       * of an electrode is selected */
      udptr->channum[udptr->nchan] = chptr->electchan;
      udptr->electnum[udptr->nchan++] = chptr->number;
    }
  }
  /* set the pointer for the continuous data */
  dataptr = cptr->data;
  outdataptr = udptr->data;
  udptr->nsamp = dptr->nsampout;
  /* copy the channels to be saved into the fsdatacontbuf
   * data buffer */
  for (i = 0; i < udptr->nsamp; i++) {
    for (j = 0; j < dptr->nchan; j++, dataptr++) {
      if (savechan[j]) {
        *(outdataptr++) = *dataptr;
      }
    }
  }
  return sizeof(FSDataContBuffer);

}


void ExtractSpikes(DataBuffer *databuf, int dspnum)
  /* Go through each electrode looking for threshold crossings. To do so, 
   * we search the set of channels corresponding to each electrode for 
   * threshold crossings and save a window of data around each threshold 
   * crossing.  */

  /* The previous processing run copied the last NPOINTS_PER_SPIKE * nchan elements
   * from the last buffer */
{
  int          electnum, i, id, j, k, l;     //temporary variables
  int         snum;
  int         endindex, startind;     // start and stop indeces for the matrix
  int         startchan;         // start index for this channel
  int            nprepoints;        // the number of prespike points in the buffer
  int            dataptrinc;        // the increment to move to the next set of data points for this electrode
  int            spikefound;        // 1 if a spike was found
  int            spikelen;        // the number of points to go through to get a complete spike
  int            spikesep;        // the number of points to skip after a threshold crossing 
  int            nchan;            // the number of channels
  int         nelect;            // the number of electrodes to process
  int         dataptrspikeinc;    // the increment for dataptr when a spike is found
  int            electsize;        // the size of the data for each set of n channels
  int         npostthresh;        // the number of post threshold points
  double        indsamprateconv;    // the multiplier to convert the index within the buffer to a timestamp
  u32            starttime;        // the time for the start of this buffer
  short         *dataptr;        // a pointer to the data
  short         *tmpptr;        // a temporary pointer to the data
  ChannelInfo     *chanptr;        // a pointer to the channelinfo structure
  SpikeBuffer     spikebuf[MAX_SPIKES_PER_BUF];        // the data structure for extracted spikes
  SpikeBuffer     fsdataspikebuf[MAX_SPIKES_PER_BUF];        // the data structure for spikes that will be sent to FSData
  int         nfsspikes;
  SpikeBuffer     *sptr;        // a pointer to the electrode data structure
  DSPInfo        *dptr;

   u32 lasttime = 0;


  short min, max;
   
  int lastspikeind;
  short lastspikeval;

  min = MAX_DATAVAL;
  max = MIN_DATAVAL;
  snum = 0;
  nfsspikes = 0;


  dptr = sysinfo.dspinfo + dspnum;
  /* get the number of channels and electrodes in this buffer */
  nchan = dptr->nchan;
  nelect = dptr->nelectrodes;

  indsamprateconv = (double)  (1.0  / (double) dptr->samprate) * 
                        SEC_TO_TSTAMP;
  /* the end index is the last point where finding a spike would leave the 
   * entire waveform entirely within this buffer, which is the point 
   * NPOST_THRESH_POINTS from the end of the buffer */
  npostthresh = nchan * NPOST_THRESH_POINTS;
  endindex = databuf->npoints - npostthresh;

  /* the number of prespike points is the number of points to back up once 
   * a threshold crossing is identified */ 
  nprepoints = NPRE_THRESH_POINTS * nchan;

  spikelen = NPOINTS_PER_SPIKE * nchan;
  spikesep = POST_THRESH_INC * nchan;

  /* the increment to move from the end of one set of channels corresponding 
   * to an electrocde to the first of the next set if no spike was found*/
  dataptrinc = nchan - NCHAN_PER_ELECTRODE;

  /* if there was a spike, we need to move backwards a bit */
  dataptrspikeinc = spikesep - npostthresh ;
  
  /* initialized the pointer to the channel list */
  chanptr = sysinfo.channelinfo[sysinfo.machinenum];

  /* set the size of NCHAN_PER_ELECTRODE elements */
  electsize = sizeof(short) * NCHAN_PER_ELECTRODE;

  /* set the starttime for this buffer. The buffer actually starts spikelen 
   * points before the new data, as the end of the last buffer will have 
   * been copied into this buffer*/
  starttime = databuf->timestamp - (u32) (NPOINTS_PER_SPIKE *indsamprateconv);
  //fprintf(stderr, "starttime = %u, indconv = %f\n", starttime, indsamprateconv);
  sptr = spikebuf;
  snum = 0;

  for (electnum = 0; electnum < nelect; electnum++) {
    /* we assume that the channels are sequential in the channelinfo
     * structure */
    startchan =  dptr->electinfo[electnum].channelinfochan[0];
    startind = databuf->startindex[electnum] + startchan; 
    dataptr = databuf->data + startind; 
    /* reset the startindex for the next buffer */
    databuf->startindex[electnum] = nprepoints;
    for (i = startind; i < endindex; i += nchan) {
      chanptr = sysinfo.channelinfo[sysinfo.machinenum] + startchan;
      spikefound = 0;
      for (j = 0; j < NCHAN_PER_ELECTRODE; j++) {
        if (*dataptr > 4 * chanptr->thresh) {
          nfsspikes++;
        }
        if (*dataptr > chanptr->thresh) {
          /* this represents a threshold crossing */
          spikefound = 1;
          /* set the timestamp */
          sptr->timestamp = starttime + (u32) 
            (i / nchan * indsamprateconv);
          lasttime = sptr->timestamp;
          /* set the electrode number */
          sptr->electnum = dptr->electinfo[electnum].number;
          tmpptr = sptr->data;
          /* copy the sections of the buffer corresponding to the 
           * spike to the current electrode structure */
          dataptr -= (j + nprepoints);
          lastspikeind = i;
          lastspikeval = *(dataptr +(j + nprepoints)); 
          for (k = 0; k < spikelen;  k += nchan) {
            memcpy(tmpptr, dataptr, electsize);
            tmpptr += NCHAN_PER_ELECTRODE;
            dataptr += nchan;
          }
          sptr++;
          snum++;

          /* add the spike separation so that this spike is not 
           * detected twice */
          i += spikesep;
          /* if this puts us past the end of the buffer, we need to 
           * adjust the startindex for the next buffer so that we 
           * don't detect the same spike twice */
          if (i > endindex) {
            databuf->startindex[electnum] +=  (i - endindex - 
                startchan); 
            //if (electnum == 0)
            //  fprintf(stderr, "spike at offset %d, new start %d, endindex = %d\n", i - spikesep, startindex[electnum], endindex);
          }
          /* nchan will be added to i at the top of the for loop, 
           * so subtract it off now */
          i -= nchan;
          /* because our spike window is wider than the the skip 
           * between spikes, we need to decrement dataptr */
          dataptr += dataptrspikeinc;
          /* move to the next event; there is no need to look at the 
           * other  channels of this electrode */
          break;
        }
        /* increment the data and channel pointers */
        dataptr++;
        chanptr++;
      }
      if (!spikefound) {
        /* increment the data pointer to the next set of samples from 
         * this electrode * */
        dataptr += dataptrinc;
      }
    }
  }
  /* set the timestamp of the next spike to 0 to indicate the end of the buffer */
  sptr->timestamp = 0;
  /* now that we're done, send out the data structures to the save and 
   * display programs */
  l = 0;
  if (snum) {
    if (sysinfo.fsdataon) {
      // send the relevant spikes to the user program
      for (i = 0; i < snum; i++) {
	if ((fsdatainfo.spikeelect[spikebuf[snum].electnum])) {
	  /* copy this spike to the fsdata structure */
	  memcpy(fsdataspikebuf + nfsspikes++, spikebuf + i, 
	      sizeof(SpikeBuffer));
	}
      }
      /* if any of the spikes should go to fsdata, send them */
      if (nfsspikes) {
	if (SendMessage(fsdatafd, SPIKE_DATA, 
	    (char *) fsdataspikebuf, nfsspikes * sizeof(SpikeBuffer)) == -1) {
	  fprintf(stderr, "Error sending spike data to spike_fsdata\n");
        }
      }
    }
    while ((id = netinfo.dataoutfd[l++]) != -1) {
      if (client_data[id].toid != SPIKE_FS_DATA) {
	//fprintf(stderr, "sending spike to %d\n", id);
	if (SendMessage(client_data[id].fd, SPIKE_DATA, 
	    (char *) &spikebuf, sizeof(SpikeBuffer) * snum) 
	    == -1) {
	  fprintf(stderr, "Error sending spike data to %d\n", client_data[id].toid);
	} 
      }
    }
  }
  /* copy the last elements from the 
   * data buffer to the beginning so that spikes that overlap between 
   * buffers are handled correctly. */
  memcpy(databuf->data, databuf->endptr - spikelen, spikelen * sizeof(short));
  /* reset the endptr */
  databuf->endptr = databuf->data + spikelen;
  /* reset the number of points and the number of samples in the buffer */
  databuf->npoints = spikelen;
  databuf->nsampperchan = NPOINTS_PER_SPIKE;
  return;
}


void daqexit(int status)
{
   SendMessage(client_message[SPIKE_MAIN].fd, EXITING, NULL, 0);
   /* sleep so that all of the other programs have a chance to get the message*/
   sleep(1);
   CloseSockets(server_message);
   CloseSockets(client_message);
   CloseSockets(client_data);
   fclose(STATUSFILE);
   exit(status);

}

