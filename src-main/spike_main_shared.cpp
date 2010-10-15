/*
 * spike_main_shared.cpp:  shared code used by spike_main, spike_main_rt and
 * spike_main_matlab
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

extern EventBuffer event;

void FormatTS(char *timestring, u32 time)
{
    /* write the time to timestring in hh:mm:ss.s format */
    sprintf(timestring,"%1lu:%02lu:%02lu %04lu", time/36000000L, (time/600000L)%60,
                                                (time/10000L)%60, time%10000L);
}

void Usage() 
{
   printf("Usage: nspike2 -config configfilename [-v]\n\t");
   printf("\t-v returns the current version of the code\n");
   return;
}



void SendSystemConfig(void)
    /* send the sysconfig structure and the internal tetrode, eeg, or 
     * position structures to the local modules */
{
  //int data[1];
  //int message;
  //int tmpsize;
    int i, id;
    int channelnum;

    channelnum = 0;

    i = 0;
     while ((id = netinfo.messageinfd[i++]) != -1) {
	/* only send the message to the local modules */
	if (strcmp(netinfo.myname, client_message[id].to) == 0) {
	    SendMessage(client_message[id].fd, SYSTEM_INFO, (char *) &sysinfo, 
	                sizeof(SysInfo)); 
	}
    } 
    return;
}



int StartModuleAcquisition(void)
{
    int 	i, idtmp, id; 
    /* send out a start acquisition message to each of the subprocesses. It's
     * best to do this in reverse order so that we start the data acquisiton
     * routines last */

    idtmp = -1;
    while (netinfo.messageoutfd[++idtmp] != -1) {};
    idtmp--;
    for (i = idtmp; i >= 0; i--) {
	/* check to make sure this is a local module, as remote modules have
	 * their own spike main to start acquisition */
	id = netinfo.messageoutfd[i];
	if (strcmp(netinfo.myname, client_message[id].to) == 0) {
	    SendMessage(client_message[id].fd, START_ACQUISITION, NULL, 0);
	    //fprintf(STATUSFILE, "Starting acquisition on %d\n", id);
	    if (!WaitForMessage(server_message[id].fd, 
			ACQUISITION_STARTED, 2)) {
		fprintf(STATUSFILE, "spike_main: Error starting acquisition on program %d\n", id);
	    }
	    else {
	    }
	}
    }

    if (sysinfo.fileopen) {
	/* output an event to the saving program */
	event.timestamp = sysinfo.approxtime;
	event.type = EVENT_ACQUISITION_STARTED;
	event.descript[0] = '\0';
	SendMessage(client_data[SPIKE_SAVE_DATA].fd, EVENT, (char *) &event, sizeof(EventBuffer));
    }
    
    return 1;
}

int StartModuleAcquisition(int modulenum)
{
    /* send out a start acquisition message to the specified local Module */
    SendMessage(client_message[modulenum].fd, START_ACQUISITION, NULL, 0);
    //fprintf(STATUSFILE, "Starting acquisition on %d\n", id);
    if (!WaitForMessage(server_message[modulenum].fd, 
		ACQUISITION_STARTED, 1)) {
	fprintf(STATUSFILE, "spike_main: Error starting acquisition on program %d\n", modulenum);
	return 0;
    }
    else {
    }

    return 1;
}



int StartAcquisition(void)
   /* send messages to spike_daq, spike_process_data, and spike_save telling them to
    * start acquisition. This should be initiated by a message from the master machine
    * (or this machine if this is the master )  */
{
    int 	i, ret; 

    
    sysinfo.acq = 1;
    sysinfo.dspacq = 1;
    if (sysinfo.system_type[sysinfo.machinenum] == MASTER) {
	/* Start acquisition on each of the slave machines */
	for(i = 0; i < netinfo.nslaves; i++) {
	    SendMessage(netinfo.slavefd[i], START_ACQUISITION, NULL, 0);
	    /* Wait for the reply */
	    if (!WaitForMessage(netinfo.masterfd[i], ACQUISITION_STARTED, 2)) {
	        sprintf(tmpstring,"spike_main: no response to START_ACQUISITON message from slave %s\n", netinfo.slavename[i]);
		fprintf(STATUSFILE, "%s", tmpstring);
	        DisplayStatusMessage(tmpstring);
		return 0;
            }
        }
	ret = StartModuleAcquisition();
    }
    else {
	ret = StartModuleAcquisition();
	SendMessage(netinfo.slavefd[netinfo.myindex], ACQUISITION_STARTED, NULL, 0);
    }
    return  ret;
}


int StopModuleAcquisition(void)
{
  int i, id;

  /* send out a stop acquisition message to each of the subprocesses */
  i = 0;
  while ((id = netinfo.messageoutfd[i]) != -1) {
    if (strcmp(netinfo.myname, client_message[id].to) == 0) {
      SendMessage(client_message[id].fd, STOP_ACQUISITION, NULL, 0);
      if (!WaitForMessage(server_message[id].fd, ACQUISITION_STOPPED, 5)) {
        sprintf(tmpstring, "spike_main: Error stopping acquisition on program %d\n", id);
        fprintf(STATUSFILE, "%s\n", tmpstring);
        DisplayStatusMessage(tmpstring);
      }
      else {
      }
    }
    i++;
  }
  /* clear out all of the incoming data structures so that we can start
   * afresh */
  if (sysinfo.fileopen) {
    /* output an event to the saving program */
    event.timestamp = sysinfo.approxtime;
    event.type = EVENT_ACQUISITION_STOPPED;
    event.descript[0] = '\0';
    SendMessage(client_data[SPIKE_SAVE_DATA].fd, EVENT, (char *) &event, sizeof(EventBuffer));
  }
  return 1;
}

int StopModuleAcquisition(int modulenum)
{
    /* send out a start acquisition message to the specified local Module */
    SendMessage(client_message[modulenum].fd, STOP_ACQUISITION, NULL, 0);
    if (!WaitForMessage(server_message[modulenum].fd, 
		ACQUISITION_STOPPED, 1)) {
	fprintf(STATUSFILE, "spike_main: Error stopping acquisition on program %d\n", modulenum);
    }
    else {
    }

    return 1;
}


int StopAcquisition(void)
   /* send messages to spike_daq, spike_process_data, and spike_save telling them to
    * stop acquisition. This should be initiated by a message from the master machine
    * (or this machine if this is the master ) */
{
  //int 	data[1];
  //int 	message;
    int 	i; 
    int 	ret; 

    sysinfo.acq = 0;
    sysinfo.dspacq = 0;
    if (sysinfo.system_type[sysinfo.machinenum] == MASTER) {
	/* Start acquisition on each of the slave machines */
	for(i = 0; i < netinfo.nslaves; i++) {
	    SendMessage(netinfo.slavefd[i], STOP_ACQUISITION, NULL, 0);
	    /* Wait for the reply */
	    if (!WaitForMessage(netinfo.masterfd[i], ACQUISITION_STOPPED, 5)) {
	        sprintf(tmpstring,"spike_main: no response from slave %s\n", 
		        netinfo.slavename[i]);
		fprintf(STATUSFILE, "%s", tmpstring);
	        DisplayStatusMessage(tmpstring);
            }
        }
	ret = StopModuleAcquisition();
    }
    else {
	ret = StopModuleAcquisition();
	SendMessage(netinfo.slavefd[netinfo.myindex], ACQUISITION_STOPPED, NULL, 0);
    }
    return ret;
}


void SendMatlabInfo(void)
    /* send the matlabinfo structure to the slaves and the local modules. This
     * is only called by the master machine */
{
   int i, id;

   for(i = 0; i < netinfo.nslaves; i++) {
	SendMessage(netinfo.slavefd[i], MATLAB_INFO, (char *) &matlabinfo, 
		    sizeof(MatlabInfo));
   }
   /* now send the matlabinfo structure to all of the modules. Most of the 
    * modules will ignore this message. */
    i = 0;
    while ((id = netinfo.messageinfd[i++]) != -1) {
	/* only send the message to the local modules */
	if (strcmp(netinfo.myname, client_message[id].to) == 0) {
	    SendMessage(client_message[id].fd, MATLAB_INFO, (char *)&matlabinfo,
		    sizeof(MatlabInfo)); 
	}
    } 
    return;
}


int MatlabStartSave(void) 
{
    int i, id;
   /* Send the MATLAB_START_SAVE message to all of the modules. Most of the 
    * modules will ignore this message. */
    i = 0;
    while ((id = netinfo.messageinfd[i++]) != -1) {
	/* only send the message to the local modules */
	if (strcmp(netinfo.myname, client_message[id].to) == 0) {
	    SendMessage(client_message[id].fd, MATLAB_START_SAVE, NULL, 0);
	   /* if (!WaitForMessage(server_message[id].fd, MATLAB_SAVE_STARTED, 2)){
		sprintf(tmpstring,"spike_main: Error starting matlab save, no response from module %d\n", id);
		DisplayStatusMessage(tmpstring);
		return 0;
	    } */
	}
    }
    sysinfo.matlabon = 1;
    FormatTS(timestring, sysinfo.approxtime);
    sprintf(tmpstring,"Matlab output ON at appoximately %s", timestring);
    DisplayStatusMessage(tmpstring);
    return 1;
} 

int MatlabStopSave(void) 
{
    int i, id;
   /* Send the MATLAB_STOP_SAVE message to all of the modules. Most of the 
    * modules will ignore this message. */
    i = 0;
    while ((id = netinfo.messageinfd[i++]) != -1) {
	/* only send the message to the local modules */
	if (strcmp(netinfo.myname, client_message[id].to) == 0) {
	    SendMessage(client_message[id].fd, MATLAB_STOP_SAVE, NULL, 0);
	    /*if (!WaitForMessage(server_message[id].fd, MATLAB_SAVE_STOPPED, 2)){
		sprintf(tmpstring,"spike_main: Error starting matlab save, no response from module %d\n", id);
		DisplayStatusMessage(tmpstring);
		return 0;
	    } */
	}
    }
    sysinfo.matlabon = 0;
    FormatTS(timestring, sysinfo.approxtime);
    sprintf(tmpstring,"Matlab output OFF at appoximately %s\n", timestring);
    DisplayStatusMessage(tmpstring);
    return 1;
}

void spikeexit(int status)
/* send all modules an EXIT message and exit */
{
   int i, id;
    

    if (sysinfo.system_type[sysinfo.machinenum] == MASTER) {
	for(i = 0; i < netinfo.nslaves; i++) {
	    SendMessage(netinfo.slavefd[i], EXIT, NULL, 0);
	}
    }
    i = 0;
    while (((id = netinfo.messageoutfd[i]) != -1) && (id < MAX_CONNECTIONS)) {
	/* only send messages to the local modules as the remote modules are
	 * handled by their own spike_main programs */
	if (strcmp(client_message[id].to, netinfo.myname) == 0) { 
	    SendMessage(client_message[id].fd, EXIT, NULL, 0); 
	    if (!WaitForMessage(server_message[id].fd, EXITING, 5)) {
		fprintf(STATUSFILE, "spike_main: Error exiting from module %d\n", id);
	    }
	}
	i++;
    }
    if (sysinfo.system_type[sysinfo.machinenum] == MASTER) {
	for(i = 0; i < netinfo.nslaves; i++) {
	    close(netinfo.slavefd[i]);
	}
	if (sysinfo.datatype[sysinfo.machinenum] & DIGITALIO) {
	    fprintf(stderr,"Telling user program to exit.\n");
	   if (digioinfo.inputfd) {
		SendMessage(digioinfo.outputfd, EXIT, NULL, 0);
		sleep(1);
		close(digioinfo.outputfd);
		close(digioinfo.inputfd);
		unlink(DIO_TO_USER_MESSAGE);
		unlink(USER_TO_DIO_MESSAGE);
	    }
	}
    }
    
    CloseSockets(server_message);
    CloseSockets(client_message);
    CloseSockets(server_data);
    fclose(STATUSFILE);
    exit(status);
}

