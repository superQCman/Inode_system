CC=gcc

# C/C++ Source file
C_SRCS = fileSystem.c main.c
C_OBJS = obj/fileSystem.o obj/main.o
C_TARGET = bin/fileSystem

all: bin_dir obj_dir C_target

# C language target
C_target: $(C_OBJS)
	$(CC) $(C_OBJS) -o $(C_TARGET) -g

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

run:
	./bin/fileSystem

gdb:
	gdb ./bin/fileSystem

