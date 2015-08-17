#ifndef _TRACE_H_
#define _TRACE_H_
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#undef NDEBUG
#if ! DEBUG
#define NDEBUG 1
#endif
#include <assert.h>

extern void error_printf(const char *prog, const char *func, int line,
                         const char *fmt, ...) __attribute__((weak));

#if DEBUG
#define TRACE(fmt,args...) do {                         \
    int _saved_errno = errno;                           \
    if (error_printf != NULL)                           \
      error_printf(program_invocation_short_name, __func__, __LINE__, fmt, ##args); \
    else                                                \
      fprintf(stderr, "%s:%d: "fmt, __func__, __LINE__, ##args); \
    errno = _saved_errno;                               \
  } while (0)
#else
__attribute__((format (printf, 1, 2)))
static inline void TRACE(const char *fmt, ...) { }
#endif

#define ERROR(fmt,args...) do {                     \
    int _saved_errno = errno;                       \
    if (error_printf != NULL)                       \
      error_printf(program_invocation_short_name, __func__, __LINE__, fmt, ##args); \
    else                                            \
      fprintf(stderr, "%s: "fmt, program_invocation_short_name, ##args); \
    errno = _saved_errno;                           \
  } while (0)

#define FATAL(fmt,args...) do { \
    ERROR(fmt, ##args);         \
    exit(1);                    \
  } while (0)

#define ASSERT assert

#define OOM() FATAL("out of memory\n");

#endif
