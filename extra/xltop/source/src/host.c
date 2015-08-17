#include <errno.h>
#include <malloc.h>
#include "string1.h"
#include "clus.h"
#include "host.h"
#include "job.h"

struct x_node *x_host_lookup(const char *name, struct x_node *p, int flags)
{
  size_t hash;
  struct hlist_head *head;
  struct x_node *x;
  struct clus_node *c;

  x = x_lookup_hash(X_HOST, name, &hash, &head);
  if (x != NULL)
    return x;

  if (!(flags & L_CREATE))
    return NULL;

  if (p != NULL)
    goto have_p;

  c = clus_lookup_for_host(name);
  if (c == NULL)
    return NULL;

  if (c->c_idle_job == NULL) {
    errno = ESRCH;
    return NULL;
  }

  p = &c->c_idle_job->j_x;

 have_p:
  x = malloc(sizeof(*x) + strlen(name) + 1);
  if (x == NULL)
    return NULL;

  x_init(x, X_HOST, p, hash, head, name);

  return x;
}
