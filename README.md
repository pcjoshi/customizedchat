# customizedchat

Server Process : The server process takes in the arguments (port and clients) and spawns as many client processes. It will start the listener on the specified port and accept the connections as they come in. It will further listen into the client messages (data or close) and broker the data messages accordingly. It will log the same into a file. When all the clients are done with, it will close the log file and validate the server log file(Serverlog.txt) against all client files.

Client Process : The client process starts with connecting to the server and registering itself, further it will start listening on the socket while on a timeout of 1sec, it will send a random peer and a random string. On receiving a client message, it will acknowledge the same. After an expiry time of 30 secs, it will disconnect from the server. With every message (read or written), it will log the same in a client file (client-<clientid>).

Use "make" to build chat_run executable.

run "chat_run <port> <no_of_clients>" to run the module.

