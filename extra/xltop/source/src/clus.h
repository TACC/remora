#ifndef _CLUS_H_
#define _CLUS_H_
#include "x_node.h"

struct job_node;

struct clus_node {
  void *c_auth;
  double c_interval, c_offset, c_modified;
  struct job_node *c_idle_job;
  struct x_node c_x;
};

int clus_type_init(size_t nr_domains);

struct clus_node *clus_lookup(const char *name, int flags);

struct clus_node *clus_lookup_for_host(const char *name);

int clus_add_domain(struct clus_node *c, const char *domain);

#endif
