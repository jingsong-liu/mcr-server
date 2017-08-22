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
char *
mcr_http_protocal(char *buf);
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
mcr_make_http_reponse(int status_code, int errnum, const char *msg, const char *content_type,  const char *version, char *buf);


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
    http_parser_settings *settings = malloc(sizeof(http_parser));
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

    } else {
        if (nparsed == 0) ;
        //printf("request method:%s \n", http_method_str(parser->method));
        /* response */

    }

    return 0;
}


mcr_http *
mcr_make_http()
{
    mcr_http * mhttp = malloc(sizeof(mcr_http));
    if (mhttp == NULL) {
        return NULL;
    }

    http_context *pcontext = mcr_make_http_context(&(mhttp->sock), 1024);
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
    printf("mcr_message_begin_callback\n");
    return 0;
}


char *
mcr_make_http_reponse(int status_code, int errnum, const char *msg, const char *content_type,  const char *version, char *buf);



int mcr_url_callback(http_parser* _, const char *at, size_t length) {
    printf("Url: %.*s\n", (int)length, at);

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
    printf("mcr_body_callback: %.*s\n", (int)length, at);

    return 0;
}


int mcr_message_complete_callback(http_parser *_) {
    http_context *pcontext = (http_context *)_->data;
    char msg_buffer[2048];
    int msg_len = 0;

    int fd = open("index.html", O_RDONLY);
    if (fd > 0 )  {
        msg_len = read(fd, msg_buffer, 2048);
    }

    if (msg_len > 0) {
        mcr_make_http_reponse(200, 0, msg_buffer, NULL, NULL, pcontext->buffer);
        pcontext->buf_len = strlen(pcontext->buffer) + 1;
        send(*pcontext->sock, pcontext->buffer, pcontext->buf_len, 0);
    }

    close(fd);

    return 0;
}

int mcr_chunk_callback(http_parser *_) {
    printf("mcr_chunk_callback\n");
    return 0;
}


int mcr_chunk_complete_callback(http_parser *_) {
    printf("mcr_chunk_complete_callback\n");
    return 0;
}


char *
mcr_http_protocal(char *buf) {
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
mcr_make_http_reponse(int status_code, int errnum, const char *msg, const char *content_type,  const char *version, char *buf)
{
    /* statu line */
    mcr_http_protocal(buf);
    if (version == NULL ) {
        mcr_http_version(DEFAULT_HTTP_VERSION, buf);
    }
    else {
        mcr_http_version(version, buf);
    }
    mcr_http_status(status_code, buf);
    mcr_http_errno(errnum, buf);
    mcr_http_newline(buf);

    /* server line */
    mcr_http_servername(SERVER_NAME, buf);
    mcr_http_newline(buf);
    
    /* content-len */
    mcr_http_content_lenth(2048, buf);
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
