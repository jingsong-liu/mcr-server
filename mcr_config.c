#include "include/mcr_define.h"
#include "include/mcr_config.h"
#include "include/mcr_dict.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

#define SERVER_OPT_BASE         0x80000000
#define OPT_HOSTNAME            SERVER_OPT_BASE + 1
#define OPT_SERVICE             SERVER_OPT_BASE + 2
#define OPT_BACKLOG             SERVER_OPT_BASE + 3
#define OPT_PORT                SERVER_OPT_BASE + 4
#define OPT_WWWROOT             SERVER_OPT_BASE + 5


struct config_opt {
    char opt[64];
    int index;
    int size;
};

static const struct config_opt server_opt_array[] = {
    {"HostName",       OPT_HOSTNAME,   HOST_NAME_MAX},
    {"Service",        OPT_SERVICE,    SERVICE_NAME_MAX},
    {"BackLog",        OPT_BACKLOG,    BACKLOG_MAX},
    {"Port",           OPT_PORT,       PORT_MAX},
    {"wwwroot",        OPT_WWWROOT,    FILEPATH_MAX},
};


static char* get_optname(int index);
static int get_optsize(int index);


static char*
get_optname(int index)
{
    int i;
    for (i = 0; i< sizeof(server_opt_array)/sizeof(struct config_opt); i++) {
        if (index == server_opt_array[i].index) {
            return (char*)server_opt_array[i].opt;
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

    char buf[4*1024];
    list *dlist = listCreate();
    if (dlist == NULL) {
        close(fd);
        return -1;
    }

    ssize_t len = read(fd, buf, sizeof(buf));

    if (len > 0) {
        //parse host and service from buf
        to_dict(buf, dlist);

        listIter *li = listGetIterator(dlist, AL_START_HEAD);
		listNode* node = NULL;
        while (NULL != (node = listNext(li))) {
			dict *d = node->value;
            if (!strcmp(get_optname(OPT_HOSTNAME), d->key)) {
                strncpy(sc->hostname, *(d->values), sizeof(sc->hostname));
            }
            else if (!strcmp(get_optname(OPT_SERVICE), d->key)) {
                strncpy(sc->service, *(d->values), sizeof(sc->service));
            }
            else if (!strcmp(get_optname(OPT_PORT), d->key)) {
               sc->port = atoi(*(d->values)); 
            }
            else if (!strcmp(get_optname(OPT_BACKLOG), d->key)) {
                sc->backlog = atoi(*(d->values));
            }
            else if (!strcmp(get_optname(OPT_WWWROOT), d->key)) {
                strncpy(sc->wwwroot, *(d->values), sizeof(sc->wwwroot));
            }
            else {
                /* ignore */
            }
        }

		listReleaseIterator(li);
        listEmpty(dlist);
		listRelease(dlist);
        close(fd);
        return 0;
    }

	listRelease(dlist);
    close(fd);
    return -1;
}


char*
dump_server_config(struct server_config* sc, char* buf, size_t size)
{
    snprintf(buf, size, "%s\n%s\n%d\n%d\n",
        sc->hostname, sc->service, sc->port, sc->backlog);

    return buf;
}


#ifdef SERVERCONFIG_TEST
int
main(int argc, char** argv)
{
    /* test read_server_config */
    struct server_config sc;
    if (-1 == read_server_config("./default.conf", &sc)) {
        printf("error read server config: %s\n", strerror(errno));
    }

    char buf[1024];
    printf("server_config:\n%s\n", dump_server_config(&sc, buf, sizeof(buf)));

    return 0;
}
#endif
