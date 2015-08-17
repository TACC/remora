#include <stddef.h>
#include <stdio.h>
#include <ctype.h>
#include "string1.h"
#include "query.h"
#include "trace.h"

int q_string_parse(struct query *q, char *s)
{
  q->q_u.u_string = s;
  return 0;
}

#define Q_PARSE(type, c_type, strto, ...)                       \
  int q_ ## type ## _parse(struct query *q, char *s)       \
  {                                                             \
    q->q_u.u_ ## type = strto(s, __VA_ARGS__);                  \
    return 0;                                                   \
  }

Q_PARSE(double, double, strtod, NULL);
Q_PARSE(int, int, strtol, NULL, 0);
Q_PARSE(long, long, strtol, NULL, 0);
Q_PARSE(llong, long long, strtoll, NULL, 0);
Q_PARSE(size, size_t, strtoull, NULL, 0);
Q_PARSE(uint, unsigned int, strtoul, NULL, 0);
Q_PARSE(ulong, unsigned long, strtoul, NULL, 0);
Q_PARSE(ullong, unsigned long long, strtoull, NULL, 0);

static int q_decode(char *s)
{
  char *p = s;

  do {
    if (*s == '+')
      *s = ' ';

    if (*s != '%') {
      *(p++) = *(s++);
      continue;
    }

    if (!(isxdigit(s[1]) && isxdigit(s[2])))
      break;

    *(p++) = strtoul(((char []) { s[1], s[2], 0 }), NULL, 16);
    s += 3;
  } while (*s != 0);

  *p = 0;

  return *s == 0 ? 0 : -1;
}

int query_parse(struct query *q, size_t n, char *s, int flags)
{
  size_t i;

  while (s != NULL) {
    char *f, *v;

    v = strsep(&s, "&;");
    if (*v == 0)
      continue;

    f = strsep(&v, "=");
    if (*f == 0)
      continue;

    q_decode(f);

    if (v == NULL)
      v = f + strlen(f);

    q_decode(v);

    TRACE("f `%s', v `%s'\n", f, v);

    for (i = 0; i < n; i++) {
      if (strcmp(q[i].q_field, f) == 0) {
        if ((q[i].q_parse)(&q[i], v) < 0)
          return -1;

        q[i].q_is_set = 1;
        break;
      }
    }

    if (i == n && !(flags & QUERY_IGNORE_UNKNOWN_FIELDS)) {
      TRACE("unknown field `%s'\n", f);
      return -1;
    }
  }

  for (i = 0; i < n; i++) {
    if (q[i].q_is_req && !q[i].q_is_set) {
      TRACE("missing argument `%s'\n", q[i].q_field);
      return -1;
    }
  }

  return 0;
}
