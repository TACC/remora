#ifndef _HASH_H_
#define _HASH_H_
#include "stddef1.h"
#include "list.h"

#define L_CREATE (1 << 0)
/* TODO #define L_EXCLUSIVE (1 << 1) */

struct hash_table {
  struct hlist_head *t_table;
  size_t t_shift, t_mask;
};

struct str_table_entry {
  struct hlist_node e_node;
  void *e_value;
  char e_key[];
};

int hash_table_init(struct hash_table *t, size_t hint);

size_t str_hash(const char *s, size_t nr_bits);

char *_str_table_lookup(struct hash_table *t, const char *key,
                        struct hlist_head **head, size_t str_offset);

#define str_table_lookup_entry(t, key, head, type, m_node, m_str)       \
  ({                                                                    \
    char *_str = _str_table_lookup((t), (key), (head),                  \
                                  offsetof(type, m_str) - offsetof(type, m_node)); \
    _str != NULL ? container_of(_str, type, m_str[0]) : NULL;           \
  })

int str_table_set(struct hash_table *t, const char *key, void *value);

void *str_table_ref(struct hash_table *t, const char *key);

struct str_table_entry *
str_table_lookup(struct hash_table *t, const char *key, int flags);

static inline int str_table_for_each(struct hash_table *t,
                                     size_t *i, struct hlist_node **n,
                                     char **key, void **value)
{
  int found = 0;

  while (*i < (1ULL << t->t_shift) && !found) {
    if (*n == NULL)
      *n = t->t_table[*i].first;

    if (*n != NULL) {
      struct str_table_entry *e;

      e = hlist_entry(*n, struct str_table_entry, e_node);
      *key = e->e_key;
      *value = e->e_value;
      *n = (*n)->next;
      found = 1;
    }

    if (*n == NULL)
      (*i)++;
  }

  return found;
}

static inline size_t pair_hash(size_t h0, size_t h1, size_t nr_bits)
{
  size_t GOLDEN_RATIO_PRIME = 0x9e37fffffffc0001UL;

  h0 += (h1 ^ GOLDEN_RATIO_PRIME);
  h0 = h0 ^ ((h0 ^ GOLDEN_RATIO_PRIME) >> nr_bits);
  return h0;
}

#endif
