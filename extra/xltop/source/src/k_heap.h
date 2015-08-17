#ifndef _K_HEAP_H_
#define _K_HEAP_H_
#include <stddef.h>
#include "x_node.h"

struct k_heap {
  struct k_node **h_k;
  size_t h_count;
  size_t h_limit;
};

struct k_top {
  struct k_heap t_h;
  size_t t_spec[3 * NR_STATS + 1]; /* k_pending[i], k_rate[i], k_sum[i], k_t */
  char *t_owner;
};

#define T_SPEC_LEN (sizeof(t->t_spec) / sizeof(t->t_spec[0]))

/* k_heap_filt_t: Return > 0 to traverse/enqueue this node and all
   below; 0 to enqueue traverse/enqueue this node and call filt on
   descendents, < 0 to ignode this node. */

typedef int (k_heap_filt_t)(struct k_heap *, struct k_node *);
typedef int (k_heap_cmp_t)(struct k_heap *, struct k_node *, struct k_node *);

int k_heap_init(struct k_heap *h, size_t limit);
void k_heap_destroy(struct k_heap *h);
void k_heap_top(struct k_heap *h, struct x_node *x0, size_t d0,
                struct x_node *x1, size_t d1, k_heap_filt_t *filt,
                k_heap_cmp_t *cmp, double now);

void k_heap_order(struct k_heap *h, k_heap_cmp_t *cmp);

int k_top_cmp(struct k_heap *h, struct k_node *k0, struct k_node *k1);

#endif
