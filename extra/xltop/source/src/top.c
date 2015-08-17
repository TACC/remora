#include "stddef1.h"
#include <ev.h>
#include <unistd.h>
#include <ctype.h>
#include "botz.h"
#include "string1.h"
#include "x_node.h"
#include "k_heap.h"
#include "xltop.h"
#include "trace.h"
#include "query.h"
#include "job.h"

#define TOP_LIMIT_MAX ((size_t) 4096)

int q_x_parse(struct query *q, char *s)
{
  q->q_u.u_void_p = x_lookup_str(s);
  return 0;
}

static inline void k_top_spec_init(struct k_top *t)
{
  size_t i, n = 0;

  memset(t, 0, sizeof(*t));

  for (i = 0; i < NR_STATS && n < T_SPEC_LEN; i++)
    t->t_spec[n++] = offsetof(struct k_node, k_rate[i]);

  for (i = 0; i < NR_STATS && n < T_SPEC_LEN; i++)
    t->t_spec[n++] = offsetof(struct k_node, k_sum[i]);

  for (i = 0; i < NR_STATS && n < T_SPEC_LEN; i++)
    t->t_spec[n++] = offsetof(struct k_node, k_pending[i]);

  if (n < T_SPEC_LEN)
    t->t_spec[n++] = offsetof(struct k_node, k_t);
}

int q_k_top_parse(struct query *q, char *s)
{
  struct k_top *t = q->q_u.u_void_p;
  size_t n = 0;

  while (n < T_SPEC_LEN && s != NULL) {
    char *u = strsep(&s, ",");
    size_t i = atoi(u + 1);

    if (!(i < NR_STATS))
      return -1;

    switch (*u) {
    case 'p':
      t->t_spec[n++] = offsetof(struct k_node, k_pending[i]);
      break;
    case 'r':
      t->t_spec[n++] = offsetof(struct k_node, k_rate[i]);
      break;
    case 's':
      t->t_spec[n++] = offsetof(struct k_node, k_sum[i]);
      break;
    case 't':
      t->t_spec[n++] = offsetof(struct k_node, k_t);
      break;
    default:
      return -1;
    }
  }

  if (s != NULL)
    return -1;

  while (n < T_SPEC_LEN)
    t->t_spec[n++] = -1;

  return 0;
}

static int k_heap_filt_owner(struct k_heap *h, struct k_node *k)
{
  struct x_node *x = k->k_x[0];

  if (x_is_job(x)) {
    struct job_node *j = container_of(x, struct job_node, j_x);
    struct k_top *t = container_of(h, struct k_top, t_h);

    return (strcmp(j->j_owner, t->t_owner) == 0) ? 1 : -1;
  }

  return 0;
}

static void top_query_cb(EV_P_ struct botz_response *r,
                         struct x_node *x0, struct x_node *x1,
                         size_t d0, size_t d1, size_t limit,
                         struct k_top *t, char *owner)
{
  struct k_heap *h = &t->t_h;
  k_heap_filt_t *filt = NULL;
  size_t i;

  memset(h, 0, sizeof(*h));

  if (x0 == NULL) {
    r->r_status = BOTZ_NOT_FOUND;
    goto out;
  }

  if (x1 == NULL) {
    r->r_status = BOTZ_NOT_FOUND;
    goto out;
  }

  TRACE("x0 `%s', x1 `%s', d0 %zu, d1 %zu, limit %zu\n",
        x0->x_name, x1->x_name, d0, d1, limit);

  /* TODO AUTH. */

  if (limit == 0)
    limit = TOP_LIMIT_MAX;
  else
    limit = MIN(limit, TOP_LIMIT_MAX);

  if (k_heap_init(h, limit) < 0) {
    r->r_status = BOTZ_INTERVAL_SERVER_ERROR;
    goto out;
  }

  if (owner != NULL) {
    t->t_owner = owner;
    filt = &k_heap_filt_owner;
  }

  k_heap_top(h, x0, d0, x1, d1, filt, &k_top_cmp, ev_now(EV_A));
  k_heap_order(h, &k_top_cmp);

  for (i = 0; i < h->h_count; i++) {
    struct k_node *k = h->h_k[i];
    n_buf_printf(&r->r_body, PRI_K_NODE_FMT"\n", PRI_K_NODE_ARG(k));
  }

 out:
  k_heap_destroy(h);
}

static void top_get_cb(EV_P_ struct botz_entry *e,
                             struct botz_request *q,
                             struct botz_response *r)
{
  struct k_top top;

  k_top_spec_init(&top);

#define TOP_QUERY(X, Q)                            \
  X(Q, 0, void_p, x0,    NULL, q_x_parse,      1), \
  X(Q, 1, void_p, x1,    NULL, q_x_parse,      1), \
  X(Q, 2, size,   d0,    0,    q_size_parse,   0), \
  X(Q, 3, size,   d1,    0,    q_size_parse,   0), \
  X(Q, 4, size,   limit, 0,    q_size_parse,   0), \
  X(Q, 5, void_p, sort,  &top, q_k_top_parse,  0), \
  X(Q, 6, string, owner, NULL, q_string_parse, 0)

  DEFINE_QUERY(TOP_QUERY, top_query);

  if (QUERY_PARSE(TOP_QUERY, top_query, q->q_query) < 0) {
    r->r_status = BOTZ_BAD_REQUEST;
    return;
  }

  top_query_cb(EV_A_ r, QUERY_VALUES(TOP_QUERY, top_query));
}

const struct botz_entry_ops top_entry_ops = {
  .o_method = {
    [BOTZ_GET] = &top_get_cb,
  }
};
