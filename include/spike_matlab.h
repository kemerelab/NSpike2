#ifndef __SPIKE_MATLAB_H__
#define __SPIKE_MATLAB_H__

/* below are the default sizes for the data buffer in spike_matlab.cpp.  Note
 * that these can be overridden on the spike_matlab command line */
#define MATLAB_BUFFER_SIZE 	10000000  // the size of each matlab buffer in spike_matlab
#define MATLAB_NUM_BUFFERS	10 // the default number of buffers in the circularly linked list

#define MATLAB_SOCKET_NAME		"/tmp/matlab_spike_socket"

/* structure defining data types to save for export to matlab */
typedef struct _MatlabInfo {
    bool	savecont;  // 1 if we are supposed to save continuous data
    bool	savespike;  // 1 if we are supposed to save spike data
    bool	savepos;  // 1 if we are supposed to save position data
    bool	savedigio;  // 1 if we are supposed to save digital IO data
    bool	contelect[MAX_ELECTRODE_NUMBER]; // 1 for continuous electrode that should be saved for matlab.  This will cause the continuous system to send whichever channel of the selected electrode out to spike_matlab
    bool	spikeelect[MAX_ELECTRODE_NUMBER]; // 1 for spiking electrodes that should be saved for matlab
} MatlabInfo;

typedef struct _MatlabContBuffer {
    u32         timestamp;
    short       samprate;
    short       nchan;
    short       nsamp;
    short	dspchan[MAX_CHAN_PER_DSP];  // this is used for calibration only
    short       electnum[MAX_CHAN_PER_DSP];
    short       data[MAX_CONT_BUF_SIZE]; 
} MatlabContBuffer;


typedef struct _MatlabDataBufferInfo {
    MatlabInfo	matlabinfo;
    int		nspikes[MAX_ELECTRODE_NUMBER];
    int		maxspiketetnum;
    u32		conttimes[2];
    int		ncontsamp[MAX_ELECTRODE_NUMBER];
    int		maxconttetnum;
    int		nposbuf;
    int		ndigiobuf;
} MatlabDataBufferInfo;

#endif
