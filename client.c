#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
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

#define DEFAULT_SERVER_IP "120.77.173.228"
#define DEFAULT_SERVER_PORT 8082
#define INPUT_BUF_SIZE  1024


int sk_connect(const char* host, int port, int type, int timeout)
{
    int sock;
    struct sockaddr_in addr;
    sock = socket(AF_INET, type, 0);
    if (-1 == sock) {
        printf("Fail to create socket\n");
        goto err1;
    }

    addr.sin_family = AF_INET;
    inet_aton(host, &(addr.sin_addr));
    addr.sin_port = htons(port);
    
    while (timeout) {
        if (0 != connect(sock, (struct sockaddr*)&addr, sizeof(addr))) {
            printf("connect error: %s\n", strerror(errno));
        } else {
            break;
        }

        timeout --;
        if (timeout == 0){
            printf("connect timeout");
            goto err2;
        }

        sleep(2);
    }

    printf("successfully connected to host:%s:%d\n",inet_ntoa(addr.sin_addr),
        ntohs(addr.sin_port));

    return sock;

    err2:
        close(sock);
    err1:
        return -1;
}

int
main(int argc, char** argv)
{
    char server[32] = DEFAULT_SERVER_IP;
    int port = DEFAULT_SERVER_PORT;
    int cli_sock = 0;
    
    if(argc == 2)
        memcpy(server, argv[1], strlen(argv[1]));
    if (argc == 3)
        port = atoi(argv[2]);

    cli_sock = sk_connect(server, port, SOCK_STREAM, 3);
    if (cli_sock == -1) {
        exit(-1);
    }

    char buf[INPUT_BUF_SIZE];
    while(1) {
        char* l = fgets(buf, INPUT_BUF_SIZE, stdin);
        if (l == NULL) {
            printf("fgets error.");
            continue;
        } else if (strlen(buf) == 0 ||buf[0] == 0x0A) {
            printf("cannot send empty message, please input again:\n");
            continue;
        }

        int len = send(cli_sock, buf, strlen(buf), 0);
        if ( -1 == len) {
            printf("send error: %s\n", strerror(errno));
            goto err2;
        }
        else {
            printf("%d bytes have sent\n", len);
        }
    }

    err2:
        close(cli_sock);
        printf("close socket, exit");
    err1:
        return -1;
}



