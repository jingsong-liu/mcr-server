#ifndef _MCR_DICT_H
#define _MCR_DICT_H
#include "mcr_list.h"

typedef struct dict {
	char *key;
	char **values;
	int valnum;
}dict;

dict*
dict_init(void);

void
dict_deinit(dict *dic);

int
to_dict(char* buf, list *diclist);

char *
dict_dump(const dict *dic, char* buf, size_t size);

#endif
