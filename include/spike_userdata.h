#ifndef __SPIKE_USER_DATA_H__
#define __SPIKE_USER_DATA_H__

/* below are the default sizes for the data buffer in spike_userdata.cpp.  Note
 * that these can be overridden on the spike_userdata command line */
#define USER_DATA_BUFFER_SIZE 	10000000  // the size of each buffer in spike_userdata
#define USER_DATA_NUM_BUFFERS	10 // the default number of buffers in the circularly linked list

#define USER_DATA_SOCKET_NAME		"/tmp/user_data_spike_socket"

/* structure defining data types to save for export to spike_userdata*/
typedef struct _UserDataInfo {
    bool	savecont;  // 1 if we are supposed to save continuous data
    bool	savespike;  // 1 if we are supposed to save spike data
    bool	savepos;  // 1 if we are supposed to save position data
    bool	savedigio;  // 1 if we are supposed to save digital IO data
    bool	contelect[MAX_ELECTRODE_NUMBER]; // 1 for continuous electrode that should be sent to spike_userdata  This will cause the continuous system to send whichever channel of the selected electrode out to spike_userdata
    bool	spikeelect[MAX_ELECTRODE_NUMBER]; // 1 for spiking electrodes that should be sent out to spike_userdata
} UserDataInfo;

typedef struct _UserDataContBuffer {
    u32         timestamp;
    short       samprate;
    short       nchan;
    short       nsamp;
    short	dspchan[MAX_CHAN_PER_DSP];  // this is used for calibration only
    short       electnum[MAX_CHAN_PER_DSP];
    short       data[MAX_CONT_BUF_SIZE]; 
} UserDataContBuffer;


typedef struct _UserDataBufferInfo {
    UserDataInfo	userdatainfo;
    int		nspikes[MAX_ELECTRODE_NUMBER];
    int		maxspiketetnum;
    u32		conttimes[2];
    int		ncontsamp[MAX_ELECTRODE_NUMBER];
    int		maxconttetnum;
    int		nposbuf;
    int		ndigiobuf;
} UserDataBufferInfo;

#endif
