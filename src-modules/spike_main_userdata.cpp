/* spike_main_userdata.cpp: Program to allow running userdata on a separate machine
 * than used for data acquisition.  
 *
 * This code will eventually allow nspike_userdata to run, like nspike, on a
 * separate machine.  That machine would recieve data from the data acquisition
 * system and store it for transmission to a local userdata process.  This is not
 * yet working
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
#include "spike_dio.h"

void Usage(void);
void DisplayStatusMessage(char *message);


/* Global variables */
SysInfo 		sysinfo;
NetworkInfo		netinfo;
DigIOInfo		digioinfo;
UserDataInfo		userdatainfo;
EventBuffer		event;
CommonDSPInfo		cdspinfo;

SocketInfo 		server_message[MAX_CONNECTIONS]; // the structure for the server messaging
SocketInfo 		client_message[MAX_CONNECTIONS]; // the structure for the client messaging
SocketInfo 		server_data[MAX_CONNECTIONS]; // the structure for receiving data
SocketInfo 		client_data[MAX_CONNECTIONS]; // the structure for sendin data (currently unused)

char 			tmpstring[100];	// a temporary buffer 
char 			timestring[100];// a temporary buffer 

/* Include the shared code */
#include "../src-main/spike_main_shared.cpp"

int main(int argc, char **argv) 
{
    int			buffersize;
    // we save the buffer index each time we increment the size of the buffer

    int			maxfds;
    fd_set	   	readfds; 

    char 		command[200];
    char 		configname[200];
    char 		netconfigname[200];
    int                 message;	// a temporary message variable
    int			messagedata[MAX_BUFFER_SIZE]; // message data can contain a sysinfo or channelinfo structure
    int			messagedatalen[1]; // the length of the data in the message
    int 		i, j, id, nxtarg;

    sysinfo.statusfile == NULL;
    if (STATUSFILE == NULL) {
	/* open up the status file if it is not stderr*/
	gethostname(tmpstring,80);
	sprintf(command, "spike_main_userdata_status_%s", tmpstring);
	if ((STATUSFILE = fopen(command, "w")) == NULL) {
	    fprintf(stderr, "spike_userdata: error opening status file\n");
	    exit(-1);
	}
    }

    /* set the default configuration file names */
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
        else {
	    Usage();
        }
    }

    sysinfo.acq = 0;
    sysinfo.userdataon = 0;

    /* set the type of program we are in for messaging */
    sysinfo.program_type = SPIKE_MAIN;

    /* Read in the configuration file */
    fprintf(STATUSFILE, "spike_main_userdata: Reading config file\n");
    if (ReadConfigFile(configname, 0) < 0) {
       fprintf(STATUSFILE, "Error in configuration file, exiting.\n");
       exit(-1);
    }

    if (InitializeMasterSlaveNetworkMessaging() < 0) {
	fprintf(STATUSFILE, "spike_main_userdata: Error initializing master/slave network messaging\n");
	spikeexit(1);
    }


    fprintf(stderr,"system type %d,%d\n",sysinfo.datatype[sysinfo.machinenum],sysinfo.machinenum);

    /* Set up the default module messaging */
    SetupModuleMessaging();

    /* Establish the and messaging ethernet sockets */
    if (StartNetworkMessaging(server_message, client_message, server_data, 
		client_data) < 0) {
	fprintf(STATUSFILE, "spike_main_userdata: Error starting network data messaging\n");
	spikeexit(1);
    }

    
    while (1) { 
	/* set up the initial list of file descriptors to watch */
	SetupFDList(&readfds, &maxfds, server_message, server_data);
	select(maxfds, &readfds, NULL, NULL, NULL);
	/* check for messages */
	select(maxfds, &readfds, NULL, NULL, NULL);
	j = 0;
	while ((id = netinfo.messageinfd[j]) != -1) {
	    if (FD_ISSET(server_message[id].fd, &readfds)) {
	        message = GetMessage(server_message[id].fd, 
			(char *) messagedata, messagedatalen, 0);
		switch(message) {
		case ERROR_MESSAGE:
		    /* getting an error requires that we stop and restart processing and
		    * saving*/
		    SendMessage(netinfo.slavefd[netinfo.myindex], ERROR_MESSAGE, (char *) messagedata, *messagedatalen);
		    fprintf(STATUSFILE, "spike_main_rt: error message from %d: %s/n", id, (char *)messagedata);
		    break;
		case STATUS_MESSAGE:
		    SendMessage(netinfo.slavefd[netinfo.myindex], STATUS_MESSAGE, (char *) messagedata, *messagedatalen);
		    break;
                case EXIT:
		    /* one of the modules encountered a fatal error, so exit */
		    spikeexit(-1);
                    break;
		default:
		    break;
		}
	    }
	    j++;
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
		//fprintf(STATUSFILE, "slave message %d\n", message);
		switch(message) {
		case EXIT:
		    if (!sysinfo.fileopen) {
			spikeexit(0);
                    }
		    break;
		case START_ACQUISITION:
		    StartAcquisition();
		    break;
		case STOP_ACQUISITION:
		    StopAcquisition();
		    break;
		case USER_DATA_START:
		    if (UserDataStart()) {
			SendMessage(netinfo.slavefd[netinfo.myindex], 
				USER_DATA_STARTED, NULL, 0);
		    }
		    break;
		case USER_DATA_STOP:
		    if (UserDataStop()) {
			SendMessage(netinfo.slavefd[netinfo.myindex], 
				USER_DATA_STOPPED, NULL, 0);
		    }
		    break;
		/* the following commands require a reply, but we don't have to
		 * actually do anything */
		case START_SAVE:
		    SendMessage(netinfo.slavefd[netinfo.myindex], SAVE_STARTED, NULL, 0);
		    break;
		case STOP_SAVE:
		    SendMessage(netinfo.slavefd[netinfo.myindex], SAVE_STOPPED, NULL, 0);
		    break;
		case CLEAR_SCREEN: 
	  	    SendMessage(netinfo.slavefd[netinfo.myindex], SCREEN_CLEARED, NULL, 0);
		    break;
		case OPEN_FILE: 
	  	    SendMessage(netinfo.slavefd[netinfo.myindex], FILE_OPENED, NULL, 0);
		case CLOSE_FILE: 
	  	    SendMessage(netinfo.slavefd[netinfo.myindex], FILE_CLOSED, NULL, 0);
		    break;
		default:
		    break;
		}
	    }
	}
    }
    return 0;
}

void DisplayStatusMessage(char *message)
{
    fprintf(stderr, "%s\n", message);
    return;
}


