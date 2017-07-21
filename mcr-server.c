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
/* 
int socket(int domain, int type, int protocol);

int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

struct sockaddr {
    sa_family_t sa_family;
    char        sa_data[14];
}

int listen(int sockfd, int backlog);
*/

#define HOST_ADDRESS    "localhost"
#define LISTEN_PORT     8082


struct cli_args {
    int fd;
};

void *
sk_conn(void* arg) {
    char buf[1024];
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
sk_listen() {
    int sock;
    struct sockaddr_in addr;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sock) {
        printf("create socket error: %s\n", strerror(errno));
        goto err1;
    }

    addr.sin_family = AF_INET;
    inet_aton(HOST_ADDRESS, &(addr.sin_addr));
    addr.sin_port = htons(LISTEN_PORT);

    if (0 != bind(sock, (struct sockaddr *)&addr, sizeof(addr))) {
        printf("bind error: %s\n", strerror(errno));
        goto err2;
    }

    if (0 != listen(sock, 3)) {
        printf("listen error: %s\n", strerror(errno)); 
        goto err2;
    }
    printf("start listenning on port %d...\n", ntohs(addr.sin_port));
        return sock;

    err2:
        close(sock);
    err1:
        return -1;

}

int
main(void)
{
    int sock = 0;
    if ((sock = sk_listen()) == -1)
        exit(0);
         
    struct sockaddr_in client_addr;
    int client_addr_len= sizeof(client_addr);
    pthread_t pid;
    struct cli_args cargs;
    while(1) {
        int new_fd = accept(sock, (struct sockaddr*)&client_addr, &client_addr_len);
        if (new_fd < 0) {
            printf("accept error: %s\n", strerror(errno));
            goto err;
        }
        
        printf("accept a connect from %s:%d\n",inet_ntoa(client_addr.sin_addr),
            ntohs(client_addr.sin_port));

        cargs.fd = new_fd;
        if(0 != pthread_create(&pid, NULL, sk_conn, (void *)&cargs)) {
            printf("create pthread error: %s\n", strerror(errno));
            goto err;
        }
        //pthread_detach(pid);

    }

    err:
        close(sock);
        return -1;
}


