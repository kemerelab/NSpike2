#ifndef __spike_dio_h
#define __spike_dio_h__

/* digital io related defines for use in user programs */
/* defines for the programs associated with behavior */
#define MAX_DIO_PROGRAMS 	10

#define DIO_TO_FS_MESSAGE		"/tmp/spike_digio_to_fs_message"
#define FS_TO_DIO_MESSAGE		"/tmp/spike_fs_to_digio_message"

#define DAQ_TO_FS_DATA      "/tmp/spike_daq_to_fs_data"

// state machine commands
/* commands that can be sent to the master dsps state machine */
#define DIO_COMMAND			200	// for simplicity, this is set to avoid overlap with the messages in spike_defines.h
#define DIO_FS_MESSAGE		201	// for simplicity, this is set to avoid overlap with the messages in spike_defines.h
#define DIO_EVENT			202	// a status packet from the master dsp
#define DIO_POSITION			203	// the current animal position
#define DIO_EXPECT_DIO_RESPONSE		204	// 
#define DIO_COMMAND_TO_STATEMACHINE	205	// 
#define DIO_RUN_STATEMACHINE		206	// 
#define DIO_SPEC_STATEMACHINE		207	// 


#define DIO_MAX_COMMAND_LEN	DIO_STATE_SIZE // the maximum length of a dio command
#define DIO_S_HALT		0xffff  // halt the state machine
#define DIO_S_WAIT		0x8000  // wait for # samples in bits 1-15
#define DIO_S_WAIT_TIME		0x7000  // wait for absolute time given in argument
#define DIO_S_WAIT_MASKED_INPUT	0x6000  // wait for masked input to change
#define DIO_S_WAIT_INPUT_HIGH	0x4100  // wait for input in bits 1-6 to be high
#define DIO_S_WAIT_INPUT_LOW	0x4000  // wait for input in bits 1-6 to be low
#define DIO_S_SET_OUTPUT_HIGH	0x3100  // set output in bits 1-6 to be high
#define DIO_S_SET_OUTPUT_LOW	0x3000  // set output in bits 1-6 to be low
#define DIO_MAX_WAIT_SAMP	16382   // We can wait at most 16382 samples per wait command

#define DIO_S_SET_PORT		0x2000  // set port in bits 1-3 to argument

#define DIO_S_JUMP_ABS		0x0100  // jump to absolute address in bits 1-8
#define DIO_S_JUMP_REL		0x0000  // jump to bits 1-8 instructions relative to current address
/* For Loop defines.  Note that these require MainDSP code rev 9 or greater */
#define	DIO_S_FOR_BEGIN		0x5000  // start for loop using bits 1-12 as iteration count
#define	DIO_S_FOR_END		0x5000  // end for loop

#define DIO_RESET_STATE_MACHINES 0xfffe	// command (interpreted by the main program) to reset all of the state machine pointers and stop execution of all state machines 

/* definitions specific to location of DIO */
#ifdef DIO_ON_MASTER_DSP
#define DIO_STATE_SIZE		62	// instructions per state machine. The first instruction is always left at "wait forever", and the final instruction is always a jump to instruction 0, so there are 64 - 2 = 62 instructions available for programming 
#else
#define DIO_STATE_SIZE	   	  65534	// instructions per state machine. The first instruction is always left at "wait forever", and the final instruction is always a jump to instruction 0, so there are 65536 - 2 instructions available for programming 
#define DIO_S_WAIT_WAIT		0xC000  // wait from last wait command
#define DIO_AOUT1_PORT		0x0004  // the port index for AOUT 1 to be used in conjuction with DIO_S_SET_PORT
#define DIO_AOUT2_PORT		0x0005  // the port index for AOUT 2 to be used in conjuction with DIO_S_SET_PORT

#define DIO_AOUT_BASE_ADDR	0xC000
#define DIO_AOUT1_ADDR		0x0000
#define DIO_AOUT2_ADDR		0x0001
#define DIO_ARB_WAVE_ADDR	  0x4114 // base location for the arbitrary waveform generator

#define DIO_ARB_MAX_WAVE_LEN	DIO_STATE_SIZE // the maximum length of an analog waveform for the arbitrary waveform generator
#define DIO_ARB_MAX_WAVE_LEN_MS	(DIO_STATE_SIZE / SAMP_TO_TIMESTAMP / 10) // the maximum length in milliseconds of an analog waveform for the arbitrary waveform generator
#define DIO_ARB_ENABLE		0x00C4  // 0 disables, 1 enables, read for status
#define DIO_ARB_TRIGGER	0x00C5  // first trigger info in high byte, retrigger in low byte
#define DIO_ARB_NEVER_TRIGGER  0
#define DIO_ARB_ALWAYS_TRIGGER  1
#define DIO_ARB_DIO_OUTPUT1_TRIGGER   0x0020  // OR with 0x000b to trigger off of high bit b of port 1 output
#define DIO_ARB_DIO_OUTPUT2_TRIGGER   0x0030  // OR with 0x000b to trigger off of high bit b of port 2 output
#define DIO_ARB_DIO_OUTPUT3_TRIGGER   0x0040  // OR with 0x000b to trigger off of high bit b of port 3 output
#define DIO_ARB_DIO_OUTPUT4_TRIGGER   0x0050  // OR with 0x000b to trigger off of high bit b of port 4 output
#define DIO_ARB_DIO_INPUT1_TRIGGER   0x0060  // OR with 0x000b to trigger off of high bit b of port 1 input
#define DIO_ARB_DIO_INPUT2_TRIGGER   0x0070  // OR with 0x000b to trigger off of high bit b of port 2 input
#define DIO_ARB_DIO_INPUT3_TRIGGER   0x0080  // OR with 0x000b to trigger off of high bit b of port 3 input
#define DIO_ARB_DIO_INPUT4_TRIGGER   0x0090  // OR with 0x000b to trigger off of high bit b of port 4 input

#define DIO_ARB_LENGTH			0x00C6 // 16 bit # of points in buffer
#define DIO_ARB_POINTER		0x00C7 // 16 bit pointer to offset of data (set to 0 when initializing)
#define DIO_ARB_AOUT_CHANNEL_ADDR		0x00C8 // Address for AOUT channel setting
#define DIO_ARB_AOUT_CHANNEL_1		0x0000 // use AOUT 1
#define DIO_ARB_AOUT_CHANNEL_2		0x0001 // use AOUT 2
#endif


#define		MAX_BITS			64
#define		MAX_PORTS			4

#define		OUTPUT_PULSE_LENGTH		2100 // 2000 timesteps (.2 seconds) 
#define		MAX_OUTPUT_PULSE_LENGTH		50000 // (5 seconds) 

#define 	DEFAULT_DIO_DEBOUNCE_TIME	3000 // 3000 samples = 100 ms

#define		INPUT_PORT			0
#define		OUTPUT_PORT			1

/* bits for one output port */
#define 	O0				1
#define 	O1				(1<<1)
#define 	O2				(1<<2)
#define 	O3				(1<<3)
#define 	O4				(1<<4)
#define 	O5				(1<<5)
#define 	O6				(1<<6)
#define 	O7				(1<<7)
#define 	O8				(1<<8)
#define 	O9				(1<<9)
#define 	O10				(1<<10)
#define 	O11				(1<<11)
#define 	O12				(1<<12)
#define 	O13				(1<<13)
#define 	O14				(1<<14)
#define 	O15				(1<<15)

#endif


