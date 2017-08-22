#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "include/mcr_http.h"


#define SERVER_NAME "mcr-server"
#define DEFAULT_HTTP_VERSION "1.1"
int mcr_message_begin_callback(http_parser *_);
int mcr_url_callback(http_parser* _, const char *at, size_t length);
int mcr_status_callback(http_parser *_, const char *at, size_t length);
int mcr_header_filed_callback(http_parser *_, const char *at, size_t length);
int mcr_header_value_callback(http_parser *_, const char *at, size_t length);
int mcr_headers_complete_callback(http_parser *_);
int mcr_body_callback(http_parser *_, const char *at, size_t length);;;;
int mcr_message_complete_callback(http_parser *_);
int mcr_chunk_callback(http_parser *_);
int mcr_chunk_complete_callback(http_parser *_);
int
mcr_route(http_context *context);
char *
mcr_http_protocol(char *buf);
char *
mcr_http_version(const char *version, char *buf);
char*
mcr_http_status(int status_code, char *buf);
char *
mcr_http_errno(int errnum, char *buf);
char *
mcr_http_newline(char *buf);
char *
mcr_http_servername (const char * servername, char *buf);
char *
mcr_http_content_lenth(int content_len, char *buf);
char *
mcr_http_content_type(const char *content_type, char *buf);
char *
mcr_http_body(const char *msg, char *buf); 
char *
mcr_make_http_response(int status_code, int errnum, const char *body, const char *content_type,  const char *version, char *response);


http_context *
mcr_make_http_context(int *sock, size_t buf_len)
{
    http_context *pcontext = malloc(sizeof(http_context));
    if (pcontext == NULL) {
        return NULL;
    }

    pcontext->sock = sock;
    pcontext->buffer = malloc(buf_len*sizeof(char));
    if (pcontext->buffer == NULL) {
        free(pcontext);
        return NULL;
    }

    pcontext->buf_len = buf_len;
    return pcontext;
}


void
mcr_free_http_context(http_context *pcontext)
{
    if (pcontext == NULL) {
        return ;
    }

    if (pcontext->buffer != NULL ) {
        free(pcontext->buffer);
    }

    free(pcontext);
}


http_parser *
mcr_make_http_parser()
{


    http_parser *parser = malloc(sizeof(http_parser));
    if (parser == NULL)
        return NULL;

    http_parser_init(parser, HTTP_BOTH);

    return parser;

}


void
mcr_free_http_parser(http_parser *hp)
{
    if (hp == NULL)
        return;

    free(hp);
}



http_parser_settings*
mcr_make_http_hook()
{
    http_parser_settings *settings = malloc(sizeof(http_parser_settings));
    if (settings == NULL) {
        return NULL;
    }

    settings->on_message_begin = mcr_message_begin_callback;
    settings->on_url = mcr_url_callback;
    settings->on_status = mcr_status_callback;
    settings->on_header_field = mcr_header_filed_callback;
    settings->on_header_value = mcr_header_value_callback;
    settings->on_headers_complete = mcr_headers_complete_callback;
    settings->on_body = mcr_body_callback;
    settings->on_message_complete = mcr_message_complete_callback;
    settings->on_chunk_header = mcr_chunk_callback;
    settings->on_chunk_complete = mcr_chunk_complete_callback;

    return settings;
}
    

void
mcr_free_http_hook(http_parser_settings *settings)
{
    if (settings != NULL) {
        free(settings);
    }
}


/*
 * make http object and register it to recv loop or event handler
 */

void
mcr_register_http(mcr_http *mhttp, int sock, char *input, size_t *input_len)
{
    mhttp->sock = sock;
    mhttp->input = input;
    mhttp->input_len = input_len;
}


void
mcr_unregister_http(mcr_http *mhttp)
{
    mhttp->sock = -1;
    mhttp->input = NULL;
    mhttp->input_len = 0;
}


int
mcr_http_parse(mcr_http *mhttp)
{
    size_t nparsed;

    nparsed = http_parser_execute(mhttp->parser, mhttp->hooks, mhttp->input, *(mhttp->input_len));

    if (mhttp->parser->upgrade) {
        /* websocket */

    } else if (nparsed != *(mhttp->input_len)) {
        printf("http parsed failed\n");
        return -1;

    } else {
        if (nparsed == 0) return 0;
        //printf("request method:%s \n", http_method_str(parser->method));
    }

    return -1;
}


mcr_http *
mcr_make_http()
{
    mcr_http * mhttp = malloc(sizeof(mcr_http));
    if (mhttp == NULL) {
        return NULL;
    }

    http_context *pcontext = mcr_make_http_context(&(mhttp->sock), 1024);
    pcontext->complete_flag = 0;
    if (pcontext == NULL) {
        mcr_free_http(mhttp);
        return NULL;
    }
    mhttp->context = pcontext;

    http_parser* hp = mcr_make_http_parser();
    if (hp == NULL) {
        mcr_free_http(mhttp);
        mcr_free_http_context(pcontext);
        return NULL;
    }
    hp->data = pcontext;
    mhttp->parser = hp;

    http_parser_settings *hooks= mcr_make_http_hook();
    if (hooks == NULL) {
        mcr_free_http(mhttp);
        mcr_free_http_context(pcontext);
        mcr_free_http_parser(hp);
        return NULL;
    }
    mhttp->hooks = hooks;
    
    mhttp->attach = mcr_register_http;
    mhttp->unattach = mcr_unregister_http;
    mhttp->parse = mcr_http_parse;
    return mhttp;
}


void
mcr_free_http(mcr_http *mhttp)
{
    if (mhttp == NULL)
        return;

    if (mhttp->context)
        mcr_free_http_context(mhttp->context);
    if (mhttp->parser)
        mcr_free_http_parser(mhttp->parser);
    if(mhttp->hooks)
        mcr_free_http_hook(mhttp->hooks);
    
    free(mhttp);
}

int mcr_message_begin_callback(http_parser *_)
{
    return 0;
}


const char *local_url_table[] = {"/index.html", "test"};

int
mcr_url_check(const char* url, size_t len) 
{
    int i = 0;
    /* is the url available for us? */
    for (i = 0; i < sizeof(local_url_table); i ++) {
        if (!strncmp(*(local_url_table + i), url, strlen(*(local_url_table + i)))) {
            return 0;
        }
    }
    return -1;
}


int mcr_url_callback(http_parser* _, const char *at, size_t length) {
    printf("url: %.*s\n", (int)length, at);
    http_context *context = _->data;
    /* url filter */
    if (-1 == mcr_url_check(at, length)) {
        /* stop parse right now. */
        return -1;
    }

    /* save url */
    snprintf(context->url, URL_MAX_LENGTH, "%.*s", (int)length, at);
    return 0;
}


int mcr_status_callback(http_parser *_, const char *at, size_t length) {

    return 0;
}


int mcr_header_filed_callback(http_parser *_, const char *at, size_t length) {
    return 0;
}


int mcr_header_value_callback(http_parser *_, const char *at, size_t length) {

    return 0;
}


int mcr_headers_complete_callback(http_parser *_) {
    return 0;
}


int mcr_body_callback(http_parser *_, const char *at, size_t length) {
    return 0;
}


int mcr_message_complete_callback(http_parser *_) {
    http_context *context = (http_context *)_->data;

    if (0 == mcr_route(context)) {
        send(*context->sock, context->buffer, context->buf_len, 0);
        context->complete_flag = 1;
    }

    return 0;
}


int mcr_chunk_callback(http_parser *_) {
    return 0;
}


int mcr_chunk_complete_callback(http_parser *_) {
    return 0;
}


int
url_handler(const char* path) {
    /* map the path to costume method */
    return -1;
}


/* TODO: implment translate url to workdir resource. */
char*
mcr_uri_of_workdir(const char *url, const char *workdir) {

    /* translate url to file path in workdir */
    char *filep = (char*)url + 1;   // just right move.
    return filep;
}


int
mcr_route(http_context *context)
{
    /* costume handler */
    if (url_handler(context->url) == 0) {
        return  0;
    }

    /* static file */
    char body[2048];
    char *sfile = mcr_uri_of_workdir(context->url, ".");
    int fd = open(sfile, O_RDONLY);
    if (fd < 0)  {
        return -1;
    }

    context->content_len = read(fd, body, 2048);

    if (context->content_len > 0) {
        mcr_make_http_response(200, 0, body, NULL, NULL, context->buffer);
        context->buf_len = strlen(context->buffer) + 1;
        goto ok;
    } else {
        goto err;
    }

ok:
    close(fd);
    return 0;
err:
    close(fd);
    return -1;
}


char *
mcr_http_protocol(char *buf) {
    char *HTTP = "HTTP/";
    strcpy(buf, HTTP);
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
    snprintf(numstr, sizeof(numstr), "%d ", status_code);;
    strcat(buf, numstr);
    return buf;

}
    
    
char *
mcr_http_errno(int errnum, char *buf) {
    char numstr[8]; 
    snprintf(numstr, sizeof(numstr), "%d ", errnum);;
    strcat(buf, numstr);
    return buf;
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
mcr_http_content_lenth(int content_len, char *buf) {
    char numstr[8];
    snprintf(numstr, sizeof(numstr), "%d ", content_len);;
    strcat(buf, "Content-Length: ");
    strcat(buf, numstr);
    return buf;
}


char *
mcr_http_content_type(const char *content_type, char *buf) {
    strcat(buf, "Content-Type: ");
    strcat(buf, content_type);
    return buf;
}

char *
mcr_http_body(const char *msg, char *buf) {
        strcat(buf, msg);
    return buf;
}


#define SERVER_NAME "mcr-server"
#define DEFAULT_HTTP_VERSION "1.1"


char *
mcr_make_http_response(int status_code, int errnum, const char *body, const char *content_type,  const char *version, char *response)
{
    /* status line */
    mcr_http_protocol(response);
    if (version == NULL ) {
        mcr_http_version(DEFAULT_HTTP_VERSION, response);
    }
    else {
        mcr_http_version(version, response);
    }
    mcr_http_status(status_code, response);
    mcr_http_errno(errnum, response);
    mcr_http_newline(response);

    /* server line */
    mcr_http_servername(SERVER_NAME, response);
    mcr_http_newline(response);
    
    /* content-len */
    mcr_http_content_lenth(2048, response);
    mcr_http_newline(response);

    /* content-type */
    if (content_type == NULL) {
       mcr_http_content_type("text/html;charset=utf-8", response);   //this should be read from file
    }
    else {
       mcr_http_content_type(content_type, response);
    }

    mcr_http_newline(response);
    mcr_http_newline(response);

    /* content-body */
    mcr_http_body(body, response);

    return response;
}
