INCDIR = include
OBJDIR = obj

CC=gcc
CFLAGS= -I$(INCDIR) -Wall
DBG_FLAG= -g
TEST_FLAG = -DTEST

LIBS ?= -lpthread

DEPS = $(wildcard $(INCDIR)/*.h)

SRC = $(wildcard *.c)
OBJ = $(patsubst %.c, $(OBJDIR)/%.o, $(SRC))


mcr-server: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)  $(DBG_FLAG)

$(OBJDIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) 

.PHONY: clean

clean:
	rm -f *.o $(OBJDIR)/*.o
