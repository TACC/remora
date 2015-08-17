#ifndef _XLTOP_H_
#define _XLTOP_H_
#include "string1.h"

/* Common defines for xltopd, servd, ... */
#define STAT_WR_BYTES 0
#define STAT_RD_BYTES 1
#define STAT_NR_REQS  2
#define NR_STATS 3 /* MOVEME */

#define PRI_STATS_FMT(s) s" "s" "s
#define PRI_STATS_ARG(v) (v)[0], (v)[1], (v)[2]

#define SCN_STATS_FMT(s) s" "s" "s
#define SCN_STATS_ARG(v) &(v)[0], &(v)[1], &(v)[2]

#define PRI_K_NODE_FMT "%s:%s %s:%s %f "\
  PRI_STATS_FMT("%f")" "PRI_STATS_FMT("%f")" "PRI_STATS_FMT("%f")

#define PRI_K_NODE_ARG(k) \
  (k)->k_x[0]->x_type->x_type_name, (k)->k_x[0]->x_name, \
  (k)->k_x[1]->x_type->x_type_name, (k)->k_x[1]->x_name, \
  (k)->k_t, \
  PRI_STATS_ARG((k)->k_pending), \
  PRI_STATS_ARG((k)->k_rate), \
  PRI_STATS_ARG((k)->k_sum)

#define SCN_K_STATS_FMT \
  "%lf %lf %lf %lf %lf %lf %lf %lf %lf"

#define SCN_K_STATS_ARG(k) \
  SCN_STATS_ARG((k)->k_pending), \
  SCN_STATS_ARG((k)->k_rate), \
  SCN_STATS_ARG((k)->k_sum)

#define NR_K_STATS (3 * NR_STATS)

struct serv_status {
  double ss_time, ss_uptime;
  double ss_load[3];
  size_t ss_total_ram, ss_free_ram, ss_shared_ram, ss_buffer_ram;
  size_t ss_total_swap, ss_free_swap;
  size_t ss_nr_task;
  size_t ss_nr_mdt, ss_nr_ost, ss_nr_nid;
};

#define PRI_SERV_STATUS_FMT \
  "%.0f %.0f %.2f %.2f %.2f %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu"

#define PRI_SERV_STATUS_ARG(s) \
  (s).ss_time, (s).ss_uptime, (s).ss_load[0], (s).ss_load[1], (s).ss_load[2], \
  (s).ss_total_ram, (s).ss_free_ram, (s).ss_shared_ram, (s).ss_buffer_ram, \
  (s).ss_total_swap, (s).ss_free_swap, (s).ss_nr_task, \
  (s).ss_nr_mdt, (s).ss_nr_ost, (s).ss_nr_nid

#define SCN_SERV_STATUS_FMT \
  "%lf %lf %lf %lf %lf %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu"

#define SCN_SERV_STATUS_ARG(s) \
  &(s).ss_time, &(s).ss_uptime, &(s).ss_load[0], &(s).ss_load[1], \
  &(s).ss_load[2], &(s).ss_total_ram, &(s).ss_free_ram, &(s).ss_shared_ram, \
  &(s).ss_buffer_ram, &(s).ss_total_swap, &(s).ss_free_swap, &(s).ss_nr_task, \
  &(s).ss_nr_mdt, &(s).ss_nr_ost, &(s).ss_nr_nid

#define NR_SCN_SERV_STATUS_ARGS 15

#endif
