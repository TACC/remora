#include <malloc.h>
#include <string.h>
#include "sub.h"
#include "x_node.h"
#include "trace.h"

void sub_init(struct sub_node *s, struct k_node *k, struct user_conn *uc,
              void (*cb)(EV_P_ struct sub_node *, struct k_node *,
                         struct x_node *, struct x_node *, double *))
{
  memset(s, 0, sizeof(*s));

  list_add_tail(&s->s_x_link[0], &k->k_x[0]->x_sub_list);
  list_add_tail(&s->s_x_link[1], &k->k_x[1]->x_sub_list);
  list_add_tail(&s->s_k_link, &k->k_sub_list);
  INIT_LIST_HEAD(&s->s_u_link); /* XXX */
  s->s_cb = cb;
  s->s_u_conn = uc;
}

void sub_cancel(EV_P_ struct sub_node *s)
{
  /* struct cl_conn *cc = &s->s_u_conn->uc_conn; */

  list_del_init(&s->s_x_link[0]);
  list_del_init(&s->s_x_link[1]);
  list_del_init(&s->s_k_link);
  list_del_init(&s->s_u_link);

  /* cl_conn_writef(EV_A_ cc, "%csub_end %"PRI_TID"\n",
     CL_CONN_CTL_CHAR, s->s_tid); */

  free(s);
}
