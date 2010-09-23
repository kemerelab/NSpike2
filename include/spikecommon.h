#ifndef __SPIKE_COMMON_H
#define __SPIKE_COMMON_H
/* includes for all modules */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <limits.h>
#include <zlib.h>	// for reading and writing compressed files 

#include "spike_position.h"
#include "spike_defines.h"
#include "spike_dsp.h"
#include "spike_functions.h"
#include "spike_rtl.h"
#include "spike_matlab.h"

#endif
