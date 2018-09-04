
CC=g++
CFLAGS=-std=c++11 -I.
DEPS = chatutil.h chat.h chatclient.h
OBJ = chatutil.o chatclient.o chatserver.o 

%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

chat_run: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

