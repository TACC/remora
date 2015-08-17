#include "lnet.h"
#include "x_node.h"
#include "host.h"
#include "string1.h"
#include "trace.h"

static LIST_HEAD(lnet_list);

struct lnet_struct *lnet_lookup(const char *name, int flags, size_t hint)
{
  struct lnet_struct *l;

  list_for_each_entry(l, &lnet_list, l_link) {
    if (strcmp(name, l->l_name) == 0)
      return l;
  }

  if (!(flags & L_CREATE))
    return NULL;

  l = malloc(sizeof(*l) + strlen(name) + 1);
  if (l == NULL)
    return NULL;

  memset(l, 0, sizeof(*l));
  strcpy(l->l_name, name);

  if (hash_table_init(&l->l_hash_table, hint) < 0)
    goto err;

  list_add(&l->l_link, &lnet_list);

  if (0) {
  err:
    free(l);
    l = NULL;
  }

  return l;
}

struct x_node *
lnet_lookup_nid(struct lnet_struct *l, const char *nid, int flags)
{
  struct str_table_entry *e;
  struct x_node *x;

  e = str_table_lookup(&l->l_hash_table, nid, flags);
  if (e == NULL)
    return NULL;

  if (e->e_value != NULL)
    return e->e_value;

  /* Create a new host using NID as its name. */
  x = x_host_lookup(nid, NULL, L_CREATE);
  if (x == NULL) {
    hlist_del(&e->e_node);
    free(e);
    return NULL;
  }

  e->e_value = x;

  return x;
}

static inline int
lnet_set_nid(struct lnet_struct *l, const char *nid, struct x_node *x)
{
  return str_table_set(&l->l_hash_table, nid, x);
}

int lnet_read(struct lnet_struct *l, const char *path)
{
  int rc = -1;
  FILE *file = NULL;
  char *line = NULL;
  size_t line_size = 0;

  file = fopen(path, "r");
  if (file == NULL)
    goto out;

  while (getline(&line, &line_size, file) >= 0) {
    char *str, *nid, *name;
    struct x_node *x;

    str = chop(line, '#');

    if (split(&str, &nid, &name, (char **) NULL) != 2)
      continue;

    x = x_host_lookup(name, NULL, L_CREATE);
    if (x == NULL)
      continue;

    if (lnet_set_nid(l, nid, x) < 0) {
      rc = -1;
      break;
    }
  }

  rc = 0;

 out:
  free(line);
  if (file != NULL)
    fclose(file);

  return rc;
}
