#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

#include <string.h>
#include <errno.h>


#define HOST_NAME_MAX           256
#define SERVICE_NAME_MAX        128
#define QUEUE_SIZE_MAX          6

struct server_config {
    char hostname[HOST_NAME_MAX];
    char service[SERVICE_NAME_MAX];
    int port;
    int queue_size;

};

int
read_server_config(const char* path, struct server_config* sc);

#endif
