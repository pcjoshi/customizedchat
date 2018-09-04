//
//  chat.h
//
//  Created by Praveen C Joshi on 8/30/18.
//  Copyright © 2018 All rights reserved.
//
//#include <iostream>
//
#ifndef __CHAT_H__
#define __CHAT_H__ 1

#include <fstream>
#include <ctime>
#include <vector>
#include <set>

extern "C" {
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> 
#include <sys/mman.h>
#include <sys/wait.h>
}

using namespace std;

//#define _DEBUG__ 1

#define PORT 8888
#define CLIENTMSG 1
#define CLIENTACK 2
#define REGISTERMSG 3
#define MAX_CLIENTS 50


typedef struct client_info_ {
    uint32_t sd;
    uint32_t clientid;
    long pid;
    string clientfile;
} client_info_t;

typedef struct log_event_ {
    double timestamp;
    string evtStr;

    log_event_(double ts, char *evt)
        : timestamp(ts), evtStr(string(evt))
    {
    }
} log_event_t;


typedef struct chat_header_ {
    uint32_t to_clientid;
    uint32_t from_clientid;
    uint32_t msgtype;
    uint32_t msglen;
} chat_header_t;

typedef struct chat_message_ {
    chat_header_t header;
    char message[0];
} chat_message_t;

extern uint32_t max_clients;
extern uint16_t  portNum;

#endif
