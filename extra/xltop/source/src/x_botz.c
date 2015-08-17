#include "x_botz.h"
#include "string1.h"
#include "trace.h"

struct botz_listen x_listen;

void x_printf(struct n_buf *nb, struct x_node *x)
{
  n_buf_printf(nb,
               "x_name: %s\n"
               "x_type: %s\n"
               "x_parent: %s\n"
               "x_parent_type: %s\n"
               "x_nr_child: %zu\n"
               "x_hash: %016zx\n",
               x->x_name,
               x->x_type->x_type_name,
               x->x_parent != NULL ? x->x_parent->x_name : "NONE",
               x->x_parent != NULL ? x->x_parent->x_type->x_type_name : "NONE",
               x->x_nr_child,
               x->x_hash);
}

void x_info_cb(struct x_node *x,
               struct botz_request *q,
               struct botz_response *r)
{
  if (q->q_method != BOTZ_GET) {
    r->r_status = BOTZ_FORBIDDEN;
    return;
  }

  x_printf(&r->r_body, x);
}

void x_child_list_cb(struct x_node *x,
                     struct botz_request *q,
                     struct botz_response *r)
{
  struct x_node *c;

  if (q->q_method != BOTZ_GET) {
    r->r_status = BOTZ_FORBIDDEN;
    return;
  }

  x_for_each_child(c, x)
    n_buf_printf(&r->r_body, "%s\n", c->x_name);
}

void x_type_hash_cb(struct x_type *type,
                    struct botz_request *q,
                    struct botz_response *r)
{
  struct hash_table *t = &type->x_hash_table;
  struct hlist_node *node;
  struct x_node *x;
  size_t i;

  if (q->q_method != BOTZ_GET) {
    r->r_status = BOTZ_FORBIDDEN;
    return;
  }

  for (i = 0; i < (1ULL << t->t_shift); i++)
    hlist_for_each_entry(x, node, t->t_table + i, x_hash_node)
      n_buf_printf(&r->r_body, "%zu %s\n", i, x->x_name);
}

void x_type_info_cb(struct x_type *type,
                    struct botz_request *q,
                    struct botz_response *r)
{
  if (q->q_method != BOTZ_GET) {
    r->r_status = BOTZ_FORBIDDEN;
    return;
  }

  n_buf_printf(&r->r_body,
               "x_type_name: %s\n"
               "x_nr: %zu\n"
               "x_nr_hint: %zu\n"
               "x_type: %d\n"
               "x_which: %d\n",
               type->x_type_name,
               type->x_nr,
               type->x_nr_hint,
               type->x_type,
               type->x_which);
}

struct botz_entry *x_entry_lookup_cb(EV_P_ struct x_node *x,
                                           struct botz_lookup *p,
                                           struct botz_request *q,
                                           struct botz_response *r)
{
  if (p->p_name == NULL)
    return BOTZ_RESPONSE_READY;

  if (strcmp(p->p_name, "_child_list") == 0 && p->p_rest == NULL) {
    x_child_list_cb(x, q, r);
    return BOTZ_RESPONSE_READY;
  }

  if (strcmp(p->p_name, "_info") == 0 && p->p_rest == NULL) {
    x_info_cb(x, q, r);
    return BOTZ_RESPONSE_READY;
  }

  return NULL;
}

struct botz_entry *x_dir_lookup_cb(EV_P_ struct botz_lookup *p,
                                         struct botz_request *q,
                                         struct botz_response *r)
{
  struct x_type *type = p->p_entry->e_data;
  struct x_node *x = x_lookup(type->x_type, p->p_name, NULL, 0);

  if (x != NULL) {
    p->p_name = pathsep(&p->p_rest);
    return x_entry_lookup_cb(EV_A_ x, p, q, r);
  }

  if (p->p_rest != NULL)
    return NULL;

  if (strcmp(p->p_name, "_hash") == 0) {
    x_type_hash_cb(type, q, r);
    return BOTZ_RESPONSE_READY;
  }

  if (strcmp(p->p_name, "_info") == 0) {
    x_type_info_cb(type, q, r);
    return BOTZ_RESPONSE_READY;
  }

  return NULL;
}

void x_dir_get_cb(EV_P_ struct botz_entry *e,
                        struct botz_request *q,
                        struct botz_response *r)
{
  struct x_type *type = e->e_data;
  struct hash_table *t = &type->x_hash_table;
  struct hlist_node *node;
  struct x_node *x;

  size_t i;
  for (i = 0; i < (1ULL << t->t_shift); i++)
    hlist_for_each_entry(x, node, t->t_table + i, x_hash_node)
      n_buf_printf(&r->r_body, "%s\n", x->x_name);
}

static const struct botz_entry_ops x_dir_ops_default = {
  X_DIR_OPS_DEFAULT,
};

int x_dir_init(int i, const struct botz_entry_ops *ops)
{
  const char *name = x_types[i].x_type_name;
  void *data = &x_types[i];

  if (ops == NULL)
    ops = &x_dir_ops_default;

  if (botz_add(&x_listen, name, ops, data) < 0) {
    ERROR("cannot add x_dir `%s': %m\n", name);
    return -1;
  }

  return 0;
}
