#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

#include <string.h>
#include <errno.h>
#include <linux/limits.h>

#define SERVICE_NAME_MAX        128
#define BACKLOG_MAX             6
#define PORT_MAX                6
#define FILEPATH_MAX            (NAME_MAX + NAME_MAX) 


struct server_config {
    char servername[SERVICE_NAME_MAX];
    char service[SERVICE_NAME_MAX];
    char port[PORT_MAX];
    int backlog;
    char wwwroot[FILEPATH_MAX];

};

int
read_server_config(const char* path, struct server_config* sc);

char*
dump_server_config(struct server_config* sc, char* buf, size_t size);

#endif
