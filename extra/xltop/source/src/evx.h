#ifndef _EV_LISTEN_H_
#define _EV_LISTEN_H_
#include <ev.h>
#include <fcntl.h>
#include <sys/socket.h>
#include "list.h"

struct evx_listen {
  struct list_head el_bind_list;
  void (*el_connect_cb)(EV_P_ struct evx_listen *, int,
                        const struct sockaddr *, socklen_t);
  /* void (*el_error_cb)(EV_P_ struct ev_listen *, int err); */
  int el_backlog;
  /* All of the bits below are set by default (in init).  cloexec and
     nonblock are always set on the listening socket, these two bits
     only control what happens to sockets returned by accept().
     reuseaddr only applies to the listening socket. */
  unsigned int el_cloexec:1, el_nonblock:1, el_reuseaddr:1;
};

void evx_listen_init(struct evx_listen *el,
                     void (*cb)(EV_P_ struct evx_listen *, int,
                                const struct sockaddr *, socklen_t),
                     int backlog);

/* evx_listen_add: el must be initialized. fd must be bound and
   listening.  Calls getsockaddr if addr is NULL or addrlen is 0.  You
   must call evx_listen_start() after adding a bind, but you need not
   stop el before doing so. */
int evx_listen_add(struct evx_listen *el, int fd,
                   const struct sockaddr *addr, socklen_t addrlen);

/* evx_listen_add_bind: As above el must be initialized and stopped.
   Creates a socket, sets flags, binds it to addr, and calls
   listen(). */
int evx_listen_add_addr(struct evx_listen *el,
                        const struct sockaddr *addr, socklen_t addrlen);

/* evx_listen_add_name: As above, but looks up host:serv using
   getaddrinfo(), tries to bind to the addresses returned, stopping on
   success.  Warning!  This function may block in getaddrinfo(). As
   with getaddrinfo(), either host or serv, but not both, may be
   NULL. */
int evx_listen_add_name(struct evx_listen *el,
                        const char *host, const char *serv,
                        int family /* Use AF_*. AF_UNSPEC == 0 */);

void evx_listen_start(EV_P_ struct evx_listen *el);

void evx_listen_stop(EV_P_ struct evx_listen *el);

/* el_listen_close: el must be stopped.  Closes and destroys all
   binds. */
void evx_listen_close(struct evx_listen *el);

static inline void evx_set_nonblock(int fd)
{
  long flags = fcntl(fd, F_GETFL);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static inline void evx_set_cloexec(int fd)
{
  long flags = fcntl(fd, F_GETFD);
  fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
}

#endif
