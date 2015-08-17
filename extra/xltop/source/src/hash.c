#include <stdlib.h>
#include <malloc.h>
#include "string1.h"
#include "trace.h"
#include "hash.h"

#define HASH_SHIFT_MIN 4
#define HASH_SHIFT_MAX (8 * (sizeof(size_t) - 1))

int hash_table_init(struct hash_table *t, size_t hint)
{
  size_t shift = HASH_SHIFT_MIN, len, i;

  memset(t, 0, sizeof(*t));

  len = ((size_t) 1) << shift;
  while (len < 2 * hint && shift < HASH_SHIFT_MAX) {
    shift++;
    len <<= 1;
  }

  t->t_table = malloc(len * sizeof(t->t_table[0]));
  if (t->t_table == NULL)
    return -1;

  for (i = 0; i < len; i++)
    INIT_HLIST_HEAD(&t->t_table[i]);

  t->t_shift = shift;
  t->t_mask = len - 1;

  TRACE("hint %zu, len %zu, shift %zu, mask %zx\n",
        hint, len, t->t_shift, t->t_mask);

  return 0;
}

size_t str_hash(const char *s, size_t nr_bits)
{
  size_t c, h = 0;

  for (; *s != 0; s++) {
    c = *(unsigned char *) s;
    h = (h + (c << 8) + c) * 11;
  }

  return h;
}

char *_str_table_lookup(struct hash_table *t, const char *key,
                        struct hlist_head **head_ref, size_t str_offset)
{
  size_t hash = str_hash(key, t->t_shift);
  struct hlist_head *head = t->t_table + (hash & t->t_mask);
  struct hlist_node *node;

  /* TRACE("hash %zx, i %8zx, key `%s'\n", hash, hash & t->t_mask, key); */

  if (head_ref != NULL)
    *head_ref = head;

  hlist_for_each(node, head) {
    char *str = ((char *) node) + str_offset;
    if (strcmp(key, str) == 0)
        return str;
  }

  return NULL;
}

struct str_table_entry *
str_table_lookup(struct hash_table *t, const char *key, int flags)
{
  struct hlist_head *head;
  struct str_table_entry *e;

  e = str_table_lookup_entry(t, key, &head, struct str_table_entry, e_node, e_key);
  if (e != NULL)
    return e;

  if (!(flags & L_CREATE))
    return NULL;

  e = malloc(sizeof(*e) + strlen(key) + 1);
  if (e == NULL)
    return NULL;

  memset(e, 0, sizeof(*e));
  hlist_add_head(&e->e_node, head);
  strcpy(e->e_key, key);

  return e;
}

int str_table_set(struct hash_table *t, const char *key, void *value)
{
  struct str_table_entry *e = str_table_lookup(t, key, L_CREATE);
  if (e == NULL)
    return -1;

  e->e_value = value;

  return 0;
}

void *str_table_ref(struct hash_table *t, const char *key)
{
  struct str_table_entry *e = str_table_lookup(t, key, 0);

  return (e != NULL) ? e->e_value : NULL;
}
