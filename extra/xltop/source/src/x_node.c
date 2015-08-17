#include <math.h>
#include <string.h>
#include "trace.h"
#include "x_node.h"
#include "sub.h"

size_t nr_k;
struct hash_table k_hash_table;

double k_tick = K_TICK, k_window = K_WINDOW;

/* TODO Move default hints to a header. */

struct x_node *x_all[2];

struct x_type x_types[] = {
  [X_HOST] = {
    .x_type_name = "host",
    .x_nr_hint = 4096,
    .x_type = X_HOST,
    .x_which = 0,
  },
  [X_JOB] = {
    .x_type_name = "job",
    .x_nr_hint = 256,
    .x_type = X_JOB,
    .x_which = 0,
  },
  [X_CLUS] = {
    .x_type_name = "clus",
    .x_nr_hint = 1,
    .x_type = X_CLUS,
    .x_which = 0,
  },
  [X_U] = {
    .x_type_name = "u",
    .x_nr_hint = 1,
    .x_type = X_U,
    .x_which = 0,
  },
  [X_SERV] = {
    .x_type_name = "serv",
    .x_nr_hint = 128,
    .x_type = X_SERV,
    .x_which = 1,
  },
  [X_FS] = {
    .x_type_name = "fs",
    .x_nr_hint = 1,
    .x_type = X_FS,
    .x_which = 1,
  },
  [X_V] = {
    .x_type_name = "v",
    .x_nr_hint = 1,
    .x_type = X_V,
    .x_which = 1,
  },
};

void x_init(struct x_node *x, int type, struct x_node *parent, size_t hash,
            struct hlist_head *head, const char *name)
{
  memset(x, 0, sizeof(*x));

  x->x_type = &x_types[type];
  x->x_type->x_nr++;

  ASSERT(parent != NULL || type == X_U || type == X_V);

  if (parent == NULL) {
    INIT_LIST_HEAD(&x->x_parent_link);
  } else {
    parent->x_nr_child++;
    list_add_tail(&x->x_parent_link, &parent->x_child_list);
    x->x_parent = parent;
  }

  INIT_LIST_HEAD(&x->x_child_list);
  INIT_LIST_HEAD(&x->x_sub_list);

  x->x_hash = hash;

  /* FIXME We don't look to see if name is already hashed. */
  if (head == NULL) {
    struct hash_table *t = &x->x_type->x_hash_table;
    head = t->t_table + (hash & t->t_mask);
  }

  hlist_add_head(&x->x_hash_node, head);
  strcpy(x->x_name, name);
}

void x_set_parent(struct x_node *x, struct x_node *p)
{
  if (x->x_parent == p)
    return;

  if (x->x_parent != NULL) {
    list_del_init(&x->x_parent_link);
    x->x_parent->x_nr_child--;
  }

  if (p != NULL) {
    ASSERT(x_which(x) == x_which(p));
    list_move_tail(&x->x_parent_link, &p->x_child_list);
    p->x_nr_child++;
  }

  x->x_parent = p;
}

/* TODO s/k_destroy/k_destroy_and_free_rec.../ */

void x_destroy(EV_P_ struct x_node *x)
{
  struct x_node *c, *t;
  struct sub_node *s, *u;

  if (x_which(x) == 0)
    k_destroy(EV_A_ x, x_all[1], 0);
  else
    k_destroy(EV_A_ x_all[0], x, 1);

  if (x->x_parent != NULL)
    x->x_parent->x_nr_child--;
  x->x_parent = NULL;

  list_del_init(&x->x_parent_link);

  x_for_each_child_safe(c, t, x)
    x_set_parent(c, x_all[x_which(x)]);

  ASSERT(x->x_nr_child == 0);

  /* Do we need this with k_destroy() above? */
  list_for_each_entry_safe(s, u, &x->x_sub_list, s_x_link[x_which(x)])
    sub_cancel(EV_A_ s);

  hlist_del(&x->x_hash_node);

  x->x_type->x_nr--;
  memset(x, 0, sizeof(*x));
}

struct x_node *
x_lookup(int type, const char *name, struct x_node *p, int flags)
{
  struct hash_table *t = &x_types[type].x_hash_table;
  size_t hash = str_hash(name, 64); /* XXX */
  struct hlist_head *head = t->t_table + (hash & t->t_mask);
  struct hlist_node *node;
  struct x_node *x;

  hlist_for_each_entry(x, node, head, x_hash_node)
    if (strcmp(name, x->x_name) == 0)
      return x;

  if (!(flags & L_CREATE))
    return NULL;

  x = malloc(sizeof(*x) + strlen(name) + 1);
  if (x == NULL)
    return NULL;

  x_init(x, type, p, hash, head, name);

  return x;
}

struct x_node *x_lookup_hash(int type, const char *name,
                             size_t *hash_ref, struct hlist_head **head_ref)
{
  struct hash_table *t = &x_types[type].x_hash_table;
  size_t hash = str_hash(name, 64); /* XXX */
  struct hlist_head *head = t->t_table + (hash & t->t_mask);
  struct hlist_node *node;
  struct x_node *x;

  hlist_for_each_entry(x, node, head, x_hash_node)
    if (strcmp(name, x->x_name) == 0)
      return x;

  *hash_ref = hash;
  *head_ref = head;

  return NULL;
}

struct x_node *x_lookup_str(const char *str)
{
  size_t i;

  /* "host:i101-101.ranger.tacc.utexas.edu" => x_lookup(X_HOST, "i101-...") */

  for (i = 0; i < NR_X_TYPES; i++) {
    size_t len = strlen(x_types[i].x_type_name);

    if (strncmp(str, x_types[i].x_type_name, len) == 0 &&
        (str[len] == ':' || str[len] == '='))
      return x_lookup(i, str + len + 1, NULL, 0);
  }

  return NULL;
}

int x_types_init(void)
{
  size_t i;
  size_t nr[2] = { 0, 0 };
  size_t k_nr_hint;

  TRACE("sizeof(struct x_node) %zu\n", sizeof(struct x_node));
  TRACE("sizeof(struct k_node) %zu\n", sizeof(struct k_node));

  for (i = 0; i < NR_X_TYPES; i++) {
    if (hash_table_init(&x_types[i].x_hash_table, x_types[i].x_nr_hint) < 0)
      return -1;
    nr[x_types[i].x_which] += x_types[i].x_nr_hint;
  }

  for (i = 0; i < 2; i++) {
    x_all[i] = x_lookup((i == 0) ? X_U : X_V,
                        (i == 0) ? X_U_NAME : X_V_NAME,
                        NULL,
                        L_CREATE);

    if (x_all[i] == NULL)
      return -1;
  }

  k_nr_hint = nr[0] * nr[1];
  TRACE("nr[0] %zu, nr[1] %zu, k_nr_hint %zu\n", nr[0], nr[1], k_nr_hint);

  if (hash_table_init(&k_hash_table, k_nr_hint) < 0)
    return -1;

  return 0;
}

void x_update(EV_P_ struct x_node *x0, struct x_node *x1, double *d)
{
  struct x_node *i0, *i1;
  struct k_node *k;

  for (i0 = x0; i0 != NULL; i0 = i0->x_parent) {
    for (i1 = x1; i1 != NULL; i1 = i1->x_parent) {
      k = k_lookup(i0, i1, L_CREATE);

      if (k != NULL)
        k_update(EV_A_ k, x0, x1, d);
    }
  }
}

struct k_node *k_lookup(struct x_node *x0, struct x_node *x1, int flags)
{
  struct hash_table *t = &k_hash_table;
  size_t hash = pair_hash(x0->x_hash, x1->x_hash, t->t_shift);
  struct hlist_head *head = t->t_table + (hash & t->t_mask);
  struct hlist_node *node;
  struct k_node *k;

  hlist_for_each_entry(k, node, head, k_hash_node) {
    if (k->k_x[0] == x0 && k->k_x[1] == x1)
      return k;
  }

  if (!(flags & L_CREATE))
    return NULL;

  if (x_which(x0) != 0 || x_which(x1) != 1) {
    errno = EINVAL;
    return NULL;
  }

  k = malloc(sizeof(*k));
  if (k == NULL)
    return NULL;

  /* k_init() */
  memset(k, 0, sizeof(*k));
  hlist_add_head(&k->k_hash_node, head);
  k->k_x[0] = x0;
  k->k_x[1] = x1;
  INIT_LIST_HEAD(&k->k_sub_list);
  nr_k++;

  return k;
}

void k_destroy(EV_P_ struct x_node *x0, struct x_node *x1, int which)
{
  struct k_node *k = k_lookup(x0, x1, 0);
  struct x_node *c;
  struct sub_node *s, *t;

  if (k == NULL)
    return;

  list_for_each_entry_safe(s, t, &k->k_sub_list, s_k_link)
    sub_cancel(EV_A_ s);

  hlist_del(&k->k_hash_node);
  free(k);
  nr_k--;

  if (which == 0) {
    x_for_each_child(c, x1)
      k_destroy(EV_A_ x0, c, which);
  } else {
    x_for_each_child(c, x0)
      k_destroy(EV_A_ c, x1, which);
  }
}

void k_freshen(struct k_node *k, double now)
{
  if (k->k_t <= 0)
    k->k_t = now;

  double n = floor((now - k->k_t) / k_tick); /* # ticks. */

  k->k_t += fmax(n, 0) * k_tick;

  size_t i;
  for (i = 0; i < NR_STATS; i++) {
    if (n > 0) {
      /* Apply pending. */
      double r = k->k_pending[i] / k_tick;
      k->k_pending[i] = 0;

      /* TODO (n > K_TICKS_HUGE || k_rate < K_RATE_EPS) */
      if (k->k_rate[i] <= 0)
        k->k_rate[i] = r;
      else
        k->k_rate[i] += (k->k_rate[i] - r) * expm1(-k_tick / k_window);
    }


    if (n > 1)
      /* Decay rate for missed intervals. */
      k->k_rate[i] *= exp((n - 1) * (-k_tick / k_window));
  }
}

void k_update(EV_P_ struct k_node *k, struct x_node *x0, struct x_node *x1, double *d)
{
  double now = ev_now(EV_A);

  TRACE("%s %s, k_t %f, now %f, d "PRI_STATS_FMT("%f")"\n",
        k->k_x[0]->x_name, k->k_x[1]->x_name, k->k_t, now, PRI_STATS_ARG(d));

  k_freshen(k, now);

  size_t i;
  for (i = 0; i < NR_STATS; i++) {
    k->k_sum[i] += d[i];
    k->k_pending[i] += d[i];
    /* TRACE("now %8.3f, t %8.3f, p %12f, A %12f %12e\n", now, t, p, A, A); */
  }

  struct sub_node *s;
  list_for_each_entry(s, &k->k_sub_list, s_k_link)
    (*s->s_cb)(EV_A_ s, k, x0, x1, d);
}
