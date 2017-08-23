#include <string.h>
#include <stdio.h>
#include "include/mcr_define.h"


char*
readline(const char* buf, int * len)
{
    char * pos = (char*)buf;
    while (*pos != '\0' && *pos != '\n') {
        pos ++;
    }

    *len = pos - buf;

    return (*pos == '\0') ? NULL : (pos + 1);
}


int
str_split2(const char* buf, char* seperator, char* s1, char* s2)
{
    char* s = strstr(buf, seperator);
    if (s == NULL) {
        return -1;
    }

    /* more than one seperator existence is not allowed */
    int l1 = s - buf;
    if (strlen(buf) > l1 && NULL != strstr(s + strlen(seperator), seperator)) {
        return -1;
    }


    if (l1 > 0) {
        memcpy(s1, buf, l1);
        s1[l1] = '\0';
    }

    strcpy(s2, s +strlen(seperator));

    return 0;
}


int
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
                (*w_start >= '0' && *w_start <= '9') ||
                (*w_start == '_')                    ||
                (*w_start == '/'))
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
                    (*(w_end + 1) == '\0') ) {
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


#ifdef TEXT_TEST
#include <stdlib.h>
int main(int argc, char** argv) 
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
    int ret = to_words("China is the greatest country in the world! do you agree? _MLGBD 2333\n", words, 20);
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

    return 0;
}

#endif
