/*
  File: collect_eth.c
*/

#include <dirent.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/socket.h>
#include <linux/if.h>

#define PATH_BUFFER_SIZE 360

__attribute__((format(scanf, 2, 3)))
  static inline int pscanf(const char *path, const char *fmt, ...)
{
  int rc = -1;
  FILE *file = NULL;
  char file_buf[4096];
  va_list arg_list;
  va_start(arg_list, fmt);

  file = fopen(path, "r");
  if (file == NULL)
    goto out;
  setvbuf(file, file_buf, _IOFBF, sizeof(file_buf));

  rc = vfscanf(file, fmt, arg_list);

 out:
  if (file != NULL)
    fclose(file);
  va_end(arg_list);
  return rc;
}

int main(int argc, char *argv[])
{
  if (argc != 4) {
    fprintf(stderr, "Usage: %s %s %s %s\n", argv[0], "node", "outdir", "tmpdir");
    return EXIT_FAILURE;
  }

  DIR *dir = NULL;

  const char *dir_path = "/sys/class/net";

  dir = opendir(dir_path);
  if (dir == NULL) {
    fprintf(stderr, "cannot open %s\n", dir_path);
    return EXIT_FAILURE;
  }

  unsigned long long * eth_list_old = malloc(sizeof(unsigned long long int) * 8);
  unsigned long long * eth_list_new = malloc(sizeof(unsigned long long int) * 8);
  long long int * eth_list_diff = malloc(sizeof(long long int) * 8);

  // Previous data

  char file_buf[4096];
  char *line_buf = NULL;
  size_t line_buf_size = 0;

  char *rest;
  char *time_str;
  double time_old;

  bool is_tmp_file_exists = false;

  FILE *file = NULL;
  file = fopen("/dev/shm/remora_eth.tmp","r");

  if (file != NULL)  {

    is_tmp_file_exists = true;

    while (getline(&line_buf, &line_buf_size, file) >= 0) {
      rest = line_buf;
      time_str = strsep(&rest, " \t\n\v\f\r");
      time_old = atof(time_str);
    }

    int idx = 0;
    unsigned long long val;

    while (true)  {
      char *end = NULL;
      val = strtoull(rest, &end, 0);
      if (rest == end)
          break;
      else {
        eth_list_old[idx] = val;
        idx++;
      }
      rest = end;
    }
    fclose(file);
  }

  // New data

  struct timespec now;

  clock_gettime(CLOCK_REALTIME, &now);

  double time_new = ((double)now.tv_sec*1e9 + now.tv_nsec) / 1000000000;

  char path_tx_packets[PATH_BUFFER_SIZE];
  char path_rx_packets[PATH_BUFFER_SIZE];
  char path_tx_bytes[PATH_BUFFER_SIZE];
  char path_rx_bytes[PATH_BUFFER_SIZE];
  char path_tx_errors[PATH_BUFFER_SIZE];
  char path_tx_dropped[PATH_BUFFER_SIZE];
  char path_rx_frame_errors[PATH_BUFFER_SIZE];
  char path_rx_crc_errors[PATH_BUFFER_SIZE];

  struct dirent *ent;

  for (int i = 0; i < 8; i++)
    eth_list_new[i] = 0;

  while ((ent = readdir(dir)) != NULL) {

    unsigned int flags;
    char flags_path[PATH_BUFFER_SIZE];
    char stats_path[PATH_BUFFER_SIZE];

    if (ent->d_name[0] == '.')
      continue;

    snprintf(flags_path, sizeof(flags_path), "/sys/class/net/%s/flags", ent->d_name);

    if (pscanf(flags_path, "%x", &flags) != 1)
      continue;

    if (flags & IFF_UP) {
      snprintf(path_tx_packets, sizeof(path_tx_packets), "/sys/class/net/%s/statistics/tx_packets", ent->d_name);
      snprintf(path_rx_packets, sizeof(path_rx_packets), "/sys/class/net/%s/statistics/rx_packets", ent->d_name);
      snprintf(path_tx_bytes, sizeof(path_tx_bytes), "/sys/class/net/%s/statistics/tx_bytes", ent->d_name);
      snprintf(path_rx_bytes, sizeof(path_rx_bytes), "/sys/class/net/%s/statistics/rx_bytes", ent->d_name);
      snprintf(path_tx_errors, sizeof(path_tx_errors), "/sys/class/net/%s/statistics/tx_errors", ent->d_name);
      snprintf(path_tx_dropped, sizeof(path_tx_dropped), "/sys/class/net/%s/statistics/tx_dropped", ent->d_name);
      snprintf(path_rx_frame_errors, sizeof(path_rx_frame_errors), "/sys/class/net/%s/statistics/rx_frame_errors", ent->d_name);
      snprintf(path_rx_crc_errors, sizeof(path_rx_crc_errors), "/sys/class/net/%s/statistics/rx_crc_errors", ent->d_name);

      file = NULL;

      // opa_xmit_pack_cntr

      file = fopen(path_tx_packets, "r");
      getline(&line_buf, &line_buf_size, file);
      eth_list_new[0] += strtoull(line_buf, NULL, 10);
      fclose(file);

      file = fopen(path_rx_packets, "r");
      getline(&line_buf, &line_buf_size, file);
      eth_list_new[1] += strtoull(line_buf, NULL, 10);
      fclose(file);

      file = fopen(path_tx_bytes, "r");
      getline(&line_buf, &line_buf_size, file);
      eth_list_new[2] += strtoull(line_buf, NULL, 10);
      fclose(file);

      file = fopen(path_rx_bytes, "r");
      getline(&line_buf, &line_buf_size, file);
      eth_list_new[3] += strtoull(line_buf, NULL, 10);
      fclose(file);

      file = fopen(path_tx_errors, "r");
      getline(&line_buf, &line_buf_size, file);
      eth_list_new[4] += strtoull(line_buf, NULL, 10);
      fclose(file);

      file = fopen(path_tx_dropped, "r");
      getline(&line_buf, &line_buf_size, file);
      eth_list_new[5] += strtoull(line_buf, NULL, 10);
      fclose(file);

      file = fopen(path_rx_frame_errors, "r");
      getline(&line_buf, &line_buf_size, file);
      eth_list_new[6] += strtoull(line_buf, NULL, 10);
      fclose(file);

      file = fopen(path_rx_crc_errors, "r");
      getline(&line_buf, &line_buf_size, file);
      eth_list_new[7] += strtoull(line_buf, NULL, 10);
      fclose(file);
    }
  }
  if (dir != NULL)
    closedir(dir);

  for (int i = 0; i < 8; i++)
    eth_list_diff[i] = eth_list_new[i] - eth_list_old[i];

  // update tmp file

  file = fopen("/dev/shm/remora_eth.tmp","w");

  fprintf(file, "%.6f ", time_new);

  for (int i = 0; i < 8; i++) {
    fprintf(file, "%llu", eth_list_new[i]);
    if (i <= 6)
      fprintf(file, " ");
    else
      fprintf(file, "\n");
  }
  fclose(file);

  // Write data

  if (is_tmp_file_exists) {
    char output_path[PATH_BUFFER_SIZE];
    snprintf(output_path, sizeof(output_path), "%s/network_eth_traffic-%s.txt", argv[3], argv[1]);
    file = fopen(output_path, "a+");

    fprintf(file, "%.3f ", 0.5 * (time_new + time_old));

    for (int i = 0; i < 8; i++) {
      fprintf(file, "%lld", eth_list_diff[i]);
      if (i <= 6)
        fprintf(file, " ");
      else
        fprintf(file, "\n");
    }
    fclose(file);
  }
  return EXIT_SUCCESS;
}
