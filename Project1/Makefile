TARGET = Project1

CC = g++
CFLAGS = -g -Wall -std=c++11

LINKER = g++ -o
LFLAGS = -Wall

SRCDIR = src
OBJDIR = obj
BINDIR = bin

SRCS := $(wildcard $(SRCDIR)/*.cc)
INC := $(wildcard $(SRCDIR)/*.h)
OBJS := $(SRCS:$(SRCDIR)/%.cc=$(OBJDIR)/%.o)
rm = rm -f

$(BINDIR)/$(TARGET):$(OBJS)
	$(LINKER) $@ $(LFLAGS) $(OBJS)

$(OBJS):$(OBJDIR)/%.o:$(SRCDIR)/%.cc
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(rm) $(BINDIR)/$(TARGET) obj/$(TARGET).o

