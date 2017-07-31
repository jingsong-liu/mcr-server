#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "include/mcr_define.h"
#include "include/mcr_text.h"
#include "include/mcr_dict.h"


struct dict*
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


void
dict_deinit(struct dict* dic) {
    if (dic == NULL ) {
        return ;
    }

    /* Although all memory of dict should be freed by dict_deinit(), still check NULL to 
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


int
to_dict(char* buf, struct dict *diclist) {
    char *cl = buf, *nl = buf;
    int n = 0;
    struct dict *dic = diclist;
    while (1) {
        if (dic == NULL || nl == NULL) {
            break;
        }

        int l;
        nl = readline(cl, &l);

        if (nl == NULL) {
            dic->next = NULL;
        }

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

        if (1 != to_words(key_area, &(dic->key), 1)) {
            /* number of key isn't 1 should be ingnored. */
            goto next;
        }

        dic->valnum = to_words(values_area, dic->values, MAX_WORD_NUM);

        dic = dic->next;

        n++;
        
next: /* go to next line */
        cl = nl;
    }

    return n;
}


char *
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


void dictlist_init_head(struct dict *head)
{
    head->next = NULL;

}


static inline void
_dictlist_add(struct dict *cur,  struct dict *new)
{
    struct dict *tmp = cur->next;
    cur->next = new;
    new->next = tmp;
}



void diclist_insert(struct dict *cur, struct dict *new)
{
    _dictlist_add(cur, new);
}


void dictlist_add_tail(struct dict *head, struct dict *new)
{
    struct dict *tail = head;
    while (tail->next != NULL) {
        tail = tail->next;
    }
    _dictlist_add(tail, new);
}


void dictlist_del_tail(struct dict *head)
{
    struct dict *tail = head;
    struct dict *tail_1 = NULL;
    while (tail->next != NULL) {
        tail_1 = tail;
        tail = tail->next;
    }
    tail_1->next = NULL;
}


#ifdef DICT_TEST
int
main(int argc, char** argv)
{
    /* test to_dict */
    struct dict *dl_test, *dl_test1, *dl_test2;
    dl_test2 = dict_init(NULL);
    dl_test1 = dict_init(dl_test2);
    dl_test = dict_init(dl_test1);
    if (!dl_test || !dl_test1) {
        printf("error init dict: %s\n", strerror(errno));
    }

    int ret = to_dict("host: www.exmple.com zz.com dead.\n#service:mysql web\nPort:8080 8081", dl_test);
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
}
#endif
