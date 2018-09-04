//
//  chatutil.h
//
//  Created by Praveen C Joshi on 8/30/18.
//  Copyright © 2018 All rights reserved.
//


#ifndef __CHATUTIL_H__
#define __CHATUTIL_H__ 1
#include <fstream>
#include "chat.h"

using namespace std;

typedef struct logstr_ {
    uint32_t clientid;
    string evt;
    
    logstr_(uint32_t clientid, string evt)
    :clientid(clientid), evt(evt)
    {
    }
}logstr_t;

std::ostream& operator<<(std::ostream& stream, const log_event_t& log_evt);


double getTime();
uint32_t  readn(uint32_t  readSocket, uint8_t *buf, uint32_t size);
char *randomString(int len);
bool ValidateServerLog();

#endif
