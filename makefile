CC=gcc

# C/C++ Source file
C_SRCS = fileSystem.c server.c client.c main.c
C_OBJS = obj/fileSystem.o obj/server.o 
C_OBJS_Client = obj/client.o
C_OBJS_MAIN = obj/main.o obj/fileSystem.o
C_TARGET = bin/server
C_TARGET_Client = bin/client
C_TARGET_MAIN = bin/main

all: bin_dir obj_dir C_target C_target_Client C_target_MAIN

# C language target
C_target: $(C_OBJS)
	$(CC) $(C_OBJS) -o $(C_TARGET) -g -lpthread

C_target_Client: $(C_OBJS_Client)
	$(CC) $(C_OBJS_Client) -o $(C_TARGET_Client) -g

C_target_MAIN: $(C_OBJS_MAIN)
	$(CC) $(C_OBJS_MAIN) -o $(C_TARGET_MAIN) -g

# Directory for binary files.
bin_dir:
	mkdir -p bin

# Directory for object files for C.
obj_dir:
	mkdir -p obj

# Rule for C object
obj/%.o: %.c
	$(CC) -c $< -o $@ -g

clean:
	rm -rf bin obj

run_server:
	./bin/fileSystem

run_client:
	./bin/client

run_main:
	./bin/main

gdb:
	gdb ./bin/fileSystem

gdb_client:
	gdb ./bin/client

gdb_server:
	gdb ./bin/server

