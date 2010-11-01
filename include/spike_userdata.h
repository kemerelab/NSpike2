#ifndef __SPIKE_USERDATA_H__
#define __SPIKE_USERDATA_H__

/* below are the default sizes for the data buffer in spike_userdata.cpp.  Note
 * that these can be overridden on the spike_userdata command line */
#define USER_DATA_BUFFER_SIZE 	10000000  // the size of each buffer in spike_userdata
#define USER_DATA_NUM_BUFFERS	10 // the default number of buffers in the circularly linked list

#define USER_DATA_SOCKET_NAME		"/tmp/user_data_spike_socket"

/* structure defining data types to send for export to spike_userdata*/
typedef struct _UserDataInfo {
    bool	sendcont;  // 1 if we are supposed to send continuous data
    bool	sendspike;  // 1 if we are supposed to send spike data
    bool	sendpos;  // 1 if we are supposed to send position data
    bool	senddigio;  // 1 if we are supposed to send digital IO data
    bool	contelect[MAX_ELECTRODE_NUMBER]; // 1 for continuous electrode that should be sent to spike_userdata  This will cause the continuous system to send whichever channel of the selected electrode out to spike_userdata
    bool	spikeelect[MAX_ELECTRODE_NUMBER]; // 1 for spiking electrodes that should be sent out to spike_userdata
    int 	ncont;  // the number of continuous electrodes selected
    int 	nspike;  // the number of spike electrodes selected
    int 	contnum[MAX_ELECTRODE_NUMBER];  // the numbers for the selected continuous electrodes, in order
    int 	spikenum[MAX_ELECTRODE_NUMBER];  // the numbers for the selected continuous electrodes, in order
} UserDataInfo;

typedef struct _UserDataContBuffer {
    u32         timestamp;
    short       samprate;
    short       nchan;
    short       nsamp;
    short	channum[MAX_CHAN_PER_DSP];  
    short       electnum[MAX_CHAN_PER_DSP];
    short       data[MAX_CONT_BUF_SIZE]; 
} UserDataContBuffer;

typedef SpikeBuffer UserSpikeDataBuffer;
typedef DIOBuffer UserDIODataBuffer;

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
