//
//  chatclient.h
//  multiClientChat
//
//  Created by Praveen C Joshi on 8/30/18.
//  Copyright © 2018 All rights reserved.
//
//#include <iostream>
#ifndef __CHATCLIENT_H__
#define __CHATCLIENT_H__

#include <fstream>
#include "chat.h"

#define CLIENT_PROCESS_TIMEOUT 1 // 1sec
#define P_EXPIRY_TIME 30
#define CLIENT_MESSAGES_TO_SEND 10

#ifdef __cplusplus
extern "C" {
#endif
extern void startClient(client_info_t *client);

extern uint32_t my_clientid;

#ifdef __cplusplus
}
#endif

#endif
