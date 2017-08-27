#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "include/mcr_http.h"
#include "include/mcr_define.h"

#define SERVER_NAME "mcr-server/0.90"
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
mcr_http_protocol_version(const char *version, char *buf);
char*
mcr_http_status(int status_code, char *buf);
char *
mcr_http_errno(int errnum, char *buf);
char *
mcr_http_newline(char *buf);
char *
mcr_http_servername (const char * servername, char *buf);
char *
mcr_http_content_lenth(size_t content_len, char *buf);
char *
mcr_http_content_type(const char *content_type, char *buf);
char *
mcr_http_body(const char *body, ssize_t body_len, char *buf);
size_t
mcr_make_http_response(int status_code, int errnum, const char *body, ssize_t content_len, const char *content_type,  const char *version, char *response);
http_context *
mcr_make_http_context(int *sock, const char *wwwroot, size_t buf_len);

http_context *
mcr_make_http_context(int *sock, const char *wwwroot, size_t buf_len)
{
    http_context *context = malloc(sizeof(http_context));
    if (context == NULL) {
        return NULL;
    }
    context->sock = sock;
    context->buffer = malloc(buf_len*sizeof(char));
    if (context->buffer == NULL) {
        free(context);
        return NULL;
    }
    context->buf_len = buf_len;
    context->wwwroot = wwwroot;
    return context;
}


void
mcr_free_http_context(http_context *context)
{
    if (context == NULL) {
        return ;
    }

    if (context->buffer != NULL ) {
        free(context->buffer);
    }

    free(context);
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
mcr_register_http(mcr_http *mhttp, int sock, const char *input, size_t *input_len)
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


/* return OK, ERR, CONTINUE */
int
mcr_http_parse(mcr_http *mhttp)
{
    size_t nparsed;

    nparsed = http_parser_execute(mhttp->parser, mhttp->hooks, mhttp->input, *(mhttp->input_len));

    if (mhttp->parser->upgrade) {
        /* websocket */

    } else if (nparsed != *(mhttp->input_len)) {
        printf("http parse error: %s(%s)\n", http_errno_name(mhttp->parser->http_errno),
            http_errno_description(mhttp->parser->http_errno));
        return MCR_ERR;

    } else {
        if (nparsed == 0)
            return MCR_OK;
        if (mhttp->context->complete_flag == 1)
            return MCR_OK;
        //printf("request method:%s \n", http_method_str(parser->method));
    }

    return MCR_EAGAIN;
}


mcr_http *
mcr_make_http(const char *wwwroot)
{
    mcr_http * mhttp = malloc(sizeof(mcr_http));
    if (mhttp == NULL) {
        return NULL;
    }

    http_context *context = mcr_make_http_context(&(mhttp->sock), wwwroot, 80*1024);
    context->complete_flag = 0;
    if (context == NULL) {
        mcr_free_http(mhttp);
        return NULL;
    }
    mhttp->context = context;

    http_parser* hp = mcr_make_http_parser();
    if (hp == NULL) {
        mcr_free_http(mhttp);
        mcr_free_http_context(context);
        return NULL;
    }
    hp->data = context;
    mhttp->parser = hp;

    http_parser_settings *hooks= mcr_make_http_hook();
    if (hooks == NULL) {
        mcr_free_http(mhttp);
        mcr_free_http_context(context);
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


int
mcr_url_check(const char* url, size_t len, const char ** filter)
{
    int i = 0;

    if (filter == NULL) {
        return 0;
    }

    /* is the url available for us? */
    for (i = 0; i < sizeof(filter); i ++) {
        if (!strncmp(*(filter + i), url, strlen(*(filter + i)))) {
            return 0;
        }
    }
    return -1;
}


int mcr_url_callback(http_parser* _, const char *at, size_t length)
{
    printf("url: %.*s\n", (int)length, at);
    http_context *context = _->data;
    /* url filter */
    if (-1 == mcr_url_check(at, length, NULL)) {
        /* stop parse right now. */
        return -1;
    }

    /* save url */
    snprintf(context->url, URL_MAX_LENGTH, "%.*s", (int)length, at);
    return 0;
}


int mcr_status_callback(http_parser *_, const char *at, size_t length)
{

    return 0;
}


int mcr_header_filed_callback(http_parser *_, const char *at, size_t length)
{
    return 0;
}


int mcr_header_value_callback(http_parser *_, const char *at, size_t length)
{

    return 0;
}


int mcr_headers_complete_callback(http_parser *_)
{
    return 0;
}


int mcr_body_callback(http_parser *_, const char *at, size_t length)
{
    return 0;
}


int mcr_message_complete_callback(http_parser *_)
{
    http_context *context = (http_context *)_->data;

    mcr_route(context);
    if (-1 == send(*context->sock, context->buffer, context->buf_len, 0)) {
        return -1;
    }

    context->complete_flag = 1;
    return 0;
}


int mcr_chunk_callback(http_parser *_)
{
    return 0;
}


int mcr_chunk_complete_callback(http_parser *_)
{
    return 0;
}


int
url_handler(const char* path)
{
    /* map the path to costume method */
    return -1;
}


char*
mcr_uri_of_wwwroot(const char *wwwroot, const char *url)
{

    /* translate url to file path in workdir */
    const char index_html[] = "/index.html";
    int extl = strlen(url) > strlen(index_html) ? strlen(url) : strlen(index_html);
    int uri_len = strlen(wwwroot) + extl + 1;
    char *uri= malloc(uri_len*sizeof(char));
    if (uri == NULL)
        return NULL;
    memcpy(uri, wwwroot, strlen(wwwroot));
    if (!strcmp(url, "/")) {
        memcpy(uri + strlen(wwwroot), index_html, strlen(index_html) + 1);
    }
    else {
        memcpy(uri + strlen(wwwroot), url, strlen(url) + 1);
    }
    return uri;
}


/*TODO  easy implment of get mimetype from file name. */
int
mcr_get_mimetype(const char *filename, char *mimetype)
{
    const char *support_mimes[] = {"html", "css", "js", "jpg", "jpeg", "png", };
    const char *mimeT[] = {"text/", "image/",};

    int i = 0;
    char * ext = strrchr(filename, '.');
    ext ++;
    for (i = 0; i < sizeof(support_mimes)/sizeof(char*); i++) {
        if (!strcmp(ext, support_mimes[i])) {
            if (i < 4) {
                strcpy(mimetype, mimeT[0]);
            }
            else {
                strcpy(mimetype, mimeT[1]);
            }
            strcat(mimetype, support_mimes[i]);
            return 0;
        }
    }

    return -1;
}


int
mcr_route(http_context *context)
{
    /* costume handler */
    if (url_handler(context->url) == 0) {
        return  0;
    }

    /* static file */
    char *body = NULL;
    char *sfile = mcr_uri_of_wwwroot(context->wwwroot, context->url);
    int fd = open(sfile, O_RDONLY);
    if (fd < 0) {
        close(fd);
        free(sfile);
        goto not_found;
    }

    body = malloc(64*1024*sizeof(char));
    if (body == NULL) {
        close(fd);
        free(sfile);
        close(fd);
        return -1;
    }
    context->content_len = read(fd, body, 64*1024);

    if (context->content_len > 0) {
        char content_type[128];
        mcr_get_mimetype(sfile, content_type);
        context->buf_len = mcr_make_http_response(200, 0, body, context->content_len, content_type, NULL, context->buffer);
        goto ok;
    } else {
        close(fd);
        free(sfile);
        free(body);
        goto not_found;
    }

ok:
    close(fd);
    free(sfile);
    free(body);
    return 0;

not_found:
    context->buf_len = mcr_make_http_response(404, 0, NULL, 0, 0, NULL, context->buffer);
    return -1;
}


char *
mcr_http_protocol(char *buf)
{
    char *HTTP = "HTTP/";
    strcpy(buf, HTTP);
    return buf;
}


char *
mcr_http_protocol_version(const char *version, char *buf)
{
    strcat(buf, version);
    strcat(buf, " ");

    return buf;
    
}


char*
mcr_http_status(int status_code, char *buf)
{
    char numstr[8];
    snprintf(numstr, sizeof(numstr), "%d ", status_code);;
    strcat(buf, numstr);
    return buf;

}
    
    
char *
mcr_http_errno(int errnum, char *buf)
{
    char numstr[8]; 
    snprintf(numstr, sizeof(numstr), "%d ", errnum);;
    strcat(buf, numstr);
    return buf;
}


char *
mcr_http_newline(char *buf)
{
    strcat(buf, "\r\n");
    return buf;
}


char *
mcr_http_servername (const char * servername, char *buf)
{
    char *serverfield = "Server: ";
    strcat(buf, serverfield);
    strcat(buf, servername);
    return buf;
}


char *
mcr_http_content_lenth(size_t content_len, char *buf)
{
    char numstr[8];
    snprintf(numstr, sizeof(numstr), "%d ", (int)content_len);;
    strcat(buf, "Content-Length: ");
    strcat(buf, numstr);
    return buf;
}


char *
mcr_http_content_type(const char *content_type, char *buf)
{
    strcat(buf, "Content-Type: ");
    strcat(buf, content_type);
    return buf;
}


/*TODO: body content maybe raw data, take care of it, */
char *
mcr_http_body(const char *body, ssize_t body_len, char *buf)
{
    int offset = strlen(buf);
    if (body && (body_len > 0))
        memcpy(buf + offset, body, body_len);
    return buf;
}


char *
mcr_http_accept_ranges(const char *ranges, char *buf)
{
    strcat(buf, "Accept-Ranges: ");
    strcat(buf, "bytes ");
    if (ranges)
        strcat(buf, ranges);
    return buf;
}


char *
mcr_http_date(const char *date, char *buf)
{
    strcat(buf, "Date: ");
    if (date) {
        strcat(buf, date);
    }
    else {
        char tbuf[128];
        time_t now = time(0);
        struct tm tm = *gmtime(&now);
        strftime(tbuf, sizeof(tbuf), "%a, %d %b %Y %H:%M:%S %Z", &tm);
        strcat(buf, tbuf);
    }
    return buf;
}


size_t
mcr_make_http_response(int status_code, int errnum, const char *body, ssize_t content_len, const char *content_type,  const char *protocol, char *response)
{
    size_t response_len = 0;

    /* status line */
    mcr_http_protocol(response);
    if (protocol == NULL ) {
        mcr_http_protocol_version(DEFAULT_HTTP_VERSION, response);
    }
    else {
        mcr_http_protocol_version(protocol, response);
    }

    mcr_http_status(status_code, response);
    mcr_http_errno(errnum, response);
    mcr_http_newline(response);

    /* server line */
    mcr_http_servername(SERVER_NAME, response);
    mcr_http_newline(response);
    
    /* accept-ranges */
    mcr_http_accept_ranges(NULL, response);
    mcr_http_newline(response);

    /* content-len */
    mcr_http_content_lenth(content_len, response);
    mcr_http_newline(response);

    /* content-type */
    if (content_type == NULL) {
       mcr_http_content_type("text/html;charset=utf-8", response);   //this should be read from html file or given by caller.
    }
    else {
       mcr_http_content_type(content_type, response);
    }
    mcr_http_newline(response);

    /* Date */
    mcr_http_date(NULL, response);
    mcr_http_newline(response);

    /* blank line */
    mcr_http_newline(response);

    response_len = strlen(response);

    /* content-body */
    mcr_http_body(body, content_len, response);

    response_len += content_len;
    return response_len;
}
