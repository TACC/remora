#ifndef _X_NODE_H_
#define _X_NODE_H_
#include <ev.h>
#include <stddef.h>
#include "list.h"
#include "hash.h"
#include "xltop.h"

enum {
  X_HOST,
  X_JOB,
  X_CLUS,
  X_U,
  X_SERV,
  X_FS,
  X_V,
  NR_X_TYPES,
};

#define X_U_NAME "ALL"
#define X_V_NAME "ALL"

#define K_TICK 10.0
#define K_WINDOW 600.0

extern double k_tick, k_window;

struct x_type {
  struct hash_table x_hash_table;
  const char *x_type_name;
  size_t x_nr, x_nr_hint;
  int x_type, x_which;
};

struct x_node {
  struct x_type *x_type;
  struct x_node *x_parent;
  struct list_head x_parent_link;
  size_t x_nr_child;
  struct list_head x_child_list;
  struct list_head x_sub_list;
  size_t x_hash;
  struct hlist_node x_hash_node;
  char x_name[];
};

extern struct x_type x_types[];
extern struct x_node *x_all[2];

struct k_node {
  struct hlist_node k_hash_node;
  struct x_node *k_x[2];
  struct list_head k_sub_list;
  double k_t; /* Timestamp. */
  double k_pending[NR_STATS];
  double k_rate[NR_STATS]; /* EWMA bytes (or reqs) per second. */
  double k_sum[NR_STATS];
};

extern size_t nr_k;
extern struct hash_table k_hash_table;

int x_types_init(void);
void x_init(struct x_node *x, int type, struct x_node *parent, size_t hash,
            struct hlist_head *hash_head, const char *name);
void x_set_parent(struct x_node *x, struct x_node *p);

/* p is only used if L_CREATE is set in flags. */
struct x_node *
x_lookup(int type, const char *name, struct x_node *p, int flags);

/* No create. */
struct x_node *x_lookup_hash(int type, const char *name, size_t *hash_ref,
                             struct hlist_head **head_ref);

/* No create. */
struct x_node *x_lookup_str(const char *str);

void x_update(EV_P_ struct x_node *x0, struct x_node *x1, double *d);

void x_destroy(EV_P_ struct x_node *x);

static inline int x_which(struct x_node *x)
{
  return x->x_type->x_which;
}

static inline int x_is_type(struct x_node *x, int type)
{
  return x->x_type == &x_types[type];
}

static inline const char *x_type_name(int type)
{
  switch (type) {
  case X_HOST:  return "host";
  case X_JOB:   return "job";
  case X_CLUS:  return "clus";
  case X_U:     return "u";
  case X_SERV:  return "serv";
  case X_FS:    return "fs";
  case X_V:     return "v";
  default:      return NULL;
  }
}

static inline int x_str_type(const char *s)
{
  if (strcmp(s, "host") == 0)
    return X_HOST;
  else if (strcmp(s, "job") == 0)
    return X_JOB;
  else if (strcmp(s, "clus") == 0)
    return X_CLUS;
  else if (strcmp(s, "u") == 0)
    return X_U;
  else if (strcmp(s, "serv") == 0)
    return X_SERV;
  else if (strcmp(s, "fs") == 0)
    return X_FS;
  else if (strcmp(s, "v") == 0)
    return X_V;
  else
    return -1;
}

#define x_for_each_child(c, x)                          \
  list_for_each_entry(c, &((x)->x_child_list), x_parent_link)

#define x_for_each_child_safe(c, t, x)                          \
  list_for_each_entry_safe(c, t, &((x)->x_child_list), x_parent_link)

struct k_node *k_lookup(struct x_node *x0, struct x_node *x1, int flags);

void k_freshen(struct k_node *k, double now);

void k_update(EV_P_ struct k_node *k, struct x_node *x0, struct x_node *x1, double *d);

void k_destroy(EV_P_ struct x_node *x0, struct x_node *x1, int which);

#endif
