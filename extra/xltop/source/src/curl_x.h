#ifndef _CURL_X_H_
#define _CURL_X_H_
#include <curl/curl.h>

struct n_buf;

struct curl_x {
  CURL *cx_curl;
  char *cx_host;
  long cx_port;
};

int curl_x_init(struct curl_x *cx, const char *host, const char *port);

void curl_x_destroy(struct curl_x *cx);

int curl_x_get_url(struct curl_x *cx, char *url, struct n_buf *nb);

int curl_x_get(struct curl_x *cx, const char *path, const char *query,
               struct n_buf *nb);

typedef int (msg_cb_t)(void *, char *, size_t);

int curl_x_get_iter(struct curl_x *cx,
                    const char *path, const char *query,
                    msg_cb_t *cb, void *data);

int curl_x_put_url(struct curl_x *cx, const char *url, struct n_buf *nb /* [2] */);

int curl_x_put(struct curl_x *cx, const char *path, const char *query,
               struct n_buf *nb /* [2] */);

#endif
