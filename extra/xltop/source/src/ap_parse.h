#ifndef _AP_PARSE_H_
#define _AP_PARSE_H_
#include <netdb.h>

struct ap_struct {
  char ap_addr[NI_MAXHOST];
  char ap_port[NI_MAXSERV];
};

int ap_parse(struct ap_struct *ap, const char *s, const char *a, const char *p);

#endif
