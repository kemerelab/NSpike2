/*
 * spike_util.cpp:  utility programs for the nspike package
 *
 * Copyright 2005 Loren M. Frank
 *
 * This program is part of the nspike data acquisition package.
 * nspike is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * nspike is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with nspike; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "spikecommon.h"

int strcount(char *s,char c);

u32 ParseTimestamp(char *s)
{
char	*ptr;
char	*ptr2;
u32	time;
u32 hour;
u32 min;
u32 sec;
float fracsec;
int	ncolons;
char	*fracptr;
char	timestr[100];	

    if(s == NULL){
	return(0);
    }
    /*
    ** copy the passed argument to the timestring for
    ** manipulation
    */
    strcpy(timestr,s);
    /*
    ** check for hr:min:sec.fracsec format vs min:sec
    */
    ncolons = strcount(timestr,':');
    fracsec = 0;
    if((fracptr = strchr(timestr,'.')) != NULL){
	sscanf(fracptr,"%f",&fracsec);
	*fracptr = '\0';
    };
    switch(ncolons){
    case 0:
	if(fracptr){
	    sscanf(timestr,"%d",(u32*)(&sec));
	    time = (u32) (((u32) sec)*1e4 + (fracsec*1e4 + 0.5)); 
	} else {
	    /*
	    ** straight timestamp
	    */
	    sscanf(timestr,"%d",(u32*)(&time));
	}
	break;
    case 1:
	/*
	** find the colon
	*/
	ptr = strchr(timestr,':');
	/*
	** separate the minutes and the seconds into two strings
	*/
	*ptr = '\0';
	/*
	** read the seconds after the colon
	*/
	sscanf(ptr+1,"%d",&sec);
	/*
	** read the minutes before the colon
	*/
	sscanf(timestr,"%d",&min);
	/*
	** compute the timestamp
	*/
	time = (u32) (min*6e5 + sec*1e4 + (fracsec*1e4 + 0.5)); 
	break;
    case 2:
	/*
	** find the first colon
	*/
	ptr = strchr(timestr,':');
	/*
	** find the second colon
	*/
	ptr2 = strchr(ptr+1,':');
	/*
	** separate the hours, minutes and the seconds into strings
	*/
	*ptr = '\0';
	*ptr2 = '\0';
	/*
	** read the hours before the first colon
	*/
	sscanf(timestr,"%d",&hour);
	/*
	** read the minutes before the second colon
	*/
	sscanf(ptr+1,"%d",&min);
	/*
	** read the seconds after the colon
	*/
	sscanf(ptr2+1,"%d",&sec);
	/*
	** compute the timestamp
	*/
	time = (u32) (hour*36e6 + min*6e5 + sec*1e4 + (fracsec*1e4 + 0.5)); 
	break;
    default:
	fprintf(stderr,"unable to parse timestamp '%s'\n",timestr);
	return(0);
    }
    return(time);
}

int strcount(char *s,char c)
{
int	count;

    if(s == NULL) return(0);
    count = 0;
    while(*s != '\0'){
	if(*s == c) count++;
	s++;
    }
    return(count);
}


