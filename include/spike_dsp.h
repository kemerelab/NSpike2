#ifndef __SPIKE_DSP_H__
#define __SPIKE_DSP_H__

#include "spike_dio.h"
#include "spike_dsp_shared.h"

/* DSP digital IO commands */
void StartDigIOProgram(int prognum);
int TriggerOutput(int output);
void ChangeOutput(int output, int raise);
//void SendDAQFSMessage(int message, char *data, int datalen);
void SendDigIOFSMessage(char *message, int len);

/* Defines for DSPs */

#define NDSP_LOW_FILTERS	5
#define DSP_LOW_FILTERS		{1,10,100,300,600}

#define NDSP_HIGH_FILTERS	6
#define DSP_HIGH_FILTERS	{100,400,3000,6000,9000,11000}

#define NDSP_AUDIO_CHANNELS	2
#define MAX_DSP_AUDIO_GAIN	128
#define MAX_DSP_AUDIO_CUTOFF	32767
#define MAX_DSP_AUDIO_DELAY	500	// in ms
#define DEFAULT_DSP_AUDIO_GAIN		100  
#define DEFAULT_DSP_AUDIO_CUTOFF	0
#define DEFAULT_DSP_AUDIO_DELAY		0
#define MAX_DSP_DIO_PORTS	4  // Note that this could cause problems if there are only 2 ports (32 channels) of digitial IO

#define DSP_PAUSE_USEC		10000  // the time to sleep between dsp writes or reads

#define DSP_POS_SYNC_CHAN	63  // the channel that can be used for the position sync signal
#define DSP_VIDEO1_SYNC_ODD	BIT15	// value for odd frame
#define DSP_ODD_TO_EVEN_SYNC	1 // Odd to even transition
#define DSP_EVEN_TO_ODD_SYNC	0 // Even to odd transition

/* delays for analog output. This can be used to improve the synchronization of
 * the visual and auditory traces */
#define DAC_0_DELAY		0  // the delay, in samples, of analog output 0
#define DAC_1_DELAY		0  // the delay, in samples, of analog output 1


/* addresses to check or send commands to devices */
#define ENUMERATE_ADDR		0xFFFE   // identify device
#define DEVICE_CONTROL_ADDR	0xFFFB   // control device

/* DSP data locations */
#define DSP_SRAM		0x4000  // base location for dsp memory. This is passed in as the high word of addresses when writing to the DSP 
#define DSP_PROGRAM_ADDR	0x4200  // base location for writing to the DSP EEPROM for reprogramming
#define DSP_PROGRAM_SIZE	4080  // 4080 words in a DSP program

/* Digital / Analog IO DSP data locations */
#define DSP_ANALOG_OUT_ADDR	0xC000  // base location for analog outs from DIO DSP

#ifdef DIO_ON_MASTER_DSP
#define DIO_STATE0_BASE_ADDR  DSP_SRAM // base location for programming
#define DIO_STATE1_BASE_ADDR  DSP_SRAM // base location for programming
#define DIO_STATE2_BASE_ADDR  DSP_SRAM // base location for programming
#define DIO_STATE3_BASE_ADDR  DSP_SRAM // base location for programming
#define DIO_STATE0_BUFFER_START	0x0200 // the start offset for the state pointer for state machine 0
#define DIO_STATE1_BUFFER_START	0x0240 // the start offset for the state pointer for state machine 1
#define DIO_STATE2_BUFFER_START	0x0280 // the start offset for the state pointer for state machine 2
#define DIO_STATE3_BUFFER_START	0x02C0 // the start offset for the state pointer for state machine 3

#else
/* each state machine has its own base address  */
#define DIO_STATE0_BASE_ADDR  0x4110 // base location for programming
#define DIO_STATE1_BASE_ADDR  0x4111 // base location for programming
#define DIO_STATE2_BASE_ADDR  0x4112 // base location for programming
#define DIO_STATE3_BASE_ADDR  0x4113 // base location for programming
#define DIO_STATE0_BUFFER_START	  0 // the start offset for the state pointer for state machine 0
#define DIO_STATE1_BUFFER_START	  0 // the start offset for the state pointer for state machine 1
#define DIO_STATE2_BUFFER_START	  0 // the start offset for the state pointer for state machine 2
#define DIO_STATE3_BUFFER_START	  0 // the start offset for the state pointer for state machine 3
#define NUM_ARBS	1 // Currently we have one arbitrary waveform generator
#endif



/* commands to read and write data */
#define SHORT_READ		0x80     // read a few bytes
#define SHORT_WRITE		0x82     // write a few bytes
#define MAX_SHORT_WRITE_SIZE	1000	// we can write at most 1000 bytes at a time
#define SERIAL_WRITE_READ	0x85     // write bytes and then read them back
#define RESET_DEVICE		0x84	 // reset the device as if it had been power cycled

/* DSP addresses */
#define	DSP_CODE_REV_ADDR	0x13  // The current revision number
#define RESYNC_COUNT_ADDR	0xC3  // test for synchronization error (not functional)
#define DAC_0_DELAY_ADDR	0xCC  // delay, in samples, for audio output 0
#define DAC_1_DELAY_ADDR	0xCD  // delay, in samples, for audio output 1
#define SYNC_CONTROL_ADDR	0xCF  // control synchronization betwen dsps

/* Main DSP addresses */
#define DAC_0_GAIN_ADDR		0xD1  // gain for DAC 0
#define DAC_1_GAIN_ADDR		0xD2  // gain for DAC 1
#define DAC_0_POS_THRESH_ADDR	0xD3  // the positive threshold for the diode emulation for DAC 0
#define DAC_0_NEG_THRESH_ADDR	0xD4  // the negative threshold for the diode emulation for DAC 0
#define DAC_1_POS_THRESH_ADDR	0xD5  // the positive threshold for the diode emulation for DAC 1
#define DAC_1_NEG_THRESH_ADDR	0xD6  // the negative threshold for the diode emulation for DAC 1
#define MASTER_SAMPLE_COUNT_LOW_ADDR	0xCE  // sample count low word for master
#define MASTER_SAMPLE_COUNT_HIGH_ADDR 0xD0  // sample count high word for master

/* Main DSP Digital IO address */
#define DIO_OUT_ENABLE		0xD9   	// bit 0 = 1 -> bits 1-16 output
					// bit 1 = 1 -> bits 17-32 output
					// bit 2 = 1 -> bits 33-48 output
					// bit 3 = 1 -> bits 49-64 output
					// value of 0 = input
#define DIO_OUT_1		0xDA	// control output bits 1-16
#define DIO_OUT_2		0xDB	// control output bits 17-32
#define DIO_OUT_3		0xDC	// control output bits 33-48
#define DIO_OUT_4		0xDD	// control output bits 49-64
#define DIO_IN_1		0xDE	// state of input bits 1-16
#define DIO_IN_2		0xDF	// state of input bits 17-32
#define DIO_IN_3		0xE0	// state of input bits 33-48
#define DIO_IN_4		0xE1	// state of input bits 33-48
#define DIO_N_STATE_MACHINES	4	// the total number of state machines
#define DIO_STATE_AVAILABLE	0xE7	// the index of a currently available state machine
#define DIO_STATE_MACHINES_BUSY -1		// the result of reading from the DIO_S_AVAILABLE pointer if no state machines are free

#ifdef DIO_ON_MASTER_DSP
#define DIO_STATE_ENABLE	0xED	// if bit 0 = 1, run the state machine
#else
#define DIO_STATE0_ENABLE	0xD4	// if bit 0 = 1, run the state machine
#define DIO_STATE1_ENABLE	0xD5	// if bit 0 = 1, run the state machine
#define DIO_STATE2_ENABLE	0xD6	// if bit 0 = 1, run the state machine
#define DIO_STATE3_ENABLE	0xD7	// if bit 0 = 1, run the state machine
#endif



#define DIO_STATE0_PTR		0xF0	// Pointer to current instruction for state machine 0
#define DIO_STATE1_PTR		0xF3	// Pointer to current instruction for state machine 1
#define DIO_STATE2_PTR		0xF6	// Pointer to current instruction for state machine 2
#define DIO_STATE3_PTR		0xF9	// Pointer to current instruction for state machine 3
#define DIO_MESSAGE_SIZE	14	// 7 * sizeof(unsigned short) bytes per digital IO message packet

#define DIO_IN_1_MASK		0xE2	// Mask for input 1
#define DIO_IN_2_MASK		0xE3	// Mask for input 2
#define DIO_IN_3_MASK		0xE4	// Mask for input 3
#define DIO_IN_4_MASK		0xE5	// Mask for input 4
#define DIO_IN_DEBOUNCE_TIME	0xE8	// debounce time for inputs
#define DIO_PIPE_ID		0xEE	// ID for packets containing changed state information

/* aux DSP addresses */
#define PIPE_ID_ADDR		0xD0  // the ID # for each DSP
#define BLOCKS_TO_SEND_ADDR	0xD2  // the number of packets to send
#define NUM_CHAN_ADDR		0xD3  // the number of channels on this dsp
#define NUM_SAMPLES_ADDR	0xD4  // the number of samples per packet
#define DECIMATION_ADDR		0xD5  // the decimation factor
#define SAMPLE_COUNT_LOW_ADDR   0xD8  // 
#define SAMPLE_COUNT_HIGH_ADDR  0xD9  // 
#define DIO_IN_DEBOUNCE_TIME	0xE8	// debounce time for inputs


/* Main and aux DSP addresses for channel information */
#define CHANNEL_SETTINGS_ADDR	0x180 // the start of the channel settings
#define DSP_CHAN_ADDR_INC	24    // the number of words per channel

/* The offsets from the base address of the channel settings. Normally these
 * will be written in blocks */
#define SRC1_ADDR		0     // the channel number for source 1
#define SRC1_GAIN_ADDR		1     // the gain for source 1 
#define SRC2_ADDR		2     // the channel number for source 2
#define SRC2_GAIN_ADDR		3     // the gain for source 2 
#define FILTER_ADDR		4     // the starting address for the filters
#define LPF_A2_LO_ADDR		4     // low pass filter coeff a2 low word
#define DIO_IN_DEBOUNCE_TIME	0xE8	// debounce time for inputs
#define LPF_A2_HI_ADDR		5     // low pass filter coeff a2 high word
#define LPF_A1_LO_ADDR		6     // low pass filter coeff a1 low word
#define LPF_A1_HI_ADDR		7     // low pass filter coeff a1 high word
#define LPF_A0_LO_ADDR		8     // low pass filter coeff a0 low word
#define LPF_A0_HI_ADDR		9     // low pass filter coeff a0 high word
#define LPF_B2_LO_ADDR		10     // low pass filter coeff b2 low word
#define LPF_B2_HI_ADDR		11     // low pass filter coeff b2 high word
#define LPF_B1_LO_ADDR		12     // low pass filter coeff b1 low word
#define LPF_B1_HI_ADDR		13     // low pass filter coeff b1 high word
#define HPF_A2_LO_ADDR		14     // high pass filter coeff a2 low word
#define HPF_A2_HI_ADDR		15     // high pass filter coeff a2 high word
#define HPF_A1_LO_ADDR		16     // high pass filter coeff a1 low word
#define HPF_A1_HI_ADDR		17     // high pass filter coeff a1 high word
#define HPF_A0_LO_ADDR		18     // high pass filter coeff a1 low word
#define HPF_A0_HI_ADDR		19     // high pass filter coeff a0 high word
#define HPF_B2_LO_ADDR		20     // high pass filter coeff b2 low word
#define HPF_B2_HI_ADDR		21     // high pass filter coeff b2 high work
#define HPF_B1_LO_ADDR		22     // high pass filter coeff b1 low word
#define HPF_B1_HI_ADDR		23     // high pass filter coeff b1 high word

/* DSP Ports */
#define DSP_ECHO_PORT		7
#define DSP_MESSAGE_PORT	4001
#define DSP_DATA_PORT		4003


typedef struct _DigIOInfo {
    unsigned short statemachineptr[DIO_N_STATE_MACHINES];  // the current pointer for each state machines */
    unsigned short statemachinebaseaddr[DIO_N_STATE_MACHINES];  // the base address for each state machine
    unsigned short statemachinebuffer[DIO_N_STATE_MACHINES];  // the start of the memory buffer to be written to for each state machine
    int nports;
    int porttype[MAX_DSP_DIO_PORTS];  // 0 for input, 1 for output 
    int nprograms;
    char progname[MAX_DIO_PROGRAMS][200]; 
    int currentprogram;
    int inputfd;
    int outputfd;
    int rewardlength[MAX_BITS];
    int raised[MAX_BITS];
    int enable_DAQ_TO_FS;
} DigIOInfo;


typedef struct _CommonDSPInfo {
    short nLowFilters;
    short lowFilt[NDSP_LOW_FILTERS];
    short nHighFilters;
    short highFilt[NDSP_HIGH_FILTERS];
    ChannelInfo audiochan[NDSP_AUDIO_CHANNELS];
    short audiogain[NDSP_AUDIO_CHANNELS];
    short audiocutoff[NDSP_AUDIO_CHANNELS]; // used to emulate back to back diodes.  The DSP takes the output signal and, if it is positive, subtracts the cutoff.  If the result is less than zero, the output is set to zero.  For negative signals the cutoff is added to the signal. 
    short audiodelay[NDSP_AUDIO_CHANNELS]; // delays the audio output to make it more coincident with the display. 
    short mute;
    short dspcal[MAX_ELECTRODES];  // the calibration factors for each channel
} CommonDSPInfo;

typedef struct _ArbInfo {
    int aout;
    bool continuous;
    unsigned short trigger_pin;
    unsigned short trigger;
    unsigned short wavefm[DIO_ARB_MAX_WAVE_LEN];
    unsigned short len;
} ArbInfo; 

#endif
