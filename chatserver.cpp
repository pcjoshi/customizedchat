//
//  chatserver.cpp
//  multiClientChat
//
//  Created by Praveen C Joshi on 8/30/18.
//  Copyright © 2018 All rights reserved.
//
//#include <iostream>

#include <fstream>
#include <set>

extern "C" {
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
}
#include "chat.h"
#include "chatutil.h"
#include "chatclient.h"


using namespace std;

uint32_t  max_clients = MAX_CLIENTS;
uint16_t  portNum = PORT;


uint32_t clientid_to_sockfd_arr[MAX_CLIENTS];
std::set<int> sockset;

/* readServerMsg reads the clients message */
chat_message_t *readServerMsg(uint32_t  readSocket, ofstream &file) {
    uint32_t  readBytes=0;
    uint32_t  n;
    chat_message_t *msg;
    chat_header_t hdr = {0};
    uint32_t msglen;
    
    // Read the header first
    n = readn(readSocket, (uint8_t *)&hdr, sizeof(hdr));
    if (n == -1)
        return NULL;
    readBytes += n;
    msglen = ntohl(hdr.msglen);
#ifdef _DEBUG__
    printf("Server: Read %d bytes from socket %d, next msglen %d \n", n, readSocket, msglen);
#endif
    
    msg = (chat_message_t *)malloc( sizeof(chat_message_t) + msglen + 1);
    bzero(msg, sizeof(chat_message_t) + msglen + 1);
    
    // Reading it back from the Network -ntohs
    msg->header = hdr;
    if (msglen == 0) {
        // Probably register message
        return msg;
    }
    // Read the Message now
    n = readn(readSocket, (uint8_t *)msg->message, msglen);
    if (n == -1) {
        printf("Server: Readn Failed\n");
        return NULL;
    }
    msg->message[msglen]= '\0';
#ifdef _DEBUG__
    printf("Server: Read Message \n to client: %d, from client: %d, message : %s\n", ntohl(hdr.to_clientid), ntohl(hdr.from_clientid), msg->message);
#endif
    file << getTime() << " Read " << ntohl(hdr.from_clientid) << " " << ntohl(hdr.to_clientid) << " " << msg->message << "\n";
    return msg;
}

/* WriteServerMsg - just picks up the message and writes to where it is intended. It doesn't
 * have any other need to write a message to the client
 */
void writeServerMsg(uint32_t sockfd, chat_message_t *sendbuf, ofstream &file) {
    uint32_t  tmpfd = clientid_to_sockfd_arr[ntohl(sendbuf->header.to_clientid)];
    send(tmpfd,(void *)sendbuf, sizeof(chat_message_t) + ntohl(sendbuf->header.msglen), 0 );
}

/* Main function that starts the server process and forks the client processes. It will bind
 * and listen on the port specified at the prompt. Will log the client messages received which it
 * will forward as expected. At the end, it will validate the server log with all the client
 * files accordingly
 */

int main(int  argc , char *argv[])
{
    uint32_t  server_socket , addrlen , new_socket;
    int32_t activity;
    uint32_t  max_sd;
    uint16_t  portNum = PORT;
    long  pid;
    struct sockaddr_in address;
    ofstream myfile;
    
    if (argc == 1 || argc > 3) {
       perror("Usage : chatServer <port_no> <max_clients>");
       return -1;
    }

    portNum = atoi(argv[1]);
    if((portNum > 65535) || (portNum < 2000)) {
       perror("Please enter a port number between 2000 - 65535");
       return 0;
    }
    
    max_clients = atoi(argv[2]);
    if((max_clients > MAX_CLIENTS) || (max_clients < 1)) {
       printf("Please enter a number less than %d", MAX_CLIENTS);
       return 0;
    }

    //set of socket descriptors
    fd_set readfds;
   
    if( (server_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    //type of socket created
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    /* enabling iostream breaks the bind definition */
    int status = ::bind(server_socket, (struct sockaddr *)&address, sizeof(address));
    if (status < 0) {
        perror("bind: error");
        exit(EXIT_FAILURE);
    }
    if (listen(server_socket, 5) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    //create the child process
    for (uint32_t  i = 0; i < max_clients; i++) {
        pid = fork(); // forking here
        if (pid < 0){
            printf("forking child process %d failed", i);
            return -1;
        }
        if (pid == 0 ) {
            // This is a Client process
            client_info_t client;
            printf("Server: CHILD: %d CREATED %d \n",i,  getpid());

            my_clientid = i;
            client.clientid = i;
            client.pid = getpid();
            client.clientfile = std::string("client-" + std::to_string(my_clientid));
            
            startClient(&client);
            exit(0);
        }
    } //end child creation for loop
    
    // We are in the server process
    chat_message_t *chatMsg;
 
    //accept the incoming connection
    addrlen = sizeof(address);
    /* Opening the Server log file */
    myfile.open("Serverlog.txt");
    while (true) {
        max_sd = server_socket;
        FD_ZERO(&readfds);
        FD_SET(server_socket, &readfds);
#ifdef _DEBUG__
        printf("Server: Inside the while loop for max_sid %d\n", max_sd);
#endif
        //add child sockets to set
        for (auto iter = sockset.begin(); iter != sockset.end(); ++iter) {
            FD_SET( *iter , &readfds);
            //highest file descriptor number, need it for the select function
            if(*iter > max_sd) {
                max_sd = *iter;
            }
        }
        
#ifdef _DEBUG__
        printf("Server: Waiting on Select %d\n", max_sd);
#endif
        //wait indefinitely(timeout is null) for any activity fds
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
        if (activity < 0) {
            if (errno!=EINTR) {
                printf("error");
                break;
            } else {
                perror("Interrupted");
                continue;
            }
        }
        
#ifdef _DEBUG__
        printf("Server: Received  an event \n");
#endif
        //If something happened on the server socket then its an incoming connection
        if (FD_ISSET(server_socket, &readfds)) {
            if ((new_socket = accept(server_socket,(struct sockaddr *)&address, (socklen_t*)&addrlen)) <= 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            //inform user of socket number - used in send and receive commands
            printf("Server: New connection, socket fd is %d , ip is : %s , port : %d\n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
            sockset.insert(new_socket);
        }
#ifdef _DEBUG__
        printf("Server: Checking on an event from the clients\n");
#endif
        for (auto iter = sockset.begin(); iter != sockset.end(); ++iter) {
            if (FD_ISSET( *iter , &readfds)) {
                //incoming message or closing
                chatMsg = readServerMsg(*iter, myfile);
                if (chatMsg == NULL) {
                    //Somebody disconnected , get his details and print
                    getpeername(*iter, (struct sockaddr*)&address , \
                                (socklen_t*)&addrlen);
                    printf("Host disconnected , ip %s , port %d \n" ,
                           inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
                    close(*iter);
                    sockset.erase(*iter);
                    if (!sockset.size()) {
                        printf("Closing the server file and process\n");
                        myfile.close(); //close the server log file
                        printf("Validate client files now\n");
                        ValidateServerLog();
                        exit(0);
                    }
                    break;
                }
                else {
                    uint32_t msgtype = ntohl(chatMsg->header.msgtype);
                    if (msgtype == CLIENTMSG || msgtype == CLIENTACK) {
#ifdef _DEBUG__
                        printf("Server: Recvd Client message type:%d from Client %d to Client %d, Writing the server msg back\n", msgtype, ntohl(chatMsg->header.from_clientid), ntohl(chatMsg->header.to_clientid));
#endif
                        writeServerMsg(*iter, chatMsg, myfile);
                    } else if (msgtype == REGISTERMSG) {
                        uint32_t clientid = ntohl(chatMsg->header.from_clientid);
#ifdef _DEBUG__
                        printf("Server: Recvd Register message from Client %d, socket %d\n", clientid, *iter);
#endif                        
                        clientid_to_sockfd_arr[clientid] = *iter;
                    } else {
                        printf("Server: Received unknown message %d", msgtype);
                    }
                    free(chatMsg);
                    break;
                }
            }
        }
    }
    myfile.close();
    ValidateServerLog();
    return 0;
}
