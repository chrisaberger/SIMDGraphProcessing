# the compiler: gcc for C program, define as g++ for C++
CC = g++

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -g -Wall -O3 -fopenmp -std=c++0x -mavx -ftree-vectorize

# the build target executable:
TARGET1 = main

all: $(TARGET1)

$(TARGET1): $(TARGET1).cpp
	$(CC) $(CFLAGS) -o $(TARGET1) $(TARGET1).cpp

clean:
	$(RM) $(TARGET1)
