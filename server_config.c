#include "include/server_config.h"
#include <unistd.h>
#include <fcntl.h>

#define SERVER_OPT_BASE         0x80000000
#define OPT_HOSTNAME            SERVER_OPT_BASE + 1
#define OPT_SERVICE             SERVER_OPT_BASE + 2


struct config_opt {
    char opt[64];
    int index;
    int size;
};

static const struct config_opt server_opt_array[] = {
    {"HostName",       OPT_HOSTNAME,   HOST_NAME_MAX},
    {"Service",        OPT_SERVICE,    SERVICE_NAME_MAX},
    
    {NULL,             NULL,            NULL}, 
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
    char *p;
    buf[len-1] = '\0';
    p = strstr(buf, key) + strlen(key) + 1;
    
    strncpy(value, p, strchr(p, ' ') - p);

    return 0;
}
