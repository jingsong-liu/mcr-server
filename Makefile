INCDIR = include
OBJDIR = /tmp

CC=gcc
CFLAGS= -I$(INCDIR) -Wall
DBG_FLAG= -g
TEST_FLAG = -DTEST

LIBS ?= -lpthread

DEPS = $(wildcard $(INCDIR)/*.h)

SRC = $(wildcard *.c)
OBJ = $(patsubst %.c, $(OBJDIR)/%.o, $(SRC)) http-parser/http_parser.o


mcr-server: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)  $(DBG_FLAG)

$(OBJDIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)  ${DBG_FLAG}

.PHONY: clean

clean:
	rm -f *.o $(OBJDIR)/*.o
