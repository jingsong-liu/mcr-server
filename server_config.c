#include "include/server_config.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#define SERVER_OPT_BASE         0x80000000
#define OPT_HOSTNAME            SERVER_OPT_BASE + 1
#define OPT_SERVICE             SERVER_OPT_BASE + 2
#define OPT_QUEUESIZE           SERVER_OPT_BASE + 3


#define MAX_WORD_SIZE	128
#define MAX_WORD_NUM	10
struct dict {
	char *key;
	char **values;
	int valnum;
	struct dict *next;
};


static struct dict*
dict_init(struct dict *next) {
	struct dict *dic = (struct dict*)malloc(sizeof(struct dict));
	if (dic == NULL) {
		return NULL;
	}

	dic->key =  (char*)malloc(MAX_WORD_SIZE*sizeof(char));
	if (dic->key == NULL) {
		free(dic);
		return NULL;
	}

	dic->values = (char**)malloc(MAX_WORD_NUM*sizeof(char*));
	if (dic->values == NULL) {
		free(dic->key);
		free(dic);
		return NULL;
	}

	for (int i =0; i < MAX_WORD_NUM; i++) {
		*(dic->values + i) = (char*)malloc(MAX_WORD_SIZE*sizeof(char));
		if (*(dic->values +i) == NULL) {
			for (int j = i -1 ;j >=0; j--) {
				free(*(dic->values + j));
			}
			free(dic->values);
			free(dic->key);
			free(dic);
			return NULL;
		}
	}

	dic->valnum = 0;

	dic->next = next;

	return dic;
}


static void
dict_deinit(struct dict* dic) {
	if (dic == NULL ) {
		return ;
	}

	/* Although all memory of dict should be freed by dict_init(), still check NULL to 
	 * keep from freeing fialed on a NULL pointer, this appears when somewhere else free
	 *  memory of dict member and give it NULL out of dict_deinit().
	 */
	for (int i = 0; i < MAX_WORD_NUM; i++) {
		if (*(dic->values + i) == NULL) {
			continue;
		}
		free(*(dic->values +i));
	}

	if (dic->values == NULL)
		return;
	free(dic->values);

	if (dic->key == NULL)
		return;
	free(dic->key);

	free(dic);
}


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
static char * readline(const char * buf, int* llen);
static int str_split2(const char* buf, char* seperator, char* s1, char* s2);
static int to_words(const char* buf, char** words, int w_maxnum);
static int to_dict(char* buf, struct dict **dictlist);


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
	struct dict *dlist = (struct dict *)malloc(sizeof(struct dict));
	dlist->next = NULL;

    ssize_t len = read(fd, buf, sizeof(buf));
    if (-1 == len) {
        goto err;
    } else if (0 == len) {
        goto err;
    } else {
        //parse host and service from buf
        to_dict(buf, &dlist);
    }

	free(dlist);
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


static char*
readline(const char* buf, int * len)
{
	char * pos = (char*)buf;
	while (*pos != '\0' && *pos != '\n') {
			pos ++;
	}

	*len = pos - buf;

	return (*pos == '\0') ? NULL : (pos + 1);
}


static int
str_split2(const char* buf, char* seperator, char* s1, char* s2)
{

    char* s = strstr(buf, seperator);
    if (s == NULL) {
        errno = EINVAL;
        return -1;
    }

	/* more than one seperator existence is not allowed */
	int l1 = s - buf;
	if (strlen(buf) > l1 && NULL != strstr(s + strlen(seperator), seperator)) {
		errno = EINVAL;
		return -1;
	}


	if (l1 > 0) {
		memcpy(s1, buf, l1);
		s1[l1] = '\0';
	}

    strcpy(s2, s +strlen(seperator));

    return 0;
}


static int
to_words(const char* buf, char **words, int w_maxnum)
{
    char* p, * w_start, * w_end;
    int i, l;
	char *buf_end = strchr(buf, '\0');

    for (p = (char*)buf, i =0; (p < buf_end )&&(i < w_maxnum); i++) {
        /* find word start */
        for (w_start = p; w_start < buf_end; w_start ++) {
            if ((*w_start >= 'a' && *w_start <= 'z') ||
				(*w_start >= 'A' && *w_start <= 'Z') ||
				(*w_start == '_'))
                break;
        }

		/* no word left */
        if (w_start == buf_end) {
            break;
        }

        /* find word end */
        for (w_end = w_start; w_end < buf_end; w_end++) {
            if ((*w_end == ' ')  ||
                (*w_end == '\t') ||
                (*w_end == '\n') ||
                (*w_end == '\0') )
                break;
			
			else if ((*w_end == '.') ||
					 (*w_end == ',') ||
					 (*w_end == '!') ||
					 (*w_end == '?')) {

				if ((*(w_end + 1) == ' ')  ||
					(*(w_end + 1) == '\t') ||
					(*(w_end + 1) == '\n') ||
					(*(w_end + i) == '\0') ) {
					break;
				}
			}
		}

        l = w_end - w_start;

		if (l > MAX_WORD_SIZE - 1) {
			l = MAX_WORD_SIZE - 1;
		}

        strncpy(*(words + i), w_start , l);
		/* force words endup with '\0' */
		*(*(words + i) + l) = '\0';

        p = w_end;
    }

    return i;
}


static int
to_dict(char* buf, struct dict **diclist)
{
	char * line = buf;
	char *nl = NULL;
	int n = 0;
	struct dict **dic = diclist;
    while (1) {
		int l;
		nl = readline(line, &l);

        if (l == 0) {
			goto next;
        }

        char key_area[MAX_WORD_SIZE*2], values_area[MAX_WORD_SIZE*(MAX_WORD_NUM + 1)];
        if (-1 == str_split2(line, ":", key_area, values_area)) {
            goto next;
        }

		if (*dic == NULL) {
			break;
		}

        if (1 != to_words(key_area, &((*dic)->key), 1)) {
        
            /* number of key isn't 1 should be ingnored. */
            goto next;
        }

        (*dic)->valnum = to_words(values_area, (*dic)->values, MAX_WORD_NUM);

		*dic = (*dic)->next;

		n++;
        
next: /* next line */
		if (nl == NULL) {
			(*dic)->next = NULL;
			break;
		}

		line = nl;
    }

    return n;
}


static char *
dict_dump(const struct dict *dic, char* buf, size_t size) {
	char values[MAX_WORD_SIZE*(MAX_WORD_NUM + 1)], *ptmp = values;

	memset(buf, 0, size);
	memset(values, 0, sizeof(values));

	for (int i = 0; i < dic->valnum; i++) {
		strcat(ptmp, *((dic->values) + i));
		ptmp += strlen(*((dic->values) + i));

		if (i != dic->valnum - 1) {
			strcat(ptmp, ", ");
			ptmp += strlen(", ");
		}
		
		if (ptmp - values > size - strlen(dic->key) - 14) {
			break;
		}
	}

	snprintf(buf, size, "key:%s | value:[%s] | valnum:%d", dic->key, values, dic->valnum);

	return buf;
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

	/* test realine */
	char rl_test[] = "aaaaaaaaaaaa\nbbbbbbbbbbbb\n\n  \n";
	int len = 0;
	char * s = rl_test;
	char tmp[128];
	char *nl;
	while (1) {
		nl = readline(s, &len);
		memcpy(tmp, s, len);
		tmp[len]= '\0';
		printf("line: %s\n", tmp);
		
		if (nl == NULL) {
			break;
		}
		s = nl;
	}


	/* test seperator */
	char spl_test[] = "lover: bing";
	char spl_test1[] = ":bing";
	char spl_test2[] = "lover:";
	char s1[128], s2[128];
	str_split2(spl_test, ":", s1, s2);
	printf ("%s\n", s1);
	printf ("%s\n", s2);
	str_split2(spl_test1, ":", s1, s2);
	printf ("%s\n", s1);
	printf ("%s\n", s2);
	str_split2(spl_test2, ":", s1, s2);
	printf ("%s\n", s1);
	printf ("%s\n", s2);


	/* test to_words */
	char **words = (char**)malloc(sizeof(char*)*20);
	for (int i = 0; i < 20 ; i++) {
		*(words + i) = (char*)malloc(sizeof(char)*128);
	}
	int ret = to_words("China is the greatest country in the world! do you agree? _QNMLGBD\n", words, 20);
	printf("to_words ret:%d\n", ret);
	for ( int i = 0; i < 20; i++ ) {
		if (strlen(*(words+i)) == 0)
			continue;
		printf("i=%d, word=%s\n", i, *(words +i));
	}

	for (int i = 0; i < 20; i++) {
		free(*(words + i));
	}
	free(words);


	/* test to_dict */
	struct dict *dl_test, *dl_test1;
	dl_test1 = dict_init(NULL);
	dl_test = dict_init(dl_test1);
	if (!dl_test || !dl_test1) {
		printf("error init dict: %s\n", strerror(errno));
	}

	ret = to_dict("host: www.exmple.com zz.com dead.", &dl_test);
	printf("to_dict ret:%d\n", ret);
	struct dict *dl = dl_test;
	char buf_test_dict[MAX_WORD_NUM*MAX_WORD_SIZE];
	while (dl != NULL) {
		printf("dict: %s\n", dict_dump(dl, buf_test_dict, sizeof(buf_test_dict)));
		dl = dl->next;
	}

	dict_deinit(dl_test1);
	dict_deinit(dl_test);

	return 0;



    struct server_config sc;
    read_server_config("./default.conf", &sc);
    char buf[1024];
    dump_server_config(&sc, buf, sizeof(buf));
    printf("server_config:\n %s\n", buf);
}
#endif
