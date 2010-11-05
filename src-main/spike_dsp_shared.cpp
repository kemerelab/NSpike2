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

/* local functions */
int WriteDSPData(short dspnum, unsigned short baseaddr, unsigned short address,
	short n, unsigned short *data);
int ReadDSPData(short dspnum, unsigned short baseaddr, unsigned short address, 
	short n, unsigned short *data);
int GetDSPResponse(int dspnum, int n, unsigned short *data);
void ByteSwap(unsigned short *sptr, int nelem);
void ByteSwap(long *lptr, int nelem);

#ifndef DIO_ON_MASTER_DSP
int WriteArbWaveForm(unsigned short *wavefm, int len) 
{
    unsigned short tmplen = (unsigned short) len; 

    /* check that the waveform length is within limits */
    if (len > DIO_ARB_WAVE_LEN) {
	sprintf(tmpstring, "Error: waveform length %d too long to write to digital IO state machine. Must be <= %d", len, DIO_ARB_WAVE_LEN);
	DisplayErrorMessage(tmpstring);
	return 0;
    }
    /* write out the waveform to the waveform generator */
    if (!WriteDSPData(DSPDIO, DIO_ARB_WAVE_ADDR, 0, len, wavefm)) {
	sprintf(tmpstring, "Error writing waveform to arbitrary waveform generator");
	DisplayErrorMessage(tmpstring);
	return 0;
    }
    /* write out the waveform length */
    if (!WriteDSPData(DSPDIO, DSP_SRAM, DIO_ARB_LENGTH, 1, &tmplen)) {
	sprintf(tmpstring, "Error writing waveform length for arbitrary waveform generator");
	DisplayErrorMessage(tmpstring);
	return 0;
    }
    ReadDSPData(DSPDIO, DSP_SRAM, DIO_ARB_LENGTH, 1, &tmplen);
	    fprintf(stderr, "length %d\n", tmplen);
    return SetArbPointer(0);
}

int SetArbPointer(unsigned short offset)
    /* reset the arbitrary waveform generator's pointer to the beginning of the buffer */
{
    if (!WriteDSPData(DSPDIO, DSP_SRAM, DIO_ARB_POINTER, 1, &offset)) {
	sprintf(tmpstring, "Error setting pointer to offset %d for arbitrary waveform generator", offset);
	DisplayErrorMessage(tmpstring);
	return 0;
    }
    return 1;
}


int SetArbAOutChan(unsigned short aout)
    /* set the analog output for the arbitrary waveform generator */
{
    if ((aout != DIO_ARB_AOUT_CHANNEL_1) && (aout != DIO_ARB_AOUT_CHANNEL_2)){
	sprintf(tmpstring, "Error: Analog output for arbitrary waveform generator must be %d or %d", DIO_ARB_AOUT_CHANNEL_1, DIO_ARB_AOUT_CHANNEL_2);
	DisplayErrorMessage(tmpstring);
	return 0;
    }
    if (!WriteDSPData(DSPDIO, DSP_SRAM, DIO_ARB_AOUT_CHANNEL, 1, &aout)) {
	sprintf(tmpstring, "Error setting analog output of arbitrary waveform generator");
	DisplayErrorMessage(tmpstring);
	return 0;
    }
    return 1;
}

int EnableArb(unsigned short enable)
    /* enable or disable the arbitrary waveform generator */
{
   if (!WriteDSPData(DSPDIO, DSP_SRAM, DIO_ARB_ENABLE, 1, &enable)) {
	sprintf(tmpstring, "Error enabling or disabling arbitrary waveform generator");
	DisplayErrorMessage(tmpstring);
	return 0;
   }
   return 1;
}

int SetArbTrigger(unsigned short trigger)
    /* set the trigger for the aritrary waveform generator */
{
   if (!WriteDSPData(DSPDIO, DSP_SRAM, DIO_ARB_TRIGGER, 1, &trigger)) {
	sprintf(tmpstring, "Error setting trigger for arbitrary waveform generator to 0x%x", trigger);
	DisplayErrorMessage(tmpstring);
	return 0;
   }
   return 1;
}

#endif


int ResetStateMachines()
{
    int i;
    unsigned short addr;
    unsigned short command[DIO_MAX_COMMAND_LEN];

    /* reset each state machines to the beginning of its buffer and
     * set its output to be 0 */
    command[0] = 0;
    for (i = 0; i <  DIO_N_STATE_MACHINES; i++) {
	if (!WriteDSPData(DSPDIO, DSP_SRAM, digioinfo.statemachineptr[i], 
		    1, command)) {
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
	if (!WriteDSPData(DSPDIO, DSP_SRAM, addr, 1, command)) {
	    sprintf(tmpstring, "Error setting port %d to output 0", i);
	    DisplayErrorMessage(tmpstring);
	    return 0;
	}
    }

/*    fprintf(stderr, "TEST: setting port 0 bit 8 high\n");
    usleep(500000);
    command[0] = O8;
    addr = DIO_OUT_1;
    if (!WriteDSPData(DSPDIO, DSP_SRAM, addr, 1, command)) {
	sprintf(tmpstring, "Error setting port %d to output 0", i);
	DisplayErrorMessage(tmpstring);
	return 0;
    }  */

    /* TEST of arbitrary waveform generator */
    /* create 1000 point ramp 
    for (i = 0; i < 30000; i++) {
	command[i] = i / 2;
    }
    WriteArbWaveForm(command, 30000);
    SetArbAOutChan(DIO_ARB_AOUT_CHANNEL_2);
    EnableArb(1);
    SetArbTrigger(257); */


    return 1;
}

int WriteDSPDIOCommand(unsigned short *command, int len, int statemachine, int sendResetStateMachine)
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
	if (ResetStateMachines()) {
	    return 1 ;
	}
	else {
	    return 0;
	}
    }


    /* we first need to find a free state machine */
    if ((s == -1) && ((s = NextDIOStateMachine()) == -1)) {
	sprintf(tmpstring, "Error: all state machines busy, cannot program digital IO");
	DisplayErrorMessage(tmpstring);
	return 0;
    }
    //fprintf(stderr,"GOT STATE MACHINE: %d\n",s);

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
    if (!WriteDSPData(DSPDIO, digioinfo.statemachinebaseaddr[s], 
		digioinfo.statemachinebuffer[s], len, command)) {
        sprintf(tmpstring, "Error writing digital IO command to master DSP");
        DisplayErrorMessage(tmpstring);
	return 0;
    }
    
    if (sendResetStateMachine == 1) {
      /* Finally, we reset the state machine pointer to the beginning of the 
       * new command, which is at offset 1 from the beginning of the buffer */
      command[0] = 1;
      if (!WriteDSPData(DSPDIO, DSP_SRAM, digioinfo.statemachineptr[s], 1, command)) {
          sprintf(tmpstring, "Error writing digital IO state machine pointer to master DSP");
          DisplayErrorMessage(tmpstring);
    return 0;
      }
    }
    /* TEST: read back the pointer */
/*    ReadDSPData(DSPDIO, DSP_SRAM, digioinfo.statemachineptr[s], 1, command);
	    fprintf(stderr, "pointer at %d\n", command[0]);
    ReadDSPData(DSPDIO, digioinfo.statemachinebaseaddr[s], digioinfo.statemachinebuffer[s] + command[0] - 1, 1, command);
	    fprintf(stderr, "contents of pointer: %x\n", command[0]); */


    return 1;
}

int SendStartDIOCommand(int s)
    /* write data to the dsp. data must contain < 24 words */
{
    unsigned short command[1];
    command[0] = 1;
    if (!WriteDSPData(DSPDIO, DSP_SRAM, digioinfo.statemachineptr[s], 1, command)) {
        sprintf(tmpstring, "Error writing digital IO state machine pointer to DIO DSP");
        DisplayErrorMessage(tmpstring);
	return 0;
      }
    return 1;
}


int NextDIOStateMachine(void)
    /* returns the index of the next available state machine or -1 if 
     * all are busy */
{
    unsigned short status[1];
    if (ReadDSPData(DSPDIO, DSP_SRAM, DIO_STATE_AVAILABLE, 1, status) != 1) {
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
    if (!GetDSPResponse(DSPDIO, 0, NULL)) {
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
    if (!WriteDSPData(DSPDIO, DSP_SRAM, digioinfo.statemachineptr[s], 1, &command)) {
        sprintf(tmpstring, "Error writing digital IO state machine pointer to master DSP");
        DisplayErrorMessage(tmpstring);
	return 0;
    }
    return 1;
}

int WriteDSPData(short dspnum, unsigned short baseaddr, unsigned short address,
	short n, unsigned short *data)
    /* write data to the dsp. data must contain < 24 words */
{
    int 		size, writesize, totalsize, ntowrite;
    unsigned short 	dataout[1000], offset;
    int 		dspacq, acqmessage = 0;
    (void) dspnum; // prevent compiler warnings

    totalsize = n*sizeof(unsigned short);

    offset = address;

#ifdef NO_DSP_DEBUG
    fprintf(stderr, "Data to DSP %d: %d shorts \n ", dspnum, n);
    return 1;
#else
    dspacq = 0;
    do {
	size = totalsize < MAX_SHORT_WRITE_SIZE ? totalsize : 
	    MAX_SHORT_WRITE_SIZE;
	/* create the programming command */
	dataout[0] = DEVICE_CONTROL_ADDR;
	dataout[1] = SHORT_WRITE;
	dataout[2] = baseaddr;
	dataout[3] = offset;
	dataout[4] = size;
	/* append data to the end of dataout */
	memcpy(dataout + 5, data, size);

	/* increment the data pointer to move past the data we've added */
	ntowrite = size / sizeof(unsigned short);
	data += ntowrite;
	offset += ntowrite;

	/* add five elements to include the command information */
	ntowrite += 5;
	/* increment size to be the size of the full dataout array */
	writesize = ntowrite * sizeof(unsigned short);
	ByteSwap(dataout, ntowrite);

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
	    if ((dspnum != DSPDIO) && !acqmessage && sysinfo.dspacq) {
		StopLocalAcq();
		dspacq = 1;
	    }
	}

	/* send out the command */
	do {
	    if (write(client_message[dspnum].fd, dataout, writesize) != 
		    writesize) {
		sprintf(tmpstring, "Error sending message to dsp %d", dspnum);
		DisplayErrorMessage(tmpstring);
	    }
	    usleep(DSP_PAUSE_USEC);
	} while (!GetDSPResponse(dspnum, 0, NULL));
	totalsize -= size;
    } while (size > 0);
    if (!acqmessage && dspacq) {
	StartLocalAcq();
    }
    return 1;
#endif
}


int ReadDSPData(short dspnum, unsigned short baseaddr, unsigned short address, 
	short n, unsigned short *data)
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
    dataout[2] = baseaddr;
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


