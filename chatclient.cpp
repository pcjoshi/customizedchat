//
//  chatclient.cpp
//  multiClientChat
//
//  Created by Praveen C Joshi on 8/30/18.
//  Copyright © 2018 All rights reserved.
//
//#include <iostream>
#include <fstream>
extern "C" {
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/time.h> 
}

#include "chat.h"
#include "chatutil.h"
#include "chatclient.h"

using namespace std;


uint32_t my_clientid;

/* Reading a chat message on a clientThread
 * make sure the caller cleans up the allocated 
 * message returned 
 */

chat_message_t *readClientMsg(uint32_t  readSocket, ofstream &file) {
    uint32_t  readBytes=0;
    uint32_t  nbytes;
    chat_message_t *msg;
    chat_header_t hdr = {0};
    uint32_t msglen;
    
    // Read the header first    
    nbytes = readn(readSocket, (uint8_t *)&hdr, sizeof(hdr));
    if (nbytes == -1)
        return NULL;
    readBytes += nbytes;
    msglen = ntohl(hdr.msglen);

    msg = (chat_message_t *)malloc( sizeof(chat_message_t) + msglen + 1);
    bzero(msg, sizeof(chat_message_t) + msglen + 1);

    // Reading it back from the Network -ntohs
    msg->header.from_clientid = ntohl(hdr.from_clientid);
    msg->header.to_clientid = ntohl(hdr.to_clientid);
    msg->header.msgtype = ntohl(hdr.msgtype);
    msg->header.msglen = msglen;
    
#ifdef _DEBUG__    
    printf("Client %d: Read Msg from clientid: %d, to_client_id: %d, msglen = %d, msgtype = %d\n", my_clientid, ntohl(hdr.from_clientid), ntohl(hdr.to_clientid), ntohl(hdr.msglen), ntohl(hdr.msgtype));
#endif
    
    
    // Read the Message now 
    nbytes = readn(readSocket, (uint8_t *)&msg->message, msglen);
    if (nbytes == -1)
        return NULL;
    msg->message[msg->header.msglen]= '\0';

#ifdef _DEBUG__
    printf("Client %d: to client: %d, from client: %d, message : %s\n", my_clientid, ntohl(hdr.to_clientid), ntohl(hdr.from_clientid), msg->message);
#endif

    //Log the Read Event
    file << getTime() << " Read " << msg->header.from_clientid << " " <<  msg->message << "\n";
    return msg;
}

/* WriteCLientAck responds to the other client with an ACK message, this could be further
 * optimized to not carry any message but just the header msgtype
 */
void writeClientAck(uint32_t sockfd, chat_message_t *chatMsg, ofstream &file) {
    chat_message_t *msg;
    uint32_t  msglen;
    char ackMsg[] = "Ok"; // FIXME: Close on what it should be.
    
    //Create Message
    msglen = strlen(ackMsg);
    msg = (chat_message_t *)malloc(sizeof(chat_message_t) + msglen + 1);
    bzero(msg, sizeof(chat_message_t) + msglen);
    msg->header.from_clientid = htonl(chatMsg->header.to_clientid);
    msg->header.to_clientid = htonl(chatMsg->header.from_clientid);
    msg->header.msglen = htonl(msglen);
    msg->header.msgtype = htonl(CLIENTACK);
    strcpy(msg->message, &ackMsg[0]);
    
#ifdef _DEBUG__
    printf("Client %d: Write:Ack: clientid = %d, msglen = %d, msgtype = %d\n", my_clientid, msg->header.from_clientid, msg->header.msglen, msg->header.msgtype);
#endif
    send(sockfd, (void *)msg , sizeof(chat_message_t) + msglen, 0);
    
    //Log the Write event
    file << getTime() << " Write " << chatMsg->header.from_clientid << " " << msg->message << "\n";
    
    /*Clear the allocated message here only */
    free(msg);
}

/* Send Client Message - random peer and random string
 */
void writeClientMsg(uint32_t sockfd, ofstream &file) {
    uint32_t  randPeer;
    chat_message_t *msg;
    uint32_t  msglen;
    char *randString;
    
    while (true) {
        randPeer = rand() % max_clients;
        if (randPeer != my_clientid) {
            break;
        }
    }

    randString = randomString(8);
    msglen = strlen(randString);

    msg = (chat_message_t *)malloc(sizeof(chat_message_t) + msglen + 1);
    bzero(msg, sizeof(chat_message_t) + msglen + 1);
    msg->header.from_clientid = htonl(my_clientid);
    msg->header.to_clientid = htonl(randPeer);
    msg->header.msglen = htonl(msglen);
    msg->header.msgtype = htonl(CLIENTMSG);
    memcpy(msg->message, randString, msglen);
    
#ifdef _DEBUG__
    printf("Client %d: Write Message: to_clientid = %d, msglen = %d, msgtype = %d\n msg:%s\n", my_clientid, randPeer,msglen, ntohl(msg->header.msgtype), msg->message);
#endif
    
    //Send the message to sockfd - Message is written to the server
    send(sockfd, (void *)msg , sizeof(chat_message_t) + msglen, 0);
    
    //Log the Write event
    file << getTime() << " Write " << randPeer << " " << msg->message << "\n";
    
    //Free the allocations done for msg and random string
    free(msg);
    free(randString);
}

void writeRegisterMsg(uint32_t sockfd) {
    chat_message_t msg;
    
    msg.header.from_clientid = htonl(my_clientid);
    msg.header.to_clientid = 0;
    msg.header.msglen = 0;
    msg.header.msgtype = htonl(REGISTERMSG);

    //Send the message to sockfd - Message is written to the server
    send(sockfd, (void *)&msg , sizeof(chat_message_t), 0);
    
#ifdef _DEBUG__
    printf("Client: %d: Write Register:Header: \n", my_clientid);
#endif
}

uint32_t  getConnected() {
    uint32_t  cSocket = 0;
    sockaddr_in sAddr;
    
    sAddr.sin_family = AF_INET;
    sAddr.sin_addr.s_addr = INADDR_ANY;
    sAddr.sin_port = htons(portNum);

    if((cSocket = socket(AF_INET, SOCK_STREAM, 0)) <= 0) {
        printf("Socket not created \n");
        return 0;
    }

    if(connect(cSocket, (struct sockaddr *)&sAddr, sizeof(sAddr)) < 0)
    {
        printf("Connection failed due to port and ip problems %d, %d\n", errno, PORT);
        return 0;
    }
    
    //Register with the server right away
    writeRegisterMsg(cSocket);
    return cSocket;
}

/* This is the core client function which sends the messages to the peer and
 * reads from the peer. The process will exit after 20 secs of running in which time,
 * it is expected to have written the 20 messages and received similar messages. This
 * expiry time is just for closing the thread and could be set accordingly.
 */
void startClient(client_info_t *client) {
    uint32_t  clientSocket = 0;
    uint32_t  n = 0;
    ofstream myfile;
    uint32_t  numTries = 3;
    timeval begin_time, current_time; 
    double time_duration=0;
    uint32_t sentMsgs = 0;
    
    while (n < numTries) {
        clientSocket = getConnected();
        if (clientSocket == 0) {
            n++;
            sleep(1);
            continue;
        }
        else
            break;
    }
    if (clientSocket == 0) {
        printf("Closing client Thread\n");
        return;
    }
    client->sd = clientSocket;

    fd_set rfds;
    struct timeval tv;
    uint32_t  retval;
    double timestamp;
    
    tv.tv_sec = CLIENT_PROCESS_TIMEOUT;  //Timeout set to 1 sec
    tv.tv_usec = 0;
    
    // Open the client log event file
    myfile.open(client->clientfile.c_str());
    chat_message_t *chatMsg;
    
    // Running the client process for
    gettimeofday(&begin_time, 0);
    while (time_duration <= P_EXPIRY_TIME) {
        FD_ZERO(&rfds);
        FD_SET(clientSocket, &rfds);
        retval = select(clientSocket + 1, &rfds, NULL, NULL, &tv);
        if (retval == -1) {
            perror("select()");
        }
        else if (retval) {
            if (FD_ISSET(clientSocket, &rfds)) {
                chatMsg = readClientMsg(clientSocket, myfile);
                if (chatMsg == NULL) {
                    myfile.close(); //closing the log event file
                    return;
                }
#ifdef _DEBUG__
                printf("Client %d: Recvd on %d socket from this socket %d, this msg:%s\n", my_clientid, clientSocket, chatMsg->header.from_clientid, chatMsg->message);
#endif
                timestamp = getTime();
                /* Acknowledge the peer with a ACK message and log event
                 * make sure, it is not a ACK message received
                 */
                if (chatMsg->header.msgtype != CLIENTACK) {
                    writeClientAck(clientSocket, chatMsg, myfile);
                }
            }
        }
        else {
            //printf("Pid : %d Client socket %d Timed out\n", getpid(), clientSocket);
            /* With every select timeout value kicking in, client will
             * pick a random peer and send a random string
             */
            if (++sentMsgs <= CLIENT_MESSAGES_TO_SEND) {
               writeClientMsg(clientSocket, myfile);

            }
        }
        /* Checking for if the total time elapsed is not more than expiry time */
        gettimeofday(&current_time, 0);
        time_duration = current_time.tv_sec - begin_time.tv_sec;
    }

    myfile.close();
}
