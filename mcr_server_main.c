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
#include "include/http_parser.h"


#define SERVER_CONFIG_FILE      "./default.conf"

struct cli_args {
    int fd;
    int reserved;
};



typedef struct parser_data {
    int sock;
    void *buffer;
    int buf_len;
} parser_data;


int mcr_message_begin_callback(http_parser *_) {
    printf("mcr_message_begin_callback\n");
    return 0;
}


int mcr_url_callback(http_parser* _, const char *at, size_t length) {
    printf("Url: %.*s\n", (int)length, at);


    return 0;
}


int mcr_status_callback(http_parser *_, const char *at, size_t length) {

    return 0;
}


int mcr_header_filed_callback(http_parser *_, const char *at, size_t length) {
    printf("mcr_header_filed_callback: %.*s\n", (int)length, at);
    return 0;
}


int mcr_header_value_callback(http_parser *_, const char *at, size_t length) {
    printf("mcr_header_value_callback: %.*s\n", (int)length, at);

    return 0;
}


int mcr_headers_complete_callback(http_parser *_) {

    return 0;
}


int mcr_body_callback(http_parser *_, const char *at, size_t length) {
    printf("mcr_body_callback: %.*s\n", (int)length, at);

    return 0;
}


int mcr_message_complete_callback(http_parser *_) {
    printf("mcr_message_complete_callback\n");

    return 0;
}

int mcr_chunk_callback(http_parser *_) {

    return 0;
}


int mcr_chunk_complete_callback(http_parser *_) {
    return 0;
}


char *
mcr_http_protocal(char *buf) {
    char *HTTP = "HTTP/";
    strcat(buf, HTTP);
    return buf;
}


char *
mcr_http_version(const char *version, char *buf) {
    strcat(buf, version);
    strcat(buf, " ");

    return buf;
    
}


char*
mcr_http_status(int status_code, char *buf) {
    char numstr[8];
    memset(numstr, 0, sizeof(numstr));
    itoa(status_code, numstr, 10);
    strcat(buf, numstr);
    strcat(buf, " ");
    return buf;

}
    
    
char *
mcr_http_errno(int errno, char *buf) {
    char numstr[8]; 
    itoa(errno, numstr, 10);
    strcat(buf, numstr);
    strcat(buf, " ");
}


char *
mcr_http_newline(char *buf) {
    strcat(buf, "\r\n");
    return buf;
}


char *
mcr_http_servername (const char * servername, char *buf) {
    char *serverfield = "Server: ";
    strcat(buf, serverfield);
    strcat(buf, servername);
    return buf;
}


char *
mcr_http_content_len(int conten_len, char *buf) {
    char numstr[8];
    memset(numstr, 0, sizeof(numstr));
    itoa(conten_len, numstr, 10);
    strcat(buf, numstr);
    strcat(buf, " ");
    return buf;
}


char *
mcr_http_content_type(const char *content_type, char *buf) {
    strcat(buf, "Content-type: ");
    strcat(buf, content_type);
    return buf;
}


char *mcr_http_body(const char *msg, char *buf) {
        strcat(buf, msg);
    return buf;
}


#define SERVER_NAME "mcr-server"
#define DEFAULT_HTTP_VERSION "1.1"


char *
mcr_make_http_reponse(int status_code, int errno, const char *msg, const char *content_type,  const char *version, char *buf) {
    /* protocal line */
    mcr_http_protocal(buf);
    if (version == NULL ) {
        mcr_http_version(DEFAULT_HTTP_VERSION, buf);
    }
    else {
        mcr_http_version(version, buf);
    }
    mcr_http_status(status_code, buf);
    mcr_http_errno(errno, buf);
    mcr_http_newline(buf);

    /* server line */
    mcr_http_servername(SERVER_NAME, buf);
    mcr_http_newline(buf);
    
    /* content-len */
    mcr_http_content_len(2048, buf);
    mcr_http_newline(buf);

    /* content-type */
    if (content_type == NULL) {
       mcr_http_content_type("text/html;charset=utf-8", buf);   //this should be read from file
    }
    else {
       mcr_http_content_type(content_type, buf);
    }

    mcr_http_newline(buf);
    mcr_http_newline(buf);

    /* content-body */
    mcr_http_body(msg, buf);

    return buf;

}
    

void *
cli_conn(void* arg) {
    int sock = ((struct cli_args*)arg)->fd;

    parser_data *pd = malloc(sizeof(parser_data));
    if (pd == NULL) {
        goto err1;
    }
    pd->sock = sock;

    http_parser_settings settings;
    settings.on_message_begin = mcr_message_begin_callback;
    settings.on_url = mcr_url_callback;
    settings.on_status = mcr_status_callback;
    settings.on_header_field = mcr_header_filed_callback;
    settings.on_headers_complete = mcr_headers_complete_callback;
    settings.on_body = mcr_body_callback;
    settings.on_message_complete = mcr_message_complete_callback;
    settings.on_chunk_header = mcr_chunk_callback;
    settings.on_chunk_complete = mcr_chunk_complete_callback;
    

    http_parser *parser = malloc(sizeof(http_parser));
    if (parser == NULL) {
        printf("creat http parser error:%s\n", strerror(errno));
        goto err2;
    }
    http_parser_init(parser, HTTP_BOTH);
    parser->data = pd;

    size_t http_len = 80*1024, nparsed;
    char *http_buf = malloc(http_len*sizeof(char));
    if (http_buf == NULL) {
        goto err3;
    }
    ssize_t recved;
    while(1)
    {
        recved = recv(sock, http_buf, sizeof(http_buf), 0);
        if (recved < 0) {
            printf("read error: %s\n",  strerror(errno)); break;

        } else if (recved == 0) {
            /* peer closed */
            printf("client closed\n");
            break;    

        } else {

            nparsed = http_parser_execute(parser, &settings, http_buf, recved);

            if (parser->upgrade) {
                /* websocket */

            } else if (nparsed != recved) {
                printf("http parsed failed\n");
                break;

            } else {
                printf("request method:%s \n", http_method_str(parser->method));
                /* response */

               char buf[2048];
                mcr_make_http_reponse(200, 0, "hello, Mr.liu!", NULL, NULL, buf);
            }

        }
    }


   free(http_buf);
err3:
   free(parser);
err2:
    free(pd);
err1:
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


int
main(void)
{

    struct server_config config;
    if ( -1 == read_server_config(SERVER_CONFIG_FILE, &config)) {
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

    if (-1 == serve(server_sock)) {
        printf("server loop exited unexpected, error: %s.\n", strerror(errno));
    }

    close(server_sock);
    return -1;
}
