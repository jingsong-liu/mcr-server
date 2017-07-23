#include "include/server_config.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#define SERVER_OPT_BASE         0x80000000
#define OPT_HOSTNAME            SERVER_OPT_BASE + 1
#define OPT_SERVICE             SERVER_OPT_BASE + 2
#define OPT_QUEUESIZE           SERVER_OPT_BASE + 3


struct config_opt {
    char opt[64];
    int index;
    int size;
};

static const struct config_opt server_opt_array[] = {
    {"HostName",       OPT_HOSTNAME,   HOST_NAME_MAX},
    {"Service",        OPT_SERVICE,    SERVICE_NAME_MAX},
    {"QueueSize",        OPT_QUEUESIZE,    QUEUE_SIZE_MAX},
};


static char* get_optname(int index);
static int get_optsize(int index);
static int get_dict(char* buf, ssize_t len, const char* key, char* value);


int
read_server_config(const char* path, struct server_config* sc)
{
    if (path == NULL || sc == NULL) {
        errno = EINVAL;
        return -1;
    }

    int fd;
    if ((fd = open(path, O_RDONLY)) == -1) {
        return -1;
    }

    char buf[1024];
    ssize_t len = read(fd, buf, sizeof(buf));
    if (-1 == len) {
        goto err;
    } else if (0 == len) {
        goto err;
    } else {
        //parse host and service from buf
        get_dict(buf, len, get_optname(OPT_HOSTNAME), sc->hostname);
        get_dict(buf, len, get_optname(OPT_SERVICE), sc->service);
        get_dict(buf, len, get_optname(OPT_QUEUESIZE), sc->queue_size);
    }
    close(fd);
    return 0;

    err:
        close(fd);
        return -1;
}

static char*
get_optname(int index)
{
    int i;
    for (i = 0; i< sizeof(server_opt_array)/sizeof(struct config_opt); i++) {
        if (index == server_opt_array[i].index) {
            return server_opt_array[i].opt;
        }
    }

    return NULL;
}

static int
get_optsize(int index)
{
    int i;
    for (i = 0; i< sizeof(server_opt_array)/sizeof(struct config_opt); i++) {
        if (index == server_opt_array[i].index) {
            return server_opt_array[i].size;
        }
    }

    return 0;
}

static int
get_dict(char* buf, ssize_t len, const char* key, char* value)
{
    char *pos;
    while (pos != NULL) {
        line = readline(buf, &pos);
        if (line == NULL) {
            break;
        }
        char key_area[256], value_area[256];
        char* key_words[1];
        char* value_words[32];
        if (-1 == str_split(line, ":", key_area, value_area)) {
            continue;
        }
        int n;
        n = to_words(key_area, key_words, 1);
        if (n != 1) {
            /* number of key isn't 1 should be ingnored. */
            continue;
        }
        n = to_words(value_area, value_words, 32);
        
        /* next line */
        buf = &pos;
    }

    return 0;
}

static char*
readline(const char* buf, int* pos)
{

}

static int
str_split(const char* buf, char* seperator, char* s1, char* s2)
{

    s = strstr(buf, seperator);
    if (s == NULL) {
        errno = EINVAL;
        return -1;
    }

    memcpy(s1, buf, s - buf);
    s1[s - buf] = '\0';

    strcpy(s2, s);

    return 0;
}

static int
to_words(const char* buf, ssize_t size, char** words, int w_maxnum)
{
    char* p, w_start, w_end;
    int i, l, buf_end = buf + size;

    for (p = buf, i =0; (p < buf_end)&&(i < w_maxnum); i++) {
        /* find word start */
        for (w_start = p; p < buf_end; w_start ++) {
            if ((*w_start >= 'a' && *w_start <= 'z') || (*w_start >= 'A' && *w_start <= 'Z'))
                break;
        }
        if (w_start == size) {
            /* no word left */
            break;
        }

        /* find word end */
        for (w_end = w_start; w_end < buf_end; w_end++) {
            if ((*w_end == ' ')  ||
                (*w_end == '\t') ||
                (*w_end == '\n') ||
                (*w_end == '\0') )
                break;
        }

        l = w_end - w_start;
        strncpy(*words[i], w_start , l);
        p = w_end;
    }

    return i;
}

int
dump_server_config(struct server_config* sc, char* buf, ssize_t size)
{
    return snprintf(buf, size, "%s\n %s\n %d\n %d\n",
        sc->hostname, sc->service, sc->port, sc->queue_size);
}

#ifdef TEST
int
main(int argc, char** argv)
{
    struct server_config sc;
    read_server_config("./default.conf", &sc);
    char buf[1024];
    dump_server_config(&sc, buf, sizeof(buf));
    printf("server_config:\n %s\n", buf);
}
#endif
