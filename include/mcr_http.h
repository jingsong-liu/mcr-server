#ifndef _MCR_HTTP_H
#define _MCR_HTTP_H
#include "../http-parser/http_parser.h"


#define URL_MAX_LENGTH  1024
typedef struct http_context_s {
    int *sock;
    void *buffer;
    ssize_t buf_len;

    char url[URL_MAX_LENGTH];
    const char *wwwroot;
    size_t content_len;
    int complete_flag;
}http_context;

typedef struct mcr_http_s mcr_http;

struct mcr_http_s {
    int sock;    
    const char *input;
    size_t *input_len;
    http_parser *parser;
    http_parser_settings *hooks;
    http_context *context;

    void (*attach)(mcr_http *mhttp, int sock, const char *input, size_t *input_len);
    void (*unattach)(mcr_http *mhttp);
    int (*parse)(mcr_http *mhttp); // do the parse
};


mcr_http *mcr_make_http();
void mcr_free_http(mcr_http *mhttp);
#endif
