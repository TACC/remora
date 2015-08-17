#include "stddef1.h"
#include <stdio.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <malloc.h>
#include <math.h>
#include <ncurses.h>
#include <signal.h>
#include <unistd.h>
#include <ev.h>
#include "xltop.h"
#include "x_node.h"
#include "hash.h"
#include "list.h"
#include "n_buf.h"
#include "screen.h"
#include "string1.h"
#include "trace.h"
#include "curl_x.h"
#include <sys/time.h>

struct xl_host {
  struct hlist_node h_hash_node;
  struct xl_job *h_job;
  char h_name[];
};

struct xl_job {
  struct hlist_node j_hash_node;
  struct list_head j_clus_link;
  char *j_owner, *j_title;
  double j_start;
  size_t j_nr_hosts;
  char j_name[];
};

struct xl_clus {
  struct hlist_node c_hash_node;
  struct list_head c_job_list;
  struct ev_periodic c_w;
  char c_name[];
};

static size_t nr_fs;
static LIST_HEAD(fs_list);
double fs_status_interval = 30;

struct xl_fs {
  struct hlist_node f_hash_node;
  struct list_head f_link;
  struct ev_periodic f_w;
  double f_mds_load[3], f_oss_load[3];
  size_t f_nr_mds, f_nr_mdt, f_max_mds_task;
  size_t f_nr_oss, f_nr_ost, f_max_oss_task;
  size_t f_nr_nid;
  char f_name[];
};

struct xl_k {
  char *k_x[2];
  int k_type[2];
  double k_t; /* Timestamp. */
  double k_pending[NR_STATS];
  double k_rate[NR_STATS]; /* EWMA bytes (or reqs) per second. */
  double k_sum[NR_STATS];
};

struct xl_col {
  char *c_name;
  int (*c_get_s)(struct xl_col *c, struct xl_k *k, char **s, int *n);
  int (*c_get_d)(struct xl_col *c, struct xl_k *k, double *d);
  int (*c_get_z)(struct xl_col *c, struct xl_k *k, size_t *z);
  size_t c_offset, c_scale;
  int c_width, c_right, c_prec;
};

static struct curl_x curl_x;

char *jobid = NULL;

static int show_full_names = 0;
//static int show_fs_status = 1;

static int fs_color_pair = CP_BLACK;
static int top_color_pair = CP_BLACK;
//static int status_color_pair = CP_BLACK;
static char status_bar[256];
static double status_bar_time;

static int scroll_start, scroll_delta;

static struct xl_col top_col[24];
static struct xl_k *top_k;
static size_t top_k_limit = 4096;
static size_t top_k_length;
static char *top_query = NULL;
static double top_interval = 10;
static struct ev_timer top_timer_w;
static N_BUF(top_nb);

static const char *clus_default = XLTOP_CLUS;
static const char *domain_default = XLTOP_DOMAIN;

static struct hash_table xl_hash_table[NR_X_TYPES];

char *query_escape(const char *s)
{
  char *e = malloc(3 * strlen(s) + 1), *p, x[4];
  if (e == NULL)
    return NULL;

  for (p = e; *s != 0; s++) {
    if (isalnum(*s) || *s == '.' || *s == '-' || *s == '~' || *s == '_') {
      *(p++) = *s;
    } else {
      snprintf(x, sizeof(x), "%%%02hhX", (unsigned char) *s);
      *(p++) = x[0];
      *(p++) = x[1];
      *(p++) = x[2];
    }
  }

  *p = 0;

  return e;
}

int query_add(char **s, const char *f, const char *v)
{
  char *f1 = query_escape(f), *v1 = query_escape(v);
  char *s0 = *s, *s1 = NULL;
  int rc = -1;

  if (f1 == NULL || v1 == NULL)
    goto err;

  if (s0 == NULL)
    s1 = strf("%s=%s", f1, v1);
  else
    s1 = strf("%s&%s=%s", s0, f1, v1);

  if (s1 == NULL)
    goto err;

  rc = 0;

 err:

  free(s0);
  *s = s1;

  return rc;
}

int query_addz(char **s, const char *f, size_t n)
{
  char v[3 * sizeof(n) + 1];

  snprintf(v, sizeof(v), "%zu", n);

  return query_add(s, f, v);
}

static int xl_sep(char *s, int *type, char **name)
{
  char *s_type = strsep(&s, ":=");
  int i_type;

  switch (*s_type) {
  case 'h': i_type = X_HOST; break;
  case 'j': i_type = X_JOB; break;
  case 'c': i_type = X_CLUS; break;
  case 'u': i_type = X_U; break;
  case 's': i_type = X_SERV; break;
  case 'f': i_type = X_FS; break;
  case 'v': i_type = X_V; break;
  default:  i_type = x_str_type(s_type); break;
  }

  *type = i_type;
  *name = s;

  if (i_type < 0)
    return -1;

  return 0;
}

int get_x_nr_hint(int type, size_t *hint)
{
  N_BUF(nb);
  char *path = NULL;
  int rc = -1;
  char *m, *k, *v;
  size_t m_len, n = 0;

  path = strf("%s/_info", x_type_name(type));
  if (path == NULL)
    goto out;

  if (curl_x_get(&curl_x, path, NULL, &nb) < 0)
    goto out;

  while (n_buf_get_msg(&nb, &m, &m_len) == 0) {
    if (split(&m, &k, &v, (char **) NULL) != 2)
      continue;

    if (strcmp(k, "x_nr:") == 0)
      n = MAX(n, (size_t) strtoul(v, NULL, 0));

    if (strcmp(k, "x_nr_hint:") == 0)
      n = MAX(n, (size_t) strtoul(v, NULL, 0));
  }

  *hint = n;
  TRACE("type `%s', *hint %zu\n", x_type_name(type), *hint);

  if (n != 0)
    rc = 0;

 out:
  n_buf_destroy(&nb);
  free(path);

  return rc;
}

int xl_hash_init(int type)
{
  size_t hint = 0;

  if (get_x_nr_hint(type, &hint) < 0)
    return -1;

  if (hash_table_init(&xl_hash_table[type], hint) < 0)
    return -1;

  return 0;
}

#define _xl_lookup(p, i, name, xl_type, m_hash_node, m_name, create)    \
  do {                                                                  \
    struct hash_table *_t = &xl_hash_table[(i)];                        \
    struct hlist_head *head;                                            \
    const char *_name = (name);                                         \
    typeof(xl_type) *_p;                                                \
                                                                        \
    _p = str_table_lookup_entry(_t, _name, &head, xl_type,              \
                                m_hash_node, m_name);                   \
    if (_p == NULL && (create)) {                                       \
      _p = malloc(sizeof(*_p) + strlen(_name) + 1);                     \
      if (_p == NULL)                                                   \
        OOM();                                                          \
      memset(_p, 0, sizeof(*_p));                                       \
      strcpy(_p->m_name, _name);                                        \
      hlist_add_head(&_p->m_hash_node, head);                           \
    }                                                                   \
                                                                        \
    (p) = _p;                                                           \
  } while (0)

#define xl_lookup_host(h, name, create) \
  _xl_lookup((h), X_HOST, (name), struct xl_host, h_hash_node, h_name, (create))

#define xl_lookup_job(j, name, create) \
  _xl_lookup((j), X_JOB, (name), struct xl_job, j_hash_node, j_name, (create))

#define xl_lookup_clus(c, name, create) \
  _xl_lookup((c), X_CLUS, (name), struct xl_clus, c_hash_node, c_name, (create))

#define xl_lookup_fs(f, name, create) \
  _xl_lookup((f), X_FS, (name), struct xl_fs, f_hash_node, f_name, (create))

static int xl_clus_msg_cb(struct xl_clus *c, char *m, size_t m_len)
{
  char *s_host, *s_job, *owner, *title, *s_start, *s_nr_hosts;
  struct xl_host *h;
  struct xl_job *j;

  if (split(&m, &s_host, &s_job, &owner, &title, &s_start, &s_nr_hosts,
            (char **) NULL) != 6)
    return 0;

  xl_lookup_host(h, s_host, 1);
  xl_lookup_job(j, s_job, 1);
  h->h_job = j;

  if (j->j_clus_link.next == NULL) {
    INIT_LIST_HEAD(&j->j_clus_link);
    j->j_owner = strdup(owner);
    j->j_title = strdup(title);
    j->j_start = strtod(s_start, NULL);
    j->j_nr_hosts = strtoul(s_nr_hosts, NULL, 0);
  }

  list_move(&j->j_clus_link, &c->c_job_list);

  return 0;
}

static void xl_clus_cb(EV_P_ struct ev_periodic *w, int revents)
{
  struct xl_clus *c = container_of(w, struct xl_clus, c_w);
  struct xl_job *j, *j_tmp;
  char *path = NULL;
  LIST_HEAD(tmp_list);

  TRACE("clus `%s', now %.0f\n", c->c_name, ev_now(EV_A));

  list_splice_init(&c->c_job_list, &tmp_list);

  path = strf("clus/%s", c->c_name);
  if (path == NULL)
    OOM();

  curl_x_get_iter(&curl_x, path, NULL, (msg_cb_t *) &xl_clus_msg_cb, c);

  free(path);

  list_for_each_entry_safe(j, j_tmp, &tmp_list, j_clus_link) {
    hlist_del(&j->j_hash_node);
    list_del(&j->j_clus_link);
    free(j->j_owner);
    free(j->j_title);
    free(j);
  }
}

int xl_clus_add(EV_P_ const char *name)
{
  N_BUF(nb);
  int rc = -1;
  char *info_path = NULL, *m, *k, *v;
  size_t m_len;
  struct xl_clus *c = NULL;
  double c_int = -1, c_off = -1;

  xl_lookup_clus(c, name, 1);

  if (c->c_hash_node.next != NULL)
    return 0;

  info_path = strf("clus/%s/_info", name);
  if (info_path == NULL)
    OOM();

  if (curl_x_get(&curl_x, info_path, NULL, &nb) < 0)
    goto err;

  while (n_buf_get_msg(&nb, &m, &m_len) == 0) {
    if (split(&m, &k, &v, (char **) NULL) != 2)
      continue;

    if (strcmp(k, "interval:") == 0)
      c_int = strtod(v, NULL);
    else if (strcmp(k, "offset:") == 0)
      c_off = strtod(v, NULL);
  }

  if (c_int < 0 || c_off < 0)
    goto err;

  INIT_LIST_HEAD(&c->c_job_list);
  c_off = fmod(c_off + 1, c_int); /* XXX */
  ev_periodic_init(&c->c_w, &xl_clus_cb, c_off, c_int, NULL);
  ev_periodic_start(EV_A_ &c->c_w);
  ev_feed_event(EV_A_ &c->c_w, 0);

  rc = 0;

  if (0) {
  err:
    if (c != NULL)
      hlist_del(&c->c_hash_node);
    free(c);
  }

  n_buf_destroy(&nb);
  free(info_path);

  return rc;
}

static int xl_clus_init(EV_P)
{
  N_BUF(nb);
  int rc = -1;
  char *m, *name;
  size_t m_len;

  if (xl_hash_init(X_CLUS) < 0)
    goto out;

  if (curl_x_get(&curl_x, "clus", NULL, &nb) < 0)
    goto out;

  while (n_buf_get_msg(&nb, &m, &m_len) == 0) {
    if (split(&m, &name, (char **) NULL) != 1)
      continue;

    if (xl_clus_add(EV_A_ name) < 0)
      goto out;
  }

  rc = 0;

 out:
  n_buf_destroy(&nb);

  return rc;
}

static int xl_fs_msg_cb(struct xl_fs *f, char *msg, size_t msg_len)
{
  char *s_serv;
  struct serv_status ss = {};
  int i;

  if (split(&msg, &s_serv, (char **) NULL) != 1 || msg == NULL)
    return 0;

  if (sscanf(msg, SCN_SERV_STATUS_FMT, SCN_SERV_STATUS_ARG(ss)) !=
      NR_SCN_SERV_STATUS_ARGS)
    return 0;

  TRACE("serv `%s', status "PRI_SERV_STATUS_FMT"\n",
        s_serv, PRI_SERV_STATUS_ARG(ss));

  /* TODO Detect and skip stale status. */

  if (ss.ss_nr_mdt > 0) {
    for (i = 0; i < 3; i++)
      f->f_mds_load[i] = MAX(f->f_mds_load[i], ss.ss_load[i]);
    f->f_nr_mds += 1;
    f->f_max_mds_task = MAX(f->f_max_mds_task, ss.ss_nr_task);
  } else if (ss.ss_nr_ost > 0) {
    for (i = 0; i < 3; i++)
      f->f_oss_load[i] = MAX(f->f_oss_load[i], ss.ss_load[i]);
    f->f_nr_oss += 1;
    f->f_max_oss_task = MAX(f->f_max_oss_task, ss.ss_nr_task);
  }
  f->f_nr_mdt += ss.ss_nr_mdt;
  f->f_nr_ost += ss.ss_nr_ost;
  f->f_nr_nid = MAX(f->f_nr_nid, ss.ss_nr_nid);

  return 0;
}

static void xl_fs_cb(EV_P_ struct ev_periodic *w, int revents)
{
  struct xl_fs *f = container_of(w, struct xl_fs, f_w);
  char *status_path = NULL;

  TRACE("fs `%s', now %.0f\n", f->f_name, ev_now(EV_A));

  status_path = strf("fs/%s/_status", f->f_name);
  if (status_path == NULL)
    OOM();

  memset(f->f_mds_load, 0, sizeof(f->f_mds_load));
  memset(f->f_oss_load, 0, sizeof(f->f_oss_load));
  f->f_nr_mds = 0;
  f->f_nr_mdt = 0;
  f->f_max_mds_task = 0;
  f->f_nr_oss = 0;
  f->f_nr_ost = 0;
  f->f_max_oss_task = 0;
  f->f_nr_nid = 0;

  curl_x_get_iter(&curl_x, status_path, NULL, (msg_cb_t *) &xl_fs_msg_cb, f);

  free(status_path);
}

int xl_fs_add(EV_P_ const char *name)
{
  struct xl_fs *f;

  xl_lookup_fs(f, name, 1);

  if (f->f_hash_node.next != NULL)
    return 0;

  list_add(&f->f_link, &fs_list);
  ev_periodic_init(&f->f_w, &xl_fs_cb, 0, fs_status_interval, NULL);
  ev_periodic_start(EV_A_ &f->f_w);
  ev_feed_event(EV_A_ &f->f_w, 0);
  nr_fs++;

  return 0;
}

static int xl_fs_init(EV_P)
{
  N_BUF(nb);
  int rc = -1;
  char *m, *name;
  size_t m_len;

  if (xl_hash_init(X_FS) < 0)
    goto out;

  if (curl_x_get(&curl_x, "fs", NULL, &nb) < 0)
    goto out;

  while (n_buf_get_msg(&nb, &m, &m_len) == 0) {
    if (split(&m, &name, (char **) NULL) != 1)
      continue;

    if (xl_fs_add(EV_A_ name) < 0)
      goto out;
  }

  rc = 0;

 out:
  n_buf_destroy(&nb);

  return rc;
}

static void top_msg_cb(char *msg, size_t msg_len)
{
  int i;
  char *s[2];
  struct xl_k *k = &top_k[top_k_length];

  if (split(&msg, &s[0], &s[1], (char **) NULL) != 2 || msg == NULL)
    return;

  for (i = 0; i < 2; i++)
    if (xl_sep(s[i], &k->k_type[i], &k->k_x[i]) < 0 || k->k_x[i] == NULL)
      return;

  if (sscanf(msg, "%lf "SCN_K_STATS_FMT, &k->k_t, SCN_K_STATS_ARG(k)) !=
      1 + NR_K_STATS)
    return;

  TRACE("%s %s "PRI_STATS_FMT("%f")"\n",
        k->k_x[0], k->k_x[1], PRI_STATS_ARG(k->k_rate));

  top_k_length++;
}

static void top_timer_cb(EV_P_ ev_timer *w, int revents)
{
  double now = ev_now(EV_A);
  char *msg;
  size_t msg_len;

  TRACE("begin, now %f\n", now);

  top_k_length = 0;
  n_buf_destroy(&top_nb);

  if (curl_x_get(&curl_x, "top", top_query, &top_nb) < 0)
    return;

  while (n_buf_get_msg(&top_nb, &msg, &msg_len) == 0)
    top_msg_cb(msg, msg_len);

  status_bar_time = 0;
  screen_refresh(EV_A);
}

int c_get_x(struct xl_col *c, struct xl_k *k, char **s, int *n)
{
  char *x = k->k_x[c->c_offset];
  int t = k->k_type[c->c_offset];

  *s = x;

  if (show_full_names || (t == X_HOST && isdigit(*x)))
    *n = strlen(x);
  else
    *n = strcspn(x, "@.");

  return 0;
}

struct xl_job *k_get_job(struct xl_k *k)
{
  struct xl_host *h;
  struct xl_job *j = NULL;

  if (k->k_type[0] == X_HOST) {
    xl_lookup_host(h, k->k_x[0], 0);
    if (h != NULL)
      j = h->h_job;
  } else if (k->k_type[0] == X_JOB) {
    xl_lookup_job(j, k->k_x[0], 0);
  }

  return j;
}

int c_get_owner(struct xl_col *c, struct xl_k *k, char **s, int *n)
{
  struct xl_job *j = k_get_job(k);

  if (j == NULL)
    return -1;

  *s = j->j_owner;
  *n = strlen(j->j_owner);

  return 0;
}

int c_get_title(struct xl_col *c, struct xl_k *k, char **s, int *n)
{
  struct xl_job *j = k_get_job(k);

  if (j == NULL)
    return -1;

  *s = j->j_title;
  *n = strlen(j->j_title);

  return 0;
}

int c_get_jobid(struct xl_col *c, struct xl_k *k, char **s, int *n)
{
  struct xl_job *j = k_get_job(k);

  if (j == NULL)
    return -1;

  *s = j->j_name;

  if (show_full_names)
    *n = strlen(j->j_name);
  else
    *n = strcspn(j->j_name, "@.");

  return 0;
}

int c_get_nr_hosts(struct xl_col *c, struct xl_k *k, size_t *z)
{
  struct xl_host *h;
  struct xl_job *j = NULL;

  if (k->k_type[0] == X_HOST) {
    xl_lookup_host(h, k->k_x[0], 0);
    if (h != NULL)
      j = h->h_job;
  } else if (k->k_type[0] == X_JOB) {
    xl_lookup_job(j, k->k_x[0], 0);
  }

  if (j == NULL)
    return -1;

  *z = j->j_nr_hosts;
  return 0;
}

int c_get_d(struct xl_col *c, struct xl_k *k, double *d)
{
  *d = *(double *) (((char *) k) + c->c_offset);

  return 0;
}

void c_print_file(FILE* pFile, int y, int x, struct xl_col *c, struct xl_k *k)
{
  if (c->c_get_s != NULL) {
    char *s = NULL;
    int n = 0;

    if ((*c->c_get_s)(c, k, &s, &n) < 0 || s == NULL) {
      return;
    }

    n = MIN(n, c->c_width);

    fprintf (pFile, "\t%s", s);
//    mvprintw(y, x, "%-*.*s", c->c_width, n, s);
  } else if (c->c_get_d != NULL) {
    double d;

    if ((*c->c_get_d)(c, k, &d) < 0) {
      return;
    }

    if (c->c_scale > 0)
      d /= c->c_scale;

    fprintf (pFile, "\t%f", d);
//    mvprintw(y, x, "%*.*f", c->c_width, c->c_prec, d);
  } else if (c->c_get_z != NULL) {
    size_t z;

    if ((*c->c_get_z)(c, k, &z) < 0) {
      return;
    }

    if (c->c_scale > 0)
      z /= c->c_scale;

    fprintf (pFile, "\t%zu\n", z);
//    mvprintw(y, x, "%*zu", c->c_width, z);
  }
}

void c_print(int y, int x, struct xl_col *c, struct xl_k *k)
{
  if (c->c_get_s != NULL) {
    char *s = NULL;
    int n = 0;

    if ((*c->c_get_s)(c, k, &s, &n) < 0 || s == NULL)
      return;

    n = MIN(n, c->c_width);

    mvprintw(y, x, "%-*.*s", c->c_width, n, s);
  } else if (c->c_get_d != NULL) {
    double d;

    if ((*c->c_get_d)(c, k, &d) < 0)
      return;

    if (c->c_scale > 0)
      d /= c->c_scale;

    mvprintw(y, x, "%*.*f", c->c_width, c->c_prec, d);
  } else if (c->c_get_z != NULL) {
    size_t z;

    if ((*c->c_get_z)(c, k, &z) < 0)
      return;

    if (c->c_scale > 0)
      z /= c->c_scale;

    mvprintw(y, x, "%*zu", c->c_width, z);
  }
}

#define COL_X(name,which,width) ((struct xl_col) { \
    .c_name = (name), \
    .c_width = (width), \
    .c_get_s = &c_get_x, \
    .c_offset = (which), \
  })

#define COL_HOST  COL_X("HOST",  0, 15)
#define COL_JOB   COL_X("JOB",   0, (show_full_names ? 15 : 8))
#define COL_CLUS  COL_X("CLUS",  0, 15)
#define COL_U     COL_X("ALL",     0,  5)
#define COL_SERV  COL_X("SERV",  1, (show_full_names ? 15 : 8))
#define COL_FS    COL_X("FS",    1, 15)
#define COL_V     COL_X("ALL",     1,  5)

#define COL_D(name,mem,width,scale,prec) ((struct xl_col) { \
    .c_name = (name),                       \
    .c_width = (width),                     \
    .c_get_d = &c_get_d,                    \
    .c_offset = offsetof(struct xl_k, mem), \
    .c_scale = (scale),                     \
    .c_right = 1,                           \
    .c_prec = (prec),                       \
  })

#define COL_MB_RATE(name,mem) COL_D(name, mem, 10, 1048576, 3)
#define COL_MB_SUM(name,mem) COL_D(name, mem, 10, 1048576, 0)

#define COL_WR_MB_RATE COL_MB_RATE("WR_MB/S", k_rate[STAT_WR_BYTES])
#define COL_RD_MB_RATE COL_MB_RATE("RD_MB/S", k_rate[STAT_RD_BYTES])
#define COL_REQS_RATE  COL_D("REQS/S", k_rate[STAT_NR_REQS], 10, 1, 3)

#define COL_WR_MB_SUM COL_MB_SUM("WR_MB", k_sum[STAT_WR_BYTES])
#define COL_RD_MB_SUM COL_MB_SUM("RD_MB", k_sum[STAT_RD_BYTES])
#define COL_REQS_SUM  COL_D("REQS", k_sum[STAT_NR_REQS], 10, 1, 0)

#define COL_JOBID ((struct xl_col) { \
  .c_name = "JOBID", .c_width = 10, .c_get_s = &c_get_jobid })

#define COL_OWNER ((struct xl_col) { \
  .c_name = "OWNER", .c_width = 10, .c_get_s = &c_get_owner })

#define COL_TITLE ((struct xl_col) { \
  .c_name = "NAME", .c_width = 10, .c_get_s = &c_get_title })

#define COL_NR_HOSTS ((struct xl_col) { \
  .c_name = "HOSTS", .c_width = 5, .c_get_z = &c_get_nr_hosts })

void status_bar_vprintf(EV_P_ const char *fmt, va_list args)
{
  vsnprintf(status_bar, sizeof(status_bar), fmt, args);
  status_bar_time = ev_now(EV_A);
  screen_refresh(EV_A);
}

void status_bar_printf(EV_P_ const char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);
  status_bar_vprintf(EV_A_ fmt, args);
  va_end(args);
}

void error_printf(const char *prog, const char *func, int line,
                  const char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);

  if (screen_is_active) {
    status_bar_vprintf(EV_DEFAULT, fmt, args);
  } else {
    fprintf(stderr, "%s: ", prog);
    vfprintf(stderr, fmt, args);
  }
  va_end(args);
}

static void screen_key_cb(EV_P_ int key)
{
  switch (tolower(key)) {
  case ' ':
  case '\n':
    ev_feed_event(EV_A_ &top_timer_w, EV_TIMER);
    break;
  case 'q':
    ev_break(EV_A_ EVBREAK_ALL); // XXX 
    return;
  case KEY_DOWN:
    scroll_delta += 1;
    break;
  case KEY_HOME:
    scroll_delta = INT_MIN / 2;
    break;
  case KEY_END:
    scroll_delta = INT_MAX / 2;
    break;
  case KEY_UP:
    scroll_delta -= 1;
    break;
  case KEY_NPAGE:
    scroll_delta += LINES;
    break;
  case KEY_PPAGE:
    scroll_delta -= LINES;
    break;
  default:
    if (isascii(key)) {
      status_bar_time = ev_now(EV_A);
      snprintf(status_bar, sizeof(status_bar), "unknown command `%c'", key);
    }
    break;
  }

  screen_refresh(EV_A);
}

static void print_k_file(FILE* pFile, int line, struct xl_col *c, struct xl_k *k)
{
  int i;
  for (i = 0; c->c_name != NULL; c++) {
    c_print_file(pFile, line, i, c, k);
    i += c->c_width + 2;
  }
}
/*
static void print_k(int line, struct xl_col *c, struct xl_k *k)
{
  int i;
  for (i = 0; c->c_name != NULL; c++) {
    c_print(line, i, c, k);
    i += c->c_width + 2;
  }
}
*/


static void fix_output_format (void) {
  char input[100], output[100];
  FILE* pFileIn, *pFileOut;
  char* line = NULL;
  size_t len = 0;
  ssize_t read;
  size_t nLines;
  char systems[100][40]; //Max 100 systems
  char wr[100][40];
  char rd[100][40];
  char reqs[100][40];
  char dump [40];
  int count = 0;
  long long unsigned int seconds;
  struct timeval tp;

  gettimeofday(&tp, NULL);
  seconds = tp.tv_sec;

  sprintf (input, "/tmp/jobstats_%s/xltop_.txt", jobid);
  struct stat st = {0};
  char folder[100];

  // Make sure that the folder actually exists (it should, but just making sure)
  sprintf (folder, "jobstats_%s", jobid);

  if (stat(folder, &st) == -1) {
    mkdir(folder, 0700);
  }

  sprintf (output, "./jobstats_%s/xltop.txt", jobid);
  pFileIn = fopen (input, "r");
  
  while ((read = getline(&line, &len, pFileIn)) !=-1) {
    sscanf (line, "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s", dump, systems[count], wr[count], rd[count], reqs[count], dump, dump, dump); 
    count++;
  }
  
  int i;
  nLines = count;
  if (!nLines)
    return;

  if( access( output, F_OK ) != -1 )
    pFileOut = fopen (output, "a");
  else {
    pFileOut = fopen (output, "w");
 
    for (i=0; i<nLines; ++i)
      if (i==0)
        fprintf (pFileOut, "Time\t\tSystem\t\tWR_MB/S\t\tRD_MB/S\t\tREQS/S\t\t");
      else
        fprintf (pFileOut, "System\t\tWR_MB/S\t\tRD_MB/S\t\tREQS/S\t\t");
    fprintf (pFileOut, "\n");
  }

  for (i=0; i<nLines; ++i) {
    if (i==0)
      fprintf (pFileOut, "%llu\t%.20s\t%.20s\t%.20s\t%.20s\t", seconds, systems[i], wr[i], rd[i], reqs[i]);
    else
      fprintf (pFileOut, "%.20s\t%.20s\t%.20s\t%.20s\t", systems[i], wr[i], rd[i], reqs[i]);
  }
  fprintf (pFileOut,"\n");
  fclose(pFileIn);
  fclose(pFileOut);
  remove (input);
}

static void screen_refresh_cb_file(EV_P_ int LINES, int COLS)
{
  int line = 1;

  int new_start = scroll_start + scroll_delta;
  int max_start = top_k_length - (LINES - line - 1);

  new_start = MIN(new_start, max_start);
  new_start = MAX(new_start, 0);

  int j = new_start;
  FILE* pFile;

  char filename[100];
  char folder[100];

  //Use /tmp for an intermediate file
  //Make sure the folder exists
  sprintf (folder, "/tmp/jobstats_%s", jobid);
  struct stat st = {0};
  
  if (stat(folder, &st) == -1) {
        mkdir(folder, 0700);
  }

  sprintf (filename, "/tmp/jobstats_%s/xltop_.txt", jobid);
  pFile = fopen (filename, "w");
  for (; j < (int) top_k_length && line < LINES - 1; j++, line++)
    print_k_file(pFile, line, top_col, &top_k[j]);
 
  fclose(pFile);

  fix_output_format ();
  
}

static char *make_top_query(int t[2], char *x[2], int d[2], size_t limit,
                            char *sort_key, char *owner)
{
  char *q = NULL, *s[2] = { NULL };

  int i;
  for (i = 0; i < 2; i++) {
    s[i] = strf("%s:%s", x_type_name(t[i]), x[i]);
    if (s[i] == NULL)
      OOM();
  }

  if (query_add(&q, "x0", s[0]) < 0)
    goto err;
  if (query_addz(&q, "d0", d[0]) < 0)
    goto err;

  if (query_add(&q, "x1", s[1]) < 0)
    goto err;
  if (query_addz(&q, "d1", d[1]) < 0)
    goto err;

  if (query_addz(&q, "limit", limit) < 0)
    goto err;

  if (sort_key != NULL && query_add(&q, "sort", sort_key) < 0)
    goto err;

  if (owner != NULL && query_add(&q, "owner", owner) < 0)
    goto err;

  TRACE("q `%s'\n", q);

  if (0) {
  err:
    free(q);
    q = NULL;
  }

  free(s[0]);
  free(s[1]);

  return q;
}

static char *parse_sort_key(const char *key, int want_sums)
{
  char *dup = strdup(key), *pos = dup;
  char *k = NULL;

  while (pos != NULL) {
    char *s = strsep(&pos, ",");
    int is_rate = (strchr(s, '/') != NULL) || !want_sums;

    while (isspace(*s))
      s++;

    if (*s == 0)
      continue;

#define K_ADD(fmt,args...) do {                 \
    if (k == NULL) {                            \
      k = strf(fmt, ##args);                    \
    } else {                                    \
      char *_k = strf("%s," fmt, k, ##args);    \
      free(k);                                  \
      k = _k;                                   \
    }                                           \
  } while (0)

    if (tolower(*s) == 'w')
      K_ADD("%c%d", is_rate ? 'r' : 's', STAT_WR_BYTES);
    else if (strchr(s, 'q') != NULL || strchr(s, 'Q') != NULL)
      K_ADD("%c%d", is_rate ? 'r' : 's', STAT_NR_REQS);
    else
      K_ADD("%c%d", is_rate ? 'r' : 's', STAT_RD_BYTES);

#undef K_ADD

  }

  free(dup);

  TRACE("key `%s', k `%s'\n", key, k != NULL ? k : "NULL");

  return k;
}

static void print_help(void)
{
  const char *p = program_invocation_short_name;

  printf("Usage: %s [OPTION]... [EXPRESSION...]\n"
	 "Expression may be one of:\n"
	 " owner=OWNER, clus[=CLUS], job[=JOB], host[=HOST], fs[=FS], serv[=SERV]\n"
	 "Types may be given by their first character; use 'u' and 'v' for the universes.\n"
	 "\nOPTIONS:\n"
	 " -c, --config=DIR_OR_FILE    read configuration from DIR_OR_FILE\n"
	 " -f, --full-names            show full host, job, and server names\n"
	 " -h, --help                  display this help and exit\n"
	 " -i, --interval=SECONDS      update every SECONDS sceonds\n"
	 " -k, --key=KEY1[,KEY2...]    sort results by KEY1,...\n"
	 " -l, --limit=NUM             limit responses to NUM results\n"
	 " -m, --master=HOST-OR-ADDR   connect to xltop-master on HOST-OR-ADDR (default %s)\n"
	 " -p, --port=PORT             connect to xltop-master at PORT (default %s)\n"
	 " -s, --sum                   show sums rather than rates\n"
	 " -u, --ubuntu                look snazzy on my terminal (terrible on xterms)\n"
	 " -v, --version               display version information and exit\n"
	 "\nSORTING:\n"
	 " Sort keys are case insensitive.  Keys containing 'W' select bytes written\n"
	 " (as a rate or sum according to use of -s, --show-sum).  Similarly keys\n"
	 " containing 'R' and 'Q' are interpreted as bytes read and requests.\n"
	 "\nEXAMPLES:\n"
	 " %s job serv (or %s j s) # Show traffic between jobs and servers\n"
	 " %s h=i101-101.ranger.tacc.utexas.edu # Show i101-101 to each filesystem\n"
	 " %s o=kbdcat h s # Show hosts in kbdcat's jobs to /scratch servers\n"
	 " %s h fs=ranger-scratch s # All hosts to /scratch servers\n"
	 " %s h s=oss23.ranger.tacc.utexas.edu # All hosts to oss23\n"
	 " %s j=2411369@ranger s # Job 2411369 to all servers\n"
	 "\nReport bugs to <%s>.\n"
	 , p, str_or(XLTOP_MASTER, "NONE"), XLTOP_PORT,
	 p, p, p, p, p, p, p, PACKAGE_BUGREPORT);
}

static void print_version(void)
{
  printf("%s (%s) %s\n", program_invocation_short_name,
	 PACKAGE_NAME, PACKAGE_VERSION);
}



int main(int argc, char *argv[])
{
  const char *conf_arg = NULL;
  const char *m_host = XLTOP_MASTER, *m_port = XLTOP_PORT;
  char *sort_key = NULL;
  int want_sum = 0;
  char jobStampede[30];

  //Only allows the code to check for the current job of the user
  jobid=getenv ("SLURM_JOB_ID");
  if (jobid==NULL)
    FATAL ("No job running\n");

  //Hardcode this as it were an argument passed to the program.
  //This way we don't have to understand what's going on in the rest of the code.
  snprintf (jobStampede, 30, "j=%s@stampede", jobid);
  argc++;
  char **newargv = malloc (argc*sizeof (*newargv));
  memmove (newargv, argv, sizeof(*newargv)*(argc-1));
  newargv[argc-1] = malloc (strlen(jobStampede)*sizeof(char));
  strcpy (newargv[argc-1],jobStampede);
  argv=newargv;
  //Now the arguments have been modified. Continue as normal

  struct option opts[] = {
    { "job",         0, NULL, 'j' },
    { "config",      1, NULL, 'c' },
    { "full-names",  0, NULL, 'f' },
    { "help",        0, NULL, 'h' },
    { "interval",    1, NULL, 'i' },
    { "key",         1, NULL, 'k' },
    { "limit",       1, NULL, 'l' },
    { "master",      1, NULL, 'm' },
    { "port",        1, NULL, 'p' },
    { "sum",         0, NULL, 's' },
    { "ubuntu",      0, NULL, 'u' },
    { "version",     0, NULL, 'v' },
    { NULL,          0, NULL,  0  },
  };

  int opt;
  while ((opt = getopt_long(argc, argv, "j:c:fhi:k:l:m:p:suv", opts, 0)) > 0) {
    switch (opt) {
    case 'j':
      jobid = optarg;
      break;
    case 'c':
      conf_arg = optarg;
      break;
    case 'f':
      show_full_names = 1;
      break;
    case 'h':
      print_help();
      exit(EXIT_SUCCESS);
    case 'i':
      top_interval = strtod(optarg, NULL);
      if (top_interval <= 0)
        FATAL("invalid interval `%s'\n", optarg);
      break;
    case 'k':
      sort_key = optarg;
      break;
    case 'l':
      top_k_limit = strtoul(optarg, NULL, 0);
      break;
    case 'm':
      m_host = optarg;
      break;
    case 'p':
      m_port = optarg;
      break;
    case 's':
      want_sum = 1;
      break;
    case 'u':
      fs_color_pair = CP_YELLOW;
      top_color_pair = CP_MAGENTA;
      break;
    case 'v':
      print_version();
      exit(EXIT_SUCCESS);
    case '?':
      FATAL("Try `%s --help' for more information.\n", program_invocation_short_name);
    }
  }

  if (str_is_set(conf_arg))
    /* TODO */;

  if (top_interval <= 0)
    FATAL("invalid interval %f, must be positive\n", top_interval);

  if (top_k_limit <= 0)
    FATAL("invalid limit %zu, must be positive\n", top_k_limit);

  if (!str_is_set(m_host))
    FATAL("no host or address specified for xltop-master\n");

  if (!str_is_set(m_port))
    FATAL("no port specified for xltop-master\n");

  int curl_rc = curl_global_init(CURL_GLOBAL_NOTHING);
  if (curl_rc != 0)
    FATAL("cannot initialize curl: %s\n", curl_easy_strerror(curl_rc));

  if (curl_x_init(&curl_x, m_host, m_port) < 0)
    FATAL("cannot initialize curl handle: %m\n");

  if (sort_key != NULL)
    sort_key = parse_sort_key(sort_key, want_sum);

  /* Parse top spec. */
  char *x[2] = { "ALL", "ALL" };
  int t[2] = { X_U, X_V };
  int c[2] = { X_JOB, X_FS };
  char *owner = NULL;

  char *x_set[NR_X_TYPES] = { };
  int t_set[NR_X_TYPES] = { };

  int i;
  for (i = optind; i < argc; i++) {
    char *s = argv[i];
    char *s_type = strsep(&s, ":=");
    int ti;
    char *xi;

    switch (*s_type) {
    case 'h': ti = X_HOST; break;
    case 'j': ti = X_JOB; break;
    case 'c': ti = X_CLUS; break;
    case 'u': ti = X_U; break;
    case 's': ti = X_SERV; break;
    case 'f': ti = X_FS; break;
    case 'v': ti = X_V; break;
    case 'o': owner = s; continue;
    default:  ti = x_str_type(s_type); break;
    }

    if (ti < 0)
      FATAL("unrecognized type `%s'\n", s_type);

    xi = s;

    TRACE("ti `%s', xi `%s'\n", x_type_name(ti), xi);

    t_set[ti] = 1;
    if (xi != NULL)
      x_set[ti] = xi;
  }
  for (i = X_U; i >= X_HOST; i--) {
    if (t_set[i])
      c[0] = i;
    if (x_set[i] != NULL) {
      x[0] = x_set[i];
      t[0] = i;
    }
  }

  for (i = X_V; i >= X_SERV; i--) {
    if (t_set[i])
      c[1] = i;
    if (x_set[i] != NULL) {
      x[1] = x_set[i];
      t[1] = i;
    }
  }

  /* Fully qualify host, serv, job if needed. */
  if (t[0] == X_HOST) {
    if (strchr(x[0], '.') == NULL) {
      /* TODO Try to pull domain from clus if given. */
      if (str_is_set(domain_default))
        x[0] = strf("%s.%s", x[0], domain_default);
      else
        FATAL("invalid host `%s': must specify a fully qualified domain name\n",
              x[0]); /* XXX */
    }
  } else if (t[0] == X_JOB) {
    if (strchr(x[0], '@') == NULL) {
      if (x_set[X_CLUS] != NULL)
        x[0] = strf("%s@%s", x[0], x_set[X_CLUS]);
      else if (str_is_set(clus_default))
        x[0] = strf("%s@%s", x[0], clus_default);
      else
        FATAL("must specify job as JOBID@CLUS or pass clus=CLUS\n");
    }
  }

  if (t[1] == X_SERV) {
    if (strchr(x[1], '.') == NULL) {
      /* TODO Try to pull domain from clus if given. */
      if (str_is_set(domain_default))
        x[1] = strf("%s.%s", x[1], domain_default);
      else
        FATAL("invalid serv `%s: must specify a fully qualified domain name\n",
              x[1]); /* XXX */
    }
  }

  int d[2] = { t[0] - c[0], t[1] - c[1] };

  top_query = make_top_query(t, x, d, top_k_limit, sort_key, owner);
  if (top_query == NULL)
    FATAL("cannot initialize top query: %m\n");

  /* Initialize columns. */

  top_col[0] = c[0] == X_HOST ? COL_HOST : c[0] == X_JOB ? COL_JOB :
    c[0] == X_CLUS ? COL_CLUS: COL_U;
  top_col[1] = c[1] == X_SERV ? COL_SERV : c[1] == X_FS ? COL_FS : COL_V;
  top_col[2] = want_sum ? COL_WR_MB_SUM : COL_WR_MB_RATE;
  top_col[3] = want_sum ? COL_RD_MB_SUM : COL_RD_MB_RATE;
  top_col[4] = want_sum ? COL_REQS_SUM : COL_REQS_RATE;

  if (c[0] == X_HOST) {
    top_col[5] = COL_JOBID;
    top_col[6] = COL_OWNER;
    top_col[7] = COL_TITLE;
  } else if (c[0] == X_JOB) {
    top_col[5] = COL_OWNER;
    top_col[6] = COL_TITLE;
    top_col[7] = COL_NR_HOSTS;
    /* TODO Run time. */
  }

  top_k = calloc(top_k_limit, sizeof(top_k[0]));
  if (top_k == NULL)
    OOM();

  if (xl_hash_init(X_HOST) < 0)
    FATAL("cannot initialize host table\n");

  if (xl_hash_init(X_JOB) < 0)
    FATAL("cannot initialize job table\n");

  if (xl_fs_init(EV_DEFAULT) < 0)
    FATAL("cannot initialize fs data\n");

  if (xl_clus_init(EV_DEFAULT) < 0)
    FATAL("cannot initialize cluster data\n");

  signal(SIGPIPE, SIG_IGN);
  
//  ev_timer_init(&top_timer_w, &top_timer_cb, 0.1, 0.);
  ev_timer_init(&top_timer_w, &top_timer_cb, 0.1, top_interval);
  ev_timer_start(EV_DEFAULT_ &top_timer_w);

  screen_init(&screen_refresh_cb_file, 1.0);
  //screen_init(&screen_refresh_cb, 1.0);
  screen_set_key_cb(&screen_key_cb);
  screen_start(EV_DEFAULT);

 // ev_run(EV_DEFAULT_ 0);
  // Wait for the first event, then exit
  ev_run (EV_DEFAULT_ EVRUN_ONCE);
  screen_stop(EV_DEFAULT);

  curl_x_destroy(&curl_x);
  curl_global_cleanup();

  free(newargv);
  return 0;
}
