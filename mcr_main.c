#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>
#include <netdb.h>
#include "include/mcr_config.h"
#include "include/mcr_http.h"

#define DEFAULT_SERVER_CONFIG_FILE      "./default.conf"

struct cli_args {
    int fd;
    int reserved;
};


void *
cli_conn(void* arg) {
    int sock = ((struct cli_args*)arg)->fd;
    size_t recved = 0;

    size_t recv_buflen = 80*1024;
    char *recv_buf = malloc(recv_buflen*sizeof(char));
    if (recv_buf == NULL) {
        goto out;
    }


    mcr_http *mhttp = mcr_make_http();
    if (mhttp == NULL) {
        free(recv_buf);
        goto out;
    }

    mhttp->attach(mhttp, sock, recv_buf, &recved);

    while(1)
    {
        recved = recv(sock, recv_buf, 1024, 0);
        if (recved < 0) {
            printf("sock read error: %s\n",  strerror(errno)); 
            break;

        } else {
            /* do parse in the loop, or in event handler */
            if(0 != mhttp->parse(mhttp)) {
                break;
            }

            if (mhttp->context->complete_flag == 1) {
                break;
            }

        }
    }

    free(recv_buf);
    mcr_free_http(mhttp);

out:
    close(sock);
    pthread_exit(0);
}


int
server_init(int family, int type, struct sockaddr_in* server_addr, int backlog) {
    int sock;
    sock = socket(family, type, 0);
    if (-1 == sock) {
        printf("create socket error: %s\n", strerror(errno));
        goto err1;
    }
    
    int optval = 1;
    if (-1 ==  setsockopt(sock, SOL_SOCKET , SO_REUSEADDR, &optval, sizeof(optval))) {
        printf("set reuse addr failed: %s\n", strerror(errno));
    }

    if (0 != bind(sock, (struct sockaddr *)server_addr, sizeof(struct sockaddr_in))) {
        printf("bind to %s:%d failed: %s\n", inet_ntoa(server_addr->sin_addr), ntohs(server_addr->sin_port), strerror(errno));
        goto err2;
    }

    if (0 != listen(sock, backlog)) {
        printf("listen error: %s\n", strerror(errno)); 
        goto err2;
    }

    printf("successfully start listenning on %s:%d\n", inet_ntoa(server_addr->sin_addr),ntohs(server_addr->sin_port));
        return sock;

    err2:
        close(sock);
    err1:
        return -1;
}

int
serve(int server_sock)
{
    struct sockaddr_in client_addr;
    socklen_t client_addr_len= sizeof(client_addr);
    pthread_t pid;
    struct cli_args cargs;
    while(1) {
        int new_fd = accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_len);
        if (new_fd < 0) {
            printf("accept error: %s\n", strerror(errno));
            return -1;
        }
        
        printf("accept a connect from %s:%d\n",inet_ntoa(client_addr.sin_addr),
            ntohs(client_addr.sin_port));

        cargs.fd = new_fd;
        if(0 != pthread_create(&pid, NULL, cli_conn, (void *)&cargs)) {
            printf("create pthread error: %s\n", strerror(errno));
            return -1;
        }

        pthread_detach(pid);
    }

    //cloase child thread?

    return 0;
}


int
select_addr_info(char* hostname, char* service, struct addrinfo** ai_list) {
    struct addrinfo hint;
    memset(&hint, 0, sizeof(hint));
    hint.ai_flags = AI_ALL;
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;

    if (0 != getaddrinfo(hostname, service, &hint, ai_list))
    {
        printf("get host address error:%s", gai_strerror(errno));
        return -1;
    } else {
        return 0;
    }
}


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


extern char *optarg;
int
main(int argc, char* argv[])
{
    struct server_config config;
    char *server_config_file = DEFAULT_SERVER_CONFIG_FILE;
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
                exit(0);
            default:
                usage();
                exit(-1);
        }

    }

    if ( -1 == read_server_config(server_config_file, &config)) {
        printf("error read_server_config :%s\n", strerror(errno));
    }

    struct addrinfo * ai_list = NULL, * ai;
    select_addr_info(config.hostname, config.service, &ai_list);

    int server_sock = -1;
    for (ai = ai_list; ai != NULL; ai = ai_list->ai_next) {
        if ((server_sock = server_init(ai->ai_family, ai->ai_socktype, (struct sockaddr_in*)(ai->ai_addr), config.backlog)) != -1) {
            // just use the first available host address
            break;
        }
    }


    /* try localhost:http */
    if (-1 == server_sock) {
        printf("configured host address config seems unavailable, we try to use localhost\n");
        select_addr_info("localhost", "http", &ai_list);
        for (ai = ai_list; ai != NULL; ai = ai_list->ai_next) {
            if ((server_sock = server_init(ai->ai_family, ai->ai_socktype, (struct sockaddr_in*)(ai->ai_addr), config.backlog)) != -1) {
                // just use the first available host address
                break;
            }

            break;
        }
    }

	freeaddrinfo(ai_list);

    if (daemonize) {
        printf("daemonize mcr-server ...\n");
        /* change work directory to root, redirect stdio to /dev/null. */
        daemon(0, 0);
    }

    if (-1 == serve(server_sock)) {
        printf("server loop exited unexpected, error: %s.\n", strerror(errno));
    }

    close(server_sock);
    return -1;
}
