#ifndef _BOTZ_H_
#define _BOTZ_H_
#include <stddef.h>
#include "evx.h"
#include "hash.h"
#include "n_buf.h"

struct botz_entry;

struct botz_listen {
  struct evx_listen bl_listen;
  struct list_head bl_conn_list;
  /* TODO bl_max_conn */
  double bl_conn_timeout;
  size_t bl_q_buf_size, bl_r_header_size, bl_r_body_size;
  struct hash_table bl_entry_table;
  struct botz_entry *bl_root_entry;
};

enum {
  BOTZ_GET,
  BOTZ_POST,
  BOTZ_PUT,
  BOTZ_DELETE,
  /* BOTZ_CONNECT, */
  /* BOTZ_HEAD, */
  /* BOTZ_OPTIONS, */
  /* BOTZ_TRACE, */
  BOTZ_NR_METHODS,
};

#define BOTZ_OK 200
#define BOTZ_NO_CONTENT 204
#define BOTZ_BAD_REQUEST 400
#define BOTZ_FORBIDDEN 403
#define BOTZ_NOT_FOUND 404
#define BOTZ_METHOD_NOT_ALLOWED 405
#define BOTZ_REQUEST_TIMEOUT 408
#define BOTZ_INTERVAL_SERVER_ERROR 500
#define BOTZ_NOT_IMPLEMENTED 501

#define BOTZ_RESPONSE_READY ((struct botz_entry *) 1UL)

struct botz_entry;
struct botz_entry_ops;
struct botz_request;
struct botz_response;

struct botz_entry {
  struct hlist_node e_node;
  struct botz_entry *e_parent;
  const struct botz_entry_ops *e_ops;
  void *e_data;
  char e_name[];
};

struct botz_lookup {
  struct botz_entry *p_entry;
  char *p_name, *p_rest, *p_save;
};

struct botz_entry_ops {
  struct botz_entry *(*o_lookup)(EV_P_
                                 struct botz_lookup *,
                                 struct botz_request *,
                                 struct botz_response *);
  void (*o_method[BOTZ_NR_METHODS])(EV_P_
                                    struct botz_entry *,
                                    struct botz_request *,
                                    struct botz_response *);
  void (*o_destroy)(EV_P_ struct botz_entry *); /* Don't free(). */
};

struct botz_request {
  char *q_path, *q_query /*, *q_host */;
  struct n_buf q_body;
  int q_method;
  unsigned int q_close:1;
};

struct botz_response {
  int r_status;
  struct n_buf r_body;
  char r_body_type[80];
  unsigned int r_close:1;
};

struct botz_x { /* Request, response exchange. MOVEME */
  /* entry_table */
  struct botz_entry *x_entry;
  struct botz_request x_q;
  struct botz_response x_r;
  void (*x_read_cb)(EV_P_ struct botz_x *, char *, size_t);
  size_t x_q_body_len;
  unsigned int x_close:1, x_expect_100:1,
    x_q_start:1, x_q_body_wait:1, x_q_ready:1,
    x_r_ready:1, x_r_sent:1;
};

struct botz_conn {
  struct botz_listen *c_listen;
  struct list_head c_listen_link;
  struct ev_io c_io_w;
  struct ev_timer c_timer_w;
  struct n_buf c_q_buf, c_r_header, c_r_body;
  struct botz_x c_x;
  void (*c_close_cb)(EV_P_ struct botz_conn *, int);
};

int botz_listen_init(struct botz_listen *bl, size_t nr_entries);

int botz_add(struct botz_listen *bl, const char *path,
             const struct botz_entry_ops *ops, void *data);

struct botz_entry *
botz_new_entry(const char *name, const struct botz_entry_ops *ops, void *data);

struct botz_entry *
botz_lookup(struct botz_listen *bl, const char *path, int flags);

struct json;

int botz_add_json(struct botz_listen *bl, const char *path, struct json *j);

#endif
