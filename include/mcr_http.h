#ifndef _INCLUDE/MCR_HTTP_H
#define _INCLUDE/MCR_HTTP_H
#include "http_parser.h"


typedef struct http_data_s {
    int *sock;
    void *buffer;
    int buf_len;
} http_data;

typedef struct mcr_http_s {
    int sock;    
    char *input;
    int *input_len;
    http_parser *parser;
    http_parser_settings *hooks;
    http_data *context;

    void (*attach)(int sock, char *input, int *input_len);
    void (*unattach)(void);
    int (*parse)(void); // do the parse
}mcr_http;

char *mcr_make_http_reponse(int status_code, int errnum, const char *msg, const char *content_type,  const char *version, char *buf);

mcr_http *mcr_make_http();
void mcr_free_http();
#endif
