#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

#include <string.h>
#include <errno.h>


#define HOST_NAME_MAX           128
#define SERVICE_NAME_MAX        128
#define BACKLOG_MAX             6
#define PORT_MAX                6
#define FILEPATH_MAX            1024


struct server_config {
    char hostname[HOST_NAME_MAX];
    char service[SERVICE_NAME_MAX];
    int port;
    int backlog;
    char wwwroot[FILEPATH_MAX];

};

int
read_server_config(const char* path, struct server_config* sc);

char*
dump_server_config(struct server_config* sc, char* buf, size_t size);

#endif
