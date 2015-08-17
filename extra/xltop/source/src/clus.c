#include <ev.h>
#include "stddef1.h"
#include "string1.h"
#include "clus.h"
#include "job.h"
#include "sub.h"
#include "trace.h"
#include "x_botz.h"

#define CLUS_0_NAME "NONE"
#define IDLE_JOBID "IDLE"

static struct clus_node *clus_0; /* Default/unknown cluster. */
static struct hash_table domain_clus_table;

int clus_add_domain(struct clus_node *c, const char *domain)
{
  TRACE("adding domain `%s' to cluster `%s'\n", domain , c->c_x.x_name);

  return str_table_set(&domain_clus_table, domain, c);
}

static void domains_get_cb(EV_P_ struct botz_entry *e,
                           struct botz_request *q,
                           struct botz_response *r)
{
  struct n_buf *nb = &r->r_body;
  struct hlist_node *n = NULL;
  size_t i = 0;
  char *d = NULL;
  struct clus_node *c;

  while (str_table_for_each(&domain_clus_table, &i, &n, &d, (void **) &c))
    n_buf_printf(nb, "%s %s\n", d, c->c_x.x_name);
}

const struct botz_entry_ops domains_entry_ops = {
  .o_method = {
    [BOTZ_GET] = &domains_get_cb,
  }
};

struct clus_node *clus_lookup(const char *name, int flags)
{
  size_t hash;
  struct hlist_head *head;
  struct x_node *x;
  struct clus_node *c = NULL;
  char *idle_job_name = NULL;

  x = x_lookup_hash(X_CLUS, name, &hash, &head);
  if (x != NULL)
    return container_of(x, struct clus_node, c_x);

  if (!(flags & L_CREATE))
    return NULL;

  c = malloc(sizeof(*c) + strlen(name) + 1);
  if (c == NULL)
    goto err;
  memset(c, 0, sizeof(*c));

  size_t n = strlen(IDLE_JOBID) + 1 + strlen(name) + 1;
  idle_job_name = malloc(n);
  if (idle_job_name == NULL)
    goto err;
  snprintf(idle_job_name, n, "%s@%s", IDLE_JOBID, name);

  x_init(&c->c_x, X_CLUS, x_all[0], hash, head, name);

  c->c_idle_job = job_lookup(idle_job_name, &c->c_x, "NONE", "NONE", "0");
  if (c->c_idle_job == NULL)
    ERROR("cluster `%s': cannot create idle job: %m\n", name);
  else
    c->c_idle_job->j_fake = 1;

 err:
  /* ... */

  free(idle_job_name);

  return c;
}

struct clus_node *clus_lookup_for_host(const char *name)
{
  while (1) {
    const char *s = strchr(name, '.');
    if (s == NULL)
      return clus_0;

    name = s + 1;

    struct clus_node *c = str_table_ref(&domain_clus_table, name);

    if (c != NULL)
      return c;
  }
}

static void clus_msg_cb(EV_P_ struct clus_node *c, char *msg)
{
  struct x_node *p, *x;
  struct job_node *j;
  struct sub_node *s, *t;

  char *host_name, *job_name, *owner = "NONE", *title = "NONE", *start = "0";

  TRACE("clus `%s', msg `%s'\n", c->c_x.x_name, msg);

  /* This must stay aligned with output of qhost_j_filer().
       HOST.DOMAIN JOBID@CLUSTER OWNER TITLE START_EPOCH
     Or if host is idle:
       HOST.DOMAIN IDLE@CLUSTER
  */

  if (split(&msg, &host_name, &job_name, &owner, &title, &start,
            (char **) NULL) < 2)
    return;

#if 0
  TRACE("host_name `%s', job_name `%s', owner `%s', title `%s', start `%s'\n",
        host_name, job_name, owner, title, start);
#endif

  j = job_lookup(job_name, &c->c_x, owner, title, start);
  if (j == NULL)
    return;

  x = x_lookup(X_HOST, host_name, &j->j_x, L_CREATE);
  if (x == NULL)
    return;

  p = x->x_parent;
  if (p == &j->j_x)
    return;

  /* Cancel subscriptions on x that are not allowed to access j1. */
  list_for_each_entry_safe(s, t, &x->x_sub_list, s_x_link[x_which(x)])
    if (!sub_may_access(s, &j->j_x))
      sub_cancel(EV_A_ s);

  x_set_parent(x, &j->j_x);

  TRACE("clus set `%s' parent `%s'\n", x->x_name, j->j_x.x_name);

  if (p != NULL && x_is_job(p) && p->x_nr_child == 0)
    job_end(EV_A_ container_of(p, struct job_node, j_x));
}

static void clus_put_cb(EV_P_ struct botz_entry *e,
                        struct botz_request *q,
                        struct botz_response *r)
{
  struct clus_node *c = e->e_data;
  struct n_buf *nb = &q->q_body;
  char *msg;
  size_t msg_len;

  /* TODO AUTH. */

  c->c_modified = ev_now(EV_A);

  TRACE("clus `%s' PUT length %zu, body `%.*s'\n",
        c->c_x.x_name, n_buf_length(nb),
        (int) (n_buf_length(nb) < 40 ? n_buf_length(nb) : 40), nb->nb_buf);

  while (n_buf_get_msg(nb, &msg, &msg_len) == 0)
    clus_msg_cb(EV_A_ c, msg);
}

static void clus_get_cb(EV_P_ struct botz_entry *e,
                        struct botz_request *q,
                        struct botz_response *r)
{
  struct clus_node *c = e->e_data;
  struct n_buf *nb = &r->r_body;
  struct job_node *j;
  struct x_node *hx, *jx;

  x_for_each_child(jx, &c->c_x) {
    const char *owner = "NONE", *title = "NONE";
    double start_time = 0;

    if (x_is_job(jx)) {
      j = container_of(jx, struct job_node, j_x);
      owner = j->j_owner;
      title = j->j_title;
      start_time = j->j_start_time;
    }

    x_for_each_child(hx, jx)
      n_buf_printf(nb, "%s %s %s %s %.0f %zu\n", hx->x_name, jx->x_name,
                   owner, title, start_time, jx->x_nr_child);
  }
}

static void clus_info_cb(struct clus_node *c,
                         struct botz_request *q,
                         struct botz_response *r)
{
  if (q->q_method == BOTZ_GET)
    n_buf_printf(&r->r_body,
                 "clus: %s\n"
                 "interval: %f\n"
                 "offset: %f\n"
                 "modified: %f\n",
                 c->c_x.x_name,
                 c->c_interval,
                 c->c_offset,
                 c->c_modified);
  else
    r->r_status = BOTZ_FORBIDDEN;
}

static struct botz_entry *
clus_entry_lookup_cb(EV_P_ struct botz_lookup *p,
                           struct botz_request *q,
                           struct botz_response *r)
{
  struct clus_node *c = p->p_entry->e_data;

  if (strcmp(p->p_name, "_info") == 0 && p->p_rest == NULL) {
    clus_info_cb(c, q, r);
    x_info_cb(&c->c_x, q, r);
    return BOTZ_RESPONSE_READY;
  }

  return x_entry_lookup_cb(EV_A_ &c->c_x, p, q, r);
}

static const struct botz_entry_ops clus_entry_ops = {
  .o_lookup = &clus_entry_lookup_cb,
  .o_method = {
    [BOTZ_GET] = &clus_get_cb,
    [BOTZ_PUT] = &clus_put_cb,
  }
};

static struct botz_entry *
clus_dir_lookup_cb(EV_P_ struct botz_lookup *p,
                         struct botz_request *q,
                         struct botz_response *r)
{
  struct clus_node *c = clus_lookup(p->p_name, 0);

  if (c != NULL)
    return botz_new_entry(p->p_name, &clus_entry_ops, c);

  return x_dir_lookup_cb(EV_A_ p, q, r);
}

static const struct botz_entry_ops clus_dir_ops = {
  X_DIR_OPS_DEFAULT,
  .o_lookup = &clus_dir_lookup_cb,
};

int clus_type_init(size_t nr_domains)
{
  clus_0 = clus_lookup(CLUS_0_NAME, L_CREATE);
  if (clus_0 == NULL) {
    ERROR("cannot create cluster `%s': %m\n", CLUS_0_NAME);
    return -1;
  }

  if (x_dir_init(X_CLUS, &clus_dir_ops) < 0) {
    ERROR("cannot create cluster resource: %m\n");
    return -1;
  }

  if (hash_table_init(&domain_clus_table, nr_domains) < 0) {
    ERROR("cannot initialize cluster domain table: %m\n");
    return -1;
  }

  return 0;
}
