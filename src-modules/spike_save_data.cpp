/* spike_save_data.c: Program for saving data from spike_process_data,
 * spike_process_posdata, spike_behav, and spike_main
 * Note that the data is written in gzip-ed format to save space.
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

#define TMPSTRINGLEN 200

void saveexit(int status);

/* the global system information structure */
SysInfo 		sysinfo;
NetworkInfo		netinfo;
SocketInfo 		server_message[MAX_CONNECTIONS]; 	// the structure for server messaging
SocketInfo 		client_message[MAX_CONNECTIONS]; 	// the structure for client messaging
SocketInfo 		client_data[MAX_CONNECTIONS]; 	// the structure for client messaging

SocketInfo 		server_data[MAX_CONNECTIONS]; 	// the structure for receiving data

int			maxfds;
fd_set	   	        readfds; 

int main() 
{
    u32 	nbufsread;     	// the number of buffers read in
    char		*savebuf;       // the buffer to save 
    SysInfo		*systmp;
    ChannelInfo		*chtmp;
    EventBuffer		*event;
    DIOBuffer		*dio;
    PosBuffer		posbuf[1];
    char 		tmpstring[TMPSTRINGLEN+1];	// a temporary buffer 
    char 		filename[TMPSTRINGLEN+1];	// a temporary buffer 
    int			tmplen; 	// the length of the data in tmbbuf
    int 		tmp1, tmp2, diskfree;
    short		datatype;
    char 		*outfilename;	// the name of the file to save data to
    gzFile		outfile;	// the pointer for the output file
    int                 message;	// a temporary message variable
    int			messagedata[MAX_BUFFER_SIZE]; // message data can contain a sysinfo or channelinfo structure
    int			messagedatalen; // the length of the data in the message
    int			nspikes;
    int			savebufsize;     // the size of the input data buffer in bytes
    u32			tmpbufsize;			
    int 		error;
    int 		i, id;
    int			lastmessage;	// the last socket we got a message on
    int			tmpsize;
    int			closefile;	// 1 if the file is to be closed
    int			newdata;	// 1 if new data were received 
    FILE		*pipe;
    OpenFileMessage 	*ofm;

    sysinfo.acq = 0;
    sysinfo.diskon = 0;
    sysinfo.fileopen = 0;
    closefile = 0;
    outfile = NULL;

    /* set the type of program we are in for messaging */
    sysinfo.program_type = SPIKE_SAVE_DATA;

    sysinfo.statusfile == NULL;

    if (STATUSFILE == NULL) {
        /* open up the status file if it is not stderr*/
        gethostname(tmpstring,80);
        sprintf(filename, "spike_save_data_status_%s", tmpstring);
        if ((STATUSFILE = fopen(filename, "w")) == NULL) {
            fprintf(stderr, "spike_main: error opening status file\n");
            exit(-1);
        }
    }

    fprintf(STATUSFILE, "spike_save_data: starting messaging\n");
    if (StartNetworkMessaging(server_message, client_message, server_data, 
		client_data) < 0) {
        fprintf(STATUSFILE, "spike_save: Error establishing messaging\n");
	saveexit(1);
    }

    // we need to allocate enough space for a buffer of any type, so
    // we allocate a large buffer
    //savebuf = (char *) malloc(SAVE_BUF_SIZE);
    savebuf = (char *) malloc(1000000 * sizeof(char));

    while (1) { 
	newdata = 0;
	/* set up the initial list of file descriptors to watch */
	SetupFDList(&readfds, &maxfds, server_message, server_data);
	select(maxfds, &readfds, NULL, NULL, NULL);
	id = 0;
	/* check for incoming data */
	while ((i = netinfo.datainfd[id]) != -1) {
	    if (FD_ISSET(server_data[i].fd, &readfds)) {
		/* check to see if the disk is on, and if not, get the message
		 * and discard it. Note that this works for everyone except
		 * spike_process_posdata, as it only sends data out when the
		 * disk is on.  */
		if ((!sysinfo.diskon) && (i != SPIKE_PROCESS_POSDATA)) {
		    message = GetMessage(server_data[i].fd, savebuf, &savebufsize, 1);
		}
		else {
		    newdata = 1;
		    tmpsize = 0;
		    switch(i) {
		    case SPIKE_DAQ: 
			/* get the data */
			message = GetMessage(server_data[SPIKE_DAQ].fd, savebuf, &savebufsize, 1);
			/* check to see if this is an event */
			if (message == EVENT) {
			    event = (EventBuffer *) messagedata;
			    datatype = EVENT_DATA_TYPE;
			    if ((tmpsize = gzwrite(outfile, &datatype, 
					    sizeof(short))) != sizeof(short)) {
				error = 1;
				fprintf(STATUSFILE, "spike_save_data: writing error: %d written, %d tried\n", tmpsize, sizeof(short));

			    }
			    if ((tmpsize = gzwrite(outfile, event, sizeof(EventBuffer))) != sizeof(EventBuffer)) {
				error = 1;
				fprintf(STATUSFILE, "spike_save_data: writing error: %d written, %d tried\n", tmpsize, savebufsize);
			    }
			}
			else if (message == DIGITALIO_EVENT) {
			    /* write the behav, the port, and the status */
			    datatype = DIGITALIO_DATA_TYPE;
			    if ((tmpsize = gzwrite(outfile, &datatype, 
					    sizeof(short))) != sizeof(short)) {
				error = 1;
				fprintf(STATUSFILE, "spike_save_data: writing error: %d written, %d tried\n", tmpsize, sizeof(short));

			    }
			    /* write out the data */
			    if ((tmpsize = gzwrite(outfile, savebuf, savebufsize))
				    != savebufsize) {
				fprintf(STATUSFILE, "spike_save_data: writing error: %d written, %d tried\n", tmpsize, savebufsize);
				error = 1;
			    }
			} 
			else {
			    if (message == SPIKE_DATA) {
				datatype = SPIKE_DATA_TYPE;
			    }
			    else if (message == CONTINUOUS_DATA) {
				datatype = CONTINUOUS_DATA_TYPE;
			    }
			    /* write out the data type */
			    if ((tmpsize = gzwrite(outfile, &datatype, 
					    sizeof(short))) != sizeof(short)) {
				error = 1;
				fprintf(STATUSFILE, "spike_save_data: writing error: %d written, %d tried\n", tmpsize, sizeof(short));

			    }

			    /* write out the data size */
			    if ((tmpsize = gzwrite(outfile, &savebufsize, sizeof(int))) != sizeof(int)) {
				error = 1;
				fprintf(STATUSFILE, "spike_save_data: writing error: %d written, %d tried\n", tmpsize, savebufsize);

			    }

			    /* write out the data */
			    if ((tmpsize = gzwrite(outfile, savebuf, savebufsize)) != savebufsize) {
				error = 1;
				fprintf(STATUSFILE, "spike_save_data: writing error: %d written, %d tried\n", tmpsize, savebufsize);

			    }
			}
			break;
		    case SPIKE_PROCESS_POSDATA:
			/* get the position buffer */
			GetMessage(server_data[SPIKE_PROCESS_POSDATA].fd, 
				   savebuf, &savebufsize, 1);
			/* write out the data type */
			datatype = POSITION_DATA_TYPE;
			if ((tmpsize = gzwrite(outfile, &datatype, 
					sizeof(short))) != sizeof(short)) {
			    error = 1;
			    fprintf(STATUSFILE, "spike_save_data: writing error: %d written, %d tried\n", tmpsize, sizeof(short));

			}
			/* write out the data. Note the that first element of
			 * the data is the size of the buffer, so we don't need
			 * to*/
			//fprintf(stderr, "writing out %d position bytes \n", savebufsize);
			if ((tmpsize = gzwrite(outfile, savebuf, savebufsize))
				!= savebufsize) {
			    fprintf(STATUSFILE, "spike_save_data: writing error: %d written, %d tried\n", tmpsize, savebufsize);
			    error = 1;
			}
			break;
		    case SPIKE_MAIN: 
			/* get the event or digital IO event*/
			message = GetMessage(server_data[SPIKE_MAIN].fd, 
				savebuf, &savebufsize, 0);
			if (message == EVENT) {
			    event = (EventBuffer *) savebuf;
			    datatype = EVENT_DATA_TYPE;
			    if ((tmpsize = gzwrite(outfile, &datatype, 
					    sizeof(short))) != sizeof(short)) {
				error = 1;
				fprintf(STATUSFILE, "spike_save_data: writing error: %d written, %d tried\n", tmpsize, sizeof(short));

			    }
			    if ((tmpsize = gzwrite(outfile, event, sizeof(EventBuffer))) != sizeof(EventBuffer)) {
				error = 1;
				fprintf(STATUSFILE, "spike_save_data: writing error: %d written, %d tried\n", tmpsize, savebufsize);
			    }
			}
			else if (message == DIGITALIO_EVENT) {
			    /* write the behav, the port, and the status */
			    datatype = DIGITALIO_DATA_TYPE;
			    if ((tmpsize = gzwrite(outfile, &datatype, 
					    sizeof(short))) != sizeof(short)) {
				error = 1;
				fprintf(STATUSFILE, "spike_save_data: writing error: %d written, %d tried\n", tmpsize, sizeof(short));

			    }
			    /* write out the data */
			    if ((tmpsize = gzwrite(outfile, savebuf, savebufsize))
				    != savebufsize) {
				fprintf(STATUSFILE, "spike_save_data: writing error: %d written, %d tried\n", tmpsize, savebufsize);
				error = 1;
			    }
			} 
			else if (message == TIME_CHECK) {
			    /* write the time data */
			    datatype = TIME_CHECK_DATA_TYPE;
			    if ((tmpsize = gzwrite(outfile, &datatype, 
					    sizeof(short))) != sizeof(short)) {
				error = 1;
				fprintf(STATUSFILE, "spike_save_data: writing error: %d written, %d tried\n", tmpsize, sizeof(short));

			    }
			    /* write out the data */
			    if ((tmpsize = gzwrite(outfile, savebuf, savebufsize))
				    != savebufsize) {
				fprintf(STATUSFILE, "spike_save_data: writing error: %d written, %d tried\n", tmpsize, savebufsize);
				error = 1;
			    }
			}
			break;
		    }
		    if (error) {
			/* stop saving and send a SAVE_ERROR message */
			fprintf(STATUSFILE,"spike_save_data: Error in saving data\n");
			sysinfo.diskon = 0;
			SendMessage(client_message[SPIKE_MAIN].fd, SAVE_ERROR, NULL, 0);
		    }
		    lastmessage = i;
		    break;
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
		error = 0;
		switch(message) {
		    case STOP_ACQUISITION:
			sysinfo.acq = 0;
			SendMessage(client_message[SPIKE_MAIN].fd, ACQUISITION_STOPPED, 
				    NULL, 0);
			break;
		    case START_ACQUISITION:
			sysinfo.acq = 1;
			SendMessage(client_message[SPIKE_MAIN].fd, ACQUISITION_STARTED, 
				    NULL, 0);
			break;
		    case START_SAVE:
			sysinfo.diskon = 1;
			SendMessage(client_message[SPIKE_MAIN].fd, 
				    SAVE_STARTED, NULL, 0);
			break;
		    case STOP_SAVE:
			sysinfo.diskon = 0;
			SendMessage(client_message[SPIKE_MAIN].fd, 
				    SAVE_STOPPED, NULL, 0);
			break;
		    case OPEN_FILE:
			  /* we can get this message in the middle of saving 
			   * data if the main program has determined that the 
			   * current file size is too  large, so we need to 
			   * close the current file if it is open */
			if (outfile != NULL)  gzclose(outfile);
			ofm = (OpenFileMessage *) messagedata;

			/* we will have already checked to make sure that the 
			 * file can be opened */
			/* make sure the file name is correctly terminated */
			strncpy(sysinfo.datafilename, ofm->filename, 
				sizeof(sysinfo.datafilename));
			sysinfo.datafilename[sizeof(sysinfo.datafilename)-1] = '\0';
			if (ofm->type != OpenFileMessage::GZip) {
			    sprintf(tmpstring, "file type is not GZip, but rather was: %d; defaulting to GZip!\n", (int)ofm->type);
			    StatusMessage(tmpstring, client_message);
			}
			// build string to pass to gzopen specifying open more, 
			// compression level, etc
			snprintf(tmpstring, sizeof(tmpstring), "ab%u", 
				ofm->compression_level);
			sysinfo.use_compression = ofm->compression_level ? 
			    true : false;
			sysinfo.compression_level = ofm->compression_level;
			outfile = gzopen(sysinfo.datafilename, tmpstring);
      if( Z_NULL == outfile) {
        fprintf(STATUSFILE, "gzopen() failed to open file: %s with options %s\n", sysinfo.datafilename, tmpstring);
      }
			else{
        fprintf(STATUSFILE, "gzopen() opened file: %s with options %s\n", sysinfo.datafilename, tmpstring);
        sysinfo.fileopen = 1;
      }     
			snprintf(tmpstring, sizeof(tmpstring), "df %s\n", 
				sysinfo.datafilename);
			diskfree = 0;
			if ((pipe = popen(tmpstring, "r")) == NULL) {
			    sprintf(tmpstring, "Error opening pipe to check disk usage");
			    StatusMessage(tmpstring, client_message);
			}
			else {
			    fgets(tmpstring, TMPSTRINGLEN, pipe);
			    fscanf(pipe, "%s%d%d%d", tmpstring, &tmp1, &tmp2, &diskfree);
			}
			/* convert KB to MB for diskfree */
			diskfree /= 1024;
			pclose(pipe);
			SendMessage(client_message[SPIKE_MAIN].fd, FILE_OPENED,
			      (char *) &diskfree, sizeof(int));
			break;
		    case CLOSE_FILE:
		        closefile = 1;
			break;
		    case SYSTEM_INFO:
			/* we don't need any systeminfo information in this
			 * module */
			break;
		    case EXIT:

			saveexit(0);		        
			break;
		    default:
			break;
		}
	    }
	    id++;
	}
	/* close the file only if no new data came in on the last iteration of the for loop */
	if (closefile && !newdata) {
	    gzclose(outfile);
	    outfile = NULL;
	    sysinfo.fileopen = 0;
	    SendMessage(client_message[SPIKE_MAIN].fd, FILE_CLOSED, NULL, 0);
	    closefile = 0;
        }
    }
    return 0;
}


void saveexit(int status)
{
   int i;
   SendMessage(client_message[SPIKE_MAIN].fd, EXITING, NULL, 0);
   /* sleep so that all of the other programs have a chance to get the message*/
   sleep(1);
   CloseSockets(server_message);
   CloseSockets(client_message);
   CloseSockets(server_data);
   fclose(STATUSFILE);
   exit(status);
}
