#ifndef _X_BOTZ_H_
#define _X_BOTZ_H_
#include "botz.h"
#include "x_node.h"

extern struct botz_listen x_listen;

void x_printf(struct n_buf *nb, struct x_node *x);

void x_info_cb(struct x_node *x,
               struct botz_request *q,
               struct botz_response *r);

void x_child_list_cb(struct x_node *x,
                     struct botz_request *q,
                     struct botz_response *r);

struct botz_entry *x_dir_lookup_cb(EV_P_ struct botz_lookup *p,
                                         struct botz_request *q,
                                         struct botz_response *r);

struct botz_entry *x_entry_lookup_cb(EV_P_ struct x_node *x,
                                           struct botz_lookup *p,
                                           struct botz_request *q,
                                           struct botz_response *r);

void x_dir_get_cb(EV_P_ struct botz_entry *e,
                        struct botz_request *q,
                        struct botz_response *r);

#define X_DIR_OPS_DEFAULT \
  .o_lookup = &x_dir_lookup_cb, \
  .o_method = { [BOTZ_GET] = &x_dir_get_cb, } \

int x_dir_init(int type, const struct botz_entry_ops *ops);

#endif
