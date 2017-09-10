#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "include/mcr_config.h"
#include "include/mcr_define.h"
#include "include/mcr_server.h"

#define DEFAULT_SERVER_CONFIG_FILE      "./default.conf"


void
usage()
{
    char version[] = "1.1";
    printf("mcr-server-v%s Usage:\n"
           "  ./mcr-server [-c config_file] [-d] \n"
           ,version);
}


void
help()
{
    char version[] = "1.1";
    printf("mcr-server-v%s Help:\n"
           "  -c config file\n"
           "  -d run as deamon\n"
           "  -h show help\n",
           version);
}


int
main(int argc, char* argv[])
{
    struct server_config config;
    char *server_config_file = DEFAULT_SERVER_CONFIG_FILE;
    int server_sock = -1;
    int c = -1;
    int daemonize = 0;
    while((c = getopt(argc, argv, "c:dh")) != -1) {
        switch(c) {
            case 'c': /* config file */
                server_config_file = optarg;
                break;
            case 'd': /* run as deamon */
                daemonize = 1;
                break;
            case 'h': /* show help */
                help();
                return 0;
            default:
                usage();
                return -1;
        }

    }

    if ( -1 == read_server_config(server_config_file, &config)) {
        printf("error read_server_config :%s\n", strerror(errno));
    }

    if( -1 == (server_sock = open_serversock(config.port, config.backlog)))
        return -1;

    if (daemonize) {
        printf("daemonize mcr-server ...\n");
        /* change work directory to root, redirect stdio to /dev/null. */
        daemon(0, 0);
    }

    c = run_serverloop(server_sock, config);
    if (-1 == c) {
        printf("server loop exited unexpected, error: %s.\n", strerror(errno));
    }
    else {
        printf("server exited.\n");
    }

    close_serversock(server_sock);
    return -1;
}
