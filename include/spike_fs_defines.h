/* Defines associated with the user functions that interact with spike_fsdata
 * */
#ifndef __SPIKE_FS_DEFINES_H__
#define __SPIKE_FS_DEFINES_H__

#include "stimcontrol_defines.h"
#include "spike_stimcontrol_defines.h"


/* the main message and data processing routines */
void ProcessMessage(int message, char *messagedata, int messagedatalen);
void ProcessData(int datatype, char *data, int datalen);
void ResetFSStatus(void);

#endif
