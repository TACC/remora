#ifndef _HOST_H_
#define _HOST_H_

struct x_node;

struct x_node *x_host_lookup(const char *name, struct x_node *p, int flags);

#endif
