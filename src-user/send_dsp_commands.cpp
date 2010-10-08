
#include "stimcontrol_defines.h"

int SendStartDIOCommand(void)
    /* write data to the dsp. data must contain < 24 words */
{
    int                 size;
    unsigned short      dataout[1000];
    unsigned short address;
    unsigned short command[1];
    short n = 1;


    size = n*sizeof(unsigned short);
    command[0] = 1;
    address = DIO_STATE0_PTR;
    // address = digioinfo.statemachineptrs[s];

    /* create the programming command */
    dataout[0] = DEVICE_CONTROL_ADDR;
    dataout[1] = SHORT_WRITE;
    dataout[2] = DSP_SRAM;
    dataout[3] = address;
    dataout[4] = size;
    /* append data to the end of dataout */
    memcpy(dataout + 5, command, size);

    /* increment size to be the size of the full dataout array */
    size += 5 * sizeof(unsigned short);
    
    ByteSwap(dataout, 5 + n);

#ifdef NO_DSP_DEBUG
    return 1;
#else
    if (masterfd <= 0) {
      fprintf(stderr, "no masterfd");
      return -1;
    }

    /* send out the command */
    if (write(masterfd, dataout, size) != size) {
        fprintf(stderr, "Error sending message to dsp 0");
    }
#endif
    return 1;
}

