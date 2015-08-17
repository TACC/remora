#include <stdarg.h>
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "n_buf.h"
#include "trace.h"

int n_buf_init(struct n_buf *nb, size_t size)
{
  memset(nb, 0, sizeof(*nb));

  nb->nb_buf = malloc(size);
  if (nb->nb_buf == NULL && size > 0)
    return -1;

  nb->nb_size = size;

  n_buf_check(nb);

  return 0;
}

void n_buf_fill(struct n_buf *nb, int fd, int *eof, int *err)
{
  ssize_t rc;

  n_buf_check(nb);

  n_buf_pullup(nb);
  if (nb->nb_end == nb->nb_size) {
    *err = ENOBUFS;
    return;
  }

  errno = 0;
  rc = read(fd, nb->nb_buf + nb->nb_end, nb->nb_size - nb->nb_end);

  if (errno != 0 && errno != EAGAIN && errno != EWOULDBLOCK)
    *err = errno;

  if (rc == 0)
    *eof = 1;

  if (rc > 0)
    nb->nb_end += rc;

  n_buf_check(nb);

  TRACE("fd %d, rc %zd, errno %d\n", fd, rc, errno);
}

void n_buf_drain(struct n_buf *nb, int fd, int *eof, int *err)
{
  ssize_t rc = 0;

  n_buf_check(nb);

  errno = 0;

  if (!n_buf_is_empty(nb))
    rc = write(fd, nb->nb_buf + nb->nb_start, nb->nb_end - nb->nb_start);

  if (errno != 0 && errno != EAGAIN && errno != EWOULDBLOCK)
    *err = errno;

  if (rc > 0) {
    nb->nb_start += rc;
    n_buf_pullup(nb);
  }

  if (n_buf_is_empty(nb))
    *eof = 1;

  n_buf_check(nb);

  TRACE("fd %d, rc %zd, errno %d\n", fd, rc, errno);
}

int n_buf_get(struct n_buf *nb, size_t len, char **msg, size_t *msg_len)
{
  n_buf_check(nb);

  if (n_buf_length(nb) < len)
    return -1;

  *msg = nb->nb_buf + nb->nb_start;
  *msg_len = len;
  nb->nb_start += len;

  n_buf_check(nb);

  return 0;
}

int n_buf_get_msg(struct n_buf *nb, char **msg, size_t *msg_len)
{
  char *msg_start, *msg_end;

  n_buf_check(nb);

  msg_start = nb->nb_buf + nb->nb_start;
  msg_end = memchr(msg_start, '\n', nb->nb_end - nb->nb_start);
  if (msg_end == NULL)
    return -1;

  nb->nb_start += msg_end - msg_start + 1;

  *msg_end = 0;
  *msg = msg_start;
  *msg_len = msg_end - msg_start;

  n_buf_check(nb);

  return 0;
}

int n_buf_copy(struct n_buf *nb, const struct n_buf *src)
{
  size_t src_len = src->nb_end - src->nb_start;

  n_buf_pullup(nb);

  if (nb->nb_size - nb->nb_end < src_len)
    return ENOBUFS;

  memcpy(nb->nb_buf + nb->nb_end, src->nb_buf + src->nb_start, src_len);

  nb->nb_end += src_len;

  return 0;
}

void n_buf_destroy(struct n_buf *nb)
{
  n_buf_check(nb);

  free(nb->nb_buf);
  memset(nb, 0, sizeof(*nb));

  n_buf_check(nb);
}

size_t n_buf_write(struct n_buf *nb, void *mem, size_t len)
{
  n_buf_check(nb);

  n_buf_pullup(nb);

  if (nb->nb_size - nb->nb_end < len)
    len = nb->nb_size - nb->nb_end;

  memcpy(nb->nb_buf + nb->nb_end, mem, len);

  nb->nb_end += len;

  n_buf_check(nb);

  return len;
}

size_t n_buf_printf(struct n_buf *nb, const char *fmt, ...)
{
  ssize_t len, max = nb->nb_size - nb->nb_end;
  va_list args;

  n_buf_check(nb);

  va_start(args, fmt);
  len = vsnprintf(nb->nb_buf + nb->nb_end, max, fmt, args);
  va_end(args);

  TRACE("len %zd, max %zu\n", len, max);

  if (len < 0)
    len = 0; /* XXX */

  nb->nb_end += len < max ? len : max;

  n_buf_check(nb);

  return len;
}
