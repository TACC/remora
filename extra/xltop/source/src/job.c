#include <malloc.h>
#include "string1.h"
#include "trace.h"
#include "job.h"

#define job_zombie_timeout 300

void job_zombie_cb(EV_P_ struct ev_timer *w, int revents)
{
  struct job_node *j = container_of(w, struct job_node, j_zombie_w);

  if (j->j_fake)
    return;

  if (j->j_x.x_nr_child > 0) {
    /* XXX */
    ev_timer_start(EV_A_ &j->j_zombie_w);
    return;
  }

  TRACE("job `%s' FREE\n", j->j_x.x_name);

  free(j->j_owner);
  free(j->j_title);
  ev_timer_stop(EV_A_ w);
  x_destroy(EV_A_ &j->j_x);
  free(j);
}

/* L_CREATE is implied. */
struct job_node *job_lookup(const char *name /* jobid */,
                            struct x_node *parent,
                            const char *owner,
                            const char *title /* user's name for job */,
                            const char *start)
{
  size_t hash;
  struct hlist_head *head;
  struct x_node *x;
  struct job_node *j;

  x = x_lookup_hash(X_JOB, name, &hash, &head);
  if (x != NULL)
    return container_of(x, struct job_node, j_x);

  j = malloc(sizeof(*j) + strlen(name) + 1);
  if (j == NULL)
    return NULL;

  j->j_owner = strdup(owner);
  j->j_title = strdup(title);
  j->j_start_time = strtod(start, NULL);
  ev_timer_init(&j->j_zombie_w, &job_zombie_cb, job_zombie_timeout, 0);

  x_init(&j->j_x, X_JOB, parent, hash, head, name);

  TRACE("job `%s' START\n", j->j_x.x_name);

  return j;
}

void job_end(EV_P_ struct job_node *j)
{
  TRACE("job `%s' END\n", j->j_x.x_name);

  if (j->j_fake)
    return;

  /* hlist_del_init(&x->x_hash_node); */
  ev_timer_start(EV_A_ &j->j_zombie_w);
}
