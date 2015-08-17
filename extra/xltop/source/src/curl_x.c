#include <stdio.h>
#include <stdlib.h>
#include "curl_x.h"
#include "n_buf.h"
#include "string1.h"
#include "trace.h"

void curl_x_destroy(struct curl_x *cx)
{
  if (cx->cx_curl != NULL)
    curl_easy_cleanup(cx->cx_curl);
  free(cx->cx_host);
  memset(cx, 0, sizeof(cx));
}

int curl_x_init(struct curl_x *cx, const char *host, const char *port)
{
  int rc = -1;

  memset(cx, 0, sizeof(*cx));

  cx->cx_curl = curl_easy_init();
  if (cx->cx_curl == NULL) {
    if (errno == 0)
      errno = ENOMEM;
    goto out;
  }

  cx->cx_host = strdup(host);
  if (host == NULL)
    goto out;

  cx->cx_port = strtol(port, NULL, 0);
  if (!(0 <= cx->cx_port && cx->cx_port < 65536)) {
    errno = EINVAL;
    goto out;
  }

  rc = 0;
 out:
  if (rc < 0)
    curl_x_destroy(cx);

  return rc;
}

int curl_x_get_url(struct curl_x *cx, char *url, struct n_buf *nb)
{
  FILE *file = NULL;
  int rc = -1;

  n_buf_destroy(nb);

  file = open_memstream(&nb->nb_buf, &nb->nb_size);
  if (file == NULL) {
    ERROR("cannot open memory stream: %m\n");
    goto out;
  }

  curl_easy_reset(cx->cx_curl);
  curl_easy_setopt(cx->cx_curl, CURLOPT_URL, url);
  if (cx->cx_port > 0)
    curl_easy_setopt(cx->cx_curl, CURLOPT_PORT, cx->cx_port);
  curl_easy_setopt(cx->cx_curl, CURLOPT_UPLOAD, 0L);
  curl_easy_setopt(cx->cx_curl, CURLOPT_WRITEDATA, file);

#if DEBUG
  curl_easy_setopt(cx->cx_curl, CURLOPT_VERBOSE, 1L);
#endif

  int curl_rc = curl_easy_perform(cx->cx_curl);
  if (curl_rc != 0) {
    ERROR("cannot GET `%s': %s\n", url, curl_easy_strerror(rc));
    /* Reset curl... */
    goto out;
  }

  if (ferror(file)) {
    ERROR("error writing to memory stream: %m\n");
    goto out;
  }

  rc = 0;

 out:
  if (file != NULL)
    fclose(file);

  nb->nb_end = nb->nb_size;

  return rc;
}

int curl_x_put_url(struct curl_x *cx, const char *url, struct n_buf *nb)
{
  FILE *file[2] = { NULL, NULL };
  int rc = -1;

  /* fmemopen() fails when buffer size is zero. */
  if (n_buf_length(&nb[0]) == 0) {
    file[0] = fopen("/dev/null", "r");
    if (file[0] == NULL) {
      ERROR("cannot open `/dev/null' for reading: %m\n");
      goto out;
    }
  } else {
    file[0] = fmemopen(nb[0].nb_buf + nb[0].nb_start,
                       n_buf_length(&nb[0]), "r");
    if (file[0] == NULL) {
      ERROR("cannot open memory stream: %m\n");
      goto out;
    }
  }

  n_buf_destroy(&nb[1]);

  file[1] = open_memstream(&nb[1].nb_buf, &nb[1].nb_size);
  if (file[1] == NULL) {
    ERROR("cannot open memory stream: %m\n");
    goto out;
  }

  curl_easy_reset(cx->cx_curl);
  curl_easy_setopt(cx->cx_curl, CURLOPT_URL, url);
  if (cx->cx_port > 0)
    curl_easy_setopt(cx->cx_curl, CURLOPT_PORT, cx->cx_port);
  curl_easy_setopt(cx->cx_curl, CURLOPT_UPLOAD, 1L);
  curl_easy_setopt(cx->cx_curl, CURLOPT_READDATA, file[0]);
  curl_easy_setopt(cx->cx_curl, CURLOPT_INFILESIZE_LARGE,
                   (curl_off_t) n_buf_length(&nb[0]));
  curl_easy_setopt(cx->cx_curl, CURLOPT_WRITEDATA, file[1]);

#if DEBUG
  curl_easy_setopt(cx->cx_curl, CURLOPT_VERBOSE, 1L);
#endif

  int curl_rc = curl_easy_perform(cx->cx_curl);
  if (curl_rc != 0) {
    ERROR("cannot upload to `%s': %s\n", url, curl_easy_strerror(rc));
    goto out;
  }

  if (ferror(file[1])) {
    ERROR("error writing to memory stream: %m\n");
    goto out;
  }

  rc = 0;

 out:
  if (file[0] != NULL)
    fclose(file[0]);

  if (file[1] != NULL)
    fclose(file[1]);

  nb[1].nb_end = nb[1].nb_size;

  return rc;
}

int curl_x_put(struct curl_x *cx, const char *path, const char *query,
               struct n_buf *nb)
{
  char *url = NULL;
  int rc = -1;

  url = strf("http://%s/%s%s%s", cx->cx_host, path,
             query != NULL ? "?" : "",
             query != NULL ? query : "");
  if (url == NULL)
    OOM();

  TRACE("url `%s'\n", url);

  if (curl_x_put_url(cx, url, nb) < 0)
    goto out;

  rc = 0;

 out:
  free(url);

  return rc;
}

int curl_x_get(struct curl_x *cx, const char *path, const char *qstr,
               struct n_buf *nb)
{
  char *url = NULL;
  int rc = -1;

  url = strf("http://%s/%s%s%s", cx->cx_host, path,
             qstr != NULL ? "?" : "", qstr != NULL ? qstr : "");
  if (url == NULL)
    OOM();

  TRACE("url `%s'\n", url);

  if (curl_x_get_url(cx, url, nb) < 0)
    goto out;

  rc = 0;

 out:
  free(url);

  return rc;
}

int curl_x_get_iter(struct curl_x *cx,
                    const char *path, const char *query,
                    msg_cb_t *cb, void *data)
{
  N_BUF(nb);
  char *msg;
  size_t msg_len;
  int rc = -1;

  if (curl_x_get(cx, path, query, &nb) < 0)
    goto out;

  while (n_buf_get_msg(&nb, &msg, &msg_len) == 0) {
    rc = (*cb)(data, msg, msg_len);
    if (rc < 0)
      goto out;
  }

 out:
  n_buf_destroy(&nb);

  return rc;
}
