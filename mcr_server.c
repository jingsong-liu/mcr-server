
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
#include "include/mcr_define.h"
#include "include/mcr_server.h"


struct cli_args {
    int fd;
    char *wwwroot;
    /* resverd */
};


void *
cli_conn(void* arg)
{
    int sock = ((struct cli_args*)arg)->fd;
    char *wwwroot= ((struct cli_args*)arg)->wwwroot;
    size_t recved = 0;
    int parse_ret = MCR_OK;

    size_t recv_buflen = MCR_RBUFF_MAXSIZE;
    char recv_buf[recv_buflen];

    mcr_http *mhttp = mcr_make_http(wwwroot);
    if (mhttp == NULL) {
        goto out;
    }

    mhttp->attach(mhttp, sock, recv_buf, &recved);

    while(1)
    {
        recved = recv(sock, recv_buf, recv_buflen, 0);
        if (recved < 0) {
            printf("sock read error: %s\n",  strerror(errno)); 
            break;

        } else {
            /* do parse in the loop, or in event handler */
            parse_ret = mhttp->parse(mhttp);
            if(parse_ret == MCR_EAGAIN) {
                continue;
            }
            else {
                /* parse_ret == MCR_ERR || parse_ret == MCR_OK
                   parse ok/error ,exit
                 */
                break;
            }
        }
    }

    mcr_free_http(mhttp);

out:
    close(sock);
    pthread_exit(0);
}


int
server_init(int family, int type, struct sockaddr_in* server_addr, int backlog)
{
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
run_serverloop(int server_sock, struct server_config config)
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
        cargs.wwwroot = config.wwwroot;
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
get_addr4_by_host(const char* port, struct addrinfo** ai_list)
{
    struct addrinfo hint;
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;
    /* on any IP address, using port number */
    hint.ai_flags = AI_PASSIVE | AI_ADDRCONFIG |AI_NUMERICSERV;

    if (0 != getaddrinfo(NULL, port, &hint, ai_list))
    {
        printf("get host address error:%s", gai_strerror(errno));
        return -1;
    }
    else {
        return 0;
    }
}


int
open_serversock(const char *port, int backlog)
{
    int server_sock = -1;
    struct addrinfo * ai_list = NULL, * ai;

    if ( -1 == get_addr4_by_host(port, &ai_list)) {
        return -1;
    }

    for (ai = ai_list; ai != NULL; ai = ai_list->ai_next) {
        if ((server_sock = server_init(ai->ai_family, ai->ai_socktype, (struct sockaddr_in*)(ai->ai_addr), backlog)) != -1) {
            // just use the first available host address
            break;
        }
    }
	freeaddrinfo(ai_list);
    
    return server_sock;
}


void
close_serversock(int fd)
{
    close(fd);
}
