#include <stddef.h>
#include <errno.h>
#include <malloc.h>
#include <unistd.h>
#include "botz.h"
#include "hash.h"
#include "list.h"
#include "n_buf.h"
#include "string1.h"
#include "trace.h"

static const struct botz_entry_ops botz_default_ops;

static inline void ev_io_set1(EV_P_ struct ev_io *w, int events)
{
  if (w->events == events &&
      (ev_is_active(w) ? events != 0 : events == 0))
    return;

  ev_io_stop(EV_A_ w);
  ev_io_set(w, w->fd, events);

  if (events != 0)
    ev_io_start(EV_A_ w);
}

#define BOTZ_RESPONSE_PROTOCOL "HTTP/1.1"

static const char *botz_strstatus(int status)
{
  switch (status) {
  case BOTZ_OK:
    return "OK";
  case BOTZ_NO_CONTENT:
    return "No Content";
  case BOTZ_BAD_REQUEST:
    return "Bad Request";
  case BOTZ_FORBIDDEN:
    return "Forbidden";
  case BOTZ_NOT_FOUND:
    return "Not Found";
  case BOTZ_METHOD_NOT_ALLOWED:
    return "Method Not Allowed";
  case BOTZ_REQUEST_TIMEOUT:
    return "Request Timeout";
  case BOTZ_INTERVAL_SERVER_ERROR:
    return "Interval Server Error";
  case BOTZ_NOT_IMPLEMENTED:
    return "Not Implemented";
  default:
    return "Botzed";
  }
}

static void bc_io_cb(EV_P_ struct ev_io *w, int revents);
static void bc_timer_cb(EV_P_ struct ev_timer *w, int revents);
static void bc_close(EV_P_ struct botz_conn *c, int err);
static void bx_reset(EV_P_ struct botz_x *x);

static void bl_listen_cb(EV_P_ struct evx_listen *el, int fd,
                         const struct sockaddr *addr, socklen_t addrlen)
{
  struct botz_listen *bl = container_of(el, struct botz_listen, bl_listen);
  struct botz_conn *c = NULL;

  c = malloc(sizeof(*c));
  if (c == NULL)
    goto err;

  memset(c, 0, sizeof(*c));

  ev_io_init(&c->c_io_w, &bc_io_cb, fd, EV_READ);
  ev_io_start(EV_A_ &c->c_io_w);

  ev_timer_init(&c->c_timer_w, &bc_timer_cb, bl->bl_conn_timeout, 0);
  ev_timer_start(EV_A_ &c->c_timer_w);

  c->c_listen = bl;
  list_add(&c->c_listen_link, &bl->bl_conn_list);

  if (n_buf_init(&c->c_q_buf, bl->bl_q_buf_size) < 0)
    goto err;
  if (n_buf_init(&c->c_r_header, bl->bl_r_header_size) < 0)
    goto err;
  if (n_buf_init(&c->c_r_body, bl->bl_r_body_size) < 0)
    goto err;

  bx_reset(EV_A_ &c->c_x);

  if (0) {
  err:
    if (c != NULL)
      bc_close(EV_A_ c, -1);
  }
}

static void bt_add_entry(struct hash_table *t, struct botz_entry *e)
{
  struct hlist_head *head;
  size_t hash;

  hash = pair_hash(str_hash(e->e_name, 64), /* XXX Stupid 64. */
                   ((size_t) e->e_parent) / sizeof(void *), /* alignof */
                   t->t_shift);

  head = t->t_table + (hash & t->t_mask);

  hlist_add_head(&e->e_node, head);
}

int botz_listen_init(struct botz_listen *bl, size_t nr_entries)
{
  struct botz_entry *e = NULL;

  memset(bl, 0, sizeof(*bl));
  evx_listen_init(&bl->bl_listen, &bl_listen_cb, 128); /* XXX backlog. */
  INIT_LIST_HEAD(&bl->bl_conn_list);
  bl->bl_conn_timeout = 60.0; /* XXX Hard constants. */
  bl->bl_q_buf_size = 1048576;
  bl->bl_r_header_size = 4096;
  bl->bl_r_body_size = 1048576;

  if (hash_table_init(&bl->bl_entry_table, nr_entries) < 0)
    return -1;

  e = bl->bl_root_entry = botz_new_entry("", NULL, NULL);
  if (e == NULL)
    return -1;

  e->e_parent = e; /* Don't need unless we support '..'. */
  bt_add_entry(&bl->bl_entry_table, e);

  return 0;
}

int botz_add(struct botz_listen *bl, const char *path,
             const struct botz_entry_ops *ops, void *data)
{
  struct botz_entry *e;

  e = botz_lookup(bl, path, L_CREATE);
  if (e == NULL)
    return -1;

  e->e_ops = ops;
  e->e_data = data;

  return 0;
}

static void bc_close(EV_P_ struct botz_conn *c, int err)
{
  TRACE("botz_conn_close fd %d\n", c->c_io_w.fd);

  bx_reset(EV_A_ &c->c_x);

  if (c->c_close_cb != NULL)
    (*c->c_close_cb)(EV_A_ c, err);

  ev_io_stop(EV_A_ &c->c_io_w);
  ev_timer_stop(EV_A_ &c->c_timer_w);

  if (!(c->c_io_w.fd < 0))
    close(c->c_io_w.fd);
  c->c_io_w.fd = -1;

  list_del(&c->c_listen_link);
  n_buf_destroy(&c->c_q_buf);
  n_buf_destroy(&c->c_r_header);
  n_buf_destroy(&c->c_r_body);

  free(c);
}

static void bc_timer_cb(EV_P_ struct ev_timer *w, int revents)
{
  struct botz_conn *c = container_of(w, struct botz_conn, c_timer_w);

  TRACE("connection timeout fd %d\n", c->c_io_w.fd);

  bc_close(EV_A_ c, ETIMEDOUT);
}

static void br_write_header(struct botz_response *r, struct n_buf *nb)
{
  if (r->r_status < 0)
    r->r_status = BOTZ_INTERVAL_SERVER_ERROR;

  if (r->r_status == 0)
    r->r_status = BOTZ_OK;

  n_buf_printf(nb, "%s %d %s\r\n",
               BOTZ_RESPONSE_PROTOCOL, r->r_status,
               botz_strstatus(r->r_status));

  if (r->r_status == BOTZ_NO_CONTENT)
    n_buf_clear(&r->r_body);
  else
    n_buf_printf(nb, "Content-Type: %s\r\n" "Content-Length: %zu\r\n",
                 strlen(r->r_body_type) != 0 ? r->r_body_type : "text/plain",
                 n_buf_length(&r->r_body));

  if (r->r_status < 300)
    n_buf_printf(nb, "Access-Control-Allow-Origin: *\r\n");

  if (r->r_close)
    n_buf_printf(nb, "Connection: close\r\n");

  n_buf_printf(nb, "\r\n");
}

static void bx_read_start(EV_P_ struct botz_x *x, char *msg, size_t msg_len);

static void bx_reset(EV_P_ struct botz_x *x)
{
  struct botz_conn *c = container_of(x, struct botz_conn, c_x);

  free(x->x_q.q_path);
  free(x->x_q.q_query);
  memset(x, 0, sizeof(*x));
  x->x_read_cb = &bx_read_start;
  x->x_r.r_body = c->c_r_body;
  n_buf_clear(&x->x_r.r_body);
}

static void bx_error(struct botz_x *x, int status)
{
  /* TODO If status < 0 then use errno. */

  if (status <= 0)
    status = BOTZ_BAD_REQUEST;

  if (x->x_r.r_status < status)
    x->x_r.r_status = status;

  x->x_close = 1;
  x->x_q_ready = 1;
}

static void bq_set_cookie(struct botz_request *q, const char *cookie /*, ... */)
{
  /* ... */
}

static void bx_read_header(EV_P_ struct botz_x *x, char *msg, size_t msg_len)
{
  char *name, *value1 = NULL;

  if (split(&msg, &name, &value1, (char **) NULL) == 0) {
    if (x->x_q_body_len > 0)
      x->x_q_body_wait = 1;
    else
      x->x_q_ready = 1;
    return; /* End of headers. */
  }

  if (value1 == NULL)
    value1 = name + strlen(name);

  if (strcasecmp(name, "Connection:") == 0) {
    if (strcasecmp(value1, "close") == 0)
      x->x_close = 1;
  } else if (strcasecmp(name, "Content-Length:") == 0) {
    x->x_q_body_len = strtoul(value1, NULL, 10);
  } else if (strcasecmp(name, "Cookie:") == 0) {
    bq_set_cookie(&x->x_q, value1);
  } else if (strcasecmp(name, "Expect:") == 0) {
    if (strcasecmp(value1, "100-Continue") == 0)
      x->x_expect_100 = 1;
  }
}

static int bx_parse_method(struct botz_x *x, const char *method)
{
#define X(S)                                    \
  if (strcmp(method, #S) == 0) {                \
    x->x_q.q_method = BOTZ_ ## S;               \
    return 0;                                   \
  }
  X(GET);
  X(POST);
  X(PUT);
  X(DELETE);
#undef X

  if (strcmp(method, "CONNECT") == 0 ||
      strcmp(method, "HEAD") == 0 ||
      strcmp(method, "OPTIONS") == 0 ||
      strcmp(method, "TRACE") == 0)
    bx_error(x, BOTZ_METHOD_NOT_ALLOWED);
  else
    bx_error(x, BOTZ_NOT_IMPLEMENTED);

  return -1;
}

struct botz_entry *
botz_new_entry(const char *name, const struct botz_entry_ops *ops, void *data)
{
  struct botz_entry *e;

  e = malloc(sizeof(*e) + strlen(name) + 1);
  if (e == NULL)
    return NULL;

  if (ops == NULL)
    ops = &botz_default_ops;

  memset(e, 0, sizeof(*e));
  e->e_ops = ops;
  e->e_data = data;
  strcpy(e->e_name, name);

  return e;
}

static int bp_init(struct botz_lookup *p, struct botz_entry *e, const char *s)
{
  memset(p, 0, sizeof(*p));

  if (s == NULL) {
    errno = EINVAL;
    return -1;
  }

  p->p_rest = p->p_save = strdup(s);
  if (p->p_rest == NULL)
    return -1;

  p->p_entry = e;

  return 0;
}

static int bp_walk(struct botz_lookup *p)
{
  while (p->p_entry != NULL && (p->p_name = pathsep(&p->p_rest)) != NULL) {
    if (strcmp(p->p_name, ".") == 0)
      continue;
    else if (strcmp(p->p_name, "..") == 0)
      p->p_entry = p->p_entry->e_parent;
    else
      return 1;
  }

  return 0;
}

static struct botz_entry *
bt_lookup_1(struct hash_table *t, const struct botz_lookup *p)
{
  struct hlist_head *head;
  struct hlist_node *node;
  struct botz_entry *e;
  size_t hash;

  hash = pair_hash(str_hash(p->p_name, 64), /* XXX */
                   ((size_t) p->p_entry) / sizeof(void *), /* alignof */
                   t->t_shift);

  head = t->t_table + (hash & t->t_mask);

  hlist_for_each_entry(e, node, head, e_node)
    if (strcmp(e->e_name, p->p_name) == 0 && e->e_parent == p->p_entry)
      return e;

  return NULL;
}

#define TRACE_LOOKUP(p)                                     \
  TRACE("p_entry `%s', p_name `%s', p_rest `%s'\n",         \
        (p).p_entry != NULL ? (p).p_entry->e_name : "NULL", \
        (p).p_name != NULL ? (p).p_name : "NULL",           \
        (p).p_rest != NULL ? (p).p_rest : "NULL")

struct botz_entry *
botz_lookup(struct botz_listen *bl, const char *path, int flags)
{
  struct hash_table *t = &bl->bl_entry_table;
  struct botz_lookup p;

  /* Static lookup only; does not call any o_lookup() methods. */

  if (bp_init(&p, bl->bl_root_entry, path) < 0)
    goto out;

  while (bp_walk(&p)) {
    struct botz_entry *e;

    TRACE_LOOKUP(p);

    e = bt_lookup_1(t, &p);

    if (e == NULL) {
      if (flags & L_CREATE)
        e = botz_new_entry(p.p_name, NULL, NULL);
      else
        errno = ENOENT;
    }

    if (e != NULL) {
      if (e->e_parent == NULL)
        e->e_parent = p.p_entry;

      if (hlist_unhashed(&e->e_node))
        bt_add_entry(t, e);
    }

    p.p_entry = e;
  }

  TRACE_LOOKUP(p);

 out:
  free(p.p_save);

  return p.p_entry;
}

static struct botz_entry *bx_lookup(EV_P_ struct botz_x *x)
{
  struct botz_listen *bl = container_of(x, struct botz_conn, c_x)->c_listen;
  struct hash_table *t = &bl->bl_entry_table;
  struct botz_lookup p;

  /* Dynamic lookup. */

  if (bp_init(&p, bl->bl_root_entry, x->x_q.q_path) < 0) {
    bx_error(x, -1);
    goto out;
  }

  while (bp_walk(&p)) {
    struct botz_entry *e;

    TRACE_LOOKUP(p);

    e = bt_lookup_1(t, &p);

    if (e == NULL && p.p_entry->e_ops->o_lookup != NULL)
      e = (*p.p_entry->e_ops->o_lookup)(EV_A_ &p, &x->x_q, &x->x_r);

    if (e == NULL)
      x->x_r.r_status = BOTZ_NOT_FOUND;

    if (e == BOTZ_RESPONSE_READY) {
      x->x_r_ready = 1;
      e = NULL;
    }

    if (e != NULL) {
      if (e->e_parent == NULL)
        e->e_parent = p.p_entry;

      if (hlist_unhashed(&e->e_node))
        bt_add_entry(t, e);
    }

    p.p_entry = e;
  }

  TRACE_LOOKUP(p);

 out:
  free(p.p_save);

  return p.p_entry;
}

static void bx_handle(EV_P_ struct botz_x *x)
{
  struct botz_entry *e = NULL;

  if (x->x_r_ready)
    goto out;

  e = x->x_entry = bx_lookup(EV_A_ x);
  if (e == NULL)
    goto out;

  if (e->e_ops->o_method[x->x_q.q_method] == NULL) {
    x->x_r.r_status = BOTZ_FORBIDDEN; /* Or METHOD_NOT_ALLOWED? */
    goto out;
  }

  /* Note that on return from the handler we assume the response is
     ready. */
  (*e->e_ops->o_method[x->x_q.q_method])(EV_A_ e, &x->x_q, &x->x_r);

 out:
  x->x_r_ready = 1;
}

static void bx_read_start(EV_P_ struct botz_x *x, char *msg, size_t msg_len)
{
  char *method, *path_query, *protocol;

  int n = split(&msg, &method, &path_query, &protocol, (char **) NULL);
  if (n == 0)
    return; /* Ignore leading empty lines. */

  x->x_read_cb = &bx_read_header;
  x->x_q_start = 1;

  if (n != 3) {
    bx_error(x, BOTZ_BAD_REQUEST);
    return;
  }

  if (bx_parse_method(x, method) < 0)
    return;

  char *path = strsep(&path_query, "?");
  char *query = path_query;

  ASSERT(x->x_q.q_path == NULL && x->x_q.q_query == NULL);

  x->x_q.q_path = strdup(path);

  if (query != NULL)
    x->x_q.q_query = strdup(query);
}

static void bc_io_cb(EV_P_ struct ev_io *w, int revents)
{
  struct botz_conn *c = container_of(w, struct botz_conn, c_io_w);
  struct botz_x *x = &c->c_x;
  int eof = 0, err = 0, events = 0;

  TRACE("revents %d\n", revents);

  if (revents & EV_ERROR) {
    err = EIO;
  close:
    bc_close(EV_A_ c, err);
    return;
  }

  if (revents & EV_WRITE) {
    n_buf_drain(&c->c_r_header, w->fd, &eof, &err);
    if (err != 0)
      goto close;

    if (eof && x->x_r_ready) {
      eof = 0;
      n_buf_drain(&x->x_r.r_body, w->fd, &eof, &err);
      if (err != 0)
        goto close;

      if (eof)
        x->x_r_sent = 1;
    }
  }

  if (x->x_q_ready && !x->x_r_ready) {
    TRACE("request is ready but not response\n");
    ev_io_set1(EV_A_ w, 0); /* Danger. */
    return;
  }

  if (x->x_r_ready && !x->x_r_sent) {
    TRACE("response ready but not sent\n");
    ev_io_set1(EV_A_ w, EV_WRITE);
    return;
  }

  if (x->x_r_sent) {
    TRACE("response sent\n");
    if (x->x_close || x->x_r.r_close)
      goto close;
    bx_reset(EV_A_ x);
  }

  eof = 0;
  if (revents & EV_READ)
    n_buf_fill(&c->c_q_buf, w->fd, &eof, &err);

  TRACE("read eof %d, err %d\n", eof, err);

  if (err != 0)
    goto close;

  /* We may have left something behind in the request buffer, so we
     check it regardless of revents. */

  while (!n_buf_is_empty(&c->c_q_buf) && !x->x_q_ready) {
    char *msg;
    size_t msg_len;

    if (x->x_q_body_wait) {
      if (x->x_q_body_len <= n_buf_length(&c->c_q_buf))
        x->x_q_ready = 1;
      break;
    }

    if (n_buf_get_msg(&c->c_q_buf, &msg, &msg_len) != 0)
      break;

    (x->x_read_cb)(EV_A_ x, msg, msg_len);
  }

  if (eof) {
    if (!x->x_q_start)
      goto close;

    if (!x->x_q_ready) {
      /* Incomplete request at eof. */
      bx_reset(EV_A_ x);
      bx_error(x, BOTZ_BAD_REQUEST);
    }
  }

  if (x->x_expect_100 && !x->x_q_ready && x->x_r.r_status < 400) {
    n_buf_printf(&c->c_r_header,
                 "HTTP/1.1 100 Continue\r\n\r\n"); /* How many CRLFs? */
    x->x_expect_100 = 0;
    events = EV_READ|EV_WRITE;
  }

  if (x->x_q_ready && !x->x_r_ready) {
    size_t body_len = x->x_q_body_len;

    x->x_q.q_body.nb_buf = c->c_q_buf.nb_buf;
    x->x_q.q_body.nb_size = c->c_q_buf.nb_size;
    x->x_q.q_body.nb_start = c->c_q_buf.nb_start;
    x->x_q.q_body.nb_end = c->c_q_buf.nb_start + body_len;
    bx_handle(EV_A_ x);
    c->c_q_buf.nb_start += body_len;

    br_write_header(&x->x_r, &c->c_r_header);

    ev_timer_again(EV_A_ &c->c_timer_w);
  }

  if (!eof)
    events |= EV_READ;

  if (!n_buf_is_empty(&c->c_r_header) ||
      (x->x_r_ready && !n_buf_is_empty(&c->c_r_body)))
    events |= EV_WRITE;

  TRACE("method %d, status %d\n", x->x_q.q_method, x->x_r.r_status);

#define X(s) TRACE("%s %u\n", #s, x->x_ ##s);
  X(close);
  X(q_start);
  X(q_body_wait);
  X(q_ready);
  X(r_ready);
  X(r_sent);
#undef X

  TRACE("revents %d, events %d\n", revents, events);
  ev_io_set1(EV_A_ w, events);
}
