#include "stddef1.h"
#include "string1.h"
#include "serv.h"
#include "trace.h"
#include "xltop.h"
#include "x_botz.h"

static void fs_status_get_cb(struct x_node *x,
                             struct botz_request *q,
                             struct botz_response *r)
{
  struct x_node *c;
  struct serv_node *s;

  x_for_each_child(c, x) {
    if (!x_is_type(c, X_SERV))
      continue;

    s = container_of(c, struct serv_node, s_x);

    n_buf_printf(&r->r_body, "%s "PRI_SERV_STATUS_FMT"\n",
                 s->s_x.x_name, PRI_SERV_STATUS_ARG(s->s_status));
  }
}

static struct botz_entry *
fs_entry_lookup_cb(EV_P_ struct botz_lookup *p,
                         struct botz_request *q,
                         struct botz_response *r)
{
  struct x_node *x = p->p_entry->e_data;

  if (strcmp(p->p_name, "_status") == 0 && p->p_rest == NULL) {
    if (q->q_method == BOTZ_GET)
      fs_status_get_cb(x, q, r);
    else
      r->r_status = BOTZ_FORBIDDEN;

    return BOTZ_RESPONSE_READY;
  }

  return x_entry_lookup_cb(EV_A_ x, p, q, r);
}

static const struct botz_entry_ops fs_entry_ops = {
  .o_lookup = &fs_entry_lookup_cb,
  .o_method = {
    /* [BOTZ_GET] = &fs_get_cb, */
  }
};

static struct botz_entry *
fs_dir_lookup_cb(EV_P_ struct botz_lookup *p,
                       struct botz_request *q,
                       struct botz_response *r)
{
  struct x_node *x = x_lookup(X_FS, p->p_name, NULL, 0);

  TRACE("name `%s', x %p\n", p->p_name, x);

  if (x != NULL)
    return botz_new_entry(p->p_name, &fs_entry_ops, x);

  return x_dir_lookup_cb(EV_A_ p, q, r);
}

static struct botz_entry_ops fs_dir_ops = {
  X_DIR_OPS_DEFAULT,
  .o_lookup = &fs_dir_lookup_cb,
};

int fs_type_init(void)
{
  return x_dir_init(X_FS, &fs_dir_ops);
}
