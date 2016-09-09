# Compiler and Compile options.
CC = g++
CXXFLAGS = -g -Wall -std=c++11

# Macro specifying path for compile.
SRCS := prac_pthread.cpp
OBJS := $(SRCS:.cpp=.o)
INC = ./include/
LIB = ./lib/ -lpthread

# pre-processor.
CPPFLAGS += -I$(INC)

# compile command.
TARGET = prac_pthread
$(TARGET): $(OBJS)
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -o $(TARGET) $(OBJS) -L$(LIB)

# Delete binary & object files.
clean:
	rm $(BIN)$(TARGET) $(OBJS)
