#ifndef _QUERY_H_
#define _QUERY_H_

#define QUERY_IGNORE_UNKNOWN_FIELDS (1 << 0)

#define QUERY_TYPES \
  X(string, char *, &q_string_parse) \
  X(double, double, &q_double_parse) \
  X(int,    int,    &q_int_parse) \
  X(long,   long,   &q_long_parse) \
  X(llong,  long long, &q_llong_parse) \
  X(uint,   unsigned int, &q_uint_parse) \
  X(ulong,  unsigned long, &q_ulong_parse) \
  X(ullong, unsigned long long, &q_ullong_parse) \
  X(size,   size_t, &q_size_parse) \
  X(void_p, void *, NULL)

#define X(type, c_type, ...) typedef typeof(c_type) q_ ## type ## _t;
QUERY_TYPES;
#undef X

struct query {
  const char *q_field;
  union {
#define X(type, ...) q_ ## type ## _t  u_ ## type;
    QUERY_TYPES
#undef X
  } q_u;
  int (*q_parse)(struct query *, char *);
  unsigned int q_is_req:1, q_is_set:1;
};

int query_parse(struct query *q, size_t n, char *s, int flags);

int q_string_parse(struct query *q, char *s);

#define Q_PARSE(type, c_type, strto, ...)                       \
  int q_ ## type ## _parse(struct query *q, char *s);

Q_PARSE(double, double, strtod, NULL);
Q_PARSE(int, int, strtol, NULL, 0);
Q_PARSE(long, long, strtol, NULL, 0);
Q_PARSE(llong, long long, strtoll, NULL, 0);
Q_PARSE(size, size_t, strtoull, NULL, 0);
Q_PARSE(uint, unsigned int, strtoul, NULL, 0);
Q_PARSE(ulong, unsigned long, strtoul, NULL, 0);
Q_PARSE(ullong, unsigned long long, strtoull, NULL, 0);
#undef Q_PARSE

#define _DEFINE_QUERY(q, i, type, field, value, parse, is_req) ((struct query) { \
    .q_field = #field, \
    .q_u.u_ ## type = value, \
    .q_parse = parse, \
    .q_is_req = (is_req), \
  })

#define _QUERY_VALUE(q, i, type, ...) q[i].q_u.u_ ## type

#define DEFINE_QUERY(x, q) \
  struct query q[] = { x(_DEFINE_QUERY, q) }

#define QUERY_PARSE(x, q, s) \
  (query_parse((q), sizeof((q)) / sizeof((q)[0]), (s), 0))

#define QUERY_VALUES(x, q) x(_QUERY_VALUE, q)

#endif
