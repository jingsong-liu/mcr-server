#ifndef _MCR_HTTP_H
#define _MCR_HTTP_H
#include "../http-parser/http_parser.h"


typedef struct http_context_s {
    int *sock;
    void *buffer;
    int buf_len;
}http_context;

typedef struct mcr_http_s mcr_http;

struct mcr_http_s {
    int sock;    
    char *input;
    size_t *input_len;
    http_parser *parser;
    http_parser_settings *hooks;
    http_context *context;

    void (*attach)(mcr_http *mhttp, int sock, char *input, size_t *input_len);
    void (*unattach)(mcr_http *mhttp);
    int (*parse)(mcr_http *mhttp); // do the parse
};


mcr_http *mcr_make_http();
void mcr_free_http(mcr_http *mhttp);
#endif
