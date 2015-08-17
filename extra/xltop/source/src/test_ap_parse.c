#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include "string1.h"
#include "trace.h"
#include "ap_parse.h"

const char *good_list[] = {
  NULL,
  "",
  "9901",
  "192.0.43.10",
  "192.0.43.10 9901",
  "192.0.43.10:9901",
  "example.com",
  "example.com 9901",
  "example.com:9901",
  "2001:500:88:200::10",
  "2001:500:88:200::10 9901",
  "[2001:500:88:200::10]",
  "[2001:db8::a00:20ff:fea7:ccea] 9901",
  "[2001:db8::a00:20ff:fea7:ccea]:9901",
  "[example.com]",
  "[192.0.43.10]:9901",
  "[example.com]:echo",
  "[2001:db8::a00:20ff:fea7:ccea]:echo",
};

const char *bad_list[] = {
  "[example.com:tcpmux]", /* Passes. */
  "example.com 9901 x",
  "1 2 3",
  "1 2",
};

static int test(const char *str)
{
  char *dup = str != NULL ? strdup(str) : NULL;
  const char *a = "ADDR", *p = "PORT";

  int rc = ap_parse(dup, &a, &p);
  printf("%s `%s' `%s' `%s'\n",
         rc == 0 ? "PASS" : "FAIL",
         str != NULL ? str : "<NULL>", a, p);

  free(dup);

  return rc;
}

int main(int argc, char *argv[])
{
  int status = 0;

  if (argc > 1) {
    int i;

    for (i = 1; i < argc; i++)
      if (test(argv[i]) < 0)
        status = 1;

  } else {
    size_t i, nr_good, nr_bad;

    nr_good = sizeof(good_list) / sizeof(good_list[0]);
    for (i = 0; i < nr_good; i++)
      if (test(good_list[i]) < 0)
        status = 1;

    nr_bad = sizeof(bad_list) / sizeof(bad_list[0]);
    for (i = 0; i < nr_bad; i++)
      if (test(bad_list[i]) == 0)
        status = 1;
  }

  return status;
}
