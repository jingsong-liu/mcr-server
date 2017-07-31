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
#include <netdb.h>
#include "include/mcr_server_config.h"



#define DEFAULT_backlog           128
#define RECEIVE_BUFFER_SIZE     4096
#define SERVER_CONFIG_FILE      "./default.conf"

struct cli_args {
    int fd;
    int reserved;
};


void *
cli_conn(void* arg) {
    char buf[RECEIVE_BUFFER_SIZE];
    int fd = ((struct cli_args*)arg)->fd;
    struct sockaddr_in cli_addr ;
    int cliaddr_len = sizeof(cli_addr);
    if (-1 == getpeername(fd, (struct sockaddr*)&cli_addr, (socklen_t*)&cliaddr_len)) {
        printf("get peer name error: %s\n", strerror(errno));
    }

    printf("create thread for client from %s\n", inet_ntoa(cli_addr.sin_addr));
    while(1)
    {
        ssize_t _s = read(fd, buf, sizeof(buf));
        if (_s > 0) {
            buf[_s-1] = '\0';
        } else if (_s == 0) {
            printf("client closed\n");
            break;    
        } else {
            printf("read error: %s\n",  strerror(errno));
            break;
        }
        time_t now = time(NULL);
        printf("%s [%s:%d]: %s\n\n", asctime(localtime(&now)), inet_ntoa(cli_addr.sin_addr),
         ntohs(cli_addr.sin_port), buf);

        if (-1 == write(fd, buf, strlen(buf))) {
            printf("write back error: %s\n", strerror(errno));
            break;
        }
    }
    close(fd);
    pthread_exit(0);
}


int
server_init(int type, struct sockaddr_in* server_addr, int backlog) {
    int sock;
    sock = socket(AF_INET, type, 0);
    if (-1 == sock) {
        printf("create socket error: %s\n", strerror(errno));
        goto err1;
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
        //pthread_detach(pid);
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


int
main(void)
{

    struct server_config config;
    if ( -1 == read_server_config(SERVER_CONFIG_FILE, &config)) {
        printf("error read_server_config :%s\n", strerror(errno));
    }

    struct addrinfo * ai_list = NULL, * ai;
    select_addr_info(config.hostname, config.service, &ai_list);

    /* for test */
    ((struct sockaddr_in*)(ai_list->ai_addr))->sin_port = 8082;
    config.backlog = 5;

    int server_sock = -1;
    for (ai = ai_list; ai != NULL; ai = ai_list->ai_next) {
        if ((server_sock = server_init(SOCK_STREAM, (struct sockaddr_in*)(ai->ai_addr), config.backlog)) != -1) {
            // just use first host address
            break;
        }
    }


    if (-1 == server_sock) {
        printf("host address config seems unavailable, we try to use localhost\n");
        select_addr_info("localhost", "http", &ai_list);
        for (ai = ai_list; ai != NULL; ai = ai_list->ai_next) {
            if ((server_sock = server_init(SOCK_STREAM, (struct sockaddr_in*)(ai->ai_addr), config.backlog)) != -1) {
                // just use first host address
                break;
            }

            break;
        }
    }

	freeaddrinfo(ai_list);

    if (-1 == serve(server_sock)) {
        printf("server loop exited unexpected, error: %s.", strerror(errno));
    }

    close(server_sock);
    return -1;
}
