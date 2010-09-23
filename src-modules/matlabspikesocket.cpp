/*
    matlabspikesocket.cpp

    Reads in data from the nspike data acquisition program over a socket. 

    Note that this is a mex file, and will run only when called from Matlab
*/

/*
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


#include "mex.h"
#include "spikecommon.h"
#include "spike_dio.h"

/* necessary but unused structure defintions */
NetworkInfo		netinfo;
SysInfo			sysinfo;

void readerror();

/******************************************************************************
  INTERFACE FUNCTION
  */

void mexFunction(
    int           nlhs,           /* number of expected outputs */
    mxArray       *plhs[],        /* array of pointers to output arguments */
    int           nrhs,           /* number of inputs */
    const mxArray *prhs[]         /* array of pointers to input arguments */)
{

    MatlabDataBufferInfo bufferinfo;


    const char         *posfieldnames[] = {"time", "x", "y"};
    char		databuf[SAVE_BUF_SIZE];
    char		*charptr;
    double	 	**spike;
    double	 	**cont;
    double		**contptr;
    double		*electdspmap;
    double	 	*posbuf;
    double	 	*dptr;
    double		*tmpptr;
    double		*starttime;
    mxArray            	*mdata;
    double            	*tmpdata;
    short		datatype;
    short		*shortptr;

    SpikeBuffer		*sbuf;
    MatlabContBuffer	*mtmp;
    DIOBuffer		*dtmp;

    u32			*posinfo;
    int			posind = 0;

    int             dims[2];
    int             nspikes;
    int             offset;
    int		    fd;
    int             i, j, n, index;
    int		    size;
    int		    tmpnum;
    
    /* Check numbers of arguments */
    if ((nrhs != 0) || ((nlhs != 5) && (nlhs != 6))) {
	mexErrMsgTxt("Usage; [spikedata contdata posdata digiodata starttime] = matlabspikesocket() or \n");
	mexErrMsgTxt("Usage; [spikedata contdata posdata digiodata starttime dspchan] = matlabspikesocket()\n");
    }


    /* try to open a socket to spike_matlab */
    mexPrintf("trying to open socket\n");
    if ((fd = GetServerSocket(MATLAB_SOCKET_NAME, 5, 200000)) > 0) {
	mexPrintf("opened socket\n");
	/* set the socket to be non-blocking */
	fcntl(fd, F_SETFL, O_NONBLOCK);

	datatype = -1;
	/* the first chunk of data will be the buffer information structure */
	if (read(fd, &bufferinfo, sizeof(MatlabDataBufferInfo)) != 
		sizeof(MatlabDataBufferInfo)) {
	    readerror();
	}

	/* create the starttime array */
	plhs[4] = mxCreateDoubleMatrix(1, 1, mxREAL);
	starttime = mxGetPr(plhs[4]);
	*starttime = 1e100;

	/* the buffer information structure gives us the number of elements we
	 * need to allocate for each type of data */

	/* Spike data */
	if (bufferinfo.maxspiketetnum > 0) {
	    /* create a cell array with one element for each electrode */
	    plhs[0] = mxCreateCellMatrix(bufferinfo.maxspiketetnum,1);
	    /* allocate space for the array of spike times */
	    spike = (double **) mxCalloc(bufferinfo.maxspiketetnum, 
		                          sizeof(double *));
	    for (i = 0 ; i < bufferinfo.maxspiketetnum; i++) {
		/* create an mxArray for these spikes */
		mdata = mxCreateDoubleMatrix(NTOTAL_POINTS_PER_SPIKE+1,
			bufferinfo.nspikes[i+1], mxREAL);
		/* set this element of the cell array */
		mxSetCell(plhs[0], i, mdata);
		/* save a pointer to the mdata so we can put the spikes in it */
		spike[i] = mxGetPr(mdata);
		mdata = NULL;
	    }
	}
	else {
	    plhs[0] = mxCreateCellMatrix(1,1);
	}

	/* Continuous data */
	if (bufferinfo.maxconttetnum > 0) {
	    /* create a cell array with one element for each electrode */
	    plhs[1] = mxCreateCellMatrix(bufferinfo.maxconttetnum,1);
	    /* allocate space for the array of continuous mdata points*/
	    cont = (double **) mxCalloc(bufferinfo.maxconttetnum, 
		                          sizeof(double *));
	    /* allocate space for the pointers to the current element of each
	     * continuous data element */
	    contptr = (double **) mxCalloc(bufferinfo.maxconttetnum, 
		                          sizeof(double *));

	    mdata = mxCreateDoubleMatrix(bufferinfo.maxconttetnum, 1, 
			mxREAL);
	    electdspmap = mxGetPr(mdata);
	    for (i = 0 ; i < bufferinfo.maxconttetnum; i++) {
		if (bufferinfo.matlabinfo.contelect[i+1]) {
		    /* create an mxArray for these data */
		    mdata = mxCreateDoubleMatrix(bufferinfo.ncontsamp[i+1], 1, 
			    mxREAL);
		    /* set this element of the cell array */
		    mxSetCell(plhs[1], i, mdata);
		    /* save a pointer to the mdata */
		    cont[i] = mxGetPr(mdata);
		    contptr[i] = cont[i];
		}
	    }
	}
	else {
	    plhs[1] = mxCreateCellMatrix(1,1);
	}

	/* Position mdata */
	if (bufferinfo.nposbuf > 0) {
	    /* create a struct array with one element for each buffer */
	    plhs[2] = mxCreateStructMatrix(bufferinfo.nposbuf, 1, 3, 
		    posfieldnames);
	}
	else {
	    plhs[2] = mxCreateCellMatrix(1,1);
	}

	/* Behavior / Digitial IO mdata FIX THIS!!!*/
	if (bufferinfo.ndigiobuf > 0) {
	    /* create a double array with one element for each buffer */
	    plhs[3] = mxCreateDoubleMatrix(3, bufferinfo.ndigiobuf, mxREAL);
	    dptr = mxGetPr(plhs[3]);
	}
	else {
	    plhs[3] = mxCreateDoubleMatrix(1,1, mxREAL);
	}

	/* We now go through the mdata in the socket, reading one record at a
	 * time and putting into the appropriate structure */
	while (read(fd, &datatype, sizeof(short)) == sizeof(short)) {
	    /* read in the datasize and then read in the data */
	    if (read(fd, &size, sizeof(int)) != sizeof(int)) {
		readerror();
	    } 
	    if (read(fd, databuf, size) != size) {
		readerror();
	    } 
	    mexPrintf("got datatype %d\n", datatype);
	    switch((int) datatype) {
		case SPIKE_DATA_TYPE:
		    sbuf = (SpikeBuffer *) databuf;
		    nspikes = size / sizeof(SpikeBuffer);
		    /* convert to 0 base for matlab (tetrode 1 will be matlab
		     * index 1 */
		    for (n = 0; n < nspikes; n++, sbuf++) {
			tmpnum = sbuf->electnum - 1;
			tmpptr = spike[tmpnum];
			/* copy the timestamp to the matlab structure */
			*tmpptr++ = ((double) sbuf->timestamp) / SEC_TO_TSTAMP;
			/* copy the spike into the matlab structure */
			shortptr = sbuf->data;
			for (i = 0; i < NTOTAL_POINTS_PER_SPIKE; i++, shortptr++, 
				tmpptr++) {
			    *tmpptr = (double) *shortptr;
			}
			/* now that we have put the spike in spike[tmpnum], we need
			 * to move the pointer to the end of the data */
			spike[tmpnum] = tmpptr;
		    }
		    break;
		case CONTINUOUS_DATA_TYPE:
		    mtmp = (MatlabContBuffer *) databuf;
		    /* set the starttime if appropriate */
		    if (*starttime * SEC_TO_TSTAMP > mtmp->timestamp) {
			*starttime = ((double) mtmp->timestamp) / SEC_TO_TSTAMP;
		    }
		    /* copy the data to the correct elements of the matlab
		     * array. To correct for the fact that the user could have
		     * turned matlab saving on and off a few times, we
		     * calculate the index into the array for each element */
		    shortptr = mtmp->data;
		    for (i = 0; i < mtmp->nsamp; i++) {
			for (j = 0; j < mtmp->nchan; j++, shortptr++) {
			   *contptr[mtmp->electnum[j]-1]++ = *shortptr;
			}
		    }
		    for (j = 0; j < mtmp->nchan; j++) {
			/* assign the dsp channel numbers for each electrode */
			electdspmap[mtmp->electnum[j]-1] = 
			    mtmp->dspchan[mtmp->electnum[j]-1];
		    }
		    mexPrintf("done with one contbuf\n");
		    break;
                case POSITION_DATA_TYPE:
		    /* get the timestamp and the number of bright pixels */
		    posinfo = (u32 *) databuf;
		    memcpy(posinfo, databuf, size);
		    /* create a new mxArray for the time and put it into the
		     * position structure */
		    mdata = mxCreateDoubleMatrix(1, 1, mxREAL);
		    tmpptr = mxGetPr(mdata);
		    *tmpptr = ((double) posinfo[0]) / SEC_TO_TSTAMP;
		    mxSetField(plhs[2], posind, "time", mdata);

		    size = 2 * sizeof(u32);
		    shortptr = (short *) (databuf + size);

		    /* create a new mxArray for the x coordinates and put them 
		     * into the position structure */
		    mdata = mxCreateDoubleMatrix(1, posinfo[1], mxREAL);
		    tmpptr = mxGetPr(mdata);
		    for (i = 0; i < posinfo[1]; i++, shortptr++) {
			*tmpptr++ = *shortptr;
		    }
		    mxSetField(plhs[2], posind, "x", mdata);
		    /* create a new mxArray for the x coordinates and put them 
		     * into the position structure */
		    mdata = mxCreateDoubleMatrix(1, posinfo[1], mxREAL);
		    tmpptr = mxGetPr(mdata);
		    for (i = 0; i < posinfo[1]; i++, shortptr++) {
			*tmpptr++ = *shortptr;
		    }
		    mxSetField(plhs[2], posind, "y", mdata);
		    posind++;
		    break;
		case DIGITALIO_DATA_TYPE:
		    /* copy the three u32s */
		    dtmp = (DIOBuffer *) databuf;
		    /* put them in the digital IO variable */
		    break;
	    }
	}
	/* all of the data have been read, so close the socket and unlink the
	 * file */
	close(fd);
	unlink(MATLAB_SOCKET_NAME);
    }
    else {
	close(fd);
	unlink(MATLAB_SOCKET_NAME);
	mexPrintf("failed to open socket\n");
    }
    return;
}

void readerror(void) 
{
   usleep(500000);
   unlink(MATLAB_SOCKET_NAME);
   mexErrMsgTxt("problem reading data from socket");
}
