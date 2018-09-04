//
//  chatutil.cpp
//
//  Created by Praveen C Joshi on 8/30/18.
//  Copyright © 2018 All rights reserved.

#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <ctime>
#include <glob.h>

#include "chat.h"
#include "chatutil.h"

using namespace std;

/* Get the clientid given a filename */
uint32_t filenameIndex(string filename) {
    std::size_t pos = filename.find("-");
    std::string str3 = filename.substr(pos+1);
    uint32_t index = (uint32_t)stoi(str3);
    return index;
}

/* For each client log file, get readmap and writemap vectors created which is 
 * basically parsing the file for every event into its respective vector.
 */
class ClientFile {
public:
    string filename;
    vector <logstr_t> writeMap;
    vector <logstr_t> readMap;
    
    ClientFile(string filename)
    :filename(filename)
    {
        ifstream infile;
        double a;
        string b;
        int c;
        string d;
        
        infile.open(filename);
        if (!infile.is_open()) {
            return;
        }
        while (!infile.eof()) {
            infile >> a >> b >> c >> d;
            if (b.compare("Write") == 0) {
                writeMap.push_back(logstr_(c, d));
            }
            else {
                if (b.compare("Read") == 0) {
                    readMap.push_back(logstr_(c, d));
                }
            }
        }
        infile.close();
    }
    
    ~ClientFile()
    {
    }
};

vector<ClientFile> clientObjects;

/* Returns the client object given the container of objects and the clientid
 * part of the filename in every object
 */
ClientFile *getClientObj(vector<ClientFile>& obj, uint32_t id) {
    
    for (auto iter = obj.begin(); iter != obj.end(); ++iter) {
        int index = filenameIndex((*iter).filename);
        if (index == id) {
            return (ClientFile *)&(*iter);
        }
    }
    cout << "Failed to get object for the id " << id << endl;
    return NULL;
}

/* Returns a vector of filename strings, basically reads the directory 
 * and does ls <pattern>
 */
vector<string> findFiles(const string& pattern){
    glob_t glob_result;
    
    //glob(pattern.c_str(),GLOB_ONLYDIR,NULL,&glob_result);
    glob(pattern.c_str(),GLOB_TILDE,NULL,&glob_result);
    vector<string> files;
    for(unsigned int i=0;i<glob_result.gl_pathc;++i){
#ifdef _DEBUG__
        printf("Found Client files : %s\n", string(glob_result.gl_pathv[i]).c_str());
#endif
        files.push_back(string(glob_result.gl_pathv[i]));
    }
    globfree(&glob_result);
    return files;
}

/* Given a string read from the server, the parsed message carries the fromid and toid
 * which are the clientids, this function gets the vectors (readMap and writeMap)
 * for the objects in consideration and checks for the first event(read or write)
 * depending on the object. 
 * If found, erase the objects from the vector and return true to move in the sorted order
 */
bool checkClientFiles(string evt, vector<logstr_t>& wObj, vector<logstr_t>& rObj) {
    cout << "Events to compare Write:" << evt << ":" << wObj[0].evt << " Read:" << rObj[0].evt << endl;
    if (wObj[0].evt.compare(evt) == 0) {
        if (rObj[0].evt.compare(evt) == 0) {
            wObj.erase(wObj.begin());
            rObj.erase(rObj.begin());
            return true;
        }
        cout << "Failed event comparison on Read" << wObj[0].evt << rObj[0].evt;
    } else {
        cout << "Failed event comparison on Write\n";
    }
    return false;
}


/* Read the serverlog file, read each line and for each to and from, pick the 
 * corresponding clientFile objects and check on the corresponding readMap or writeMap
 * vectors (as part of CheckCientFiles) for the message. Any failure is a failure
 * with the validation and otherwise until the end of the serverfile is read.
 * Return successful at the end.
 */
bool validateClientFiles() {
    
    ifstream serverFile;
    double time;
    string type;
    uint32_t fromid;
    uint32_t toid;
    string evt;
    bool success = true;
    
    
    serverFile.open("Serverlog.txt");
    if (!serverFile.is_open()) {
        return false;
    }

    cout << "ServerFile Opened" << endl;
    serverFile >> time >> type >> fromid >> toid >> evt;
    while (!serverFile.eof()) {
        cout << "Line Reads:" << "Message from Client " << fromid << " to Client " << toid << " msg: " << evt << endl;
        ClientFile *writeObj = (ClientFile *)getClientObj(clientObjects, fromid);
        if (writeObj != NULL) {
            ClientFile *readObj = (ClientFile *)getClientObj(clientObjects, toid);
            if (readObj != NULL) {
                if (checkClientFiles(evt, writeObj->writeMap, readObj->readMap)) {
                    serverFile >> time >> type >> fromid >> toid >> evt;
                    continue;
                }
                cout << "Failed : " << evt << endl;
                success = false;
                break;
            }
            cout << "Validating Server Log Failed : couldn't get the readObj" << toid << endl;
            success = false;
            break;
        } else {
            cout << "Validating Server Log Failed : couldn't get the writeObj" << fromid << endl;
            success = false;
            break;
        }
    }
    if (success == true) {
        printf("Successfully validated all client log files\n");
    }
    serverFile.close();
    return success;
}

/* Main function : Picks up all the files matching the pattern "client-*", the server log
 * file is already fixed to be Serverlog.txt. Creates the ClientFile objects for every 
 * file found and calls the ValidateClientFiles() for the match
 */
bool ValidateServerLog() {
    bool retval = false;
    vector<string> files = findFiles("./client-*");
    for (std::vector<string>::iterator file = files.begin() ; file != files.end(); ++file) {
        ClientFile obj = ClientFile(*file);
        clientObjects.push_back(obj);
    }
    retval = validateClientFiles();
    return retval;
}

/* Random String generator - some logic found on stackoverflow, works
 * ok given the length, it will form a fixed length random string
 * Probably there are better ways - need more thought on this
 * Also, the current logic is based on a single word and not 
 * string of words
 */
char *randomString(int len) {
    static const char ascii_chars[] =
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    
    char *random_str = (char *)malloc(len+1);
    for (int i = 0; i < len; ++i) {
        random_str[i] = ascii_chars[rand() % (sizeof(ascii_chars) - 1)];
    }
    random_str[len] = 0;
    return random_str;
}

std::ostream& operator<<(std::ostream& stream, const log_event_t& log_evt)
{
    stream << log_evt.timestamp << "," << log_evt.evtStr;
    return stream;
}


/* Picked up function from the web
 * Get Time in seconds since January 1, 2000 
 * in the current timezone 
 */
double getTime() {

    std::time_t result = std::time(nullptr);
#ifdef _DEBUG__
    printf("%s %ld seconds since the Epoch\n", std::asctime(std::localtime(&result)),result);
#endif
    return result;
}


/* Utility function to read until x bytes 
 * given that TCP is a byte stream protocol - may need to re-look at this
 * currently server is blocked in reading from this socket until it has read x 
 * bytes from the client
 */
uint32_t  readn(uint32_t  readSocket, uint8_t *buf, uint32_t size) {

    int32_t  n=0, readBytes=0;

    bzero(buf, size);
#ifdef _DEBUG__
    printf ("ReadN: PID %d \n", getpid());
#endif
    while (readBytes < size) {
        n = read(readSocket, (void *)&buf[readBytes], (size-readBytes));
#ifdef _DEBUG__
        printf ("%d bytes read out of %d size from Socket %d\n", n, size, readSocket);
#endif
        if (n < 0) {
            return -1;
        }
        if (n == 0 ) {
            if (readBytes == 0) {
               return -1;
            } 
            break;
        }
        readBytes += n;
    }
#ifdef _DEBUG__
    for (int i=0;i < readBytes; i++){
        printf("Read Bytes %02X ", buf[i]);
    }
#endif
    return readBytes;
}

