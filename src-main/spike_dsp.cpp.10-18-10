/*
 * spike_dsp.cpp: functions for setting up the dsp related structures and for
 * writing to and reading from the DSPs.  Compiled into nspike.
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
#include "spike_main.h"

#include <string>
#include <sstream>

extern SysInfo sysinfo;
extern NetworkInfo netinfo;
extern CommonDSPInfo cdspinfo;
extern DigIOInfo digioinfo;

extern SocketInfo         server_message[MAX_CONNECTIONS];
extern SocketInfo         client_message[MAX_CONNECTIONS];
extern SocketInfo         server_data[MAX_CONNECTIONS];
extern SocketInfo         client_data[MAX_CONNECTIONS]; 

extern char tmpstring[200];
char tmpbuf[SAVE_BUF_SIZE];


/* local functions */
int ProgramAuxDSP(short dspnum);
int ProgramMasterDSP(void);
int ReprogramDSP(int dspnum, unsigned short *program);
unsigned short GetDSPCodeRev(short dspnum);
int ReadDSPProgram(char *filename, unsigned short *program);
int GetLocalDSPCodeRev(unsigned short *tmpcoderev);
void GetDSPLowFilterCoeff(short highpass, unsigned short *data);
void GetDSPHighFilterCoeff(short lowpass, unsigned short *data);
void ByteSwap(unsigned short *sptr, int nelem);
void ByteSwap(long *lptr, int nelem);


    
int ProgramAllDSPS(void)
   /*  This should be initiated by a message from the master machine
    * (or this machine if this is the master )  */
{
    int         i, ret; 

    if (sysinfo.system_type[sysinfo.machinenum] == MASTER) {
        /* Program the DSPS on each of the slave machines */
        for(i = 0; i < netinfo.nslaves; i++) {
	    /* send this only to the non-rt slaves (netinfo.rtslave - 1 is the
	     * index in slavefd of the rt slave */
	    if (i != netinfo.rtslave - 1) {
		SendMessage(netinfo.slavefd[i], PROGRAM_DSPS, NULL, 0);
		/* Wait for the reply */
		if (!WaitForMessage(netinfo.masterfd[i], DSPS_PROGRAMMED, 10)) {
		    sprintf(tmpstring,"spike_main: no response to START_ACQUISITON message from slave %s", netinfo.slavename[i]);
		    DisplayStatusMessage(tmpstring);
		    return 0;
		}
	    }
        }
        ret = ProgramLocalDSPS();
    }
    else {
        ret = ProgramLocalDSPS();
        SendMessage(netinfo.slavefd[netinfo.myindex], DSPS_PROGRAMMED, NULL, 0);
    }
    return  ret;
}

int ProgramLocalDSPS(void)
    /* go through the dsps that send data to this machine and program them */
{
    int         i, error;
    DSPInfo         *dptr;

    error = 0;
    for (i = 0; i < sysinfo.ndsps; i++) {
        dptr = sysinfo.dspinfo + i;
        if (dptr->machinenum == sysinfo.machinenum) {
            if (i == 0) {
                /* this is the master DSP */
                if ((error = ProgramMasterDSP()) == -1) {
                    fprintf(stderr, "Error programming Master DSP, exiting\n");
                }
            }
            else {
                if ((dptr->nchan > 0) && 
			(error = ProgramAuxDSP(i)) == -1) {
                    fprintf(stderr, "Error programming Aux DSP %d, exiting\n", i);
                }
            }
        }
    }
    if (error) {
        return 0;
    }
    return 1;
}

int ProgramMasterDSP(void)
    /* this programming MUST take place before acquisition has been started */
    /* note that we won't program the audio channels until the user selects a channel */
{
    unsigned short data[24];
    int i, error = 0;
    
    fprintf(stderr, "Programming Master DSP\n");
    /* set up the analog outputs */
    for (i = 0; i < NDSP_AUDIO_CHANNELS; i++) {
	SetDSPAudioGain(i, cdspinfo.audiogain[i]);
        SetDSPAudioCutoff(i, cdspinfo.audiocutoff[i]);
        SetDSPAudioDelay(i, cdspinfo.audiodelay[i]);
    }

    /* if we do digital IO, set up the ports and the state machine */
    if (sysinfo.datatype[sysinfo.machinenum] & DIGITALIO) {
	/* first set up the ports for input or output */
	data[0] = 0;
	for (i = 0; i < digioinfo.nports; i++) {
	    if (digioinfo.porttype[i] == OUTPUT_PORT) {
		data[0] |= 1 << i;
	    }
	}
	if (!WriteDSPData(0, DIO_OUT_ENABLE, 1, data)) {
	    sprintf(tmpstring, "Error writing Digital IO port information to master DSP");
	    DisplayErrorMessage(tmpstring);
	    error = 1;
	}

	/* Set all of the output port bits to be 0 */ 
	data[0] = 0;
	for (i = 0; i < digioinfo.nports; i++) {
	    if (digioinfo.porttype[i] == OUTPUT_PORT) {
		if (!WriteDSPData(0, DIO_OUT_1 + i, 1, data)) {
		    sprintf(tmpstring, "Error writing Digital IO port status to master DSP");
		    DisplayErrorMessage(tmpstring);
		    error = 1;
		}
	    }
	}
	
	/* Set the mask for all of the ports to be 1's so that all changes are
	 * reported on the used ports, and set the mask to be all zeron if there are 
	 * unused ports*/
	for (i = 0; i < MAX_DIO_PORTS; i++) {
	    if (i < digioinfo.nports) {
		data[0] = 0xFFFF;
	    }
	    else {
		data[1] = 0;
	    }
	    if (!WriteDSPData(0, DIO_IN_1_MASK + i, 1, data)) {
		sprintf(tmpstring, "Error writing Digital IO port mask to master DSP");
		DisplayErrorMessage(tmpstring);
		error = 1;
	    }
	}

	/* Set the debounce time for the inputs to be 0 ms  */
	data[0] = 0;
	if (!WriteDSPData(0, DIO_IN_DEBOUNCE_TIME, 1, data)) {
	    sprintf(tmpstring, "Error writing Digital IO input debounce time to master DSP");
	    DisplayErrorMessage(tmpstring);
	    error = 1;
	}

	/* Enable the state machine */
	data[0] = 1;
	if (!WriteDSPData(0, DIO_STATE_ENABLE, 1, data)) {
	    sprintf(tmpstring, "Error writing Digital IO state machine enable to master DSP");
	    DisplayErrorMessage(tmpstring);
	    error = 1;
	}
    }

    if (error) {
        return 0;
    }
    else {
        return 1;
    }
}

int ProgramAuxDSP(short dspnum)
    /* this programming should take place before acquisition has been started */
{
    unsigned short data[24];
    int         i, error = 0;
    DSPInfo         *dptr;
    ChannelInfo        *ch;


    fprintf(stderr, "Programming Aux DSP %d\n", dspnum);
    dptr = sysinfo.dspinfo + dspnum;

    /* set the number of blocks to send to 0 */
    StopDSPAcquisition(dspnum);

    /* set the PipeID */
    data[0] = dspnum;
    WriteDSPData(dspnum, PIPE_ID_ADDR, 1, data);

    /* set the number of channels per sample */
    data[0] = (unsigned short) dptr->nchan;
    WriteDSPData(dspnum, NUM_CHAN_ADDR, 1, data);

    /* set the number of samples per block */
    data[0] = dptr->nsamp;
    WriteDSPData(dspnum, NUM_SAMPLES_ADDR, 1, data);

    /* set the decimation factor */
    data[0] = DSP_BASE_SAMP_RATE / dptr->samprate;
    WriteDSPData(dspnum, DECIMATION_ADDR, 1, data);

    /* set up all of the channels */
    ch = sysinfo.channelinfo[sysinfo.machinenum];
    for (i = 0; i < sysinfo.nchannels[sysinfo.machinenum]; i++, ch++) {
        if (ch->dspnum == dspnum) {
            /* this is the next channel on this dsp */
            if (!UpdateChannel(ch, 0)) {
                sprintf(tmpstring, "Error programming DSP %d channel %d, electrode %d\n", ch->dspnum, ch->dspchan, ch->number);
                DisplayErrorMessage(tmpstring);
                error = 1;
            }
        }
    }
    if (error) {
        return 0;
    }
    else {
        return 1;
    }
}

int StartLocalDSPAcquisition(void)
{
    int i, error = 0;
    /* now start output on the Aux DSPS */
    for (i = 1; i < sysinfo.ndsps; i++) {
	if ((sysinfo.dspinfo[i].nchan > 0) &&
	    (sysinfo.dspinfo[i].machinenum == sysinfo.machinenum)) {
	    if (!StartDSPAcquisition((unsigned short) i)) {
		//fprintf(STATUSFILE, "Error starting acquisition on dsp %d\n", i);
		error = 1;
	    }
	    else {
		//fprintf(STATUSFILE, "Started acquisition on dsp %d\n", i);
	    }
	}
    }
    if (!error) {
	sysinfo.dspacq = 1;
    }
    return error;
}

int StopLocalDSPAcquisition(void)
{
    int i, error = 0;
    /* stop output on the Aux DSPS */
    for (i = 1; i < sysinfo.ndsps; i++) {
	if ((sysinfo.dspinfo[i].nchan > 0) &&
	    (sysinfo.dspinfo[i].machinenum == sysinfo.machinenum)) {
	    if (!StopDSPAcquisition((unsigned short) i)) {
		//fprintf(STATUSFILE, "Error stopping acquisition on dsp %d\n", i);
		error = 1;
	    }
	    else {
		//fprintf(STATUSFILE, "Stopped acquisition on dsp %d\n", i);
	    }
	}
    }
    if (!error) {
	sysinfo.dspacq = 0;
    }
    return error;
}

int StartDSPAcquisition(short dspnum)
{
    unsigned short data[1];

    /* set to a number > SHRT_MAX to tell the dsp to send packets forever */
    data[0] = USHRT_MAX;
    if (!WriteDSPData(dspnum, BLOCKS_TO_SEND_ADDR, 1, data)) {
	return 0;
    }
    else {
	return 1;
    }
}


int StopDSPAcquisition(short dspnum)
{
    unsigned short data[1];

    /* set to 0 to indicate that it should stop sending packets */
    data[0] = 0;
    if (!WriteDSPData(dspnum, BLOCKS_TO_SEND_ADDR, 1, data)) {
	return 0;
    }
    else {
	return 1;
    }
}



int SetAudio(int num, ChannelInfo *ch, int commandorigin)
{
    int i;
    int tmpbuf[1000];
    unsigned short data[24];
    /* the audio message must be sent to the master dsp so we check if we are
     * the master, and if not, we send the message on */
    /* commandorigin is 1 if we initiated this and 2 if we recieved it  */

    /* First check to see if we've already set the audio correctly */
    if (memcmp(ch, cdspinfo.audiochan + num, sizeof(ChannelInfo)) == 0) {
         return 1;
    }

    memcpy(cdspinfo.audiochan + num, ch, sizeof(ChannelInfo));

    tmpbuf[0] = num;
    memcpy(tmpbuf + 1, ch, sizeof(ChannelInfo));
    if (sysinfo.system_type[sysinfo.machinenum] == MASTER) {
        /* Send the update message to the slaves. This ends up sending the
         * message back to the slave that originated it but the extra cpu usage 
         * is minimal */
        for(i = 0; i < netinfo.nslaves; i++) {
            SendMessage(netinfo.slavefd[i], SET_AUDIO,
                    (char *) tmpbuf, sizeof(int) + sizeof(ChannelInfo));
        }
        // Send message to DSP
        /* create the array for programming the channel */
        data[0] = ch->dspchan;
        data[1] = cdspinfo.dspcal[ch->dspchan];
	if (ch->refelect == 0) {
	    /* reference to ground */
	    data[2] = 0;
	    data[3] = 0;
	}
	else {
	    data[2] = ch->dsprefchan;
	    data[3] = -1 * cdspinfo.dspcal[ch->dsprefchan];
	}
        GetDSPHighFilterCoeff(ch->highfilter, data+4);
        GetDSPLowFilterCoeff(ch->lowfilter, data+14);
        WriteDSPData(0, CHANNEL_SETTINGS_ADDR + DSP_CHAN_ADDR_INC * num, 24, 
                data);
    }
    else if (commandorigin) {
        /* send the message to the master */
         SendMessage(netinfo.slavefd[netinfo.myindex], SET_AUDIO, 
                 (char *) tmpbuf, sizeof(ChannelInfo) + sizeof(int));
    }
    return 1;
}


void UpdateTetrodeLowFilt(int elect, short newval) 
    /* Update the low filter setting for this tetrode */
{
    int i, chan, set = 1, dspacq = 0;
    ChannelInfo *ch;
    chan = elect * NCHAN_PER_ELECTRODE;

    ch = sysinfo.channelinfo[sysinfo.machinenum] + chan;

    /* this routine can get called multiple times for the same tetrode, so we check to see if all 
     * values are already set */
    for (i = 0; i < NCHAN_PER_ELECTRODE; i++, ch++) {
	if (ch->lowfilter != newval) {
	    set = 0;
	    break;
	}
    }

    if (!set) {
	/* stop DSP acquisition if it is on */
	if (sysinfo.dspacq) {
	    StopLocalAcq();
	    dspacq = 1;
	}
	for (i = 0; i < NCHAN_PER_ELECTRODE; i++, chan++) {
	    UpdateChanLowFilt(chan, newval);
	}
	if (dspacq) {
	    StartLocalAcq();
	}
    }
    return;
}

int UpdateChanLowFilt(int chan, short newval)
{
    ChannelInfo *ch;

    ch = sysinfo.channelinfo[sysinfo.machinenum] + chan;
    /* Send an update message to the dsp and wait for a reply */
    ch->lowfilter = newval;
    /* update this channel on all machines */
    UpdateChannel(ch, 0);
    return 1;
}

void UpdateTetrodeHighFilt(int elect, short newval) 
    /* Update the high filter setting for this tetrode */
{
    int i, chan, set = 1, dspacq = 0;
    ChannelInfo *ch;

    chan = elect * NCHAN_PER_ELECTRODE;

    ch = sysinfo.channelinfo[sysinfo.machinenum] + chan;

    /* this routine can get called multiple times for the same tetrode, so we check to see if all 
     * values are already set */
    for (i = 0; i < NCHAN_PER_ELECTRODE; i++, ch++) {
	if (ch->highfilter != newval) {
	    set = 0;
	    break;
	}
    }

    if (!set) {
	/* stop DSP acquisition if it is on */
	if (sysinfo.dspacq) {
	    StopLocalAcq();
	    dspacq = 1;
	}
	for (i = 0; i < NCHAN_PER_ELECTRODE; i++, chan++) {
	    UpdateChanHighFilt(chan, newval);
	}
	if (dspacq) {
	    StartLocalAcq();
	}
    }
    return;
}

int UpdateChanHighFilt(int chan, short newval)
{
    ChannelInfo *ch;

    ch = sysinfo.channelinfo[sysinfo.machinenum] + chan;
    /* Send an update message to the dsp and wait for a reply */
    ch->highfilter = newval;
    /* update this channel on all machines */
    UpdateChannel(ch, 0);
    return 1;
}

int UpdateChannel(ChannelInfo *ch, int local)
    /* Reprograms the DSPs for this channel and sends the channel information
     * out if local == 0 */
{
    unsigned short         data[24];
    int 		   i;

    /* look up the dsp channel number for the specified reference channel */
    ch->dsprefchan = sysinfo.electinfo[ch->refelect].dspchan[ch->refchan];

    // Send message to DSP
    /* create the array for programming the channel */
    data[0] = ch->dspchan;
    data[1] = cdspinfo.dspcal[ch->dspchan];
    if (ch->refelect != 0) {
        data[2] = ch->dsprefchan;
        data[3] = -1 * cdspinfo.dspcal[ch->dsprefchan];
    }
    else {
        /* use channel 0 with a gain of 0 to reference to ground */
        data[2] = 0;
        data[3] = 0;
    }
    GetDSPHighFilterCoeff(ch->highfilter, data+4);
    GetDSPLowFilterCoeff(ch->lowfilter, data+14);

    WriteDSPData(ch->dspnum, CHANNEL_SETTINGS_ADDR + DSP_CHAN_ADDR_INC * 
            ch->dspind, 24, data);

    /* check to see if this channel is selected for audio output, and if so 
     * update the audio */
    if (sysinfo.defaultdatatype & SPIKE) {
	for (i = 0; i < NDSP_AUDIO_CHANNELS; i++) {
	    if (ch->dspchan == cdspinfo.audiochan[i].dspchan) {
		SetAudio(i, ch, 1);
	    }
	}
    }
    if (!local) {
	/* update this channel on all machines */
	SendChannelInfo(ch, netinfo.myindex);
    }
    return 1;
}

int SetDSPAudioGain(int channel, short gain)
{
    unsigned short address;
    if (channel == 0) {
	address = DAC_0_GAIN_ADDR;
    }
    else {
	address = DAC_1_GAIN_ADDR;
    }
    cdspinfo.audiogain[channel] = gain;
    /* the gains from 0 to 128 should be mapped to 0 to 32767 */
    gain = (gain < 128) ? (gain * 128) : 32767;
    WriteDSPData(0, address, 1, (unsigned short *) &gain);
    return 1;
}

int SetDSPAudioCutoff(int channel, short cutoff)
{
    unsigned short address1;
    unsigned short address2;

    if (channel == 0) {
	address1 = DAC_0_POS_THRESH_ADDR;
	address2 = DAC_0_NEG_THRESH_ADDR;
    }
    else {
	address1 = DAC_1_POS_THRESH_ADDR;
	address2 = DAC_1_NEG_THRESH_ADDR;
    }
    cdspinfo.audiocutoff[channel] = cutoff;
    WriteDSPData(0, address1, 1, (unsigned short *) &cutoff);
    cutoff = cutoff * -1;
    WriteDSPData(0, address2, 1, (unsigned short *) &cutoff);
    return 1;
}

int SetDSPAudioDelay(int channel, short delay)
{
    unsigned short address;

    if (channel == 0) {
	address = DAC_0_DELAY_ADDR;
    }
    else {
	address = DAC_1_DELAY_ADDR;
    }
    cdspinfo.audiodelay[channel] = delay;
    /* map the delay in ms to the number of samples */
    delay = delay * 30;
    WriteDSPData(0, address, 1, (unsigned short *) &delay);
    return 1;
}

int DSPMuteAudio(int mute)
{
    unsigned short gain = 0;
    unsigned short address;
    cdspinfo.mute = mute;
    fprintf(stderr, "mute = %d\n", mute);
    if (mute) {
	/* set the gain on both channels to 0 */
	address = DAC_0_GAIN_ADDR;
	WriteDSPData(0, address, 1, &gain);
	address = DAC_1_GAIN_ADDR;
	WriteDSPData(0, address, 1, &gain);
    }
    else {
	/* set the gains to their normal values */
	address = DAC_0_GAIN_ADDR;
	WriteDSPData(0, address, 1, (unsigned short *) &cdspinfo.audiogain[0]);
	address = DAC_1_GAIN_ADDR;
	WriteDSPData(0, address, 1, (unsigned short *) &cdspinfo.audiogain[1]);
    }
    return 1;
}

void ResetClock(void) 
{
    int error, acq;
    unsigned short data[1];
    struct timezone ctz;

    error = 0;
    if (!sysinfo.fileopen) {
	acq = 0;
	if (sysinfo.acq) {
	    acq = 1;
	    StopAllAcquisition();
	}
        data[0] = 0;
        WriteDSPData(0, SYNC_CONTROL_ADDR, 1, data);
        /* now send a message to the DSPs to reset their clocks */
        if (!error) {
            /* restart the counter and the 10 KHz outputs */
            data[0] = BIT0 | BIT1 | BIT2;
            WriteDSPData(0, SYNC_CONTROL_ADDR, 1, data);
            sprintf(tmpstring, "Resetting Clock");
            DisplayStatusMessage(tmpstring);
        }
	/* get the current time and send it to SPIKE_POSDAQ if appropriate*/
	gettimeofday(&sysinfo.computer_start_time, &ctz);
	if (sysinfo.datatype[sysinfo.machinenum] & POSITION) {
	    SendMessage(client_message[SPIKE_POSDAQ].fd, START_TIME, 
		    (char *) &sysinfo.computer_start_time, 
		    sizeof(struct timeval));
	}
	/* the dsps require about 3 seconds to correctly reset their clocks, 
	 * so we wait for 3 seconds. Note that this means the computer start
	 * time should always be AFTER the actual start time */
	sleep(3);
	sprintf(tmpstring, "Clock Reset");
	DisplayStatusMessage(tmpstring);

	if (acq) {
	    /* restart acquisition */
	    StartAllAcquisition();
	}
    }
    if (sysinfo.fileopen || error) {
        sprintf(tmpstring, "Error: cannot reset clock");
        DisplayStatusMessage(tmpstring);
    }
    /* reset the sysinfo counters */
    sysinfo.lastdisplayedtime = 0;    
    sysinfo.lastfilesizetime = 0;    	

}
    
u32 ReadDSPTimestamp(short dspnum)
    /* read the current sample count from the specified DSP. */
{

    unsigned short  	tmpcount[3];
    u32			count;

    if (dspnum > 0) {
	/* get the time from the specified Aux DSP */
	ReadDSPData(dspnum, SAMPLE_COUNT_LOW_ADDR, 2, tmpcount);
    }
    else {
	/* Read in the three words */
	ReadDSPData(dspnum, MASTER_SAMPLE_COUNT_LOW_ADDR, 3, tmpcount);
	/* the middle work is the sync control, so overwrite it */
	tmpcount[1] = tmpcount[2];
    }
    memcpy(&count, tmpcount, sizeof(u32));
    /* convert count to timestamp units and return*/
    return count / (u32) SAMP_TO_TIMESTAMP;
}



void GetDSPLowFilterCoeff(short highpass, unsigned short *data)
{
    /* data are as follows:
    * data[0]                 HPFa0lowval        
    * data[1]                 HPFa0highval        
    * data[2]                 HPFa1lowval        
    * data[3]                 HPFa1highval        
    * data[4]                 HPFa0lowval        
    * data[5]                 HPFa0highval        
    * data[6]                 HPFb2lowval        
    * data[7]                 HPFb2highval        
    * data[8]                 HPFb1lowval        
    * data[9]                 HPFb1highval        
    */

    double         d;                //damping factor
    double        alpha;
    double        alphaout, alphaoutx2;
    double        beta;
    double        nbetaout;
    double        gamma;
    double        gammaout;
    double        fsample;        // sampling rate
    double        thetac;                // sampling rate
    double 	  tmpmax;
    unsigned short        HPFa0loval;
    unsigned short         HPFa0hival;        
    unsigned short         HPFa1loval;        
    unsigned short         HPFa1hival;        
    unsigned short         HPFb2loval;        
    unsigned short         HPFb2hival;        
    unsigned short         HPFb1loval;        
    unsigned short         HPFb1hival;        

    if (highpass < 2) {
        memset(data, 0, 10 * sizeof(unsigned short));
        data[5] = 16384;
        return;
    }
    // set the sampling rate 
    fsample = DSP_BASE_SAMP_RATE;

    // get the cutoff frequency in radians
    thetac = TWOPI * (double) highpass / fsample;

    // dampling factor.  2.0 is for butterworth filters
    d = 2.0;


    // alpha, beta, and gamma are the three filter coefficients 
    // derived from theta_c
    beta = 0.5 * (1 - 0.5 * d * sin(thetac))/(1 + 0.5 * d * sin(thetac));

    gamma = (0.5 + beta) * cos(thetac);

    alpha = (0.5 + beta + gamma)/4;
    
    tmpmax = ::pow(2,32);
    /* convert the coefficients to signed longs */
    nbetaout = trunc(-beta * 32768 * 65536);
    if (nbetaout < 0) {
	nbetaout = tmpmax + nbetaout;
    }
    HPFb2hival = (unsigned short) floor(nbetaout / 65536);
    HPFb2loval = (unsigned short) (((u32) nbetaout) % 65536);

    gammaout = (int) trunc(gamma * 32768 * 65536);
    if (gammaout < 0) {
	gammaout = tmpmax + gammaout;
    }
    HPFb1hival = (unsigned short) floor(gammaout / 65536);
    HPFb1loval = (unsigned short) (((u32) gammaout) % 65536);

    alphaout = (int) trunc(alpha * 32768 * 65536);
    if (alphaout < 0) {
	alphaout = tmpmax + alphaout;
    }
    HPFa0hival = (unsigned short) floor(alphaout / 65536);
    HPFa0loval = (unsigned short) (((u32) alphaout) % 65536);

    alphaoutx2 = (int) trunc(-alpha * UINT_MAX);
    if (alphaoutx2 < 0) {
	alphaoutx2 = UINT_MAX + alphaoutx2;
    }
    HPFa1hival = (unsigned short) floor(alphaoutx2 / 65536);
    HPFa1loval = (unsigned short) (((u32) alphaoutx2) % 65536);


    data[0] = HPFa0loval;
    data[1] = HPFa0hival;
    data[2] = HPFa1loval;
    data[3] = HPFa1hival;
    data[4] = HPFa0loval;
    data[5] = HPFa0hival;
    data[6] = HPFb2loval;
    data[7] = HPFb2hival;
    data[8] = HPFb1loval;
    data[9] = HPFb1hival;

    return;
}


void GetDSPHighFilterCoeff(short lowpass, unsigned short *data)
{
    /* data are as follows:
    * data[0]                 LPFa0lowval        
    * data[1]                 LPFa0highval        
    * data[2]                 LPFa1lowval        
    * data[3]                 LPFa1highval        
    * data[4]                 LPFa0lowval        
    * data[5]                 LPFa0highval        
    * data[6]                 LPFb2lowval        
    * data[7]                 LPFb2highval        
    * data[8]                 LPFb1lowval        
    * data[9]                 LPFb1highval        
    */

    double         d;                //damping factor
    double        alpha;
    double	  alphaout, alphaoutx2;
    double        beta;
    double	  nbetaout;
    double        gamma;
    double	  gammaout;
    double        fsample;        // sampling rate
    double        thetac;                // sampling rate
    double	  tmpmax;
    unsigned short        LPFa0loval;
    unsigned short         LPFa0hival;        
    unsigned short         LPFa1loval;        
    unsigned short         LPFa1hival;        
    unsigned short         LPFb2loval;        
    unsigned short         LPFb2hival;        
    unsigned short         LPFb1loval;        
    unsigned short         LPFb1hival;        

    if (lowpass > 10000) {
        memset(data, 0, 10 * sizeof(unsigned short));
        data[5] = 16384;
        return;
    }

    // set the sampling rate 
    fsample = DSP_BASE_SAMP_RATE;

    // get the cutoff frequency in radians
    thetac = TWOPI * (double) lowpass / fsample;

    // dampling factor.  2.0 is for butterworth filters
    d = 2.0;


    // alpha, beta, and gamma are the three filter coefficients 
    // derived from theta_c
    beta = 0.5 * (1 - 0.5 * d * sin(thetac))/(1 + 0.5 * d * sin(thetac));

    gamma = (0.5 + beta) * cos(thetac);

    alpha = (0.5 + beta - gamma)/4;
    
    /* convert the coefficients to signed longs */
    nbetaout = trunc(-beta * 32768 * 65536);
    tmpmax = ::pow(2,32);
    if (nbetaout < 0) {
	nbetaout = tmpmax + nbetaout;
    }
    LPFb2hival = (unsigned short) floor(nbetaout / 65536);
    LPFb2loval = (unsigned short) (((u32) nbetaout) % 65536);

    gammaout = (int) trunc(gamma * 32768 * 65536);
    if (gammaout < 0) {
	gammaout = tmpmax + gammaout;
    }
    LPFb1hival = (unsigned short) floor(gammaout / 65536);
    LPFb1loval = (unsigned short) (((u32) gammaout) % 65536);

    alphaout = (int) trunc(alpha * 32768 * 65536);
    if (alphaout < 0) {
	alphaout = tmpmax - (u32) alphaout;
    }
    LPFa0hival = (unsigned short) floor(alphaout / 65536);
    LPFa0loval = (unsigned short) (((u32) alphaout) % 65536);

    alphaoutx2 = (int) trunc(alpha * UINT_MAX);
    if (alphaoutx2 < 0) {
	alphaoutx2 = UINT_MAX + alphaoutx2;
    }
    LPFa1hival = (unsigned short) floor(alphaoutx2 / 65536);
    LPFa1loval = (unsigned short) (((u32) alphaoutx2) % 65536);



    data[0] = LPFa0loval;
    data[1] = LPFa0hival;
    data[2] = LPFa1loval;
    data[3] = LPFa1hival;
    data[4] = LPFa0loval;
    data[5] = LPFa0hival;
    data[6] = LPFb2loval;
    data[7] = LPFb2hival;
    data[8] = LPFb1loval;
    data[9] = LPFb1hival;
    return;
}


int TriggerOutput(int output)
{
    unsigned short command[DIO_MAX_COMMAND_LEN];
    int tmptime, len;

    tmptime = digioinfo.rewardlength[output];


    /* to trigger an output we send in a seqence of commands to the current 
     * state machine */
    command[0] = DIO_S_SET_OUTPUT_HIGH | output;
    tmptime = digioinfo.rewardlength[output];
    len = 1;
    /* string together a set of wait commands to wait for the specified
     * interval */
    while (tmptime > 10000) {
	command[len++] = DIO_S_WAIT | (10000 * SAMP_TO_TIMESTAMP);
	tmptime -= 10000;
    }
    command[len++] = DIO_S_WAIT | (tmptime * SAMP_TO_TIMESTAMP);
    /* now turn off the output */
    command[len++] = DIO_S_SET_OUTPUT_LOW | output;
    /* send the command off to the DSP */
    if (!WriteDSPDIOCommand(command, len)) { 
        sprintf(tmpstring, "Error writing DIO command to master DSP");
        DisplayErrorMessage(tmpstring);
	return 0;
    }

    digioinfo.raised[output] = 0;

    sprintf(tmpstring, "Output triggered on port %d for %f ms", output, 
	    (float) digioinfo.rewardlength[output] / 10.0);
    DisplayStatusMessage(tmpstring);
    return 1;
}

void ChangeOutput(int output, int raise)
{
    unsigned short command[DIO_MAX_COMMAND_LEN];

    /* to trigger an output we send in a seqence of commands to the current 
     * state machine */
    if (raise) {
	command[0] = DIO_S_SET_OUTPUT_HIGH | output;
    }
    else {
	command[0] = DIO_S_SET_OUTPUT_LOW | output;
    }
    /* send the command off to the DSP */
    if (!WriteDSPDIOCommand(command, 1)) { 
        sprintf(tmpstring, "Error writing digital IO command to master DSP");
        DisplayErrorMessage(tmpstring);
	return;
    }
    if (raise) {
	sprintf(tmpstring, "Output on port %d raised", output);
    }
    else {
	sprintf(tmpstring, "Output on port %d lowered", output);
    }
    digioinfo.raised[output] = raise;
    DisplayStatusMessage(tmpstring);

    /*ReadDSPData(0, DIO_OUT_1, 10, read_data);
    fprintf(stderr, "DIO_OUT/IN data: ");
    for (i = 0; i < 10; i++) {
	fprintf(stderr, "%X ",read_data[i]);
    } */

    return;
}

int WriteDSPDIOCommand(unsigned short *command, int len, int statemachine, int sendResetStateMachine)
//int WriteDSPDIOCommand(unsigned short *command, int len, int statemachine=-1, int sendResetStateMachine=1)
{
  // ---- statemachine defaults to -1, sendreset to 0
    int i, s;
    unsigned short addr;
    
    /* check to see that the length is in bounds */
    if (len > DIO_MAX_COMMAND_LEN) {
	sprintf(tmpstring, "Error: command length %d too long to write to digital IO state machine. Must be <= %d", len, DIO_STATE_SIZE);
	DisplayErrorMessage(tmpstring);
	return 0;
    }
    /* check to see if this is a reset command */
    if (command[0] == DIO_RESET_STATE_MACHINES) {
	/* reset each state machines to the beginning of its buffer and
	 * set its output to be 0 */
	command[0] = 0;
	for (i = 0; i <  DIO_N_STATE_MACHINES; i++) {
	    if (!WriteDSPData(0, digioinfo.statemachineptr[i], 1, command)) {
		sprintf(tmpstring, "Error reseting Digital IO state machine pointer on master DSP");
		DisplayErrorMessage(tmpstring);
		return 0;
	    }
	    switch(i) {
		case 0:
		    addr = DIO_OUT_1;
		    break;
		case 1:
		    addr = DIO_OUT_2;
		    break;
		case 2:
		    addr = DIO_OUT_3;
		    break;
		case 3:
		    addr = DIO_OUT_4;
		    break;
	    }
	    /* set the output to be 0 */
	    if (!WriteDSPData(0, addr, 1, command)) {
		sprintf(tmpstring, "Error setting port %d to output 0", i);
		DisplayErrorMessage(tmpstring);
		return 0;
	    }
	}
	return 1;
    }


    /* we first need to find a free state machine */
    if ((s = NextDIOStateMachine()) == -1) {
	sprintf(tmpstring, "Error: all state machines busy, cannot program digital IO");
	DisplayErrorMessage(tmpstring);
	return 0;
    }
    fprintf(stderr,"GOT STATE MACHINE: %d\n",s);

    if (statemachine >= 0) {
      if (s > statemachine)
        s = statemachine;
      else {
	sprintf(tmpstring, "Error: requested state machine (%d) busy, cannot program digital IO", statemachine);
	DisplayErrorMessage(tmpstring);
	return 0;
      }
    }

    /* we have to append a jump statement to the command to tell the state
     * machine to jump back to the first instruction, which is "wait forever,"
     * when it completes the command */
    command[len++] = DIO_S_JUMP_ABS | 0;

    /* Now write the command to the DSP */
    if (!WriteDSPData(0, digioinfo.statemachinebuffer[s], len, command)) {
        sprintf(tmpstring, "Error writing digital IO command to master DSP");
        DisplayErrorMessage(tmpstring);
	return 0;
    }
    
    if (sendResetStateMachine==1) {
      /* Finally, we reset the state machine pointer to the beginning of the 
       * new command, which is at offset 1 from the beginning of the buffer */
      command[0] = 1;
      if (!WriteDSPData(0, digioinfo.statemachineptr[s], 1, command)) {
          sprintf(tmpstring, "Error writing digital IO state machine pointer to master DSP");
          DisplayErrorMessage(tmpstring);
    return 0;
      }
    }

    return 1;
}


int NextDIOStateMachine(void)
    /* returns the index of the next available state machine or -1 if 
     * all are busy */
{
    unsigned short status[1];
    if (ReadDSPData(0, DIO_STATE_AVAILABLE, 1, status) != 1) {
	sprintf(tmpstring, "Error reading Master DSP digital IO state pointer");
	DisplayErrorMessage(tmpstring);
	return DIO_STATE_MACHINES_BUSY;
    }
    if ((short) status[0] != DIO_STATE_MACHINES_BUSY) {
	return (int) status[0];
    }
    else {
	return -1;
    }
}

int LookForDSPDIOResponse(void)
{
    if (!GetDSPResponse(0, 0, NULL)) {
        sprintf(tmpstring, "No response from master DSP when asked");
        DisplayErrorMessage(tmpstring);
	return 0;
    }
    return 1;
}


int WriteDSPDIORestartStateMachine(int s)
{
  unsigned short command = 1;
    /* Finally, we reset the state machine pointer to the beginning of the 
     * new command, which is at offset 1 from the beginning of the buffer */
    if (!WriteDSPData(0, digioinfo.statemachineptr[s], 1, &command)) {
        sprintf(tmpstring, "Error writing digital IO state machine pointer to master DSP");
        DisplayErrorMessage(tmpstring);
	return 0;
    }
    return 1;
}

int WriteDSPData(short dspnum, unsigned short address, short n, 
                 unsigned short *data)
    /* write data to the dsp. data must contain < 24 words */
{
    int 		size;
    unsigned short 	dataout[1000];
    (void) dspnum; // prevent compiler warnings
    size = n*sizeof(unsigned short);

    /* create the programming command */
    dataout[0] = DEVICE_CONTROL_ADDR;
    dataout[1] = SHORT_WRITE;
    dataout[2] = DSP_SRAM;
    dataout[3] = address;
    dataout[4] = size;
    /* append data to the end of dataout */
    memcpy(dataout + 5, data, size);

    /* increment size to be the size of the full dataout array */
    size += 5 * sizeof(unsigned short);
    
    ByteSwap(dataout, 5 + n);

#ifdef NO_DSP_DEBUG
    //fprintf(stderr, "Data to DSP %d: %d shorts \n ", dspnum, n);
    return 1;
#else
    int 	dspacq, acqmessage = 0;
    /* if we have a connection to the dsp, create a single data buffer with the
     * write command */
    if (client_message[dspnum].fd > 0) {
	/* check to see if the message is a start or stop acquisition 
	 * message */
	acqmessage = 0;
	if (address == BLOCKS_TO_SEND_ADDR) {
	    acqmessage = 1;
	}
	/* if this is not check to see if 
	 * 1) this is not the master dsp
	 * 2) acquisition is on. 
	 * If so, turn it off. This will do nothing if acquisition has already 
	 * been stopped  */
	dspacq = 0;
	if ((dspnum > 0) && !acqmessage && sysinfo.dspacq) {
	    StopLocalAcq();
	    dspacq = 1;
	}
    }

    /* send out the command */
    do {
	if (write(client_message[dspnum].fd, dataout, size) != size) {
	    sprintf(tmpstring, "Error sending message to dsp %d", dspnum);
	    DisplayErrorMessage(tmpstring);
	}
	usleep(DSP_PAUSE_USEC);
    } while (!GetDSPResponse(dspnum, 0, NULL));

    if (!acqmessage && dspacq) {
	StartLocalAcq();
    }
#endif
    return 1;
}


int ReadDSPData(short dspnum, unsigned short address, short n, 
                unsigned short *data)
    /* write data to the dsp. data must contain < 24 words */
{
    int 		size;
    int 		nread;
    unsigned short 	dataout[6], datain[26];
    (void)data; // prevent compiler warnings

    size = (n+2)*sizeof(unsigned short);

    /* create the programming command */
    dataout[0] = DEVICE_CONTROL_ADDR;
    dataout[1] = SHORT_READ;
    dataout[2] = DSP_SRAM;
    dataout[3] = address;
    dataout[4] = size;

    if (n > 24) {
	fprintf(stderr, "Error: trying to read > 24 words from dsp %d in ReadDSPData\n", dspnum);
	return -1;
    }

#ifdef NO_DSP_DEBUG
    //fprintf(stderr, "Data to DSP %d: %d shorts \n ", dspnum, n);
    nread = 0;
    datain[0] = 0;
    return 1;
#else
    int 	dspacq, acqmessage, error, ntries;

    ntries = 100;

    /* if we have a connection to the dsp, create a single data buffer with the
     * write command */
    if (client_message[dspnum].fd > 0) {
	/* check to see if the message is a start or stop acquisition message */
	acqmessage = 0;
	if (address == BLOCKS_TO_SEND_ADDR) {
	    acqmessage = 1;
	}
	/* if this is not check to see if this is not the master DSP and
	 * if acquisition is on. If so, local dsp acquisition is stopped */
	dspacq = 0;
	if ((dspnum > 0) && !acqmessage && sysinfo.dspacq) {
	    StopLocalAcq();
	    dspacq = 1;
	}
	
	/* send the command to the DSP */
	ByteSwap(dataout, 5);
        if (write(client_message[dspnum].fd, dataout, 5 * sizeof(unsigned short)) != 
		5 * sizeof(unsigned short)) {
            fprintf(stderr, "Error writing to dsp %d in ReadDSPData", dspnum);
	    return -1;
        }
	/* read from the DSP. For some reason we have to read twice */
	usleep(DSP_PAUSE_USEC);
	while (ntries) {
	    if ((nread = read(client_message[dspnum].fd, datain, size)) != size) {
		ntries--;
	    }
	    else {
		break;
	    }
        }
	if (!ntries) {
	    fprintf(stderr, "Error reading from dsp %d\n", dspnum);
	    return -1;
	}
	
        /* swap bytes and check for a valid response */
	ByteSwap(datain, n + 2);
	if (datain[0] != DEVICE_CONTROL_ADDR) {
	    fprintf(stderr, "Error reading from dsp %d: bad PortID %u\n", 
		    dspnum, datain[0]);
	    error = 1;
	}
	if (datain[1] != 1) {
	    if (datain[1] == 3) {
		fprintf(stderr, "Error writing to dsp: bad address\n");
		return 0;
	    }
	    else if (datain[1] == 5) {
		fprintf(stderr, "Error writing to dsp: unsupported command\n");
		return 0;
	    }
	}
	/* copy the returned data to the output array */
	size -= 2 * sizeof(unsigned short);
	memcpy(data, datain + 2, size);

	if (!acqmessage && dspacq) {
	    usleep(DSP_PAUSE_USEC);
	    StartLocalAcq();
	}
    }
#endif
    return 1;
}

int GetDSPResponse(int dspnum, int n, unsigned short *data)
{
    /* read a response to a programming command */
    unsigned short resp[MAX_PACKET_SIZE];
    int size, dsize, readsize;
    int error = 0, done = 0;
    int ntries = 10;

    size = 2 * sizeof(unsigned short);
    dsize = n * sizeof(unsigned short);

    while (!done) {
	if (ntries > 0) {
	    readsize = read(client_message[dspnum].fd, resp, MAX_PACKET_SIZE * 
		    sizeof(short)); 
	    ByteSwap(resp, MAX_PACKET_SIZE);
	}
	else {
	    //sprintf(tmpstring, "Error: no response from DSP %d, resending\n", 
//		    dspnum);
//	    DisplayStatusMessage(tmpstring);
	    return 0;
	}

	/* check for status codes */
	if ((readsize > 0) && (resp[0] > sysinfo.ndsps)) {
	    done = 1;
	    /* this is not a left over data packet, so parse it */
	    if (resp[0] != DEVICE_CONTROL_ADDR) {
		fprintf(stderr, "Error writing to dsp %d: bad PortID %u\n", 
			dspnum, resp[0]);
		error = 1;
	    }
	    if (resp[1] != 1) {
		if (resp[1] == 3) {
		    fprintf(stderr, "Error writing to dsp: bad address\n");
		    return 0;
		}
		else if (resp[1] == 5) {
		    fprintf(stderr, "Error writing to dsp: unsupported command\n");
		    return 0;
		}
	    }
	}
	else if (readsize <= 0) {
	    sprintf(tmpstring, "Warning, no response from DSP %d", dspnum);
	    DisplayStatusMessage(tmpstring);
	    ntries--;
	}
    }
    /* copy the data if it is available */
    if (n) {
	fprintf(stderr, "read data, %d\n", resp[2]);
	memcpy(data, resp + 2, readsize - size);
    }
    return 1;
}


int ReprogramMasterDSP(char *filename)
   /* Reprogram the master DSP with the program in the file. This can only be
    * run from the master */
{
    unsigned short	program[DSP_PROGRAM_SIZE];

    /* read in the data from the file.  We will have already checked to
     * make sure that this file exists and is readable */
    if (!ReadDSPProgram(filename, program)) {
	return 0;
    }
    if (!ReprogramDSP(0, program)) {
	return 0;
    }

    return 1;
}

int ReprogramAuxDSPs(char *filename)
   /*  This should be initiated by a message from the master machine
    * (or this machine if this is the master )  */
{
    int         	i, ret; 
    unsigned short	program[DSP_PROGRAM_SIZE];

    if (sysinfo.system_type[sysinfo.machinenum] == MASTER) {
	/* read in the data from the file.  We will have already checked to
	 * make sure that this file exists and is readable */
        if (!ReadDSPProgram(filename, program)) {
	    return 0;
	}
        /* Reprogram the DSPS on each of the slave machines */
        for(i = 0; i < netinfo.nslaves; i++) {
	    /* send this only to the non-rt slaves (netinfo.rtslave - 1 is the
	     * index in slavefd of the rt slave */
	    if (i != netinfo.rtslave - 1) {
		SendMessage(netinfo.slavefd[i], REPROGRAM_DSPS, (char *) program, DSP_PROGRAM_SIZE * sizeof(unsigned short));
		/* Wait for the reply */
		if (!WaitForMessage(netinfo.masterfd[i], DSPS_PROGRAMMED, 10)) {
		    sprintf(tmpstring,"spike_main: no response to REPROGRAM_DSP message from slave %s", netinfo.slavename[i]);
		    fprintf(STATUSFILE, tmpstring);
		    DisplayStatusMessage(tmpstring);
		    return 0;
		}
	    }
        }
        ret = ReprogramLocalAuxDSPS(program);
    }
    return  ret;
}

int ReprogramLocalAuxDSPS(unsigned short *program)
    /* go through the AUX dsps that send data to this machine and 
     * reprogram them */
{
    int         i, ret;
    DSPInfo         *dptr;

    ret = 0;
    /* go through the Aux DSPs (#1 - ) */
    for (i = 1; i < sysinfo.ndsps; i++) {
        dptr = sysinfo.dspinfo + i;
        if (dptr->machinenum == sysinfo.machinenum) {
            if (i > 0) {
                if ((dptr->nchan > 0) && ((ret = ReprogramDSP(i, program)) == -1)) {
		    sprintf(tmpstring,"Error reprogramming Aux DSP %d, exiting", i);
                    DisplayStatusMessage(tmpstring);
		    return 0;
                }
            }
        }
    }
    return 1;
}

int ReprogramDSP(int dspnum, unsigned short *program) 
{
    /* reprogram a single DSP by writing the program out to the DSP's
     * EEPROM */
    u32		checksum;
    int		i, j, size;
    unsigned short dataout[24];
    unsigned short datain[24];
    unsigned short *uptr;

    size = 21 * sizeof(short);

    uptr = program;
    checksum = 0;
    /* write out the data in blocks of 16 words */
    for (i = 1; i <= 255; i++) {
	/* set up the programming command */
	dataout[0] = DEVICE_CONTROL_ADDR;
	dataout[1] = SHORT_WRITE;
	dataout[2] = DSP_PROGRAM_ADDR;
	dataout[3] = i * 32;
	dataout[4] = 32;
	memcpy(dataout+5, uptr, 16 * sizeof(unsigned short));
	ByteSwap(dataout, 21);
#ifdef NO_DSP_DEBUG
	fprintf(stderr, "New Program to DSP %d: 21 shorts\n", dspnum);
#else
	fprintf(stderr, "New Program to DSP %d: 21 shorts\n", dspnum);
	do {
	    if (write(client_message[dspnum].fd, dataout, size) != size) {
		sprintf(tmpstring, "Error sending program to dsp %d", dspnum);
		DisplayErrorMessage(tmpstring);
	    }
	    usleep(DSP_PAUSE_USEC);

	} while (!GetDSPResponse(dspnum, 0, NULL));

#endif
	/* add to the checksum */
	for (j = 0; j < 16; j++, uptr++) {
	    checksum += *uptr;
	}
    }

    /* now write out the high and low words of the checksum */
    dataout[0] = DEVICE_CONTROL_ADDR;
    dataout[1] = SHORT_WRITE;
    dataout[2] = DSP_PROGRAM_ADDR;
    dataout[3] = 8;
    dataout[4] = 4;
    dataout[5] = checksum % 65536; // low word
    dataout[6] = checksum / 65536; // high word
    ByteSwap(dataout, 7);
    size = 7 * sizeof(unsigned short);
#ifdef NO_DSP_DEBUG
    fprintf(stderr, "Checksum to DSP %d: 2 shorts\n", dspnum);
    datain[0] = 0;
#else
    do {
	if (write(client_message[dspnum].fd, dataout, size) != size) {
	    sprintf(tmpstring, "Error sending checksum to dsp %d", dspnum);
	    DisplayErrorMessage(tmpstring);
	}
	usleep(DSP_PAUSE_USEC);
    } while (!GetDSPResponse(dspnum, 0, NULL));

    uptr = program;
    for (i = 1; i <= 255; i++) {
	/* TEST - read back program */
	dataout[0] = DEVICE_CONTROL_ADDR;
	dataout[1] = SHORT_READ;
	dataout[2] = DSP_PROGRAM_ADDR;
	dataout[3] = i * 32;
	dataout[4] = 32;
	ByteSwap(dataout, 5);
	write(client_message[dspnum].fd, dataout, 32);
	usleep(DSP_PAUSE_USEC);
	read(client_message[dspnum].fd, datain, 36);
	ByteSwap(datain, 18);
	for (j = 0; j < 16; j++, uptr++) {
	    if (*uptr !=  datain[j+2]) {
		fprintf(stderr, "ERROR\n");
	    }
	}
	usleep(DSP_PAUSE_USEC);
    }
#endif
    return 1;
}

int ReadDSPProgram(char *filename, unsigned short *program)
{
    /* read in the unsigned shorts from the file and put them in program */
    FILE 		*progfile;
    char		tmpbuf[200];
    int 		i;
    unsigned short 	*uptr;

    if ((progfile = fopen(filename, "r")) == NULL) {
	sprintf(tmpstring,"spike_main: cannot open %s for reading", filename);
	DisplayStatusMessage(tmpstring);
	return 0;
    }

    uptr = program;
    /* read in the appropriate number of unsigned shorts */
    for (i = 0; i < DSP_PROGRAM_SIZE; i++, uptr++) {
	fgets(tmpbuf, 200, progfile);
	sscanf(tmpbuf, "%hu", uptr);
    }

    return 1;
}


int GetAllDSPCodeRev(void)
   /*  This should be initiated by a message from the master machine
int ReprogramAuxDSPS(char *filename);
    * (or this machine if this is the master )  */
{
    int         	i, j; 
    unsigned short	tmpcoderev[MAX_DSPS];
    int			size;


    /* set coderev to all -1s */
    for (i = 0; i < MAX_DSPS; i++) {
	sysinfo.dspinfo[i].coderev = 0;
	tmpcoderev[i] = 0;
    }
    

    if (sysinfo.system_type[sysinfo.machinenum] == MASTER) {
        /* Program the DSPS on each of the slave machines */
        for(i = 0; i < netinfo.nslaves; i++) {
	    /* send this only to the non-rt slaves (netinfo.rtslave - 1 is the
	     * index in slavefd of the rt slave */
	    if (i != netinfo.rtslave - 1) {
		SendMessage(netinfo.slavefd[i], GET_DSP_CODE_REV, NULL, 0);
		/* Wait for the reply */
		if (!WaitForMessage(netinfo.masterfd[i], DSP_CODE_REV, 10,
			    (char *) tmpcoderev, &size)) {
		    sprintf(tmpstring,"spike_main: no response to GET_DSP_CODE_REV from slave %s", netinfo.slavename[i]);
		    DisplayStatusMessage(tmpstring);
		    return 0;
		}
		else {
		    /* copy the nonzero elements to coderev */
		    for (j = 0; j < MAX_DSPS; j++) {
			if (tmpcoderev[j]) {
			    sysinfo.dspinfo[j].coderev = tmpcoderev[j];
			}
		    }
		}

	    }
        }
        GetLocalDSPCodeRev(tmpcoderev);
	/* copy the nonzero elements to coderev */
	for (j = 0; j < MAX_DSPS; j++) {
	    if (tmpcoderev[j]) {
		sysinfo.dspinfo[j].coderev = tmpcoderev[j];
	    }
	}
    }
    else {
        GetLocalDSPCodeRev(tmpcoderev);
        SendMessage(netinfo.slavefd[netinfo.myindex], DSP_CODE_REV, 
		(char *) tmpcoderev, MAX_DSPS * sizeof(unsigned short));
    }
    return 1 ;
}

int GetLocalDSPCodeRev(unsigned short *tmpcoderev)
    /* read in the code revision number from each of the local dsps */
{
    int         i, error;
    unsigned short rev;
    DSPInfo     *dptr;

    error = 0;
    for (i = 0; i < sysinfo.ndsps; i++) {
        dptr = sysinfo.dspinfo + i;
        if (dptr->machinenum == sysinfo.machinenum) {
	    if ((rev = GetDSPCodeRev((short)i)) == 0) {
		fprintf(stderr, "Error getting code revision for DSP %d\n", i);
		error = 1;
	    }
	    else {
		tmpcoderev[i] = rev;
	    }
        }
    }
    if (error) {
        return 0;
    }
    return 1;
}

unsigned short GetDSPCodeRev(short dspnum)
    /* read in the code revision number from the DSP */
{ 
    unsigned short coderev[1];

    coderev[0] = 0;
    ReadDSPData(dspnum, DSP_CODE_REV_ADDR, 1, coderev);
#ifdef NO_DSP_DEBUG
    /* put in a fake revision number */
    coderev[0] = 1;
#endif
    return coderev[0];
}


