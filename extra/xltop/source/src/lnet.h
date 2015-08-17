#ifndef _LNET_H_
#define _LNET_H_
#include "hash.h"

struct x_node;

struct lnet_struct {
  struct list_head l_link;
  struct hash_table l_hash_table;
  size_t l_nr_nids;
  char l_name[];
};

struct lnet_struct *lnet_lookup(const char *name, int flags, size_t hint);

int lnet_read(struct lnet_struct *l, const char *path);

struct x_node *
lnet_lookup_nid(struct lnet_struct *l, const char *nid, int flags);

#endif
