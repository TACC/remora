#include "stddef1.h"
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "evx.h"
#include "trace.h"

struct evx_bind {
  struct evx_listen *eb_listen;
  struct list_head eb_link;
  struct ev_io eb_io_w;
  struct sockaddr_storage eb_addr;
  socklen_t eb_addrlen;
};

typedef struct sockaddr SA;

static void evx_bind_io_cb(EV_P_ struct ev_io *w, int revents)
{
  struct evx_bind *eb = container_of(w, struct evx_bind, eb_io_w);
  struct evx_listen *el = eb->eb_listen;
  struct sockaddr_storage addr;
  socklen_t addrlen = sizeof(addr);
  int fd = -1;

  if (revents & EV_ERROR)
    /* TODO.  Note el_io_w is stopped. */;

  /* TODO.  Use accept4() if available. */
  fd = accept(w->fd, (SA *) &addr, &addrlen);
  if (fd < 0) {
    if (!(errno == EAGAIN || errno == EWOULDBLOCK || errno == ECONNABORTED))
      ERROR("cannot accept connections: %s\n", strerror(errno));
    /* ... */
    return;
  }

  if (el->el_nonblock)
    evx_set_nonblock(fd);

  if (el->el_cloexec)
    evx_set_cloexec(fd);

  (*el->el_connect_cb)(EV_A_ el, fd, (SA *) &addr, addrlen);
}

void evx_listen_init(struct evx_listen *el,
                     void (*cb)(EV_P_ struct evx_listen *, int,
                                const SA *, socklen_t),
                     int backlog)
{
  memset(el, 0, sizeof(*el));
  INIT_LIST_HEAD(&el->el_bind_list);
  el->el_connect_cb = cb;
  el->el_backlog = backlog;
  el->el_nonblock = 1;
  el->el_cloexec = 1;
  el->el_reuseaddr = 1;
}

int evx_listen_add(struct evx_listen *el, int fd,
                   const SA *addr, socklen_t addrlen)
{
  struct evx_bind *eb;

  eb = malloc(sizeof(*eb));
  if (eb == NULL)
    return -1;

  eb->eb_listen = el;
  list_add(&eb->eb_link, &el->el_bind_list);

  evx_set_nonblock(fd);
  evx_set_cloexec(fd);

  ev_io_init(&eb->eb_io_w, &evx_bind_io_cb, fd, EV_READ);

  if (addr != NULL && 0 < addrlen && addrlen <= sizeof(eb->eb_addr)) {
    memcpy(&eb->eb_addr, addr, addrlen);
    eb->eb_addrlen = addrlen;
  } else {
    eb->eb_addrlen = sizeof(eb->eb_addr);
    getsockname(fd, (SA *) &eb->eb_addr, &eb->eb_addrlen);
  }

  return 0;
}

static int evx_bind_exists(struct evx_listen *el,
                           const SA *addr, socklen_t addrlen)
{
  struct evx_bind *eb;

  list_for_each_entry(eb, &el->el_bind_list, eb_link) {
    if (eb->eb_addrlen == addrlen && memcmp(&eb->eb_addr, addr, addrlen) == 0)
      return 1;
  }

  return 0;
}

int evx_listen_add_addr(struct evx_listen *el,
                        const SA *addr, socklen_t addrlen)
{
  int fd = -1, rc = -1;

#if DEBUG
  char host[NI_MAXHOST], serv[NI_MAXSERV];
  int gni_rc = getnameinfo(addr, addrlen, host, sizeof(host),
                           serv, sizeof(serv), NI_NUMERICHOST|NI_NUMERICSERV);
  if (gni_rc != 0)
    ERROR("cannnot get nameinfo: %s\n", gai_strerror(gni_rc));
  else
    TRACE("host `%s', serv `%s'\n", host, serv);
#endif

  /* TODO Add flags SOCK_NONBLOCK and SOCK_NONBLOCK if available. */
  fd = socket(addr->sa_family, SOCK_STREAM, 0);
  if (fd < 0) {
    TRACE("cannot create socket: %s\n", strerror(errno));
    goto out;
  }

  if (el->el_reuseaddr) {
    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
      TRACE("cannot set SO_REUSEADDR: %s\n", strerror(errno));
  }

  if (bind(fd, addr, addrlen) < 0) {
    TRACE("cannot bind: %s\n", strerror(errno));
    if (errno == EADDRINUSE && evx_bind_exists(el, addr, addrlen))
      rc = 0;
    goto out;
  }

  if (listen(fd, el->el_backlog) < 0) {
    TRACE("cannot listen: %s\n", strerror(errno));
    goto out;
  }

  if (evx_listen_add(el, fd, addr, addrlen) < 0)
    goto out;

  fd = -1, rc = 0;

 out:
  if (!(fd < 0))
    close(fd);

  return rc;
}

/* Warning!  This function may block in getaddrinfo(). As with
   getaddrinfo(), either host or serv, but not both, may be NULL. */
int evx_listen_add_name(struct evx_listen *el,
                        const char *host, const char *serv,
                        int family /* Use AF_*. AF_UNSPEC == 0 */)
{
  struct addrinfo ai_hints = {
    .ai_family = family,
    .ai_socktype = SOCK_STREAM,
    .ai_flags = AI_PASSIVE, /* Ignored if host != NULL. */
  };
  struct addrinfo *ai, *ai_list = NULL;
  int rc = -1;

#define HOST (host != NULL ? host : "0.0.0.0")
#define SERV (serv != NULL ? serv : "0")

  TRACE("evx_bind_name host `%s', service `%s'\n", HOST, SERV);

  int gai_rc = getaddrinfo(host, serv, &ai_hints, &ai_list);
  if (gai_rc != 0) {
    ERROR("cannot resolve host `%s', service `%s': %s\n",
          HOST, SERV, gai_strerror(gai_rc));
    /* TODO errno. */
    goto out;
  }

  for (ai = ai_list; ai != NULL; ai = ai->ai_next) {
    if (evx_listen_add_addr(el, ai->ai_addr, ai->ai_addrlen) == 0) {
      rc = 0;
      goto out;
    }
  }

  ERROR("cannot bind to host `%s', service `%s': %s\n",
        HOST, SERV, strerror(errno));

 out:
  freeaddrinfo(ai_list);

  return rc;
}

void evx_listen_start(EV_P_ struct evx_listen *el)
{
  struct evx_bind *eb;

  list_for_each_entry(eb, &el->el_bind_list, eb_link)
    ev_io_start(EV_A_ &eb->eb_io_w);
}

void evx_listen_stop(EV_P_ struct evx_listen *el)
{
  struct evx_bind *eb;

  list_for_each_entry(eb, &el->el_bind_list, eb_link)
    ev_io_stop(EV_A_ &eb->eb_io_w);
}

void evx_listen_close(struct evx_listen *el)
{
  struct evx_bind *eb, *tmp;

  list_for_each_entry_safe(eb, tmp, &el->el_bind_list, eb_link) {
    list_del(&eb->eb_link);
    close(eb->eb_io_w.fd);
    free(eb);
  }
}
