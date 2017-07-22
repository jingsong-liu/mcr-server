INCDIR = include
CC=gcc
CFLAGS = -I$(INCDIR)

ODIR = obj

LIBS = -lpthread

_DEPS = server_cofnig.h
DEPS = $(_DEPS)

_OBJ = server_config.o mcr_server.o
OBJ = $(_OBJ)

mcr-server: $(OBJ)
	$(CC) -o $@ $(OBJ) $(CFLAGS) $(LIBS) 

.PHONY: clean

clean:
	rm -f *.o $(binaries)
	echo Clean Done
