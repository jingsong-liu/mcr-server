#ifndef _MCR_DICT_H
#define _MCR_DICT_H
struct dict {
	char *key;
	char **values;
	int valnum;
	struct dict *next;
};

struct dict*
dict_init(struct dict *next);

void
dict_deinit(struct dict* dic);

int
to_dict(char* buf, struct dict *diclist);

char *
dict_dump(const struct dict *dic, char* buf, size_t size);

#endif
