#include "stddef1.h"
#include <malloc.h>
#include "string1.h"
#include "trace.h"
#include "k_heap.h"
#include "x_node.h"

/* (h_cmp)(p, c) <= 0. */

/* The entry at index i has parent at index (i - 1) / 2 and children
   at indices 2 * i + 1 and 2 * i + 2. */

int k_heap_init(struct k_heap *h, size_t limit)
{
  memset(h, 0, sizeof(*h));

  h->h_k = calloc(limit, sizeof(h->h_k[0]));
  if (h->h_k == NULL && limit > 0)
    return -1;

  h->h_limit = limit;
  return 0;
}

void k_heap_destroy(struct k_heap *h)
{
  free(h->h_k);
  memset(h, 0, sizeof(*h));
}

#define H(i) (h->h_k[(i)])

static void k_heap_add_full(struct k_heap *h, struct k_node *k, k_heap_cmp_t *cmp)
{
  size_t i = 0, c, r;

  if ((*cmp)(h, k, H(0)) <= 0)
    return;

  while (1) {
    /* Set c to the index of the lesser of k children. */
    c = 2 * i + 1;
    if (!(c < h->h_count))
      break;

    r = 2 * i + 2;
    if (!(r < h->h_count))
      goto have_c;

    if ((*cmp)(h, H(r), H(c)) < 0)
      c = r;

  have_c:
    if ((*cmp)(h, k, H(c)) <= 0)
      break;

    H(i) = H(c);
    i = c;
  }

  H(i) = k;
}

static void k_heap_add(struct k_heap *h, struct k_node *k, k_heap_cmp_t *cmp)
{
  size_t i, p;

  if (h->h_limit == 0)
    return;

  if (h->h_count == h->h_limit) {
    k_heap_add_full(h, k, cmp);
    return;
  }

  i = h->h_count++;
  while (i != 0) {
    p = (i - 1) / 2;

    if ((*cmp)(h, H(p), k) <= 0)
      break;

    H(i) = H(p);
    i = p;
  }

  H(i) = k;
}

void k_heap_order(struct k_heap *h, k_heap_cmp_t *cmp)
{
  size_t i, n, c, r;
  struct k_node *k;

  n = h->h_count;
  while (n > 0) {
    n--;
    k = H(n);
    H(n) = H(0);

    i = 0;
    while (1) {
      /* Set c to the index of the lesser of k children. */
      c = 2 * i + 1;
      if (!(c < n))
        break;

      r = 2 * i + 2;
      if (!(r < n))
        goto have_c;

      if ((*cmp)(h, H(r), H(c)) < 0)
        c = r;

    have_c:
      if ((*cmp)(h, k, H(c)) <= 0)
        break;

      H(i) = H(c);
      i = c;
    }

    H(i) = k;
  }
}

void k_heap_top(struct k_heap *h, struct x_node *x0, size_t d0,
                struct x_node *x1, size_t d1,
                k_heap_filt_t *filt, k_heap_cmp_t *cmp, double now)
{
  struct k_node *k;
  struct x_node *c;

  k = k_lookup(x0, x1, 0);
  if (k == NULL)
    return;

  if (filt != NULL) {
    int f = (*filt)(h, k);
    if (f < 0)
      return;
    else if (f > 0)
      filt = NULL;
  }

  /* TODO Move up k_freshen(). Prune by comparing parent stats to root
     of heap before recursing. */

  if (d0 > 0) {
    x_for_each_child(c, x0)
      k_heap_top(h, c, d0 - 1, x1, d1, filt, cmp, now);
  } else if (d1 > 0) {
    x_for_each_child(c, x1)
      k_heap_top(h, x0, d0, c, d1 - 1, filt, cmp, now);
  } else {
    k_freshen(k, now);
    k_heap_add(h, k, cmp);
  }
}

int k_top_cmp(struct k_heap *h, struct k_node *k0, struct k_node *k1)
{
  struct k_top *t = container_of(h, struct k_top, t_h);
  size_t i;

  for (i = 0; i < T_SPEC_LEN; i++) {
    if (!(t->t_spec[i] < sizeof(struct k_node)))
      break;

    double v0 = *(double *) (((char *) k0) + t->t_spec[i]);
    double v1 = *(double *) (((char *) k1) + t->t_spec[i]);

    if (v0 < v1)
      return -1;
    if (v1 < v0)
      return 1;
  }

  return 0;
}
