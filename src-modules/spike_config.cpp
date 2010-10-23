/*
 * spike_config.cpp: functions to read in and write out configuration files
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


#include <stdarg.h>
#include "spikecommon.h"

extern SysInfo 		sysinfo;
extern NetworkInfo 	netinfo;
extern UserDataInfo 	userdatainfo;
extern CommonDSPInfo 	cdspinfo;
extern DigIOInfo 	digioinfo;

int ReadCalibrationFile(char *filename);
int CheckAudioSettings(int chan);

int ReadConfigFile(char *configfilename, int datafile)
    /* read in all of the options from the configuration file */
{
    int   i;
    int   start;
    int   tmpint[10];
    int   tmpshort[3];
    int   currentchannel;
    int		currentmachine;
    char  tmpbuf[200];
    char  tmpname[200];
    char  *tmp, *tmpptr;
    bool	*tmpbool;
    int		tmpnum;
    int 	currentslave;
    int 	currentconn;
    int		currentport;
    char 	tmp1[100];
    char 	tmp2[100];
    char 	tmp3[100];
    char 	tmp4[100];
    gzFile config;

    /* read in the specified configuration file */
    if ((config = gzopen(configfilename, "r")) == NULL) {
        fprintf(stderr, "spike_main: Error opening file %s for reading\n", 
                configfilename);
        return -1;
    }

    /* set the defaults */
    currentchannel = 0;
    currentmachine = -1;

    /* get the name of this host */
    gethostname(netinfo.myname, 80);
    /* get rid everything but the first part of the name (if a full domain name
     * was returned */
    if ((tmpptr = strchr(netinfo.myname, '.')) != NULL) {
	*tmpptr = '\0';
    }

    netinfo.nslaves = 0;
    netinfo.rtslave = -1;
    netinfo.userdataslave = -1;
    currentslave = 0;
    currentconn = 0;
    currentport = 0;

    /* initialize the electrode info array */
    for (i = 0; i < MAX_ELECTRODES; i++) {
	sysinfo.electinfo[i].number = -1;
	sysinfo.electinfo[i].nchan = 0;
    }
    /* initialize the data directories */
    for (i = 0; i < MAX_MACHINES; i++) {
	sysinfo.datadir[i][0] = '/';
	sysinfo.datadir[i][1] = '\0';
    }

    sysinfo.myhostname[0] = '\0';
    sysinfo.ndsps = 0;
    sysinfo.localref = 0;
    memset(sysinfo.dspinfo, 0, sizeof(DSPInfo));
    digioinfo.nports = 0;

    while (gzgets(config, tmpbuf, 200) != NULL) {
            /* skip any white spaces */
        start = 0;
        while (isspace(tmpbuf[start]) && (tmpbuf[start] != '\0')) start++;
        if ((tmpbuf[start] != '%') && (tmpbuf[start] != '\0') && (tmpbuf[start] != '\n')) {
            /* parse the file */
            tmp = tmpbuf + start;
	    /* parse the file */
	    if (strncmp(tmpbuf, "nslaves", 7) == 0) {
		tmpptr = tmpbuf+8;
	        sscanf(tmpptr, "%d", &(netinfo.nslaves));
		netinfo.nmachines = netinfo.nslaves + 1;
	        /* allocate space for the list of slave names and the list of slave file
	 	* descriptors */
                netinfo.slavefd = (int *) calloc(netinfo.nslaves, sizeof(int));
                netinfo.masterfd = (int *) calloc(netinfo.nslaves, sizeof(int));
                netinfo.slaveport = (unsigned short *) calloc(netinfo.nslaves, 
					             sizeof(int));
                netinfo.slavename = (char **) malloc(netinfo.nslaves * 
						     sizeof(char *));
	        for(i = 0; i < netinfo.nslaves; i++) {
	 	    netinfo.slavename[i] = (char *) malloc(80*sizeof(char));
                }
            }
	    else if (strncmp(tmpbuf, "master", 6) == 0) {
	        tmpptr = tmpbuf+7;
		while (isspace(*tmpptr)) tmpptr++;
	    	sscanf(tmpptr, "%s %hd", netinfo.mastername, &netinfo.masterport);

		/* put this machiname in the machinename array */
		strcpy(netinfo.machinename[0], netinfo.mastername);
		/* check to see if we are on the master */
		if (strcmp(netinfo.mastername, netinfo.myname) == 0) {
		    sysinfo.system_type[0] = MASTER;
                }
            }
	    else if ((strncmp(tmpbuf, "slave", 5) == 0) || 
	             (strncmp(tmpbuf, "userdataslave", 13) == 0)) {
	        if (strncmp(tmpbuf, "slave", 5) == 0) {
		    tmpptr = tmpbuf+6;
		}
		else if (strncmp(tmpbuf, "userdataslave", 13) == 0) {
		    tmpptr = tmpbuf+12;
		    sysinfo.userdataoutput = 1;
		    /* we use userdataslave to index into the machinename array, 
		     * so we need to set it to the current slave numnber + 1 */
		    netinfo.userdataslave = currentslave + 1;
		}
	        if ((netinfo.nslaves == 0) || (currentslave == netinfo.nslaves)) {
		    fprintf(STATUSFILE, "spike_main: Error in slave section of  %s\n", configfilename);
		    return -1; 
                }
		while (isspace(*tmpptr)) tmpptr++;
	    	sscanf(tmpptr, "%s %hd", tmp, netinfo.slaveport+currentslave);
                strcpy(netinfo.slavename[currentslave], tmp);
		tmpnum = GetMachineNum(netinfo.slavename[currentslave]);
                strcpy(netinfo.machinename[tmpnum], tmp);
		sysinfo.system_type[tmpnum] = SLAVE;
		currentslave++;
            }
	    else if (strncmp(tmpbuf, "port", 4) == 0) {
	        tmpptr = tmpbuf+4;
		/* this specifies the next port to be used */
		if (sscanf(tmpptr, "%hd",  netinfo.port + currentport) != 1) {
		    fprintf(STATUSFILE, "spike_main: Warning: error in config file %s at line\n%s", configfilename, tmpbuf);
		}
		/* check that the port is in the acceptable range */
		if ((netinfo.port[currentport] > 0) &&  
		    (netinfo.port[currentport] < 1025)) {
		    fprintf(STATUSFILE, "spike_main: Warning, error in network config file %s at line\n%s\n\tport number %d is reserved", configfilename, tmpbuf, netinfo.port[currentport]);
		}
		currentport++;
	    }
	    else if (strncmp(tmpbuf, "socket", 6) == 0) {
		/* this should be a line specifying a connection, so the first
		 * string is the source of the connection */
		/* allocate space for the connection */
		if (sscanf(tmpbuf, "%s%s%s%s%s%s%hd", 
			   netinfo.conn[currentconn].from, tmp1, 
		           netinfo.conn[currentconn].to, tmp2, tmp3, tmp4,
			   &netinfo.conn[currentconn].port) != 7) {
		    fprintf(STATUSFILE, "spike_main: Warning, error in config file %s at line\n%s", configfilename, tmpbuf);
		    return -1;
		}
		/* set the source and destination module ids */
		netinfo.conn[currentconn].fromid = GetModuleID(tmp1);
		netinfo.conn[currentconn].toid = GetModuleID(tmp2);

		/* set the connection type */
		if (strcmp(tmp3, "MESSAGE") == 0) {
		    netinfo.conn[currentconn].type = MESSAGE;
		}
		else if (strcmp(tmp3, "DATA") == 0) {
		    netinfo.conn[currentconn].type = DATA;
		}
		else  {
		    fprintf(STATUSFILE, "spike_config: Warning, error in config file %s at line\n%s", configfilename, tmpbuf);
		}
		//fprintf(STATUSFILE, "%stype -%s-, %d\n", tmpbuf, tmp3, netinfo.conn[currentconn].type);
		//fprintf(STATUSFILE, "data? %d\n", (strcmp(tmp3, "DATA") == 0));
		/* set the protocol type */
		if (strcmp(tmp4, "UDP") == 0) {
		    netinfo.conn[currentconn].protocol = UDP;
		}
		else if (strcmp(tmp4, "TCPIP") == 0) {
		    netinfo.conn[currentconn].protocol = TCPIP;
		}
		else if (strcmp(tmp4, "UNIX") == 0) {
		    netinfo.conn[currentconn].protocol = UNIX;
		    /* for UNIX sockets we need to set a name for the
		     * connection */
		    if (netinfo.conn[currentconn].type == MESSAGE) {
			sprintf(netinfo.conn[currentconn].name, 
				"/tmp/spike_message_%d_to_%d",
				netinfo.conn[currentconn].fromid, 
				netinfo.conn[currentconn].toid);
		    }
		    else if (netinfo.conn[currentconn].type == DATA) {
			sprintf(netinfo.conn[currentconn].name, 
				"/tmp/spike_data_%d_to_%d",
				netinfo.conn[currentconn].fromid, 
				netinfo.conn[currentconn].toid);
		    }
		    //fprintf(STATUSFILE, "\n%s\n", netinfo.conn[currentconn].name);
		}
		else  {
		    fprintf(STATUSFILE, "spike_config: Warning, error in network config file %s at line\n%s", configfilename, tmpbuf);

		}
		currentconn++;
	    }
	    else if (strncmp(tmp, "myhostname", 10) == 0) {
		/* this is only written out for data files */
                sscanf(tmp+10, "%s", sysinfo.myhostname);
		/* we set the machine number to 0 as we are only dealing with 
		 * data from one machine */
		sysinfo.machinenum = 0;
            }
	    else if (strncmp(tmp, "dspclock", 8) == 0) {
		/* this determines whether we use the internal or external
		 * clock signal */
                sscanf(tmp+8, "%d", &sysinfo.dspclock);
		if (!sysinfo.dspclock) {
		    fprintf(stderr, "********************************************\n");
		    fprintf(stderr, "**WARNING: USING INTERNAL CLOCK CARD CLOCK**\n");
		    fprintf(stderr, "********************************************\n");
		}
            }
	    else if (strncmp(tmp, "datatype", 8) == 0) {
		tmp += 9;
		/* get the name and index of this computer */
		/* check to see if this line refers to this computer */
		while (isspace(*tmp)) tmp++;
		/* get the name of the machine for this line */
		sscanf(tmp, "%s", tmpname);
		/* get the index for this machine */
		if (!datafile) {
		    tmpint[0] = GetMachineNum(tmpname);
		}
		else {
		    /* this should be the only listing in the file, so we using
		     * the 0 index for everything */
		    tmpint[0] = 0;
		}
		tmp += strlen(tmpname);
		sysinfo.datatype[tmpint[0]] = 0;
		while (*tmp != '\0') {
		    while (isspace(*tmp)) tmp++;
		    if (*tmp == '\0') break;
		    if (strncmp(tmp, "SPIKE", 5) == 0) {
			sysinfo.datatype[tmpint[0]] |= SPIKE;
			tmp+=5;
		    }
		    else if (strncmp(tmp, "POSITION", 8) == 0) {
			sysinfo.datatype[tmpint[0]] |= POSITION;
			tmp+=8;
		    }
		    else if (strncmp(tmp, "CONTINUOUS", 10) == 0) {
			sysinfo.datatype[tmpint[0]] |= CONTINUOUS;
			tmp+=10;
		    }
		    else if (strncmp(tmp, "DIGITALIO", 9) == 0) {
			sysinfo.datatype[tmpint[0]] |= DIGITALIO;
			tmp+=9;
		    }
		    else if (strncmp(tmp, "USERDATA", 8) == 0) {
			sysinfo.datatype[tmpint[0]] |= USERDATA;
			sysinfo.userdataoutput = 1;
			tmp+=8;
		    }
		    else {
			fprintf(stderr, "Warning: datatype %s unknown\n", tmp);
			break;
		    }
		}
		/* if this is my hostname, set the default datatype  */
		if (!datafile && (strcmp(tmpname, netinfo.myname) == 0)) {
		    sysinfo.defaultdatatype = sysinfo.datatype[tmpint[0]];
		}
	    }
	    else if (strncmp(tmp, "datadirectory", 13) == 0) {
		tmp += 13;
		/* get the name and index of this computer */
		/* check to see if this line refers to this computer */
		while (isspace(*tmp)) tmp++;
		/* get the name of the machine for this line */
		sscanf(tmp, "%s", tmpname);
		/* get the index for this machine */
		if (!datafile) {
		    tmpint[0] = GetMachineNum(tmpname);
		}
		else {
		    /* this should be the only listing in the file, so we using
		     * the 0 index for everything */
		    tmpint[0] = 0;
		}
		tmp += strlen(tmpname);
		/* get the directory name */
		while (isspace(*tmp)) tmp++;
		strcpy(sysinfo.datadir[tmpint[0]], tmp);
		/* find the first space in the name and terminate the string
		 * there */
		tmp = sysinfo.datadir[tmpint[0]];
		while (!isspace(*tmp)) tmp++;
		*tmp = '\0';
	    }
	    /*
	     * Color file
	     */
            else if (strncmp(tmp, "colorfile", 9) == 0) {
                sscanf(tmp+9, "%s", sysinfo.colorfilename);
            }
            /* 
             * Position Variables
             */
            else if (strncmp(tmp, "posinput", 8) == 0) {
		/* get the number of the position input that we should use */
                sscanf(tmp+8, "%d", tmpint);
                if (*tmpint >= 0) {
                    sysinfo.posinputnum =  *tmpint;
                }
	    }
            else if (strncmp(tmp, "trackdarkpixels", 15) == 0) {
                sscanf(tmp+15, "%d", tmpint);
                if ((*tmpint == 0) || (*tmpint == 1)) {
                    sysinfo.trackdarkpixels = (bool) *tmpint;
                }
                else {
                    fprintf(stderr, "Error in config file: trackdarkpixels must be 1 or 0, you gave %d\n", *tmpint);
                    sysinfo.trackdarkpixels = 0; // assume that we're tracking bright pixels if no valid setting
                }
            }
            else if (strncmp(tmp, "sendalltrackedpixels", 15) == 0) {
                sscanf(tmp+21, "%d", tmpint);
                if ((*tmpint == 0) || (*tmpint == 1)) {
                    sysinfo.sendalltrackedpixels = (bool) *tmpint;
                }
                else {
                    fprintf(stderr, "Error in config file: sendalltrackedpixels must be 1 or 0, you gave %d\n", *tmpint);
                    sysinfo.sendalltrackedpixels = 0; // assume that we're only sending the bright pixels to the user program
                }
            }
            else if (strncmp(tmp, "posthresh", 9) == 0) {
                sscanf(tmp+9, "%d", tmpint);
                if ((*tmpint >= 0) && (*tmpint < 256)) {
                    sysinfo.posthresh = (unsigned char) *tmpint;
                }
                else {
                    fprintf(stderr, "Error in config file: invalid posthresh %d\n", *tmpint);
                    sysinfo.posthresh = 255;
                }
            }
	    else if (strncmp(tmp, "posxflip", 8) == 0) {
                sscanf(tmp+8, "%d", tmpint);
                if ((*tmpint == 0) || (*tmpint == 1)) {
		    sysinfo.posxflip = (bool) *tmpint;
                }
                else {
                    fprintf(stderr, "Error in config file: invalid posxflip %d, must be 0 or 1\n", *tmpint);
                  sysinfo.posxflip = 0;
                }
            }
	    else if (strncmp(tmp, "posyflip", 8) == 0) {
                sscanf(tmp+8, "%d", tmpint);
                if ((*tmpint == 0) || (*tmpint == 1)) {
		    sysinfo.posyflip = (bool) *tmpint;
                }
                else {
                    fprintf(stderr, "Error in config file: invalid posyflip %d, must be 0 or 1\n", *tmpint);
                  sysinfo.posyflip = 0;
                }
            }
            else if (strncmp(tmp, "mpegquality", 11) == 0) {
                sscanf(tmp+11, "%d", &sysinfo.mpegquality);
		if ((sysinfo.mpegquality < 1) || (sysinfo.mpegquality > 99)) {
                    fprintf(stderr, "Error in config file: mpegquality %d must be between 1 and 99\n", sysinfo.mpegquality);
		    sysinfo.mpegquality = 75;
		}
            }
            else if (strncmp(tmp, "mpegslices", 10) == 0) {
                sscanf(tmp+10, "%d", &sysinfo.mpegslices);
		if ((sysinfo.mpegslices < 1) || 
			(sysinfo.mpegslices > MAX_MPEG_SLICES)) {
                    fprintf(stderr, "Error in config file: mpegslices %d must be between 1 and %d\n", sysinfo.mpegslices, MAX_MPEG_SLICES);
		    sysinfo.mpegslices = 2;
		}
            }
    else if (strncmp(tmp, "videocodec", 10) == 0) {
                sscanf(tmp+10, "%d", &sysinfo.videocodec);
		if ((sysinfo.videocodec < 1) || 
			(sysinfo.videocodec > MAX_CODEC)) {
        fprintf(stderr, "Error in config file: videocodec %d must be between 1 and %d\n", sysinfo.videocodec, MAX_CODEC);
		    sysinfo.videocodec = 1;
		}
            }
    else if (strncmp(tmp, "videogopsize", 12) == 0) {
                sscanf(tmp+12, "%d", &sysinfo.videogopsize);
		if ((sysinfo.videogopsize < 0) || 
			(sysinfo.videogopsize > MAX_GOPSIZE)) {
        fprintf(stderr, "Error in config file: videogopsize %d must be between 1 and %d\n", sysinfo.videogopsize, MAX_CODEC);
		    sysinfo.videogopsize = 0;
		}
            }
            /* 
             * Digital IO Variables
             */
            else if (strncmp(tmp, "ndioports", 9) == 0) {
                sscanf(tmp+9, "%d", tmpint);
		if ((*tmpint >= 0) && (*tmpint <= MAX_DIO_PORTS)) {
		    digioinfo.nports = *tmpint;
		}
		else {
		    fprintf(stderr, "Error in config file: invalid ndioports, must be between 0 and %d, setting to 2\n", MAX_DSP_DIO_PORTS);
		    digioinfo.nports = 2;
		}
		/* assume all the ports are output ports */
		for (i = 0; i < digioinfo.nports; i++) {
		    digioinfo.porttype[i] = OUTPUT_PORT;
		}
	    }
            else if (strncmp(tmp, "dioport", 7) == 0) {
                sscanf(tmp+7, "%d%s", tmpint, tmpname);
		if ((*tmpint >= 0) && (*tmpint < digioinfo.nports)) {
		    if (strncmp(tmpname, "input", 5) == 0) {
			digioinfo.porttype[*tmpint] = INPUT_PORT;
		    }
		    else if (strncmp(tmpname, "output", 6) == 0) {
			digioinfo.porttype[*tmpint] = OUTPUT_PORT;
		    }
		    else {
			fprintf(stderr, "Error in config file: invalid port type %s for port %d (must be input or output). Setting to output\n", tmpname, *tmpint);
			digioinfo.porttype[*tmpint] = 1;
		    }
		    if ((*tmpint + 1) > digioinfo.nports) {
			digioinfo.nports = *tmpint + 1;
		    }
		}
		else { 
		    fprintf(stderr, "Error in config file: invalid number for digital IO port (must be 0 <= n < %d\n",  digioinfo.nports);
		}
            }
            else if (strncmp(tmp, "nprograms", 9) == 0) {
                sscanf(tmp+9, "%d", tmpint);
                if (tmpint >= 0) {
                    digioinfo.nprograms = *tmpint;
                }
                else {
                    fprintf(stderr, "Error in config file: invalid number of digital IO programs %d\n", *tmpint);
                    digioinfo.nprograms = 0;
                }
            }
            else if (strncmp(tmp, "program", 7) == 0) {
                /* read in the program number and its name */
                sscanf(tmp+7, "%d", tmpint);
                /* to get the name, go past the number and any spaces and copy the rest
                 * of the string */
                tmpptr = tmp+7;
                while (isspace(*tmpptr) || isdigit(*tmpptr)) {
                    tmpptr++;
                }
                strcpy(digioinfo.progname[*tmpint], tmpptr);
                /* digioinfo.progname[*tmpint] now ends with a '\n', 
                 * so we need to get rid of that */
                if ((tmpptr = strchr(digioinfo.progname[*tmpint], '\n')) != NULL) {
                    *tmpptr = ' ';
                }
                /* now put a '&' at the end so that the program starts up in the
                 * background if there isn't one there already*/
                if (strchr(digioinfo.progname[*tmpint], '&') == NULL) {
                    strcat(digioinfo.progname[*tmpint], " &");
                    digioinfo.progname[*tmpint]
			[strlen(digioinfo.progname[*tmpint])+1] = '\0';
                }
            }
            else if (strncmp(tmp, "usergui", 7) == 0) {
		tmp += 7;
		while (isspace(*tmp)) tmp++;
		if (*tmp == '\0') break;
		strcpy(sysinfo.usergui, tmp);
	    }
            else if (strncmp(tmp, "enable_daq", 10) == 0) {
                /* read in the enable_daq command (0/1) */
                sscanf(tmp+10, "%d", tmpint);
                if (*tmpint >= 1) {
                    digioinfo.enable_DAQ_TO_USER = *tmpint;
                    for (i = 0; i < MAX_DSPS; i++)
                      sysinfo.daq_to_user.dsps[i] = 0;
                    for (i = 0; i < MAX_CHANNELS; i++)
                      sysinfo.daq_to_user.channels[i] = 0;
                    sysinfo.daq_to_user.is_enabled = 0;
                }
                else {
                    digioinfo.enable_DAQ_TO_USER = 0;
                }
            }
	    /* local references */
            else if (strncmp(tmp, "localref", 8) == 0) {
                sscanf(tmp+8, "%d", tmpint);
                if (tmpint > 0) {
                    sysinfo.localref = true;
                }
	    }
	    /* Matlab output */
            else if (strncmp(tmp, "userdata", 8) == 0) {
		tmp += 8;
		tmpbool = NULL;
		/* skip spaces until we get to the datatype string */
		tmpint[0] = 0;
		while (isspace(*tmp)) tmp++;
		if (strncmp(tmp, "SPIKE", 5) == 0) {
		    tmpbool = userdatainfo.spikeelect;
		    tmp+=6;
		    userdatainfo.sendspike = 1;
		}
		else if (strncmp(tmp, "CONTINUOUS", 10) == 0) {
		    tmpbool = userdatainfo.contelect;
		    tmp+=11;
		    userdatainfo.sendcont = 1;
		}
		if (tmpbool) {
		    /* it is a SPIKE or CONTINUOUS line */
		    if (sscanf(tmp, "%d", tmpint) != 1) {
			fprintf(stderr, "Error in config file: no electrode number on userdata line, ignoring\n");
		    }
		    else {
			/* set this electrode to be saved */
			tmpbool[*tmpint] = 1;
		    }
		    fprintf(stderr, "cont data chan %d\n", userdatainfo.contelect[*tmpint]);
		}
		else if (strncmp(tmp, "POSITION", 8) == 0) {
		    userdatainfo.sendpos = 1;
		}
		else if (strncmp(tmp, "DIGITALIO", 9) == 0) {
		    userdatainfo.senddigio = 1;
		}
		else {
		    fprintf(stderr, "Error in config file: no userdata datatype on line %s\n", tmpbuf);
		}
	    }
	    /* audio settings */
            else if (strncmp(tmp, "audio", 5) == 0) {
                sscanf(tmp+5, "%d%d%d%d", tmpint, tmpint+1, tmpint+2,tmpint+3);
		cdspinfo.audiogain[tmpint[0]] = tmpint[1];
		cdspinfo.audiocutoff[tmpint[0]] = tmpint[2];
		cdspinfo.audiodelay[tmpint[0]] = tmpint[3];
		/* check the settings */
		if (!CheckAudioSettings(tmpint[0])) {
		    fprintf(stderr, "Error in config file: audio settings for dac %d invalid, setting to defaults\n", tmpint[0]);
		    cdspinfo.audiogain[tmpint[0]] = DEFAULT_DSP_AUDIO_GAIN;
		    cdspinfo.audiocutoff[tmpint[0]] = DEFAULT_DSP_AUDIO_CUTOFF;
		    cdspinfo.audiodelay[tmpint[0]] = DEFAULT_DSP_AUDIO_DELAY;
		}
	    }
            /* 
             * Spike/Continuous Data Variables 
             */
            else if (strncmp(tmp, "dspinfo", 7) == 0) {
		/* parse the dspinfo string. The string should contain the dsp
		 * number, the sampling rate, a 0 or 1 to indicate whether the
		 * data should be processed for spikes and the name of the
		 * machine to which data are sent */
                sscanf(tmp+7, "%d%d%s", tmpint, tmpint+1, tmpname);
		sysinfo.dspinfo[tmpint[0]].samprate = tmpint[1];
		sysinfo.dspinfo[tmpint[0]].nsamp = 0;
		if (!datafile) {
		    sysinfo.dspinfo[tmpint[0]].machinenum = 
			GetMachineNum(tmpname);
		}
		else if (strcmp(tmpname, sysinfo.myhostname) == 0) {
		    /* this is a datafile, and this dsp communicates with the
		     * host that saved the data, so we set the machine number
		     * to 0 */
		    sysinfo.dspinfo[tmpint[0]].machinenum = 0;
		}
		else {
		    /* we can ignore this dsp, so we set the machinenum to -1*/
		    sysinfo.dspinfo[tmpint[0]].machinenum = -1;
		}
		if (tmpint[0] + 1 > sysinfo.ndsps) {
		    sysinfo.ndsps = tmpint[0] + 1;
		}
		/* check the sampling rate */
		if (round(DSP_BASE_SAMP_RATE / tmpint[1]) != 
		         (DSP_BASE_SAMP_RATE / tmpint[1])) {
		    fprintf(stderr, "Error in sampling rate %d for dsp %d\nSampling rate must be divisor of base rate (30000 Hz)\n", tmpint[1], tmpint[0]);
		    exit(-1);
		}
            }
            else if (strncmp(tmp, "dspsampperpacket", 16) == 0) {
                sscanf(tmp+16, "%d%d", tmpint, tmpint+1);
		sysinfo.dspinfo[tmpint[0]].nsamp = tmpint[1];
	    }
            else if (strncmp(tmp, "rtmode", 4) == 0) {
                sscanf(tmp+4, "%d", tmpint);
		sysinfo.rtmode = (bool) *tmpint;
	    }
            else if (strncmp(tmp, "calibrationfile", 15) == 0) {
		sscanf(tmp+15, "%s", sysinfo.calibrationfile);
		/* Read in the calibration file */
		if ((!datafile) && 
			(!ReadCalibrationFile(sysinfo.calibrationfile))) {
		    fprintf(stderr, "Error reading calibration file, exiting\n");
		    exit(-1);
		}
	    }

            else if (strncmp(tmp, "electmap", 8) == 0) {
                sscanf(tmp+8, "%d%d%d", tmpint, tmpint+1, tmpshort);
		sysinfo.electinfo[tmpint[0]].dspchan[tmpint[1]] = tmpshort[0];
		sysinfo.electinfo[tmpint[0]].nchan++;
		sysinfo.electinfo[tmpint[0]].number = tmpint[0];
            }

            else if (strncmp(tmp, "hostname", 8) == 0) {
                /* read in the hostname for all subsequent channels*/
		tmp += 8;
		while (isspace(*tmp)) tmp++;
                sscanf(tmp, "%s", tmpname);
		if (datafile || strcmp(sysinfo.myhostname, tmpname) == 0) {
		    /* this only occurs in data files */
		    currentmachine = 0;
		}
		else {
		    /* get the index for this host from the network information
		     * structure*/
		    currentmachine = GetMachineNum(tmpname);
		}
		/* if this machine is the current machine, set the machine number */
		if (strcmp(tmpname, netinfo.myname) == 0) {
		    sysinfo.machinenum = currentmachine;
		}
            }
            else if ((strncmp(tmp, "channel", 7) == 0) || 
                     (strncmp(tmp, "trace", 5) == 0)) {
		if (currentmachine == -1) {
		    fprintf(stderr, "Error in config file: you must speciry a hostname before starting to list channel numbers\n");
		      return -1;
		}
                /* read in the current channel for subsequent reads of channel parameters */
		if (strncmp(tmp, "channel", 7) == 0) {
		    sscanf(tmp+7, "%d", &currentchannel);
		}
		else {
		    sscanf(tmp+5, "%d", &currentchannel);
		}
	    
                if ((currentchannel < 0) || 
                    (currentchannel >= MAX_CHANNELS)) {
                      fprintf(stderr, "Error in config file: invalid channel numer %d > MAX_CHANNELS\n", currentchannel);
                    return -1;
                }
		sysinfo.nchannels[currentmachine]++;
		sysinfo.channelinfo[currentmachine][currentchannel].machinenum =
		    currentmachine;
		sysinfo.channelinfo[currentmachine][currentchannel].index =
		    currentchannel;
            }
            else if (strncmp(tmp, "number", 6) == 0) {
                /* read in the number for this tetrode */
                sscanf(tmp+6, "%hd", &sysinfo.channelinfo[currentmachine][currentchannel].number);
            }
            else if (strncmp(tmp, "electchan", 9) == 0) {
                /* read in the channel number within the electrode */
                sscanf(tmp+9, "%hd", &sysinfo.channelinfo[currentmachine][currentchannel].electchan);
		if ((sysinfo.channelinfo[currentmachine][currentchannel].electchan >= NCHAN_PER_ELECTRODE) && (sysinfo.datatype[sysinfo.machinenum] == SPIKE)) {
                    fprintf(stderr, "Error in config file for channel %d: SPIKE model requires <= %d channels per electrode\n", currentchannel, NCHAN_PER_ELECTRODE);
		    return 0;
		}
            }
            else if (strncmp(tmp, "dspnum", 6) == 0) {
                /* read in the dsp for this channel */
                sscanf(tmp+6, "%hd", &sysinfo.channelinfo[currentmachine][currentchannel].dspnum);
            }
            else if (strncmp(tmp, "dspchan", 7) == 0) {
                /* read in the for this channel */
                sscanf(tmp+7, "%hd", &sysinfo.channelinfo[currentmachine][currentchannel].dspchan);
            }
            else if (strncmp(tmp, "depth", 5) == 0) {
                sscanf(tmp+5, "%hd", &sysinfo.channelinfo[currentmachine][currentchannel].depth);
            }
            else if (strncmp(tmp, "refelect", 8) == 0) {
                sscanf(tmp+9, "%hd", &sysinfo.channelinfo[currentmachine][currentchannel].refelect);
            }
            else if (strncmp(tmp, "refchan", 7) == 0) {
                sscanf(tmp+9, "%hd", &sysinfo.channelinfo[currentmachine][currentchannel].refchan);
            }
            else if (strncmp(tmp, "thresh", 6) == 0) {
                /* read in the threshold for the current channel */
                sscanf(tmp+6, "%d", tmpshort);
                if ((*tmpshort < MIN_THRESH) || (*tmpshort > MAX_THRESH)) {
                      fprintf(stderr, "Error in config file: invalid threshold for channel %d\n", currentchannel);
                    sysinfo.channelinfo[currentmachine][currentchannel].thresh = DEFAULT_THRESH;
                }
                else {
                    sysinfo.channelinfo[currentmachine][currentchannel].thresh = *tmpshort;
                }
            }
            else if (strncmp(tmp, "maxdispval", 10) == 0) {
                /* read in the maximum display value for the current channel */
                sscanf(tmp+10, "%d", tmpint);
                if ((*tmpint < 0) || (*tmpint > MAX_DISP_VAL)) {
                      fprintf(stderr, "Error in config file: invalid maxdatavle for channel %d\n", currentchannel);
                    sysinfo.channelinfo[currentmachine][currentchannel].maxdispval = 250;
                }
                else {
                    sysinfo.channelinfo[currentmachine][currentchannel].maxdispval = *tmpint;
                }
            }
            else if (strncmp(tmp, "filter", 6) == 0) {
                /* read in the filter settings for the current channel */
                sscanf(tmp+6, "%d%d", tmpshort, tmpshort+1);
                if (!datafile && !CheckFilters(tmpshort[0], tmpshort[1])) {
                      fprintf(stderr, "Error in config file: one or more invalid filter settings %d and %d for channel %d, setting to %d and %d\n", tmpshort[0], tmpshort[1], currentchannel, DEFAULT_LOW_FILTER, DEFAULT_HIGH_FILTER);
                    sysinfo.channelinfo[currentmachine][currentchannel].lowfilter = DEFAULT_LOW_FILTER;
                    sysinfo.channelinfo[currentmachine][currentchannel].highfilter = DEFAULT_HIGH_FILTER;
                }
                else { 
                    sysinfo.channelinfo[currentmachine][currentchannel].lowfilter = tmpshort[0];
                    sysinfo.channelinfo[currentmachine][currentchannel].highfilter = tmpshort[1];
                }
            }
            else if (strncmp(tmp, "color", 5) == 0) {
                /* read in the color settings for the current channel */
                sscanf(tmp+5, "%d", &sysinfo.channelinfo[currentmachine][currentchannel].color);
            }
            else if (strncmp(tmp, "eegtracelength", 14) == 0) {
                /* read in the desired length of each eeg trace */
                sscanf(tmp+14, "%f", &sysinfo.eegtracelength);
            }
            else {
                fprintf(stderr, "Error in config file: %s unknown\n", tmp);
            }
	}
	else if ((strncmp(tmpbuf, "%%ENDCONFIG", 11) == 0) ||
	      ((strncmp(tmpbuf, "%ENDHEADER", 10) == 0))) {
	    /* we're done */
	    break;
	}
    }

    
    /* set the total number of electrodes. On continuous systems, this is the
     * number of channels, while on spike systems this is the number of
     * electrodes in the electrode info structure */
    sysinfo.nelectrodes = 0;
    if (sysinfo.datatype[sysinfo.machinenum] & CONTINUOUS) {
	sysinfo.nelectrodes = sysinfo.nchannels[sysinfo.machinenum];
    }
    else if (sysinfo.datatype[sysinfo.machinenum] & SPIKE) {
	sysinfo.nelectrodes = sysinfo.nchannels[sysinfo.machinenum] / 
	    NCHAN_PER_ELECTRODE;
    }
    /* set the total number of electrodes.  To do so we find the largest index
     * of the electrode array that contains information */
    for (i = 0; i < MAX_ELECTRODES; i++) {
	/* find the entries with 1 or more channels */
	if (sysinfo.electinfo[i].nchan) {
	    sysinfo.maxelectnum = i;
	}
    }
    

//    fprintf(stderr, "post: nelect = %d, nchan = %d, maxelectnum = %d\n", 
//	    sysinfo.nelectrodes, sysinfo.nchannels[sysinfo.machinenum], 
//	    sysinfo.maxelectnum);

    netinfo.nconn = currentconn;
    netinfo.nports = currentport;
    /* if this is a slave, find the index of the slave that corresponds to 
     * this machine. This is ignored for the master */
    netinfo.myindex = GetSlaveNum(netinfo.myname);

    gzclose(config);
    return 1;
}


int WriteConfigFile(char *outfilename, int gzip, int datafile)
   /* writes out all of the parameters in sysconfig.  */
{
    int         i,j;
    int         currentchannel;
    gzFile      outfile;


    /* open the file */
    if (gzip) {
        /* open it with the default level of compression */
        if ((outfile = gzopen(outfilename, "w6")) == NULL) {
            return 0;
        }
    }
    else {
        /* open it with no compression */
        if ((outfile = gzopen(outfilename, "w0")) == NULL) {
            return 0;
        }
    }

    currentchannel = 0;
    gzprintf(outfile, "%%%%BEGINCONFIG\n");

    if (datafile) {
	/* write out the hostname for the extraction program and the machine
	 * number */
	gzprintf(outfile, "myhostname\t%s\n", netinfo.myname);
	/* clock source */
	gzprintf(outfile, "dspclock\t%d\n", sysinfo.dspclock);
	/* data type for this machine */
	i = sysinfo.machinenum;
	gzprintf(outfile, "datatype\t%s\t", netinfo.machinename[i]);
	if (sysinfo.datatype[i] & SPIKE) {
	    gzprintf(outfile, "SPIKE\t");
	}
	if (sysinfo.datatype[i] & CONTINUOUS) {
	    gzprintf(outfile, "CONTINUOUS\t");
	}
	if (sysinfo.datatype[i] & POSITION) {
	    gzprintf(outfile, "POSITION\t");
	}
	if (sysinfo.datatype[i] & DIGITALIO) {
	    gzprintf(outfile, "DIGITALIO\t");
	}
	if (sysinfo.datatype[i] & USERDATA) {
	    gzprintf(outfile, "USERDATA\t");
	}
	gzprintf(outfile, "\n");
	/* write out the default data directory */
	gzprintf(outfile, "datadirectory\t%s\t%s\n", netinfo.machinename[i], 
		sysinfo.datadir[i]);
    }
    else {
	/* write out the network configuration information */
	gzprintf(outfile, "nslaves\t%d\n", netinfo.nslaves);
	gzprintf(outfile, "master\t%s\t%d\n", netinfo.mastername, 
		netinfo.masterport);
	/* write out the ports for each slave */
	for (i = 0; i < netinfo.nslaves; i++) {
	    gzprintf(outfile, "slave\t%s\t%d\n", netinfo.slavename[i], 
		    netinfo.slaveport[i]);
	}
	/* write out the port numbers */
	for (i = 0; i < netinfo.nports; i++) {
	    gzprintf(outfile, "port\t%d\n", netinfo.port[i]);
	}


	/* write out all of the data types  */
	for (i = 0; i < netinfo.nmachines; i++) {
	    gzprintf(outfile, "datatype\t%s\t", netinfo.machinename[i]);
	    if (sysinfo.datatype[i] & SPIKE) {
		gzprintf(outfile, "SPIKE\t");
	    }
	    if (sysinfo.datatype[i] & CONTINUOUS) {
		gzprintf(outfile, "CONTINUOUS\t");
	    }
	    if (sysinfo.datatype[i] & POSITION) {
		gzprintf(outfile, "POSITION\t");
	    }
	    if (sysinfo.datatype[i] & DIGITALIO) {
		gzprintf(outfile, "DIGITALIO\t");
	    }
	    gzprintf(outfile, "\n");
	}
	/* write out the data directories */
	for (i = 0; i < netinfo.nmachines; i++) {
	    if (i != netinfo.rtslave) {
		gzprintf(outfile, "datadirectory\t%s\t%s\n", 
			netinfo.machinename[i], sysinfo.datadir[i]);
	    }
	}
	/* audio settings */
	gzprintf(outfile, "audio\t0\t%d\t%d\t%d\n", cdspinfo.audiogain[0],
		cdspinfo.audiocutoff[0], cdspinfo.audiodelay[0]);
	gzprintf(outfile, "audio\t1\t%d\t%d\t%d\n", cdspinfo.audiogain[1],
		cdspinfo.audiocutoff[1], cdspinfo.audiodelay[1]);

	/* clock source */
	gzprintf(outfile, "dspclock\t%d\n", sysinfo.dspclock);

	/* write out the userdata data */
	if (userdatainfo.sendpos) {
	    gzprintf(outfile, "userdata\tPOSITION\t");
	}
	if (userdatainfo.senddigio) {
	    gzprintf(outfile, "userdata\tDIGITALIO\t");
	}
	if (userdatainfo.sendcont) {
	    for (i = 1; i < MAX_ELECTRODE_NUMBER; i++) {
		if (userdatainfo.contelect[i]) {
		    gzprintf(outfile, "userdata\tCONTINUOUS\t%d", i);
		}
	    }
	}
	if (userdatainfo.sendspike) {
	    for (i = 1; i < MAX_ELECTRODE_NUMBER; i++) {
		if (userdatainfo.spikeelect[i]) {
		    gzprintf(outfile, "userdata\tSPIKE\t%d", i);
		}
	    }
	}
    }
    /* color file */
    gzprintf(outfile, "colorfile\t%s\n", sysinfo.colorfilename);

    /* position input */
    gzprintf(outfile, "posinput\t%d\n", sysinfo.posinputnum);
    /* track dark pixels */
    gzprintf(outfile, "trackdarkpixels\t%d\n", sysinfo.trackdarkpixels);

    gzprintf(outfile, "sendalltrackedpixels\t%d\n", sysinfo.sendalltrackedpixels);
    /* posthresh */
    gzprintf(outfile, "posthresh\t%d\n", sysinfo.posthresh);

    /* x and y flips */
    gzprintf(outfile, "posxflip\t%d\n", sysinfo.posxflip);
    gzprintf(outfile, "posyflip\t%d\n", sysinfo.posyflip);

    /* mpeg quality */
    gzprintf(outfile, "mpegquality\t%d\n", sysinfo.mpegquality);
    /* mpeg slices */
    gzprintf(outfile, "mpegslices\t%d\n", sysinfo.mpegslices);
    /* videocodec */
    gzprintf(outfile, "videocodec\t%d\n", sysinfo.videocodec);
    /* videogopsize */
    gzprintf(outfile, "videogopsize\t%d\n", sysinfo.videogopsize);
    /* Digital IO variables */
    gzprintf(outfile, "ndioports\t%d\n", digioinfo.nports);
    for (i = 0 ; i < digioinfo.nports; i++) {
	if (digioinfo.porttype[i] == INPUT_PORT) {
	    gzprintf(outfile, "dioport\t%d\tinput\n", i);
	}
	else {
	    gzprintf(outfile, "dioport\t%d\toutput\n", i);
	}
    }
    gzprintf(outfile, "nprograms\t%d\n", i, digioinfo.nprograms);

    for (i = 0; i < digioinfo.nprograms; i++) {
        gzprintf(outfile, "program\t%d\t%s\n", i, digioinfo.progname[i]);
    }
    if (strlen(sysinfo.usergui)) {
	gzprintf(outfile, "usergui\t%s\n", sysinfo.usergui);
    }

	gzprintf(outfile, "enable_daq\t%d\n", digioinfo.enable_DAQ_TO_USER);

    /* dsp info */
    for (i = 0; i < sysinfo.ndsps; i++) {
	gzprintf(outfile, "dspinfo\t%d\t%d\t%s\n", i, 
		sysinfo.dspinfo[i].samprate, 
		netinfo.machinename[sysinfo.dspinfo[i].machinenum]);
    }

    /* eeg trace length */
    gzprintf(outfile, "eegtracelength\t%f\n", sysinfo.eegtracelength);

    /* calibration file */
    gzprintf(outfile, "calibrationfile\t%s\n", sysinfo.calibrationfile);

    /* write out the electrode map */
    for (i = 0; i < MAX_ELECTRODES; i++) {
	if (sysinfo.electinfo[i].number != -1) {
	    for (j = 0; j < sysinfo.electinfo[i].nchan; j++) {
		gzprintf(outfile, "electmap\t%d\t%d\t%d\n", 
			sysinfo.electinfo[i].number, j,
			sysinfo.electinfo[i].dspchan[j]);
	    }
	}
    }

    if (datafile) {
	/* write out the channel information for this machine only */
        gzprintf(outfile, "hostname\t%s\n", sysinfo.myhostname); 
	i = sysinfo.machinenum;
	for (j = 0; j < sysinfo.nchannels[i]; j++) {
	    gzprintf(outfile, "\tchannel\t%d\n", j); 
	    gzprintf(outfile, "\t\tdspnum\t\t%d\n", sysinfo.channelinfo[i][j].dspnum); 
	    gzprintf(outfile, "\t\tdspchan\t%d\n", sysinfo.channelinfo[i][j].dspchan); 
	    gzprintf(outfile, "\t\tnumber\t%d\n", sysinfo.channelinfo[i][j].number); 
	    gzprintf(outfile, "\t\telectchan\t%d\n", sysinfo.channelinfo[i][j].electchan); 
	    gzprintf(outfile, "\t\tdepth\t%d\n", sysinfo.channelinfo[i][j].depth); 
	    gzprintf(outfile, "\t\trefelect\t%d\n", sysinfo.channelinfo[i][j].refelect); 
	    gzprintf(outfile, "\t\trefchan\t%d\n", sysinfo.channelinfo[i][j].refchan); 
	    gzprintf(outfile, "\t\tthresh\t%d\n", sysinfo.channelinfo[i][j].thresh); 
	    gzprintf(outfile, "\t\tmaxdispval\t%d\n", sysinfo.channelinfo[i][j].maxdispval); 
	    gzprintf(outfile, "\t\tfilter\t%d %d\n", sysinfo.channelinfo[i][j].lowfilter, 
						  sysinfo.channelinfo[i][j].highfilter);
	    gzprintf(outfile, "\t\tcolor\t%d\n", sysinfo.channelinfo[i][j].color);
	}               
    }
    else {
	for (i = 0; i < netinfo.nmachines; i++) {
	    if (i != netinfo.rtslave) {
		gzprintf(outfile, "hostname\t%s\n", netinfo.machinename[i]); 
		for (j = 0; j < sysinfo.nchannels[i]; j++) {
		    gzprintf(outfile, "\tchannel\t%d\n", j); 
		    gzprintf(outfile, "\t\tdspnum\t%d\n", sysinfo.channelinfo[i][j].dspnum); 
		    gzprintf(outfile, "\t\tdspchan\t\t%d\n", sysinfo.channelinfo[i][j].dspchan); 
		    gzprintf(outfile, "\t\tnumber\t\t%d\n", sysinfo.channelinfo[i][j].number); 
		    gzprintf(outfile, "\t\telectchan\t%d\n", sysinfo.channelinfo[i][j].electchan); 
		    gzprintf(outfile, "\t\tdepth\t\t%d\n", sysinfo.channelinfo[i][j].depth); 
		    gzprintf(outfile, "\t\trefelect\t%d\n", sysinfo.channelinfo[i][j].refelect); 
		    gzprintf(outfile, "\t\trefchan\t\t%d\n", sysinfo.channelinfo[i][j].refchan); 
		    gzprintf(outfile, "\t\tthresh\t\t%d\n", sysinfo.channelinfo[i][j].thresh); 
		    gzprintf(outfile, "\t\tmaxdispval\t%d\n", sysinfo.channelinfo[i][j].maxdispval); 
		    gzprintf(outfile, "\t\tfilter\t\t%d %d\n", sysinfo.channelinfo[i][j].lowfilter, 
							  sysinfo.channelinfo[i][j].highfilter);
		    gzprintf(outfile, "\t\tcolor\t\t%d\n", sysinfo.channelinfo[i][j].color);
		}               
	    }
	}
    }

    gzprintf(outfile, "%%%%ENDCONFIG\n");
     
    gzclose(outfile);
    return 1;
}


int ReadCalibrationFile(char *filename)
{
    int 	start, ch, cal;
    char	tmpbuf[200];
    FILE 	*infile;

    if ((infile = fopen(filename, "r")) == NULL) {
	fprintf(stderr, "Error opening %s for reading\n", filename);
	return 0;
    }
    while (fgets(tmpbuf, 200, infile) != NULL) {
	/* skip any white spaces */
        start = 0;
        while (isspace(tmpbuf[start]) && (tmpbuf[start] != '\0')) start++;
        if ((tmpbuf[start] != '%') && (tmpbuf[start] != '\0') && (tmpbuf[start] != '\n')) {
            /* Each non-commented line should have two numbers, a dsp channel
	     * from 0 to 127 and a calibration from -32767 to 32767 where
	     * 16384 is equalent to a multiplier of +1 and -16384 is equivalent
	     * to -1 */
	    sscanf(tmpbuf, "%d%d", &ch, &cal);
	    cdspinfo.dspcal[ch] = cal;
	}
    }
    return 1;
}


int CheckAudioSettings(int chan) 
{
    if ((cdspinfo.audiogain[chan] < 0) || (cdspinfo.audiogain[chan] > 
		MAX_DSP_AUDIO_GAIN)) {
	return 0;
    }
    if (cdspinfo.audiocutoff[chan] < 0) {
	return 0;
    }
    if ((cdspinfo.audiodelay[chan] < 0) || (cdspinfo.audiodelay[chan] > 
		MAX_DSP_AUDIO_DELAY)) {
	return 0;
    }
    return 1;
}

       
int CheckFilters(short low, short high)
/*     make sure the filter settings are in the list  */
{
    int i;
    int foundlow, foundhigh;

    foundlow = 0;
    for (i = 0; i < cdspinfo.nLowFilters; i++) {
        foundlow |= (cdspinfo.lowFilt[i] == low);
    }
    foundhigh = 0;
    for (i = 0; i < cdspinfo.nHighFilters; i++) {
        foundhigh |= (cdspinfo.highFilt[i] == high);
    }
    return (foundlow && foundhigh);
} 



int SetDSPInfo(void)
{
    /* go through the channelinfo structure and set up the dsp info structures
     * */
    int i, m;
    int dspelectnum, chnum;
    int lastelect, dsp;
    int sampmult;
    ChannelInfo *ch;
    DSPInfo *dptr;

    cdspinfo.mute = 0;
    cdspinfo.audiogain[0] = DEFAULT_DSP_AUDIO_GAIN;
    cdspinfo.audiogain[1] = DEFAULT_DSP_AUDIO_GAIN;

    for (m = 0; m <= netinfo.nslaves; m++) {
        lastelect = 0;
        ch = sysinfo.channelinfo[m];
        for (i = 0; i < sysinfo.nchannels[m]; i++, ch++) {
            dsp = ch->dspnum;
	    if (dsp == 0) {
		fprintf(stderr, "Error on channel %d, machine %d, dspnum cannot be 0\n", i, m);
		return 0;
	    }
            dptr = sysinfo.dspinfo + dsp;
            /* the index on the dsp is the current number of channels */
            ch->dspind = dptr->nchan;
            /* set the number of the dsp channel that this handles and
             * increment the total number of channels on this dsp */
	    dptr->channelinfochan[dptr->nchan] = ch->index;
            dptr->dspchan[dptr->nchan++] = ch->dspchan;
             /* check to see if we are on the same electrode as the previous
             * channel */
            if (ch->number != lastelect) {
                /* we have a new electrode */
                dspelectnum = dptr->nelectrodes++;
                lastelect = ch->number;
            }
            chnum = dptr->electinfo[dspelectnum].nchan;
            dptr->electinfo[dspelectnum].number = ch->number;
            dptr->electinfo[dspelectnum].dspchan[chnum] = ch->dspchan;
            dptr->electinfo[dspelectnum].channelinfochan[chnum] = ch->index;
            /* check to make sure that the dsp number is consistent with the
             * dsp number specified in the electmap part of the configuration
             * file */
            if (ch->dspchan != 
                    sysinfo.electinfo[ch->number].dspchan[ch->electchan]) {
                fprintf(stderr, "Error: inconsistent dsp channel %d and %d for electrode %d, channel %d\n", ch->dspchan, 
                        sysinfo.electinfo[ch->number].dspchan[ch->electchan], 
                        ch->number, ch->electchan);
                return 0;
            }
            dptr->electinfo[dspelectnum].nchan++;
            //fprintf(stderr, "dspinfo chan %d, number %d\n", i, ch->number);
            /* set the dsp reference channel for this channel */
            ch->dsprefchan = sysinfo.electinfo[ch->refelect].dspchan[ch->refchan];
        }
    }
    /* check that there are no channels for dsp 0 */
    if (sysinfo.dspinfo[0].nchan) {
	fprintf(stderr, "Error: dsp 0 cannot be used for data.  This error may be caused by a skipped trace/channel number in the config file\n");
	return 0;
    }

    /* calculate the number of samples that should be in each packet for each
     * DSP as well as the buffersize for continuous packets from spike_daq*/
    m = MAX_CONT_BUF_SIZE;
    for (i = 0; i < sysinfo.ndsps; i++) {
        dptr = sysinfo.dspinfo + i;
	/* first check the settings */
	if (dptr->nchan > MAX_CHAN_PER_DSP) {
	    fprintf(stderr, "Error: dsp %d is assigned %d channels.  The maximum number of channels per dsp is %d\n", i, dptr->nchan, MAX_CHAN_PER_DSP);
	    return 0;
	}
	if (i == 0) {
	    /* this is the master dsp, so it will put out digital IO packets */
	    dptr->packetshorts = DIO_BUF_STATIC_SIZE / sizeof(unsigned short);
	}
        if (dptr->nchan > 0) {
	    /* set the number of samples per packet if it was not specified in
	     * the config file */
	    if (!dptr->nsamp) {
		dptr->nsamp = (short) (MAX_PACKET_SIZE - 3 * 
			    sizeof(unsigned short)) / dptr->nchan - 1;
		if (sysinfo.rtmode) {
		    // set the number of samples to be the smaller of the
		    // default or a rate that will give us a packet every ms
		    dptr->nsamp = MIN(dptr->nsamp, dptr->samprate / 1000);
		}
	    }
	    else {
		/* check that the number of samples is reasonable */
		if (dptr->nsamp > (short) (MAX_PACKET_SIZE - 3 * 
			sizeof(unsigned short)) / dptr->nchan - 1) {
		    fprintf(stderr, "Error: dsp %d is assigned %d samples per packet.  The maximum number of samples for this dsp is %d\n", i, dptr->nsamp, 
		    (short) (MAX_PACKET_SIZE - 3 * sizeof(unsigned short)) / 
		    dptr->nchan - 1);
		    return 0;
		}
	    }
            /* aim for NCONT_BUF_PER_SEC continuous buffers per second */
            sampmult = (dptr->samprate / NCONT_BUF_PER_SEC) / dptr->nsamp;
            if ((sampmult > 1) && (!sysinfo.rtmode)) {
                dptr->nsampout = dptr->nsamp * sampmult;
            }
            else {
                dptr->nsampout = dptr->nsamp;
            }
	    if (dptr->nsampout < m) {
		m = dptr->nsampout;
	    }
	    /* set the packet size */
	    /* each packet contains 
	     * 1 unsigned short Pipe_ID, an unsigned 
	     * 1 unsigned int sample count
	     * nchan * nsamp unsigned short data points */
	    dptr->ntotalsamp = dptr->nsamp * dptr->nchan;
	    dptr->packetshorts = 3 + dptr->ntotalsamp;
	    dptr->datasize = dptr->nsamp * dptr->nchan * sizeof(unsigned short);
	    dptr->packetsize = dptr->packetshorts * sizeof(unsigned short);
        }
    }
#ifdef NO_DSP_DEBUG
    /* if we are debugging, we need the total number of output samples to be
     * the same for each dsp */
    for (i = 0; i < sysinfo.ndsps; i++) {
        dptr = sysinfo.dspinfo + i;
        if (dptr->nchan > 0) {
	    dptr->nsampout = m;
	}
    }
#endif

    return 1;
}
