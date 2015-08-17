#ifndef _STRING1_H_
#define _STRING1_H_
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#if defined(__GNUC__) && (__GNUC__ >= 4)
#define __ATTRIBUTE__SENTINEL __attribute__ ((sentinel))
#else
#define __ATTRIBUTE__SENTINEL
#endif

static inline char *strsep_ne(char **ref, const char *delim)
{
  char *str;
  do
    str = strsep(ref, delim);
  while (str != NULL && *str == 0);
  return str;
}

static inline char *wsep(char **ref)
{
  return strsep_ne(ref, " \t\n\v\f\r");
}

__attribute__((format(printf, 1, 2)))
static inline char *strf(const char *fmt, ...)
{
  char *str = NULL;
  va_list args;

  va_start(args, fmt);
  if (vasprintf(&str, fmt, args) < 0)
    str = NULL;
  va_end(args);
  return str;
}

static inline char *chop(char *s, int c)
{
  char *p = strchr(s, c);
  if (p != NULL)
    *p = 0;
  return s;
}

__ATTRIBUTE__SENTINEL
static inline int split(char **ref, ...)
{
  int nr = 0;
  va_list args;

  va_start(args, ref);
  while (1) {
    char **p = va_arg(args, char **);
    if (p == NULL)
      break;

    char *s = wsep(ref);
    if (s == NULL)
      break;

    *p = s;
    nr++;
  }
  va_end(args);

  return nr;
}

static inline char *pathsep(char **s)
{
  char *n;

  do
    n = strsep(s, "/");
  while (n != NULL && *n == 0);

  if (n == NULL || *n == 0)
    n = *s = NULL;

  if (*s != NULL) {
    while (**s == '/')
      (*s)++;

    if (**s == 0)
      *s = NULL;
  }

  return n;
}

static inline int str_is_set(const char *s)
{
  return s != NULL && strlen(s) > 0;
}

static inline const char *str_or(const char *s1, const char *s2)
{
  return str_is_set(s1) ? s1 : s2;
}

#endif
