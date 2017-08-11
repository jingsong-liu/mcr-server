#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "include/mcr_define.h"
#include "include/mcr_text.h"
#include "include/mcr_dict.h"

dict*
dict_init(void) {
    dict *dic = (dict*)malloc(sizeof(dict));
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

	int i, j;
    for (i =0; i < MAX_WORD_NUM; i++) {
        *(dic->values + i) = (char*)malloc(MAX_WORD_SIZE*sizeof(char));
        if (*(dic->values +i) == NULL) {
            for (j = i -1 ;j >=0; j--) {
                free(*(dic->values + j));
            }
            free(dic->values);
            free(dic->key);
            free(dic);
            return NULL;
        }
    }

    dic->valnum = 0;

    return dic;
}


void
dict_deinit(dict* dic) {
    if (dic == NULL ) {
        return ;
    }

    /* Although all memory of dict should be freed by dict_deinit(), still check NULL to 
     * keep from freeing fialed on a NULL pointer, this appears when somewhere else free
     *  memory of dict member and give it NULL out of dict_deinit().
     */
	int i;
    for (i = 0; i < MAX_WORD_NUM; i++) {
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


int
to_dict(char* buf, list *diclist) {
    char *cl = buf, *nl = buf;
    int n = 0;
	dict* dic = NULL;
    while (1) {
        if (nl == NULL) {
            break;
        }

        int l;
        nl = readline(cl, &l);

        /* line started with '#' is comment */
        if (*cl == '#') {
            goto next;
        }

        if (l == 0) {
            goto next;
        }

        char line[MAX_WORD_SIZE*(MAX_WORD_NUM + 2) + 1];
        if (l > sizeof(line) - 1) {
            l = sizeof(line) - 1;
        }
        memcpy(line, cl, l);
        line[l] = '\0';

        char key_area[MAX_WORD_SIZE*2], values_area[MAX_WORD_SIZE*(MAX_WORD_NUM + 1)];
        if (-1 == str_split2(line, ":", key_area, values_area)) {
            goto next;
        }

		/* memory allocate failed, exit from the rest work. */
		if (NULL  == (dic = dict_init())) {
			break;
		}
        if (1 != to_words(key_area, &(dic->key), 1)) {
            /* number of key isn't 1 should be ingnored. */
            goto next;
        }

        dic->valnum = to_words(values_area, dic->values, MAX_WORD_NUM);

		if (NULL == listAddNodeTail(diclist, dic)) {
			dict_deinit(dic);
			break;
		}

        n++;
        
next: /* go to next line */
        cl = nl;
    }

    return n;
}


char *
dict_dump(const dict *dic, char* buf, size_t size) {
    char values[MAX_WORD_SIZE*(MAX_WORD_NUM + 1)], *ptmp = values;
	int i;

    memset(buf, 0, size);
    memset(values, 0, sizeof(values));

    for (i = 0; i < dic->valnum; i++) {
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


#ifdef DICT_TEST
int
main(int argc, char** argv)
{
    /* test to_dict */
	list *dlist = listCreate();
	listNode *node = NULL;	

    int ret = to_dict("host: www.exmple.com zz.com dead.\n#service:mysql web\nPort:8080 8081", dlist);
    printf("to_dict ret:%d\n", ret);
    char buf_test_dict[MAX_WORD_NUM*MAX_WORD_SIZE];
	listIter *li = listGetIterator(dlist, AL_START_HEAD);
	while (NULL != (node = listNext(li))) {
        printf("dict: %s\n", dict_dump((dict *)node->value, buf_test_dict, sizeof(buf_test_dict)));
	}


	listEmpty(dlist);
	listReleaseIterator(li);
	listRelease(dlist);

    return 0;
}
#endif
