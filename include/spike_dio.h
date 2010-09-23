#ifndef __spike_dio_h
#define __spike_dio_h__

/* digital io related defines for use in user programs */
/* defines for the programs associated with behavior */
#define MAX_DIO_PROGRAMS 	10

#define DIO_TO_USER_MESSAGE		"/tmp/spike_digio_to_user_message"
#define USER_TO_DIO_MESSAGE		"/tmp/spike_user_to_digio_message"

#define DAQ_TO_USER_DATA      "/tmp/spike_daq_to_user_data"

// state machine commands
/* commands that can be sent to the master dsps state machine */
#define DIO_COMMAND		200	// for simplicity, this is set to avoid overlap with the messages in spike_defines.h
#define DIO_USER_MESSAGE	201	// for simplicity, this is set to avoid overlap with the messages in spike_defines.h
#define DIO_EVENT		202	// a status packet from the master dsp
#define DIO_POSITION		203	// the current animal position
#define DIO_EXPECT_DIO_RESPONSE		204	// 
#define DIO_COMMAND_TO_STATEMACHINE		205	// 
#define DIO_RUN_STATEMACHINE		206	// 
#define DIO_SPEC_STATEMACHINE		207	// 


#define DIO_MAX_COMMAND_LEN	DIO_STATE_SIZE // the maximum length of a dio command
#define DIO_S_HALT		0xffff  // halt the state machine
#define DIO_S_WAIT		0x8000  // wait for time in bits 1-15
#define DIO_S_WAIT_TIME		0x7000  // wait for absolute time given in argument
#define DIO_S_WAIT_MASKED_INPUT	0x6000  // wait for masked input to change
#define DIO_S_WAIT_INPUT_HIGH	0x4100  // wait for input in bits 1-6 to be high
#define DIO_S_WAIT_INPUT_LOW	0x4000  // wait for input in bits 1-6 to be low
#define DIO_S_SET_OUTPUT_HIGH	0x3100  // set output in bits 1-6 to be high
#define DIO_S_SET_OUTPUT_LOW	0x3000  // set output in bits 1-6 to be low
#define DIO_S_SET_PORT		0x2000  // set port in bits 1-2 to argument
#define DIO_S_JUMP_ABS		0x0100  // jump to absolute address in bits 1-8
#define DIO_S_JUMP_REL		0x0000  // jump to bits 1-8 instructions relative to current address
/* For Loop defines.  Note that these require MainDSP code rev 9 or greater */
#define	DIO_S_FOR_BEGIN		0x5000  // start for loop using bits 1-12 as iteration count
#define	DIO_S_FOR_END		0x5000  // end for loop

#define DIO_RESET_STATE_MACHINES 0xfffe	// command (interpreted by the main program) to reset all of the state machine pointers and stop execution of all state machines 


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
