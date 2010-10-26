/* Defines associated with the user functions that interact with spike_userdata
 * */
#ifndef __SPIKE_USER_DEFINES_H__
#define __SPIKE_USER_DEFINES_H__

#include "stimcontrol_defines.h"
#include "spike_stimcontrol_defines.h"


/* the main message and data processing routines */
void ProcessMessage(int message, char *messagedata, int messagedatalen);
void ProcessData(int datatype, char *data, int datalen);
void ResetUserStatus(void);

#endif
