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



#define HOST_ADDRESS    "localhost"
#define LISTEN_PORT     8082
#define BACK_LOG        128
#define RECEIVE_BUFFER_SIZE     4096

struct cli_args {
    int fd;
};

void *
sk_conn(void* arg) {
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
server_init() {
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

    if (0 != listen(sock, BACK_LOG)) {
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
serve(int server_sock)
{
    struct sockaddr_in client_addr;
    int client_addr_len= sizeof(client_addr);
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
        if(0 != pthread_create(&pid, NULL, sk_conn, (void *)&cargs)) {
            printf("create pthread error: %s\n", strerror(errno));
            return -1;
        }
        //pthread_detach(pid);
    }

    //cloase child thread?

    return 0;
}

int
main(void)
{
    struct addrinfo hint;
    memset(&hint, 0, sizeof(hint));
    hint.ai_flags = AI_ALL;
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;

    struct addrinfo * ai_list, *aip;
    struct server_config config;
    if ( -1 == read_server_config(SERVER_CONFIG_FILE, &config)) {

    }

    if (-1 == getaddrinfo(config.hostname, config.service, &hint, &ai_list))
    {
        printf("get host address error:%s", gai_strerror(errno));
    } else {
        printf("available host address info:\n");
        struct sockaddr_in * addr_in;
        for (aip = ai_list; aip != NULL; aip = ai_list->ai_next)
        {
            addr_in = (struct sockaddr_in*)(aip->ai_addr);
            printf("%s, %s, %d \n", aip->ai_canonname, inet_ntoa(addr_in->sin_addr), ntohs(addr_in->sin_port));
        }
    }

    int server_sock = 0;
    if ((sock = server_init()) == -1)
        exit(0);
    if (-1 == serve(server_sock)) {
        printf("server loop exited unexpected.");
    }

    close(server_sock);
    return -1;
}

#define HOST_NAME_MAX           256
#define SERVICE_NAME_MAX        128
struct server_config {
    char hostname[HOST_NAME_MAX];
    char service[SERVICE_NAME_MAX];
    int port;

};

int
read_server_config(const char* path, struct server_config* sc)
{
    int fd;
    if ((fd = open(path, O_RDONLY)) == -1) {
        printf("open config error: %s\n", strerror(errno));
        return -1;
    }

    char buf[1024];
    ssize_t len = read(fd, buf, sizeof(buf));
    if (-1 == len) {
        printf("read config error: %s\n", strerror(errno));
        goto err;
    } else if (0 == len) {
        goto err;
    } else {
        //parse host and service from buf
        get_dict(buf, len, "HostName:", sc->hostname);
        get_dict(buf, len, "Service:", sc->service);
    }
    close(fd);
    return 0;

    err:
        close(fd);
        return -1;
}

int
get_dict(char* buf, ssize_t len, const char* key, char* value)
{
    char *p;
    buf[len-1] = '\0';
    p = strstr(buf, key) + strlen(key) + 1;
    
    strncpy(value, p, strchr(p, ' ') - p);

    return 0;
}
