/*
 * spike_message.cpp: Shared code for messaging between nspike modules 
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
#include <ctype.h>
extern SysInfo sysinfo;
extern NetworkInfo netinfo;

static char tmpdata[SAVE_BUF_SIZE];

int GetServerSocket(const char *name) 
    /* returns the file descriptor for a non-blocking server socket */
{
    struct sockaddr_un      address;
    int                     socketid;
    int                     conn;
    socklen_t                  addrlen;

    //fd_set                      servercon;
    //struct timeval              timeout;

    /* get the socket */
    if ((socketid = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
        fprintf(STATUSFILE,"Error in GetServerSocket: unable to get socket");
        return -1;
    }

    /* make this a UNIX domain socket */
    address.sun_family = AF_UNIX;
    /* unlink the socket */
    unlink(name);
    /* assign the path */
    strcpy(address.sun_path, name);
    /* the total length of the address is the sum of the lengths of the family
     * and path elements */
    addrlen = sizeof(address.sun_family) + sizeof(address.sun_path);
    /* bind the address to the socket */
    if (bind(socketid, (struct sockaddr *) & address, addrlen)) {
        fprintf(STATUSFILE, "GetServerSocket: Error binding server socket to address in module %d\n", sysinfo.program_type);
        return -1;
    }
    if (listen(socketid, 5)) {
        fprintf(STATUSFILE, "Error listening to server socket");
        return -1;
    }
    /* get the socket (waits for "connect" from client)*/
    if ((conn = accept(socketid, (struct sockaddr * ) &address, &addrlen)) < 0) {
        fprintf(STATUSFILE,"GetServerSocket: Error connecting to socket");
        return -1;
    }

    return conn;
}

int GetServerSocket(const char *name, int timeoutsec, int timeoutusec) 
    /* returns the file descriptor for a non-blocking server socket */
{
    struct sockaddr_un      address;
    int                     socketid;
    int                     conn;
    socklen_t                  addrlen;

    //fd_set                      servercon;
    //struct timeval              timeout;

    /* get the socket */
    if ((socketid = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
        fprintf(STATUSFILE,"Error in GetServerSocket: unable to get socket %s\n", name);
        return -1;
    }

    /* make this a UNIX domain socket */
    address.sun_family = AF_UNIX;
    /* unlink the socket */
    unlink(name);
    /* assign the path */
    strcpy(address.sun_path, name);
    /* the total length of the address is the sum of the lengths of the family
     * and path elements */
    addrlen = sizeof(address.sun_family) + sizeof(address.sun_path);
    /* bind the address to the socket */
    if (bind(socketid, (struct sockaddr *) & address, addrlen)) {
        fprintf(STATUSFILE, "GetServerSocket: Error binding server socket %s to address in module %d\n", name, sysinfo.program_type);
        return -1;
    }
    if (listen(socketid, 5)) {
        fprintf(STATUSFILE, "Error listening to server socket %s\n", name);
        return -1;
    }

    /* set the socket to be non-blocking */
    fcntl(socketid, F_SETFL, O_NONBLOCK);

    timeoutusec += timeoutsec * 1000000;
    do {
        conn = accept(socketid, (struct sockaddr * ) &address, &addrlen);
        if (conn < 0) {
            usleep(50000);
            timeoutusec -= 50000;
        }
    } while ((conn < 0) && (timeoutusec > 0));


    return conn;
}

 
int GetClientSocket(const char *name) 
    /* returns the file descriptor for a non-blocking client socket */
{
    struct sockaddr_un                address;
    int                               socketid;
    size_t                            addrlen;
    fd_set                              servercon;
    struct timeval                      timeout;

    /* set a timeout */
    timeout.tv_sec = SOCKET_CLIENT_TIMEOUT;
    timeout.tv_usec = 0;

    /* get the socket */
    if ((socketid = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
        fprintf(STATUSFILE,"GetClientSocket: unable to get socket %s\n",name);
        return 0;
    }

    /* make this a UNIX domain socket */
    address.sun_family = AF_UNIX;

    /* assign the path */
    strcpy(address.sun_path, name);

    /* the total length of the address is the sum of the lengths of the family
     * and path elements */
    addrlen = sizeof(address.sun_family) + sizeof(address.sun_path);

    /* set up fd_set to wait for a connection from the server */
    FD_ZERO(&servercon);
    FD_SET(socketid, &servercon);
    
    /* wait for a connection on the socket (blocks on server executing "accept")*/
    if (select(socketid+1, NULL, &servercon, NULL, &timeout) == 0) {
        fprintf(STATUSFILE,"GetClientSocket: Error waiting for socket to be ready\n");
        return -1;
    }

    /* connect to the socket */
    while (connect(socketid, (struct sockaddr *) & address, addrlen) < 0) {
        //fprintf(STATUSFILE,"GetClientSocket: Waiting for server on socket %s\n", name);
        usleep(100000);
    }

    return(socketid);
}

int GetClientSocket(const char *name, int timeoutsec, int timeoutusec) 
    /* returns the file descriptor for a non-blocking client socket */
{
    struct sockaddr_un                address;
    int                               socketid;
    size_t                            addrlen;
    fd_set                              servercon;
    struct timeval                      timeout;

    /* set a timeout */
    timeout.tv_sec = timeoutsec;
    timeout.tv_usec = timeoutusec;

    /* get the socket */
    if ((socketid = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
        fprintf(STATUSFILE,"GetClientSocket: unable to get socket %s\n",name);
        return -1;
    }

    /* make this a UNIX domain socket */
    address.sun_family = AF_UNIX;

    /* assign the path */
    strcpy(address.sun_path, name);

    /* the total length of the address is the sum of the lengths of the family
     * and path elements */
    addrlen = sizeof(address.sun_family) + sizeof(address.sun_path);

    /* set up fd_set to wait for a connection from the server */
    FD_ZERO(&servercon);
    FD_SET(socketid, &servercon);
    
    /* wait for a connection on the socket */
    if (select(socketid+1, NULL, &servercon, NULL, &timeout) == 0) {
        return -1;
    }

    timeoutusec += timeoutsec * 1000000;
    /* connect to the socket */
    while ((timeoutusec > 0) && 
            (connect(socketid, (struct sockaddr *) & address, addrlen) < 0)) {
        usleep(100000);
        timeoutusec -= 100000;
    }
    if (timeoutusec <= 0) {
        close(socketid);
        unlink(name);
        return -1;
    }
    return(socketid);
}

int SendMessage(int fd, int message, const char *data, int datalen)
{
    int nwritten;
    int totalwritten;
    int writesize;

    /* send the message */
    if (write(fd, &message, sizeof(int)) == -1) {
       fprintf(STATUSFILE, "Error: unable to write message in SendMessage in program %d, message %d, fd %d \n", sysinfo.program_type, message, fd);
       return -1;
    }
    write(fd, &datalen, sizeof(int));
    nwritten = 0;
    totalwritten = 0;
    while (datalen > 0) {
        writesize = datalen > MAX_SOCKET_WRITE_SIZE ? MAX_SOCKET_WRITE_SIZE : datalen;
        nwritten = write(fd, data+totalwritten, writesize);
        if (nwritten == -1) {
           fprintf(STATUSFILE, "Error: unable to write data in SendMessage in program %d\n", sysinfo.program_type);
           return -1;
        }
        else {
            totalwritten += nwritten;
            datalen -= nwritten;
        } 
    }
    return 1;
}

int SendMessage(SocketInfo *c, int message, const char *data, int datalen)
    /* overloaded version of SendMessage.  This version is necessary if you
     * want to send UDP packets */
{
    int nwritten;
    int totalwritten;
    int writesize;
    int maxwritesize;

    /* send the message */
    if (write(c->fd, &message, sizeof(int)) == -1) {
       fprintf(STATUSFILE, "Error: unable to write complete buffer in SendMessage in program %d, message %d \n", sysinfo.program_type, message);
       return -1;
    }
    write(c->fd, &datalen, sizeof(int));
    nwritten = 0;
    totalwritten = 0;
    if (c->protocol == UDP) {
        maxwritesize = 1000;
    }
    else {
        maxwritesize = MAX_SOCKET_WRITE_SIZE;
    } 
    while (datalen > 0) {
        writesize = datalen > maxwritesize ? maxwritesize : datalen;
        nwritten = write(c->fd, data+totalwritten, writesize);
        if (nwritten == -1) {
           fprintf(STATUSFILE, "Error: unable to write complete buffer in SendMessage in program %d\n", sysinfo.program_type);
           return -1;
        }
        else {
            totalwritten += nwritten;
            datalen -= nwritten;
        } 
    }
    return 1;
}

int GetMessage(int fd, char *messagedata, int *datalen, int block)
    /* gets a message, if one is present.  */
{
    int message;
    int ntoread, nread, totalread, readsize;

    /* read the message */
    if (block) {
        while (read(fd, &message, sizeof(int)) < 0) ;
    }
    else if (read(fd, &message, sizeof(int)) <= 0) {
        return -1;
    }
    read(fd, datalen, sizeof(int));
    if (*datalen > 0) {
        totalread = 0;
        /* read into messagedata */
        ntoread = *datalen;
        while (ntoread > 0) {
            readsize = ntoread > MAX_SOCKET_WRITE_SIZE ? MAX_SOCKET_WRITE_SIZE : ntoread;
            /*if (sysinfo.program_type == SPIKE_DAQ) {
                readsize = 1000;
            } */
            nread = read(fd, messagedata+totalread, readsize);
            if (nread != -1) {
                totalread += nread;
                ntoread -= nread;
            } 
	    if (block && nread == 0) {
		/* somehow we've lost some data */
		return -1;
	    }
        }
    }
    return message;
}

int WaitForMessage(int fd, int message, float sec) 
    /* wait up to about sec seconds for a message on the specified socket and return 1 
     * if the message was recieved and 0 otherwise */
{

    fd_set                readfds;  // the set of readable fifo file descriptors 
    struct timeval        timeout;
    int tmp[1];

    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    timeout.tv_sec = (int) sec;
    timeout.tv_usec = (long)((sec - (int) sec) * 1e6);
    while (1) {
        select(fd+1, &readfds, NULL, NULL, &timeout);
        if (FD_ISSET(fd, &readfds) && (GetMessage(fd, tmpdata, tmp, 0)) == message) {
            return 1;
        }
        else if ((timeout.tv_sec == 0) && (timeout.tv_usec == 0)) {
            return 0;
        }
    }
}

int WaitForMessage(int fd, int message, float sec, char *data, int *datalen) 
    /* wait up to about sec seconds for a message on the specified socket and return 1 
     * if the message was recieved and 0 otherwise */
{

    fd_set                readfds;  // the set of readable fifo file descriptors 
    struct timeval        timeout;

    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    timeout.tv_sec = (int) sec;
    timeout.tv_usec = (long)((sec - (int) sec) * 1e6);
    while (1) {
        select(fd+1, &readfds, NULL, NULL, &timeout);
        if (FD_ISSET(fd, &readfds) && (GetMessage(fd, data, datalen, 0)) == message) {
            return 1;
        }
        else if ((timeout.tv_sec == 0) && (timeout.tv_usec == 0)) {
            return 0;
        }
    }
}


void CloseSockets(SocketInfo *s) 
{
    int i;

    for (i = 0; i < MAX_SOCKETS; i++) {
       close(s[i].fd);
       if (strlen(s[i].name)) {
           unlink(s[i].name);
       }
    }
    return;
}

int GetTCPIPServerSocket(unsigned short port) 
    /* returns the file descriptor for a network server socket */
{
    struct sockaddr_in      address;
    int                     socketid;
    int                     conn;
    int                            i;
    socklen_t               addrlen;


    /* get the socket */
    if ((socketid = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(STATUSFILE,"Error in GetServerSocket: unable to get socket");
        return -1;
    }

    i = 1;
    /* bind the address to the socket */
    setsockopt(socketid, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i));

    addrlen = sizeof(struct sockaddr_in);

    /* make this a network domain socket */
    address.sin_family = AF_INET;

    /* assign the port */
    address.sin_port = htons(port);

    memset(&address.sin_addr, 0, sizeof(address.sin_addr)); 

    if (bind(socketid, (struct sockaddr *) &address, sizeof(address))) {
        fprintf(STATUSFILE, "GetTCPIPServerSocket: Error binding server socket to address in module %d, port %d\n", sysinfo.program_type, port);
        return -1;
    }
    if (listen(socketid, 5)) {
        fprintf(STATUSFILE, "Error listening to server socket");
        return -1;
    }

    /* get the socket */
    if ((conn = accept(socketid, (struct sockaddr * ) &address, &addrlen)) < 0) {
        fprintf(STATUSFILE,"GetServerSocket: Error connecting to socket");
        return -1;
    }

    return conn;
}

 
int GetTCPIPClientSocket(const char *name, unsigned short port) 
    /* returns the file descriptor for a client socket */
{
    struct sockaddr_in                address;
    struct in_addr                  inaddr;
    struct hostent                *host;
    int                               socketid;
    fd_set                              servercon;
    struct timeval                      timeout;

    /* set a timeout of 120 seconds */
    timeout.tv_sec = TCPIP_SOCKET_CLIENT_TIMEOUT;
    timeout.tv_usec = 0;

    if (inet_aton(name, &inaddr)) {
        host = gethostbyaddr((char *) &inaddr, sizeof(inaddr), AF_INET);
    }
    else {
        host = gethostbyname(name);
    }

    if (!host) {
        fprintf(STATUSFILE,"Unable to get network client socket\n");
        return -1;
    }
       
    /* get the socket */
    if ((socketid = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(STATUSFILE,"GetTCPIPClientSocket: unable to get socket to %s\n", name);
        return -1;
    }

    /* make this a network socket */
    address.sin_family = AF_INET;
    /* set the port */
    address.sin_port = htons(port);

    /* Get the first ip address associated with this machine */
    memcpy(&address.sin_addr, host->h_addr_list[0], sizeof(address.sin_addr));

    /* set up fd_set to wait for a connection from the server */
    FD_ZERO(&servercon);
    FD_SET(socketid, &servercon);
    
    /* wait for a connection on the socket */
    if (select(socketid+1, NULL, &servercon, NULL, &timeout) < 0) {
        fprintf(STATUSFILE,"GetTCPIPClientSocket: Error connecting to socket");
        return -1;
    }
    /* connect to the socket */
    while (connect(socketid, (struct sockaddr *) & address, sizeof(address)) < 0) {
        sleep(1);
        fprintf(STATUSFILE,"GetTCPIPClientSocket: waiting for socket server for connection to %s on port %d\n", name, port);
    }

    return(socketid);
}

int GetUDPServerSocket(unsigned short port) 
    /* returns the file descriptor for a UDP network server socket */
    /* used only in spike_daq.c */
{
    struct sockaddr_in      address;
    int                     socketid;
    //int                     conn;
    int                            i;
    size_t                  addrlen;

    /* get the socket */
    if ((socketid = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        return -1;
    }

    i = 1;
    /* bind the address to the socket */
    setsockopt(socketid, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i));
    
    /* make sure the address is clear */
    bzero(&address, sizeof(address));

    addrlen = sizeof(struct sockaddr_in);

    /* make this a network domain socket */
    address.sin_family = AF_INET;

    /* assign the port */
    address.sin_port = htons(port);

    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(socketid, (struct sockaddr *) &address, sizeof(address)) != 0) {
        perror("bind");
        return -1;
    }

//    fcntl(socketid, F_SETFL, O_NONBLOCK);
    return socketid;
}

int GetUDPClientSocket(const char *name, unsigned short port) 
{

    struct sockaddr_in                address;
    //int                                *data;
    struct in_addr                  inaddr;
    struct hostent                *host;
    int                               socketid;

    if (inet_aton(name, &inaddr)) {
        host = gethostbyaddr((char *) &inaddr, sizeof(inaddr), AF_INET);
    }
    else {
        host = gethostbyname(name);
    }

    if (!host) {
        fprintf(STATUSFILE,"Unable to find hostname %s\n", name);
        return -1;
    }
       
    /* get the socket */
    if ((socketid = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
        fprintf(stderr,"GetUDPClientSocket: unable to get socket");
        return -1;
    }

    /* make this a network socket */
    address.sin_family = AF_INET;
    /* set the port */
    address.sin_port = htons(port);

    /* Get the first ip address associated with this machine */
    memcpy(&address.sin_addr, host->h_addr_list[0], sizeof(address.sin_addr));

    /* connect to the socket */
    if (connect(socketid, (struct sockaddr *) & address, sizeof(address))==-1) {
        fprintf(stderr, "Error connecting UDP client socket on port %d\n", 
                port);
        return -1;
    }

    /* set this to be non-blocking.  This is done to resolve a bug in the DSP 
     * code where some messages are received by the proper acknowledgement 
     * is not sent */
    fcntl(socketid, F_SETFL, O_NONBLOCK);

    return(socketid);
}


void ClearData(int fd)
{
    /* set the socket to be non blocking temporarily */
    fcntl(fd, F_SETFL, O_NONBLOCK);
    /* read in and discard all of the data in the specified socket */
    while (read(fd, tmpdata, MAX_SOCKET_WRITE_SIZE) > 0) ;
    /* set it back to blocking */
    fcntl(fd, F_SETFL, 0);
}


void ErrorMessage(const char *errorstring, SocketInfo *client_message)
    /* display an error message */
{
    SendMessage(client_message[SPIKE_MAIN].fd, ERROR_MESSAGE, errorstring, strlen(errorstring) * sizeof(char) + 1);
    return;
}

void StatusMessage(const char *message, SocketInfo *client_message)
{
    if (sysinfo.program_type != SPIKE_MAIN) {
        SendMessage(client_message[SPIKE_MAIN].fd, STATUS_MESSAGE, message, 
                    strlen(message) * sizeof(char) + 1);
    }
    return;
}


int GetModuleID(char *modulename)
{

    if (strcmp(modulename, "DSP0") == 0)
        return DSP0;
    else if (strcmp(modulename, "DSP1") == 0)
        return DSP1;
    else if (strcmp(modulename, "DSP2") == 0)
        return DSP2;
    else if (strcmp(modulename, "DSP3") == 0)
        return DSP3;
    else if (strcmp(modulename, "DSP4") == 0)
        return DSP4;
    else if (strcmp(modulename, "DSP5") == 0)
        return DSP5;
    else if (strcmp(modulename, "DSP6") == 0)
        return DSP6;
    else if (strcmp(modulename, "DSP7") == 0)
        return DSP7;
    else if (strcmp(modulename, "DSP8") == 0)
        return DSP8;
    else if (strcmp(modulename, "DSP9") == 0)
        return DSP9;
    else if (strcmp(modulename, "DSPDIO") == 0)
        return DSPDIO;
    else if (strcmp(modulename, "DSP0ECHO") == 0)
        return DSP0ECHO;
    else if (strcmp(modulename, "DSPDIOECHO") == 0)
        return DSPDIOECHO;
    else if (strcmp(modulename, "SPIKE_DAQ") == 0)
        return SPIKE_DAQ;
    else if (strcmp(modulename, "SPIKE_POSDAQ") == 0)
        return SPIKE_POSDAQ;
    else if (strcmp(modulename, "SPIKE_PROCESS_POSDATA") == 0)
        return SPIKE_PROCESS_POSDATA;
    else if (strcmp(modulename, "SPIKE_SAVE_DATA") == 0)
        return SPIKE_SAVE_DATA;
    else if (strcmp(modulename, "SPIKE_USER_DATA") == 0)
        return SPIKE_USER_DATA;
    else if (strcmp(modulename, "SPIKE_MAIN") == 0)
        return SPIKE_MAIN;
    else {
        fprintf(STATUSFILE, "Error getting module ID for %s, check network config file\n",modulename) ;
        return -1;
    }
}


int GetModuleNum(const char *machinename, int moduleid)
{

    int snum;
    int moduleoffset;
    /* The module ID is the number from spike_defines + the slave number * the
     * maximum number of modules.  That creates a unique index for each
     * possible connection */

    /* get the slave number for target of this machine name. This will return
     * -1 if the target is not a slave  */
    snum = GetSlaveNum(machinename);
    if (snum == -1) {
        moduleoffset = 0;
    }
    else {
        moduleoffset = (snum + 1) * MAX_SOCKETS;
    }
    return moduleid - moduleoffset;
}

void SetupModuleMessaging(void)
    /* using the data type on each machine we set up the essential connections*/
{
    int i, c, p;
    int userdataind = -1;
#ifndef NO_DSP_DEBUG
    int j;
#endif

    /* start with the index of the next open connection in the netinfo
     * structure */
    c = netinfo.nconn;
    /* start at the first port in the list */
    p = 0;
    
    /* check to see if there is a user data system, and if so record it's index */
    for (i = 0; i <= netinfo.nslaves; i++) {
        if (sysinfo.datatype[i] & USERDATA) {
            userdataind = i;
            break;
        }
    }

    /* set up the dspname field of netinfo */
    for (i = 0; i < sysinfo.ndsps; i++) {
        sprintf(netinfo.dspname[i], "dsp%d", i);
    }

    /* go through each machine in order. Note that machine 0 is the master */
    for (i = 0; i <= netinfo.nslaves; i++) {
        if ((i != netinfo.rtslave) && (i != netinfo.userdataslave)) {
            /* Common messaging connections on all but the RT and standalone
             * USERDATA system */
            /* MAIN to SAVE_DATA MESSAGE*/
            strcpy(netinfo.conn[c].from, netinfo.machinename[i]);
            netinfo.conn[c].fromid = SPIKE_MAIN;
            strcpy(netinfo.conn[c].to, netinfo.machinename[i]);
            netinfo.conn[c].toid = SPIKE_SAVE_DATA;
            netinfo.conn[c].type = MESSAGE;
            netinfo.conn[c++].protocol = UNIX;
            /* SAVE_DATA to MAIN MESSAGE */
            strcpy(netinfo.conn[c].from, netinfo.machinename[i]);
            netinfo.conn[c].fromid = SPIKE_SAVE_DATA;
            strcpy(netinfo.conn[c].to, netinfo.machinename[i]);
            netinfo.conn[c].toid = SPIKE_MAIN;
            netinfo.conn[c].type = MESSAGE;
            netinfo.conn[c++].protocol = UNIX;
            /* MAIN to SAVE_DATA DATA */
            strcpy(netinfo.conn[c].from, netinfo.machinename[i]);
            netinfo.conn[c].fromid = SPIKE_MAIN;
            strcpy(netinfo.conn[c].to, netinfo.machinename[i]);
            netinfo.conn[c].toid = SPIKE_SAVE_DATA;
            netinfo.conn[c].type = DATA;
            netinfo.conn[c++].protocol = UNIX;
        }


        if ((sysinfo.datatype[i] & SPIKE) || 
            (sysinfo.datatype[i] & CONTINUOUS)) {
            /* MESSAGE Connections */
            /* MAIN to DAQ */
            strcpy(netinfo.conn[c].from, netinfo.machinename[i]);
            netinfo.conn[c].fromid = SPIKE_MAIN;
            strcpy(netinfo.conn[c].to, netinfo.machinename[i]);
            netinfo.conn[c].toid = SPIKE_DAQ;
            netinfo.conn[c].type = MESSAGE;
            netinfo.conn[c++].protocol = UNIX;
            /* DAQ to MAIN */
            strcpy(netinfo.conn[c].from, netinfo.machinename[i]);
            netinfo.conn[c].fromid = SPIKE_DAQ;
            strcpy(netinfo.conn[c].to, netinfo.machinename[i]);
            netinfo.conn[c].toid = SPIKE_MAIN;
            netinfo.conn[c].type = MESSAGE;
            netinfo.conn[c++].protocol = UNIX;

            /* DATA Connections */
            /* DAQ to MAIN */
            strcpy(netinfo.conn[c].from, netinfo.machinename[i]);
            netinfo.conn[c].fromid = SPIKE_DAQ;
            strcpy(netinfo.conn[c].to, netinfo.machinename[i]);
            netinfo.conn[c].toid = SPIKE_MAIN;
            netinfo.conn[c].type = DATA;
            netinfo.conn[c++].protocol = UNIX;
            /* DAQ to SAVE_DATA */
            strcpy(netinfo.conn[c].from, netinfo.machinename[i]);
            netinfo.conn[c].fromid = SPIKE_DAQ;
            strcpy(netinfo.conn[c].to, netinfo.machinename[i]);
            netinfo.conn[c].toid = SPIKE_SAVE_DATA;
            netinfo.conn[c].type = DATA;
            netinfo.conn[c++].protocol = UNIX;

            if (userdataind != -1) {
                /* User data connection */
                strcpy(netinfo.conn[c].from, netinfo.machinename[i]);
                netinfo.conn[c].fromid = SPIKE_DAQ;
                strcpy(netinfo.conn[c].to, netinfo.machinename[userdataind]);
                netinfo.conn[c].toid = SPIKE_USER_DATA;
                netinfo.conn[c].type = DATA;
                if (i == userdataind) {
                    /* this is a local connection */
                    netinfo.conn[c++].protocol = UNIX;
                }
                else {
                    /* this is a remote connection */
                    netinfo.conn[c].protocol = TCPIP;
                    netinfo.conn[c++].port = netinfo.port[p++];
                }
            }
#ifndef NO_DSP_DEBUG
            /* DSP Connections */
            /* we need to check each DSP to determine if it sends data to this
             * machine, and if so we set up a connection in each direction. The
             * communication is to spike_daq for the aux DSPS and to spike_main
             * for the Master DSP.
             * Note that the dsp connections are bidirectional sockets, so we
             * only open up one client socket from the appropriate program to 
             * the DSP */
	    int dspdataconn = 0;
            for (j = 0; j < sysinfo.ndsps; j++) {
                if (sysinfo.dspinfo[j].machinenum == i) {
                    /* set up the bidirectional connection to the DSP */
		    /* First we set up the messaging to / from the main program
		     * */
                    strcpy(netinfo.conn[c].from, netinfo.machinename[i]);
		    netinfo.conn[c].fromid = SPIKE_MAIN;
		    strcpy(netinfo.conn[c].to, netinfo.dspname[j]);
		    netinfo.conn[c].toid = j;
		    netinfo.conn[c].type = MESSAGE;
		    netinfo.conn[c].port = DSP_MESSAGE_PORT;
		    netinfo.conn[c++].protocol = UDP;
		    dspdataconn = 1;
		}
	    }
	    /* if we need a dsp data connection, create it. Note that only one server is required, 
	     * so we pretend that the data come from dsp 1  */
	    if (dspdataconn && !((i == 0) && (j == 0))) { 
		strcpy(netinfo.conn[c].from, "anywhere");
		netinfo.conn[c].fromid = 1;
		strcpy(netinfo.conn[c].to, netinfo.machinename[i]);
		netinfo.conn[c].toid = SPIKE_DAQ;
		netinfo.conn[c].type = DATA;
		netinfo.conn[c].port = DSP_DATA_PORT;
		netinfo.conn[c++].protocol = UDP;
		dspdataconn = 1; 
	    }
	    if (sysinfo.system_type[sysinfo.machinenum] == MASTER) {
	       /* we need to create a connection to the echo port of the master
	       * DSP so that we can send out packets once a minute to prevent the
	       * switch from losing track of this machines location */
	       strcpy(netinfo.conn[c].from, netinfo.machinename[i]);
	       netinfo.conn[c].fromid = SPIKE_MAIN;
	       strcpy(netinfo.conn[c].to, "dsp0");
	       netinfo.conn[c].toid = DSP0ECHO;
	       netinfo.conn[c].type = MESSAGE;
	       netinfo.conn[c].port = DSP_ECHO_PORT;
	       netinfo.conn[c++].protocol = UDP;
	        /* finally, if digital IO is handled by a separate DSP, we need to
	         * create the connection from the master to that DSP */
#ifndef DIO_ON_MASTER_DSP
	        strcpy(netinfo.conn[c].from, netinfo.machinename[i]);
	        netinfo.conn[c].fromid = SPIKE_MAIN;
	        strcpy(netinfo.conn[c].to, "dspdio");
	        netinfo.conn[c].toid = DSPDIO;
	        netinfo.conn[c].type = MESSAGE;
	        netinfo.conn[c].port = DSP_MESSAGE_PORT;
	        netinfo.conn[c++].protocol = UDP;
	        /* we also need to create a connection to the echo port of the 
	         * digio DSP so that we can send out packets once a minute to prevent the
	         * switch from losing track of this machines location */
	        strcpy(netinfo.conn[c].from, netinfo.machinename[i]);
	        netinfo.conn[c].fromid = SPIKE_MAIN;
	        strcpy(netinfo.conn[c].to, "dspdio");
	        netinfo.conn[c].toid = DSPDIOECHO;
	        netinfo.conn[c].type = MESSAGE;
	        netinfo.conn[c].port = DSP_ECHO_PORT;
	        netinfo.conn[c++].protocol = UDP; 
#endif
	    }
#endif
        }


        if (sysinfo.datatype[i] & POSITION) {
            /* this is a POSITION system, so we set up all of the  position 
	     * related messaging */
            /* Internal MESSAGE Connections */
            /* MAIN to POSDAQ*/
            strcpy(netinfo.conn[c].from, netinfo.machinename[i]);
            netinfo.conn[c].fromid = SPIKE_MAIN;
            strcpy(netinfo.conn[c].to, netinfo.machinename[i]);
            netinfo.conn[c].toid = SPIKE_POSDAQ;
            netinfo.conn[c].type = MESSAGE;
            netinfo.conn[c++].protocol = UNIX;
            /* POSDAQ to MAIN */
            strcpy(netinfo.conn[c].from, netinfo.machinename[i]);
            netinfo.conn[c].fromid = SPIKE_POSDAQ;
            strcpy(netinfo.conn[c].to, netinfo.machinename[i]);
            netinfo.conn[c].toid = SPIKE_MAIN;
            netinfo.conn[c].type = MESSAGE;
            netinfo.conn[c++].protocol = UNIX;

            /* MAIN to PROCESS_POSDATA */
            strcpy(netinfo.conn[c].from, netinfo.machinename[i]);
            netinfo.conn[c].fromid = SPIKE_MAIN;
            strcpy(netinfo.conn[c].to, netinfo.machinename[i]);
            netinfo.conn[c].toid = SPIKE_PROCESS_POSDATA;
            netinfo.conn[c].type = MESSAGE;
            netinfo.conn[c++].protocol = UNIX;
            /* PROCESS_POSDATA to MAIN */
            strcpy(netinfo.conn[c].from, netinfo.machinename[i]);
            netinfo.conn[c].fromid = SPIKE_PROCESS_POSDATA;
            strcpy(netinfo.conn[c].to, netinfo.machinename[i]);
            netinfo.conn[c].toid = SPIKE_MAIN;
            netinfo.conn[c].type = MESSAGE;
            netinfo.conn[c++].protocol = UNIX;

            /* Internal DATA Connections */
            /* POSDAQ to MAIN for position information */
            strcpy(netinfo.conn[c].from, netinfo.machinename[i]);
            netinfo.conn[c].fromid = SPIKE_POSDAQ;
            strcpy(netinfo.conn[c].to, netinfo.machinename[i]);
            netinfo.conn[c].toid = SPIKE_MAIN;
            netinfo.conn[c].type = DATA;
            netinfo.conn[c].protocol = UNIX;
            netinfo.conn[c++].port = netinfo.port[p++];
            /* POSDAQ to PROCESS_POSDATA*/
            strcpy(netinfo.conn[c].from, netinfo.machinename[i]);
            netinfo.conn[c].fromid = SPIKE_POSDAQ;
            strcpy(netinfo.conn[c].to, netinfo.machinename[i]);
            netinfo.conn[c].toid = SPIKE_PROCESS_POSDATA;
            netinfo.conn[c].type = DATA;
            netinfo.conn[c].protocol = UNIX;
            netinfo.conn[c++].port = netinfo.port[p++];
            /* PROCESS_POSDATA to MAIN*/
            strcpy(netinfo.conn[c].from, netinfo.machinename[i]);
            netinfo.conn[c].fromid = SPIKE_PROCESS_POSDATA;
            strcpy(netinfo.conn[c].to, netinfo.machinename[i]);
            netinfo.conn[c].toid = SPIKE_MAIN;
            netinfo.conn[c].type = DATA;
            netinfo.conn[c++].protocol = UNIX;
            /* PROCESS_POSDATA to SAVE_DATA*/
            strcpy(netinfo.conn[c].from, netinfo.machinename[i]);
            netinfo.conn[c].fromid = SPIKE_PROCESS_POSDATA;
            strcpy(netinfo.conn[c].to, netinfo.machinename[i]);
            netinfo.conn[c].toid = SPIKE_SAVE_DATA;
            netinfo.conn[c].type = DATA;
            netinfo.conn[c++].protocol = UNIX;



            if (userdataind != -1) {
                /* USERDATA data connection */
                strcpy(netinfo.conn[c].from, netinfo.machinename[i]);
                netinfo.conn[c].fromid = SPIKE_POSDAQ;
                strcpy(netinfo.conn[c].to, netinfo.machinename[userdataind]);
                netinfo.conn[c].toid = SPIKE_USER_DATA;
                netinfo.conn[c].type = DATA;
                /* this is a remote connection */
                netinfo.conn[c].protocol = TCPIP;
                netinfo.conn[c++].port = netinfo.port[p++];
            }
        }


        if (sysinfo.datatype[i] & USERDATA) {
            /* MAIN to USERDATA */
            strcpy(netinfo.conn[c].from, netinfo.machinename[i]);
            netinfo.conn[c].fromid = SPIKE_MAIN;
            strcpy(netinfo.conn[c].to, netinfo.machinename[i]);
            netinfo.conn[c].toid = SPIKE_USER_DATA;
            netinfo.conn[c].type = MESSAGE;
            netinfo.conn[c++].protocol = UNIX;
            /* USERDATA to MAIN */
            strcpy(netinfo.conn[c].from, netinfo.machinename[i]);
            netinfo.conn[c].fromid = SPIKE_USER_DATA;
            strcpy(netinfo.conn[c].to, netinfo.machinename[i]);
            netinfo.conn[c].toid = SPIKE_MAIN;
            netinfo.conn[c].type = MESSAGE;
            netinfo.conn[c++].protocol = UNIX;
        }
    }
    netinfo.nconn = c;
    /* go through the connections and set the names for the UNIX MESSAGES and
     * UNIX DATA sockets */
    for (c = 0; c < netinfo.nconn; c++) {
        if (netinfo.conn[c].protocol == UNIX) {
            if (netinfo.conn[c].type == MESSAGE) {
                sprintf(netinfo.conn[c].name, "/tmp/spike_message_%d_to_%d",
                        netinfo.conn[c].fromid, netinfo.conn[c].toid);
            }
            else if (netinfo.conn[c].type == DATA) {
                sprintf(netinfo.conn[c].name, "/tmp/spike_data_%d_to_%d",
                        netinfo.conn[c].fromid, netinfo.conn[c].toid);
            }
        }
    }
}
     


int InitializeMasterSlaveNetworkMessaging(void)
    /* if this is the master, first initialize a server to establish connection with the
     * slaves and then start up clients for each slave */
    /* if this is a slave, first initialize a client to establish connection with the
     * master and then start up a server for the master*/
{
    int i;

    /* set the sysinfo.machinenum variable */
    if ((sysinfo.machinenum = GetMachineNum(netinfo.myname)) == -1) {
	return -1;
    }

    fprintf(stderr, "machine num %d, type %d\n", sysinfo.machinenum, 
            sysinfo.system_type[sysinfo.machinenum]);

    if (sysinfo.system_type[sysinfo.machinenum] == MASTER) {
        /* start up one server for each slave */
        for(i = 0; i < netinfo.nslaves; i++) {
            fprintf(STATUSFILE, "spike_main: establishing connection with slave %s (#%d) on port %d\n", netinfo.slavename[i], i, netinfo.slaveport[i]);
            if ((netinfo.masterfd[i] = 
                        GetTCPIPServerSocket(netinfo.slaveport[i])) == -1) {
                fprintf(stderr, "spike_main: error getting TCPIP server socket from slave %d\n", i);
                return -1;
            }
            fprintf(STATUSFILE, "spike_main: got connection with slave %s (#%d)\n", netinfo.slavename[i], i);
            /* now start up a client for each slave */
            if ((netinfo.slavefd[i] = GetTCPIPClientSocket(netinfo.slavename[i],
                                              netinfo.masterport)) == -1) {
                fprintf(stderr, "program %d: error getting TCPIP client socket to slave %d\n", sysinfo.program_type, i);
                return -1;
            }
        }
     }
     else if (sysinfo.system_type[sysinfo.machinenum] == SLAVE) {
        /* start up a client for the connection to the master */
         if ((netinfo.slavefd[netinfo.myindex] = 
                 GetTCPIPClientSocket(netinfo.mastername, 
                                  netinfo.slaveport[netinfo.myindex])) == -1) {
            fprintf(stderr, "program %d: error getting TCPIP client socket to master %s on port %d\n", sysinfo.program_type, netinfo.mastername, netinfo.slaveport[netinfo.myindex]);
            return -1;
         }
        /* now start up a server for the connection from the master*/
        if ((netinfo.masterfd[netinfo.myindex] = 
                    GetTCPIPServerSocket(netinfo.masterport)) == -1) {
            fprintf(stderr, "program %d: error getting TCPIP server socket from master %s on port %d\n", sysinfo.program_type, netinfo.mastername, netinfo.masterport);
            return -1;
         }
     }
     return 1;
}


int StartNetworkMessaging(SocketInfo *server_message, 
        SocketInfo *client_message, 
        SocketInfo *server_data, 
        SocketInfo *client_data)
{
  int  i;
  //int j;
  int  mini, dini;
  int  mouti, douti;
  int  currentminconn;
  int  currentmoutconn;
  int  currentdinconn;
  int  currentdoutconn;
  int  slavenum;
  int  fdtmp;
  int  data[1000];
  int  datalen;
  int  message;

  char tmpstring[80];
  int  done = 0;
  fd_set readfds;  // the set of readable fifo file descriptors 
  int  maxfds;
  int  optval;
  socklen_t  optlen;

  SocketInfo *c;

  /* As all modules run this, the first thing to do is set the
   * sysinfo.machinenum variable */
  sysinfo.machinenum = GetMachineNum(netinfo.myname);

  /* zero out the fds for the arrays */
  for (i = 0; i < MAX_CONNECTIONS; i++) {
    server_message[i].fd = 0;
    server_data[i].fd = 0;
    client_message[i].fd = 0;
    client_data[i].fd = 0;
  }

  /* this messaging is orchastrated by the master system, so if we are the
   * master and we are within the main program, go through the list of 
   * messages and send out commands to the slaves to start their messaging 
   * and if we are a slave we wait for messages from the master */ 

  /* all of the resulting file descriptors go into the client or server 
   * data/message arrays with the index into the arrays determined as
   * follows:
   * Internal messaging / data indeces 1 to MAX_SOCKETS for all possible 
   * internal  messages. 
   *                 E.g. a message to the display program would be
   *                 client_message[SPIKE_MAIN]
   *
   * External messages / data indeces MAX_SOCKETS+1 and up. The index
   * incremented by one in each program for each new data or message socket
   * it needs to set up.*/

  /* We first check to see if this is the main program (SPIKE_MAIN) and if
   * so we start up clients for each of our subsidiary modules, as these
   * modules have started servers waiting for this program. We then start up
   * servers that connect to the clients that the modules have started. 
   * This will allow us to communicate with all of the modules for 
   * subsequent messages */
  if (sysinfo.program_type == SPIKE_MAIN) {
    for (i = 0; i < MAX_CONNECTIONS; i++) {
      /* note that we need to ignore these connections below */
      c = netinfo.conn + i;
      if ((c->fromid == SPIKE_MAIN) && 
          (strcmp(c->to, netinfo.myname) == 0) && 
          (c->type == MESSAGE) && 
          (c->protocol == UNIX)) {
        fprintf(STATUSFILE, "spike main program getting client %s\n", c->name);
        /* get the file descriptor */
        c->fd = GetClientSocket(c->name);
        /* copy this connection's information into the client_message
         * structure. This is an internal message, so the index is the
         * toid*/
        c->ind = c->toid;
        memcpy(client_message + c->ind, c, sizeof(SocketInfo));
        fprintf(STATUSFILE, "spike main program got client %s\n", c->name);
      }
    }
    for (i = 0; i < MAX_CONNECTIONS; i++) {
      /* note that we need to ignore these connections below */
      c = netinfo.conn + i;
      if ((c->toid == SPIKE_MAIN) && 
          (strcmp(c->from, netinfo.myname) == 0) && 
          (c->type == MESSAGE) && 
          (c->protocol == UNIX)) {
        fprintf(STATUSFILE, "spike main program getting server %s\n", c->name);
        /* get the file descriptor */
        if ((c->fd = GetServerSocket(c->name)) == -1) {
          fprintf(stderr, "program %d: Error in StartNetworkMessaging on %s\n", sysinfo.program_type, c->name);
          return -1;
        }
        fprintf(STATUSFILE, "spike main program got server %s\n", c->name);
        /* copy this connection's information into the client_message
         * structure. This is an internal message so the index is the
         * fromid */
        c->ind = c->fromid;
        memcpy(server_message + c->ind, c, sizeof(SocketInfo));
      }
    }
    /* now we go through the list of connections and start up all of the
     * data servers and clients */
    /* first start the data connections to this program. These are internal
     * messages, so c->fromid = c->ind*/  
    for (i = 0; i < MAX_CONNECTIONS; i++) {
      c = netinfo.conn + i;
      if ((strcmp(c->to, netinfo.myname) == 0) && 
          (strcmp(c->from, netinfo.myname) == 0) && 
          (c->toid == SPIKE_MAIN) && 
          (c->type == DATA) && 
          (c->protocol == UNIX)) {
        fprintf(STATUSFILE, "getting data server %s from %d\n", c->name,
            c->fromid);
        /* this is an internal data socket, so the index is the fromid*/
        c->ind = c->fromid;
        SendMessage(client_message[c->ind].fd,
            START_NETWORK_CLIENT, (char *) c, sizeof(SocketInfo));
        fprintf(STATUSFILE, "sent message for data server %s\n", c->name);
        /* get the file descriptor */
        if ((c->fd = GetServerSocket(c->name)) == -1) {
          fprintf(stderr, "program %d: Error in StartNetworkMessaging on %s\n", sysinfo.program_type, c->name);
          return -1;
        }
        /* copy this connection's information into the client_message
         * structure */
        memcpy(server_data + c->ind, c, sizeof(SocketInfo));
      }
    }
    for (i = 0; i < MAX_CONNECTIONS; i++) {
      /* note that we need to ignore these connections below */
      c = netinfo.conn + i;
      if ((strcmp(c->to, netinfo.myname) == 0) && 
          (strcmp(c->from, netinfo.myname) == 0) && 
          (c->toid != SPIKE_MAIN) && 
          (c->type == DATA) && 
          (c->protocol == UNIX)) {
        /* send out messages to the two client programs to start a
         * client and a server if the client is not SPIKE_MAIN*/
        /* these are internal messages so the indeces are set by the
         * modules themselves */ 
        fprintf(STATUSFILE, "sending START_NETWORK_SERVER to %d for %d to %d DATA socket\n", c->toid, c->fromid, c->toid);
        SendMessage(client_message[c->toid].fd, START_NETWORK_SERVER, (char *) c, sizeof(SocketInfo));
        if (c->fromid == SPIKE_MAIN) {
          fprintf(STATUSFILE, "getting client server %s from %d\n", 
              c->name, c->fromid);
          if ((c->fd = GetClientSocket(c->name)) == -1) {
            fprintf(stderr, "program %d: Error in StartNetworkMessaging on %s\n", sysinfo.program_type, c->name);
            return -1;
          }
          /* copy this connection's information into the client_message
           * structure */
          memcpy(client_data + c->toid, c, sizeof(SocketInfo));
        }
        else {
          fprintf(STATUSFILE, "sending START_NETWORK_CLIENT to %d for %d to %d DATA socket\n", c->fromid, c->fromid, c->toid);
          SendMessage(client_message[c->fromid].fd,
              START_NETWORK_CLIENT, (char *) c, sizeof(SocketInfo));
        }
      }
    }
  }
  /* Now we handle the rest of the messages. At this point we only need to
   * worry about UDP and TCPIP protocol messages, as we've already started up
   * all of the internal UNIX messages */
  currentminconn = MAX_SOCKETS;
  currentmoutconn = MAX_SOCKETS;
  currentdinconn = MAX_SOCKETS;
  currentdoutconn = MAX_SOCKETS;
  if ((sysinfo.program_type == SPIKE_MAIN) & 
      (sysinfo.system_type[sysinfo.machinenum] == MASTER)) {
    /* set the external current connection number to MAX_SOCKETS so that 
     * we don't overlap with internal module socket indeces */
    for (i = 0; i < netinfo.nconn; i++) {
      c = netinfo.conn + i;
      /* we first check to see if the message is to or from this module*/
      if ((c->fromid == SPIKE_MAIN) && 
          (strcmp(c->from, netinfo.myname) == 0)) {
        /* check the connection protocol */
        if (c->protocol == UDP) {
          /* start up a UDP client to send data or messages to the specified
           * server. Currently this is used only to send messages to the DSP boxes */
          if (c->type == MESSAGE) {
            fprintf(stderr, "getting UDP client socket from %s to %s on port %d\n", c->from, c->to, c->port);
            if ((c->fd = GetUDPClientSocket(c->to, c->port)) == -1) {
              fprintf(STATUSFILE, "Error opening UDP client socket from %s to %s on port %d\n", c->from, c->to, c->port);
              return -1;
            }
            /* check to see if the slot for this message is already
             * filled */
            if (client_message[c->toid].fd == 0) {
              /* this slot is not filled, so we will use it */
              c->ind = c->toid;
            }
            else {
              c->ind = currentmoutconn++;
            }
            memcpy(client_message + c->ind, c, sizeof(SocketInfo));
          } 
          else if (c->type == DATA) {
            if ((c->fd = GetUDPClientSocket(c->to, c->port)) == 0) {
              fprintf(STATUSFILE, "Error opening UDP client socket from %s to %s on port %d\n", c->from, c->to, c->port);
              return -1;
            }
            /* check to see if the slot for this message is already
             * filled */
            if (client_message[c->toid].fd == 0) {
              /* this slot is not filled, so we will use it */
              c->ind = c->toid;
            }
            else {
              c->ind = currentmoutconn++;
            }
            memcpy(client_data + c->ind, c, sizeof(SocketInfo));
          }
        }
        else if (c->protocol == TCPIP) {
          if (c->type == MESSAGE) {
            /* send a message to the client to start a server and 
             * start up a TCPIP client to send data to the 
             * specified server */ 
            /* (look up the slave number for the source machine */
            slavenum = GetSlaveNum(c->to);
            /* we need to send a message to the target machine to 
             * start its client, and once that's done we can start 
             * the local server */
            fprintf(STATUSFILE, "spike_main: sending START_NETWORK_SERVER message to slave %d for TCPIP connection from %s %d to %s %d on port %d\n", slavenum, c->from, c->fromid, c->to, c->toid, c->port);
            SendMessage(netinfo.slavefd[slavenum], 
                START_NETWORK_SERVER, (char *) c,
                sizeof(SocketInfo));
            if ((c->fd = GetTCPIPClientSocket(c->to, c->port)) 
                == -1) {
              fprintf(STATUSFILE, "Error opening TCPIP message client socket from %s %d to %s %d on port %d\n", c->from, c->fromid, c->to, c->toid, c->port);
              return -1;
            }
            if (client_message[c->toid].fd == 0) {
              /* this slot is not filled, so we will use it */
              c->ind = c->toid;
            }
            else {
              c->ind = currentmoutconn++;
            }
            memcpy(client_message + c->ind, c, sizeof(SocketInfo));
            fprintf(STATUSFILE, "Established TCPIP message client socket from %s %d to %s %d on port %d\n", c->from, c->fromid, c->to, c->toid, c->port);
          }
          else if (c->type == DATA) {
            fprintf(STATUSFILE, "Error in network config file: TCPIP DATA client socket from %s %d to %s %d on port %d is not currently supported.\n", c->from, c->fromid, c->to, c->toid, c->port);
          }
        }
      }
      else if ((c->toid == SPIKE_MAIN) && 
          (strcmp(c->to, netinfo.myname) == 0)) {
        if (c->protocol == UDP) {
          /* start up a UDP server to read data on the specified port 
           * and put the relevant information in the server_message 
           * or server_data structure */ 
          if (c->type == DATA) {
            if ((c->fd = GetUDPServerSocket(c->port)) == 0) {
              fprintf(STATUSFILE, "Error opening UDP server socket from %s %d to %s %d on port %d\n", c->from, c->fromid, c->to, c->toid, c->port);
              return -1;
            }
            if (server_data[c->fromid].fd == 0) {
              /* this slot is not filled, so we will use it */
              c->ind = c->fromid;
            }
            else {
              c->ind = currentdinconn++;
            }
            memcpy(server_data + c->ind, c, sizeof(SocketInfo));
          }
          else {
            fprintf(STATUSFILE, "Error in network config file: UDP data socket from %s %d to %s %d on port %d is an invalid connection type\n", c->from, c->fromid, c->to, c->toid, c->port);
            return -1;
          }
        }
        else if (c->protocol == TCPIP) { 
          if (c->type == MESSAGE) {
            /* look up the slave number for the source machine */
            slavenum = GetSlaveNum(c->from);
            /* we need to send a message to the target machine to 
             * start it's client, and once that's done we can 
             * start the local server */
            fprintf(STATUSFILE, "Sending START_NETWORK_CLIENT_message for TCPIP message socket from %s %d to %s %d on port %d\n", c->from, c->fromid, c->to, c->toid, c->port);
            SendMessage(netinfo.slavefd[slavenum], 
                START_NETWORK_CLIENT, (char *) c,
                sizeof(SocketInfo));
            if ((c->fd = GetTCPIPServerSocket(c->port)) == -1) {
              fprintf(STATUSFILE, "program %d: Error getting TCPIP message socket from %s %d to %s %d on port %d\n", -1, c->from, c->fromid, c->to, c->toid, c->port);
              return -1;
            }
            if (server_message[c->fromid].fd == 0) {
              /* this slot is not filled, so we will use it */
              c->ind = c->fromid;
            }
            else {
              c->ind = currentminconn++;
            }
            memcpy(server_message + c->ind, c, sizeof(SocketInfo));
          }
          else if (c->type == DATA) {
            /* look up the slave number for the source machine */
            slavenum = GetSlaveNum(c->from);
            /* we need to send a message to the target machine to start
             * it's client, and once that's done we can start the local 
             * server */
            fprintf(STATUSFILE, "Sending START_NETWORK_CLIENT_message for TCPIP data socket from %s %d to %s %d on port %d\n", c->from, c->fromid, c->to, c->toid, c->port);
            SendMessage(netinfo.slavefd[slavenum], 
                START_NETWORK_CLIENT, (char *) c,
                sizeof(SocketInfo));
            if ((c->fd = GetTCPIPServerSocket(c->port)) == -1) {
              fprintf(STATUSFILE, "program %d: Error getting TCPIP data socket from %s %d to %s %d on port %d\n", -1, c->from, c->fromid, c->to, c->toid, c->port);
              return -1;
            }
            if (server_data[c->fromid].fd == 0) {
              /* this slot is not filled, so we will use it */
              c->ind = c->fromid;
            }
            else {
              c->ind = currentdinconn++;
            }
            memcpy(server_data + c->ind, c, sizeof(SocketInfo));
          }
        }
      }
      else {
        /* this connection is between two other machines or between
         * modules.  If it is a connection between
         * two machines we  send them messages and wait for a reply  */
        /* if the message is to or from a DSP machine, we only need to
         * tell the slave computer to start it's connection */
        if ((c->toid >= DSP1) && (c->toid < MAX_DSPS)) {
          if (strcmp(c->from, netinfo.myname) == 0) {
            /* this is a connection from a local module, so we tell it 
             * to start its client */	
            fprintf(STATUSFILE, "Sending START_NETWORK_CLIENT_message for UDP data socket from %s %d to %s %d on port %d\n", c->from, c->fromid, c->to, c->toid, c->port);
            SendMessage(client_message[c->fromid].fd,
                START_NETWORK_CLIENT, (char *) c, sizeof(SocketInfo));
          }
          else {
            slavenum = GetSlaveNum(c->from);
            fprintf(STATUSFILE, "Sending START_NETWORK_CLIENT_message for UDP data socket from %s %d to %s %d on port %d\n", c->from, c->fromid, c->to, c->toid, c->port);
            SendMessage(netinfo.slavefd[slavenum], 
                START_NETWORK_CLIENT, (char *) c,
                sizeof(SocketInfo));
          }
        }
        else if (strcmp(c->to, c->from) != 0) {
          /* this is a message between two machines, so we 
           * send them both messages. */
          if (strcmp(c->from, netinfo.myname) == 0) {
            fprintf(STATUSFILE, "spike_main: Sending START_NETWORK_CLIENT message to local module %s %d\n", c->from, c->fromid);
            /* if this is from a module on this machine, send it a
             * message */
            SendMessage(client_message[c->fromid].fd,
                START_NETWORK_CLIENT, (char *) c, 
                sizeof(SocketInfo));
          }
          else {
            /* this is from a slave */
            if (c->fromid >= MAX_DSPS) {
              fprintf(STATUSFILE, "spike_main: Sending START_NETWORK_CLIENT message to remote machine %s %d\n", c->from, c->fromid);
              slavenum = GetSlaveNum(c->from);
              SendMessage(netinfo.slavefd[slavenum], 
                  START_NETWORK_CLIENT, (char *) c, 
                  sizeof(SocketInfo));
            }
          }
          if (strcmp(c->to, netinfo.myname) == 0) {
            /* if this is to a module on this machine, send it a
             * message */
            fprintf(stderr, "spike_main: Sending START_NETWORK_SERVER message to local module %s %d\n", c->to, c->toid);
            fdtmp = server_message[c->toid].fd;
            SendMessage(client_message[c->toid].fd, 
                START_NETWORK_SERVER, (char *) c, 
                sizeof(SocketInfo));
          }
          else {
            slavenum = GetSlaveNum(c->to);
            fprintf(stderr, "spike_main: Sending START_NETWORK_SERVER message to remote module %s %d\n", c->to, c->toid);
            fdtmp = netinfo.masterfd[slavenum];
            SendMessage(netinfo.slavefd[slavenum], 
                START_NETWORK_SERVER, (char *) c, 
                sizeof(SocketInfo));
          }
          /* Now wait for a message from the server */
          fprintf(STATUSFILE, "waiting for connection established message from %s %d, slavenum %d\n", c->to, fdtmp, slavenum);
          if (!WaitForMessage(fdtmp, CONNECTION_ESTABLISHED, 15)) {
            fprintf(STATUSFILE, "Error establishing connection from %s %d to %s %d on port %d\n", c->from, c->fromid, c->to, c->toid, c->port);
            return -1;
          }
          else {
            fprintf(STATUSFILE, "Established connection from %s %d to %s %d on port %d\n", c->from, c->fromid, c->to, c->toid, c->port);
          }
        }
      }
    }
    /* now send out the CONNECTION ESTABLISHED message to all of our
     * modules (except the UDP modules) */
    for (i = MAX_CONNECTIONS; i >= 0; i--) {
      if ((server_message[i].fd) && 
          (server_message[i].protocol != UDP)) {
        SendMessage(client_message[i].fd, CONNECTION_ESTABLISHED, NULL, 0);
        fprintf(STATUSFILE, "Waiting for CONNECTION_ESTABLISHED message on %d\n", i);
        if (!WaitForMessage(server_message[i].fd, CONNECTION_ESTABLISHED, 5)) {
          sprintf(tmpstring, "spike_main: Error establishing network connections on program %d\n", i);
          fprintf(STATUSFILE, "%s\n", tmpstring);
          return -1;
          //DisplayMessage(tmpstring);
        }
        fprintf(STATUSFILE, "got CONNECTION ESTABLISHED message from %d\n", i);
      }
    }
    /* now send a message to all the slaves telling them that the
     * connections have been established */
    for (i = 0; i < netinfo.nslaves; i++) {
      SendMessage(netinfo.slavefd[i], CONNECTION_ESTABLISHED, 
          NULL, 0);
    }
  }
  else if ((sysinfo.program_type == SPIKE_MAIN) & 
      (sysinfo.system_type[sysinfo.machinenum] == SLAVE)) {
    /* this is a slave system, so we need to wait for a connection from the
     * master and then send out the appropriate messages to our modules */
    maxfds = netinfo.masterfd[netinfo.myindex] + 1;
    while (!done) {
      FD_ZERO(&readfds);
      FD_SET(netinfo.masterfd[netinfo.myindex], &readfds);
      select(maxfds+1, &readfds, NULL, NULL, NULL);
      message = GetMessage(netinfo.masterfd[netinfo.myindex], 
          (char *) data, &datalen, 0);
      if (datalen > 0) {
        c = (SocketInfo *) data;
        fprintf(STATUSFILE, "program %d got message %d, socket name %s, protocol %d, type %d, connection from %s %d to %s %d\n",
            sysinfo.program_type, message, c->name, 
            c->protocol, c->type, c->from, c->fromid, c->to, c->toid);
        if (message == -1) {
          /* this only happens when a module has died unexpectedly,
           * so we should exit */
          fprintf(stderr, "Error: another program has died unexpectedly, exiting\n");
          return -1;
        }
      }
      else {
        fprintf(STATUSFILE, "program %d got message %d\n", 
            sysinfo.program_type, message);
      }
      switch(message) {
        case START_NETWORK_CLIENT:
          /* handle the DSP connection */
          if (c->toid < MAX_DSPS) {
            if ((c->fd = 
                  GetUDPClientSocket(c->to, c->port)) == 0) {
              fprintf(STATUSFILE, "Error opening UDP client socket from %s to %s on port %d\n", c->from, c->to, c->port);
              return -1;
            }
            memcpy(client_message + c->toid, c, 
                sizeof(SocketInfo));
            fprintf(STATUSFILE, "Opened UDP client socket from %s to %s on port %d, index %d\n", c->from, c->to, c->port, c->toid);
          }
          else {
            fprintf(STATUSFILE, "program %d sending START_NETWORK_CLIENT message for connection from %s %d to %s %d\n", sysinfo.program_type, c->from, c->fromid, c->to, c->toid);
            /* send a message to the client module */
            SendMessage(client_message[c->fromid].fd, START_NETWORK_CLIENT,
                (char *) c, sizeof(SocketInfo));
          }
          break;
        case START_NETWORK_SERVER: 
          fprintf(STATUSFILE, "program %d sending START_NETWORK_SERVER message for connection from %s %d to %s %d\n", sysinfo.program_type, c->from, c->fromid, c->to, c->toid);
          /* send a message to the server module */
          SendMessage(client_message[c->toid].fd, START_NETWORK_SERVER,
              (char *) c, sizeof(SocketInfo));

          if (WaitForMessage(server_message[c->toid].fd, 
                CONNECTION_ESTABLISHED, 5)) {
            /* send a connection established message to the master*/
            SendMessage(netinfo.slavefd[netinfo.myindex], 
                CONNECTION_ESTABLISHED, NULL, 0); 
          }
          break;
        case CONNECTION_ESTABLISHED:
          /* send out the CONNECTION ESTABLISHED message to the other
           * modules and wait for the response. Note that this only
           * goes out to non-UDP connected modules at the moment, as
           * the DSPs are not set up to get these messages (????)*/
          for (i = MAX_CONNECTIONS; i >= 0; i--) {
            if ((server_message[i].fd) && 
                (server_message[i].protocol != UDP)) {
              SendMessage(client_message[i].fd, CONNECTION_ESTABLISHED, NULL, 0);
              if (!WaitForMessage(server_message[i].fd, CONNECTION_ESTABLISHED, 5)) {
                sprintf(tmpstring, "spike_main: Error establishing network connections on program %d\n", i);
                fprintf(STATUSFILE, "%s\n", tmpstring);
                return -1;
                //DisplayMessage(tmpstring);
              }
            }
          }
          /* send out the final connection established message to the
           * master system */
          SendMessage(netinfo.masterfd[netinfo.myindex], 
              CONNECTION_ESTABLISHED, NULL, 0);

          done = 1;
          break;
      }
    }
  }
  else {
    /* this is one of the subsidiary modules, so we start up a server for
     * messages from SPIKE_MAIN and then wait for messages  */
    /* we first need to get the name of the socket */
    sprintf(server_message[SPIKE_MAIN].name, 
        "/tmp/spike_message_%d_to_%d", SPIKE_MAIN,
        sysinfo.program_type);
    fprintf(STATUSFILE, "module %d starting server %s\n", 
        sysinfo.program_type, server_message[SPIKE_MAIN].name);
    if (((server_message[SPIKE_MAIN].fd) = GetServerSocket( server_message[SPIKE_MAIN].name)) == -1) {
      fprintf(STATUSFILE, "Error starting server on %s", tmpstring);
      return -1;
    }
    /* now we start up a client for
     * messages to SPIKE_MAIN and then wait for messages  */
    sprintf(client_message[SPIKE_MAIN].name, 
        "/tmp/spike_message_%d_to_%d", sysinfo.program_type,
        SPIKE_MAIN);
    fprintf(STATUSFILE, "module %d starting display client %s\n", 
        sysinfo.program_type, client_message[SPIKE_MAIN].name);
    if (((client_message[SPIKE_MAIN].fd) = GetClientSocket(
            client_message[SPIKE_MAIN].name)) == 0) {
      fprintf(STATUSFILE, "Error starting server on %s", 
          client_message[SPIKE_MAIN].name);
      return -1;
    }

    maxfds = server_message[SPIKE_MAIN].fd + 1;
    while (!done) {
      FD_ZERO(&readfds);
      FD_SET(server_message[SPIKE_MAIN].fd, &readfds);
      select(maxfds+1, &readfds, NULL, NULL, NULL);
      bzero((void *) data, 1000);
      message = GetMessage(server_message[SPIKE_MAIN].fd, 
          (char *) data, &datalen, 1);
      c = (SocketInfo *) data;
      fprintf(STATUSFILE, "program %d got message %d, socket name %s, protocol %d, type %d, connection from %s %d to %s %d\n",
          sysinfo.program_type, message, c->name, c->protocol, c->type, c->from, c->fromid, c->to, c->toid);
      if (message == -1) {
        /* this only happens when a module has died unexpectedly,
         * so we should exit */
        fprintf(stderr, "Error: another module has died unexpectedly, exiting\n");
        return -1;
      }
      switch(message) {
        case START_NETWORK_CLIENT:
          /* start the network client */
          /* check the type of the connection */
          if (c->protocol == UNIX) {
            //fprintf(STATUSFILE, "about to get client socket %s\n", c->name);
            /* start up a UNIX client to send data or messages to 
             * the specified server. This is an internal message,
             * so it's index is c->toid  */
            if ((c->fd = GetClientSocket(c->name)) == 0) {
              fprintf(STATUSFILE, "Error opening UNIX client socket %s \n", c->name);
              return -1;
            }
            c->ind = c->toid;
            if (c->type == MESSAGE) {
              memcpy(client_message + c->ind, c, 
                  sizeof(SocketInfo));
            } 
            else if (c->type == DATA) {
              memcpy(client_data + c->ind, c, sizeof(SocketInfo));
            }
            else {
              return -1;
            }
            //fprintf(STATUSFILE, "got UNIX socket %s\n", c->name);
          }
          else if (c->protocol == UDP) {
            if (c->type == MESSAGE) {
              if ((c->fd = 
                    GetUDPClientSocket(c->to, c->port)) == 0) {
                fprintf(STATUSFILE, "Error opening UDP client socket from %s to %s on port %d\n", c->from, c->to, c->port);
                return -1;
              }
              if (client_message[c->toid].fd == 0) {
                /* this slot is not filled, so we will use it */
                c->ind = c->toid;
              }
              else {
                c->ind = currentmoutconn++;
              }
              memcpy(client_message + c->ind, c, 
                  sizeof(SocketInfo));
            } 
            else if (c->type == DATA) {
              fprintf(stderr, "starting UDP data client\n");
              if ((c->fd = 
                    GetUDPClientSocket(c->to, c->port)) == 0) {
                fprintf(STATUSFILE, "Error opening UDP client socket from %s to %s on port %d\n", c->from, c->to, c->port);
                return -1;
              }
              if (client_data[c->toid].fd == 0) {
                /* this slot is not filled, so we will use it */
                c->ind = c->toid;
              }
              else {
                c->ind = currentdoutconn++;
              }
              memcpy(client_data + c->ind, c, sizeof(SocketInfo));
            }
            else {
              return -1;
            }
          }
          else if (c->protocol == TCPIP) {
            /* start up a TCPIP client to send data to the specified
             * server and send a message to the client to start a
             * server. */
            if (c->type == MESSAGE) {
              if ((c->fd = 
                    GetTCPIPClientSocket(c->to, c->port)) == -1){
                fprintf(STATUSFILE, "Error opening TCPIP client message socket from %s %d to %s %d on port %d\n", c->from, c->fromid, c->to, c->toid, c->port);
                return -1;
              }
              fprintf(STATUSFILE, "program %d got TCP IP client message socket to %s, port %d\n", sysinfo.program_type, c->to, c->port);
              if (client_message[c->toid].fd == 0) {
                /* this slot is not filled, so we will use it */
                c->ind = c->toid;
              }
              else {
                c->ind = currentmoutconn++;
              }
              memcpy(client_message + c->ind, c, sizeof(SocketInfo));
            } 
            else if (c->type == DATA) {
              if ((c->fd = GetTCPIPClientSocket(c->to, 
                      c->port)) ==-1) {
                fprintf(STATUSFILE, "Error opening TCPIP client data socket from %s %d to %s %d on port %d\n", c->from, c->fromid, c->to, c->toid, c->port);
                return -1;
              }
              fprintf(STATUSFILE, "program %d got TCP IP client data socket to %s, port %d\n", sysinfo.program_type, c->to, c->port);
              if (client_data[c->toid].fd == 0) {
                /* this slot is not filled, so we will use it */
                c->ind = c->toid;
              }
              else {
                c->ind = currentdoutconn++;
              }
              memcpy(client_data + c->ind, c, sizeof(SocketInfo));
            }
          }
          else {
            fprintf(STATUSFILE, "Invalid connection protocol %d for socket from %s %d to %s %d on port %d\n", c->protocol, c->from, c->fromid, c->to, 
                c->toid, c->port);
            return -1;
          }
          break;
        case START_NETWORK_SERVER: 
          /* start the network client */
          /* check the type of the connection */
          if (c->protocol == UNIX) {
            /* start up a UNIX server to send data or messages to 
             * the specified server.  */
            c->ind = c->fromid;
            if (c->type == MESSAGE) {
              if ((c->fd = GetServerSocket(c->name)) == -1) {
                fprintf(STATUSFILE, "Error opening UNIX server socket %s\n", c->name);
                return -1;
              }
              memcpy(server_message + c->ind, c, sizeof(SocketInfo));
            } 
            else if (c->type == DATA) {
              if ((c->fd = GetServerSocket(c->name)) == -1) {
                fprintf(STATUSFILE, "Error opening UNIX server socket %s\n", c->name);
                return -1;
              }
              memcpy(server_data + c->ind, c, sizeof(SocketInfo));
            }
            else {
              return -1;
            }
          }
          else if (c->protocol == UDP) {
            /* start up a UDP server to send data or messages to the specified
             * server. Currently this is not used */
            if (c->type == MESSAGE) {
              if ((c->fd = GetUDPServerSocket(c->port)) == 0) {
                fprintf(STATUSFILE, "Error opening UDP server message socket from %s %d to %s %d on port %d\n", c->from, c->fromid, c->to, c->toid, c->port);
                return -1;
              }
              if (server_message[c->fromid].fd == 0) {
                /* this slot is not filled, so we will use it */
                c->ind = c->fromid;
              }
              else {
                c->ind = currentminconn++;
              }
              memcpy(server_message + c->ind, c, sizeof(SocketInfo));
            } 
            else if (c->type == DATA) {
              fprintf(stderr, "Starting UDP data server on port %d\n", c->port);
              if ((c->fd = GetUDPServerSocket(c->port)) == 0) {
                fprintf(STATUSFILE, "Error opening UDP server socket on port %d\n", c->port);
                return -1;
              }
              if (server_data[c->fromid].fd == 0) {
                /* this slot is not filled, so we will use it */
                c->ind = c->fromid;
              }
              else {
                c->ind = currentdinconn++;
              }
              memcpy(server_data + c->ind, c, sizeof(SocketInfo));
            }
            else {
              return -1;
            }
          }
          else if (c->protocol == TCPIP) {
            /* start up a TCPIP server to send data to the specified
             * server and send a message to the server to start a
             * server. */
            if (c->type == MESSAGE) {
              if ((c->fd = GetTCPIPServerSocket(c->port))==-1) {
                fprintf(STATUSFILE, "Error opening TCPIP server message socket from %s %d to %s %d on port %d\n", c->from, c->fromid, c->to, c->toid, c->port);
                return -1;
              }
              if (server_message[c->fromid].fd == 0) {
                /* this slot is not filled, so we will use it */
                c->ind = c->fromid;
                fprintf(stderr, "ind = fromid = %d\n", c->ind);
              }
              else {
                c->ind = currentminconn++;
                fprintf(stderr, "ind = currentconn = %d\n", c->ind);
              }
              memcpy(server_message + c->ind, c, sizeof(SocketInfo));
              fprintf(STATUSFILE, "program %d got TCP IP server message socket from %s, port %d, fd %d, ind %d\n", sysinfo.program_type, c->from, c->port, c->fd, c->ind);
            } 
            else if (c->type == DATA) {
              if ((c->fd = GetTCPIPServerSocket(c->port))==-1) {
                fprintf(STATUSFILE, "Error opening TCPIP server data socket from %s %d to %s %d on port %d\n", c->from, c->fromid, c->to, c->toid, c->port);
                return -1;
              }
              fprintf(STATUSFILE, "program %d got TCP IP server data socket from %s, port %d, fd %d, ind %d\n", sysinfo.program_type, c->from, c->port, c->fd, c->ind);
              if (server_data[c->fromid].fd == 0) {
                /* this slot is not filled, so we will use it */
                c->ind = c->fromid;
              }
              else {
                c->ind = currentdinconn++;
              }
              memcpy(server_data + c->ind, c, sizeof(SocketInfo));
            }
          }
          else {
            fprintf(STATUSFILE, "Invalid connection type %d for server socket from %s to %s on port %d\n", c->type, c->from, c->to, c->port);
            return -1;
          }
          /* send a CONNECTION ESTABLISHED MESSAGE to the main
           * program */
          SendMessage(client_message[SPIKE_MAIN].fd, 
              CONNECTION_ESTABLISHED, NULL, 0); 
          /*if (sysinfo.program_type == SPIKE_BEHAV) {
            fprintf(stderr, "spike_behav: connection type %d for server socket from %s %d to %s %d on port %d established\n", c->type, c->from, c->fromid, c->to, c->toid, c->port);
            } */

          break;
        case CONNECTION_ESTABLISHED:
          /* return the connection established message to the main
           * program */
          fprintf(STATUSFILE, "program %d sending out CONNECTION ESTABLISHED message to main program \n", sysinfo.program_type);
          SendMessage(client_message[SPIKE_MAIN].fd, 
              CONNECTION_ESTABLISHED, NULL, 0); 
          done = 1;
          break;
        case EXIT:
          /* this occurs only when the main program has failed to
           * start messaging correctly */
          fprintf(stderr, "Another module has died unexpectedly, exiting\n");
          return -1;
      }
    }
  }
  /* now that everything is established, we go through all of the connections
   * and set the maximum index. We also set the socket size for all data
   * connections */
  mini = dini = mouti = douti = 0;
  optval = DATASOCKET_WRITE_BUFFER_SIZE;
  optlen = sizeof(int);
  for (i = 0; i < MAX_CONNECTIONS; i++) {
    if (server_message[i].fd) {
      netinfo.messageinfd[mini++] = i;
    }
    if (client_message[i].fd) {
      netinfo.messageoutfd[mouti++] = i;
    }
    if (server_data[i].fd) {
      /* add this connection to the incoming data list */
      netinfo.datainfd[dini++] = i;
      /* set the socket to have a large READ SIZE */
      setsockopt(server_data[i].fd, SOL_SOCKET, SO_RCVBUF, 
          (void *) &optval, optlen);
    }
    if ((client_data[i].fd) && (client_data[i].toid >= MAX_DSPS)) {
      /* add this connection to the outgoing data list if it is not to a DSP*/
      netinfo.dataoutfd[douti++] = i;
      /* set the socket to have a large WRITE SIZE */
      setsockopt(server_data[i].fd, SOL_SOCKET, SO_SNDBUF, 
          (void *) &optval, optlen);
      /* check to see if this is a message to a DSP, in which case we
       * also want to add it to the datain list */
      if (client_data[i].toid < MAX_DSPS) {
        netinfo.datainfd[dini++] = i;
      }
    }
  }
  /* set the last element to be -1 for SetupFDList */
  netinfo.messageinfd[mini] = -1;
  netinfo.datainfd[dini] = -1;
  netinfo.messageoutfd[mouti] = -1;
  netinfo.dataoutfd[douti] = -1;
  fprintf(STATUSFILE, "Finished establishing messaging in program %d\n", 
      sysinfo.program_type);
  return 1;
}

int GetSlaveNum(const char *name)
{
    int num = 0;
    while ((num < netinfo.nslaves) && 
           (strcmp(netinfo.slavename[num], name) != 0)) {
        num++;
    }
    if (num == netinfo.nslaves) {
        return -1;
    }
    else {
        return num;
    }
}

int GetMachineNum(const char *name)
    /* return the index for this machine for the sysinfo variables.
     * the machine number is 0 for the master and the slave number + 1 for
     * slaves */
{
    int num = 0;
    if (strcmp(netinfo.mastername, name) == 0) {
        return 0;
    }
    else {
        num = GetSlaveNum(name) + 1;
            if (num == 0) {
            fprintf(stderr, "Error looking up machine number for %s\nThis is probably due to an error in the config file where this host is not listed as a master, slave, rtslave or  user data slave.\n", name);
            return -1;
        }
        return num;
    }
}

void SetupFDList(fd_set *readfds, int *maxfds, SocketInfo *server_message, 
        SocketInfo *server_data)
    /* set up the list of file descriptors to watch for incomding data or
     * messages and for outgoing */
{
    int i;
    *maxfds = 0;

    FD_ZERO(readfds);
    /* go through module file descriptors and add each one to the list */
    i = 0;
    while (netinfo.messageinfd[i] != -1) {
        FD_SET(server_message[netinfo.messageinfd[i]].fd, readfds);
        *maxfds = MAX(*maxfds, server_message[netinfo.messageinfd[i]].fd);
        i++;
    }
    i = 0;
    while (netinfo.datainfd[i] != -1) {
        FD_SET(server_data[netinfo.datainfd[i]].fd, readfds);
        *maxfds = MAX(*maxfds, server_data[netinfo.datainfd[i]].fd);
        i++;
    }
    /* now add the list of slaves if this is a master or the master if this is
     * a slave */
    if ((sysinfo.system_type[sysinfo.machinenum] == MASTER) && 
         (sysinfo.program_type == SPIKE_MAIN)) {
        for (i = 0; i < netinfo.nslaves; i++) {
            if (netinfo.masterfd[i]) {
                FD_SET(netinfo.masterfd[i], readfds);
                *maxfds = MAX(*maxfds, netinfo.masterfd[i]);
            }
        }
    }
    else if (sysinfo.program_type == SPIKE_MAIN) {
        FD_SET(netinfo.masterfd[netinfo.myindex], readfds);
        *maxfds = MAX(*maxfds, netinfo.masterfd[netinfo.myindex]);
    }
    (*maxfds)++;
}

void AddFD(int fd, SocketInfo *s, int *fdlist)
    /* add the file descriptor to the socket info structure and the fdlist */
{
    int i, j;
    /* find the first available element of the s outside the MAX_SOCKETS range
     * */
    i = MAX_SOCKETS - 1;
    while (s[++i].fd) ;
    /* set this element to be the fd */
    s[i].fd = fd;
    /* add the index to the fdlist */
    j = -1;
    while (fdlist[++j] != -1) ;
    fdlist[j] = i;
    fdlist[j+1] = -1;
    return;
}


void RemoveFD(int fd, SocketInfo *s, int *fdlist)
    /* remove the file descriptor to the socket info structure and the fdlist */
{
    int i, j;
    /* find the fd in s */
    i = -1;
    while (s[++i].fd != fd) ;
    /* remove this fd */
    s[i].fd = 0;
    /* remove the index for this fd */
    j = -1;
    while (fdlist[++j] != i) ;
    while (fdlist[j] != -1) {
        fdlist[j] = fdlist[j+1];
        j++;
    }
    return;
}


int SendPosBuffer(int fd, PosBuffer *posbuf)
    /* send out the updated parts of posbuf */
{
    char *tmpptr;
    int   tmpsize, totalsize;
    /* copy the static elements to tmpdata */
    memcpy(tmpdata, posbuf, POS_BUF_STATIC_SIZE);

    /* now copy the tracked pixels */
    totalsize = POS_BUF_STATIC_SIZE;
    tmpptr = tmpdata + POS_BUF_STATIC_SIZE;
    tmpsize = posbuf->ntracked * sizeof(int);
    memcpy(tmpptr, posbuf->trackedpixels, tmpsize);
    totalsize += tmpsize;

    /* now copy the image*/
    tmpptr += tmpsize;
    tmpsize = MAX_IMAGE_BYTES;
    memcpy(tmpptr, posbuf->image, tmpsize);
    totalsize += tmpsize;

    /* send out the data */
    return SendMessage(fd, POS_DATA, tmpdata, totalsize);
}

int GetPosBuffer(int fd, PosBuffer *posbuf)
    /* read in a position buffer from the specified file descriptor */
{
    int datalen;
    int tmpsize;
    char *tmpptr;
    
    if  (GetMessage(fd, tmpdata, &datalen, 1) == -1) {
        return -1;
    }
    /* copy the static elements to tmpdata */
    memcpy(posbuf, tmpdata, POS_BUF_STATIC_SIZE);

    /* now copy the tracked pixels */
    tmpptr = tmpdata + POS_BUF_STATIC_SIZE;
    tmpsize = posbuf->ntracked * sizeof(int);
    memcpy(posbuf->trackedpixels, tmpptr, tmpsize);
    /* now copy the new colors */
    tmpptr += tmpsize;
    tmpsize = MAX_IMAGE_BYTES;
    memcpy(posbuf->image, tmpptr, tmpsize);

    return 1;
}

int SendPosMPEGBuffer(int fd, PosMPEGBuffer *posbuf)
    /* send out the updated parts of posbuf */
{
    char *tmpptr;
    int   tmpsize, totalsize;
    /* copy the static elements to tmpdata */
    memcpy(tmpdata, posbuf, POS_MPEG_BUF_STATIC_SIZE);
    totalsize = POS_MPEG_BUF_STATIC_SIZE;
    tmpptr = tmpdata + POS_MPEG_BUF_STATIC_SIZE;

    /* now copy the new image buffer */
    tmpsize = (int) posbuf->size; 
    memcpy(tmpptr, posbuf->frame, tmpsize);
    totalsize += tmpsize;

    /* send out the data */
    return SendMessage(fd, POS_DATA, tmpdata, totalsize);
}

int SendChannelInfo(ChannelInfo *ch, int slaveorig)
    /* slave orig should be -1 if this didn't come from a slave */
{
    int i;
    /* send information about a channel to the other machines */
    if (sysinfo.system_type[sysinfo.machinenum] == MASTER) {
        /* send the channel information to the slaves */
	for(i = 0; i < netinfo.nslaves; i++) {
            /* check to see if this would go directly back to the slave from 
             * which it originated or if we are in the middle of programming a 
             * channel from that slave. */
            if ((i != slaveorig)  && (i != sysinfo.lastchslave)) {
		SendMessage(netinfo.slavefd[i], CHANNEL_INFO, (char *) ch, 
			sizeof(ChannelInfo));
	    }
        }
    }
    else {
        /* send this to the master so it can send it out to the slaves */
        SendMessage(netinfo.slavefd[netinfo.myindex], CHANNEL_INFO, (char *)ch, 
                    sizeof(ChannelInfo));
    }
    return 1;
}

void ByteSwap(short *sptr, int nelem)
{
    int i;
    char *cptr, ctmp;
    /* swap the first and second bytes of each short to create a shorts for the
     * dsps */
    cptr = (char *) sptr;
    for (i = 0; i < nelem; i++, cptr+=2) {
        ctmp = *cptr;
        *cptr = *(cptr+1);
        *(cptr+1) = ctmp;
    }
    return;
}

void ByteSwap(unsigned short *sptr, int nelem)
{
    int i;
    char *cptr, ctmp;
    /* swap the first and second bytes of each short to create a shorts for the
     * dsps */
    cptr = (char *) sptr;
    for (i = 0; i < nelem; i++, cptr+=2) {
        ctmp = *cptr;
        *cptr = *(cptr+1);
        *(cptr+1) = ctmp;
    }
    return;
}

void ByteSwap(u32 *lptr, int nelem)
{
    int i;
    char *cptr, ctmp;
    /* swap the first and second bytes of each short to create a shorts for the
     * dsps */
    cptr = (char *) lptr;
    for (i = 0; i < nelem; i++, cptr+=4) {
        ctmp = cptr[0];
        cptr[0] = cptr[3];
        cptr[3] = ctmp;

        ctmp = cptr[1];
        cptr[1] = cptr[2];
        cptr[2] = ctmp;
    }
    return;
}





