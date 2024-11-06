CC=gcc

# C/C++ Source file
C_SRCS = fileSystem.c server.c client.c
C_OBJS = obj/fileSystem.o obj/server.o 
C_OBJS_Client = obj/client.o
C_TARGET = bin/fileSystem
C_TARGET_Client = bin/client

all: bin_dir obj_dir C_target C_target_Client

# C language target
C_target: $(C_OBJS)
	$(CC) $(C_OBJS) -o $(C_TARGET) -g -lpthread

C_target_Client: $(C_OBJS_Client)
	$(CC) $(C_OBJS_Client) -o $(C_TARGET_Client) -g

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

gdb:
	gdb ./bin/fileSystem

gdb_client:
	gdb ./bin/client

gdb_server:
	gdb ./bin/fileSystem

