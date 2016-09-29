CC = g++
CXXFLAGS = -g -Wall -std=c++11

SRCDIR = src
BINDIR = bin

SRCS := $(SRCDIR)/Project1.cpp
OBJS := $(SRCS:$(SRCDIR)/.cpp=.o)
INC := ./include/

CPPFLAGS += -I $(INC)

TARGET = Project1

$(BINDIR)/$(TARGET) : $(OBJS)
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -o $(BINDIR)/$(TARGET) $(OBJS)

clean:
	rm -f $(BINDIR)/$(TARGET)
