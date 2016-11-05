CC = g++
CXXFLAGS = -Ofast -g -Wall -std=c++11

SRCDIR = src
BINDIR = bin

SRCS := $(SRCDIR)/Project2.cpp
OBJS := $(SRCS:$(SRCDIR)/.cpp=.o)
INC := ./include/
LIB = ./lib/ -lpthread

CPPFLAGS += -I $(INC)

TARGET = Project2

$(BINDIR)/$(TARGET) : $(OBJS)
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -o $(BINDIR)/$(TARGET) $(OBJS) -L$(LIB)

clean:
	rm -f $(BINDIR)/$(TARGET)
