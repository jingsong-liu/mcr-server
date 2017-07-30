#ifndef _MCR_TEXT_H
#define _MCR_TEXT_H


char * readline(const char * buf, int* llen);
int str_split2(const char* buf, char* seperator, char* s1, char* s2);
int to_words(const char* buf, char** words, int w_maxnum);


#endif
