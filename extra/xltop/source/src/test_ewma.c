#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>
#include "string1.h"
#include "trace.h"

int main(int argc, char *argv[])
{
  double T = strtod(argv[1], NULL);
  double W = strtod(argv[2], NULL);
  double t = 0;
  double p = 0;
  double A = 0;

  double now, d;
  while (scanf("%lf %lf", &now, &d) == 2) {
    double n = floor((now - t) / T);
    if (n > 0) {
      double r = p / T;

      if (A <= 0) /* (n > TICKS_RESET || A < AVG_EPS) TODO Threshold. */
        A = r;
      else
        A += expm1(-T / W) * (A - r);

      t += n * T;
      p = 0;
    }

    if (n > 1)
      A *= exp((n - 1) * (-T / W));

    p += d;

    printf("now %8.3f, t %8.3f, p %12f, A %12f %12e\n", now, t, p, A, A);
  }

  return 0;
}
