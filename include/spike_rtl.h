#include "spike_position.h"

/* defines for interaction with realtime linux kernel modules (eg. FIFOs) */
/* if you want to use more than 2 boards, change MAX_BOARDS  */
#define SPIKE_FIFO_SIZE			65536

#define SPIKE_CLOCK_INPUT_FIFO		0       // fifo for input to clock module
#define SPIKE_CLOCK_OUTPUT_FIFO		10       // fifo for output from clock module

#define SPIKE_BT878_INPUT_FIFO		2	// fifo for output from frame grabber module
#define SPIKE_BT878_OUTPUT_FIFO		3	// fifo for output from bttv module 
#define SPIKE_UEIDAQ_INPUT_FIFO		4 	// fifo for input to daq card module 
#define SPIKE_UEIDAQ_OUTPUT_FIFO	5 	// fifo for output from daq card module 
#define SPIKE_DIO_INPUT_FIFO		6 	// fifo for input to dio card module 
#define SPIKE_DIO_OUTPUT_FIFO		7 	// fifo for output from dio card module 

/* Defines for UEI AD board */
#define DAQ_DATA_TYPE		short

/* Defines for UEI DIO board */
#define NUM_PORTS		4
#define BITS_PER_PORT		16

/* BT878 frame grabber defines */
/* NOTE: changing these requires major changes to spike_BT878_module.c. I would
 * leave them alone unless you feel like hacking through the driver to get the
 * various settings right */
#define FRAME_RATE 30
#define NUM_CAMERAS 1
#define NUM_BUF_PER_CAMERA 4

#define SPIKE_UEI_SHARED_MEM_AREA1	"spike_uei_shm1" // data buffer 
#define SPIKE_UEI_SHARED_MEM_AREA2	"spike_uei_shm2" // timestamp buffer 
#define SPIKE_UEI_NRTBUFS		24  // allocate space for twenty four buffers 

#define SPIKE_BT878_SHARED_MEM_AREA1	"spike_bt878_shm1" // data buffer
#define SPIKE_BT878_SHARED_MEM_AREA2	"spike_bt878_shm2" // timestamp buffer
#define SPIKE_BT878_SHARED_MEM_AREA1_SIZE ((PIXELS_PER_LINE * LINES_PER_FIELD + sizeof(u32)) * NUM_BUF_PER_CAMERA)


/* set up some messaging defines for rtl communcation */
#define  RT_START_ACQUISITION	0
#define  RT_STOP_ACQUISITION	1
#define  RT_DAQ_INFO		2
#define  RT_DMA_SIZE		3
#define  RT_BUFFER_READY	4
#define  RT_TIMESTAMP		6
#define  RT_CLOCK_CONFIG	7
#define  RT_CLOCK_START		8
#define  RT_CLOCK_STOP		9
#define  RT_TEST_SYNC		10
#define  RT_CLOCK_READY		11
#define  RT_POS_INFO		12	
#define  RT_ERROR		16


