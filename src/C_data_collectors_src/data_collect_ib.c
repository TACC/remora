/*
  File: collect_ib.c
*/

#include <dirent.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PATH_BUFFER_SIZE 360

int main(int argc, char *argv[])
{
  if (argc != 4) {
    fprintf(stderr, "Usage: %s %s %s %s\n", argv[0], "node", "outdir", "tmpdir");
    return EXIT_FAILURE;
  }

  // Determine ib type

  const char *cntr_64b="/sys/class/infiniband/mlx4_0/ports/1/counters_ext";
  const char *cntr_32b="/sys/class/infiniband/mlx4_0/ports/1/counters";
  const char *dev_hfi1_0="/sys/class/infiniband/hfi1_0/ports/1/counters";
  const char *dev_mlx5_0="/sys/class/infiniband/mlx5_0/ports/1/counters";

  char ib_xmit_pack_cntr[PATH_BUFFER_SIZE];
  char ib_rcv_pack_cntr[PATH_BUFFER_SIZE];
  char ib_xmit_byte_cntr[PATH_BUFFER_SIZE];
  char ib_rcv_byte_cntr[PATH_BUFFER_SIZE];

  bool found_ib_type = false;

  DIR *dir = NULL;

  // cntr_64b
  dir = opendir(cntr_64b);
  if (dir) {
    snprintf(ib_xmit_pack_cntr, sizeof(ib_xmit_pack_cntr), "%s/%s", cntr_64b, "port_xmit_packets_64");
    snprintf(ib_rcv_pack_cntr, sizeof(ib_rcv_pack_cntr), "%s/%s", cntr_64b, "port_rcv_packets_64");
    snprintf(ib_xmit_byte_cntr, sizeof(ib_xmit_byte_cntr), "%s/%s", cntr_64b, "port_xmit_data_64");
    snprintf(ib_rcv_byte_cntr, sizeof(ib_rcv_byte_cntr), "%s/%s", cntr_64b, "port_rcv_data_64");
    found_ib_type = true;
  }
  if (dir != NULL)
    closedir(dir);

  // dev_hfi1_0
  if (!found_ib_type) {
    dir = opendir(dev_hfi1_0);
    if (dir) {
      snprintf(ib_xmit_pack_cntr, sizeof(ib_xmit_pack_cntr), "%s/%s", dev_hfi1_0, "port_xmit_packets");
      snprintf(ib_rcv_pack_cntr, sizeof(ib_rcv_pack_cntr), "%s/%s", dev_hfi1_0, "port_rcv_packets");
      snprintf(ib_xmit_byte_cntr, sizeof(ib_xmit_byte_cntr), "%s/%s", dev_hfi1_0, "port_xmit_data");
      snprintf(ib_rcv_byte_cntr, sizeof(ib_rcv_byte_cntr), "%s/%s", dev_hfi1_0, "port_rcv_data");
      found_ib_type = true;
    }
    if (dir != NULL)
      closedir(dir);
  }

  // dev_mlx5_0 
  if (!found_ib_type) {
    dir = opendir(dev_mlx5_0);
    if (dir) {
      snprintf(ib_xmit_pack_cntr, sizeof(ib_xmit_pack_cntr), "%s/%s", dev_mlx5_0, "port_xmit_packets");
      snprintf(ib_rcv_pack_cntr, sizeof(ib_rcv_pack_cntr), "%s/%s", dev_mlx5_0, "port_rcv_packets");
      snprintf(ib_xmit_byte_cntr, sizeof(ib_xmit_byte_cntr), "%s/%s", dev_mlx5_0, "port_xmit_data");
      snprintf(ib_rcv_byte_cntr, sizeof(ib_rcv_byte_cntr), "%s/%s", dev_mlx5_0, "port_rcv_data");
      found_ib_type = true;
    }
    if (dir != NULL)
      closedir(dir);
  }

  // cntr_32b
  if (!found_ib_type) {
    dir = opendir(cntr_32b);
    if (dir) {
      snprintf(ib_xmit_pack_cntr, sizeof(ib_xmit_pack_cntr), "%s/%s", cntr_32b, "port_xmit_packets");
      snprintf(ib_rcv_pack_cntr, sizeof(ib_rcv_pack_cntr), "%s/%s", cntr_32b, "port_rcv_packets");
      snprintf(ib_xmit_byte_cntr, sizeof(ib_xmit_byte_cntr), "%s/%s", cntr_32b, "port_xmit_data");
      snprintf(ib_rcv_byte_cntr, sizeof(ib_rcv_byte_cntr), "%s/%s", cntr_32b, "port_rcv_data");
      found_ib_type = true;
    }
    if (dir != NULL)
      closedir(dir);
  }

  if (!found_ib_type) {
    fprintf(stderr, "ib support is not available\n");
    return EXIT_FAILURE;
  }

  unsigned long long * ib_list_old = malloc(sizeof(unsigned long long int) * 4);
  unsigned long long * ib_list_new = malloc(sizeof(unsigned long long int) * 4);
  long long * ib_list_diff = malloc(sizeof(long long int) * 4);

  // Previous data

  char *line_buf = NULL;
  size_t line_buf_size = 0;

  char *rest;
  char *time_str;
  double time_old;

  bool is_tmp_file_exists = false;

  FILE *file = NULL;
  file = fopen("/dev/shm/remora_ib.tmp","r");

  if (file != NULL)  {

    is_tmp_file_exists = true;

    while (getline(&line_buf, &line_buf_size, file) >= 0) {
      rest = line_buf;
      time_str = strsep(&rest, " \t\n\v\f\r");
      time_old = atof(time_str);
    }

    int idx = 0;

    while (true)  {
      char *end = NULL;
      unsigned long long val;

      val = strtoull(rest, &end, 0);
      if (rest == end)
          break;
      else {
        ib_list_old[idx] = val;
        idx++;
      }
      rest = end;
    }
    fclose(file);
  }

  // New data

  struct timespec now;

  clock_gettime(CLOCK_REALTIME, &now);

  double time_new = ((double)now.tv_sec*1e9 + now.tv_nsec)/1000000000;

  file = NULL;

  // ib_xmit_pack_cntr

  file = fopen(ib_xmit_pack_cntr, "r");

  if (file == NULL) {
    fprintf(stderr, "Cannot open %s\n", ib_xmit_pack_cntr);
    return EXIT_FAILURE;
  }

  if (getline(&line_buf, &line_buf_size, file) <= 0) {
    fprintf(stderr, "Failed reading data from %s\n", ib_xmit_pack_cntr);
    fclose(file);
    return EXIT_FAILURE;
  }

  ib_list_new[0] = strtol(line_buf, NULL, 10);
  ib_list_diff[0] = ib_list_new[0] - ib_list_old[0];

  if (file != NULL)
    fclose(file);

  // ib_rcv_pack_cntr 

  file = fopen(ib_rcv_pack_cntr, "r");

  if (file == NULL) {
    fprintf(stderr, "Cannot open %s\n", ib_rcv_pack_cntr);
    return EXIT_FAILURE;
  }

  if (getline(&line_buf, &line_buf_size, file) <= 0) {
    fprintf(stderr, "Failed reading data from %s\n", ib_rcv_pack_cntr);
    fclose(file);
    return EXIT_FAILURE;
  }
  
  ib_list_new[1] = strtol(line_buf, NULL, 10);
  ib_list_diff[1] = ib_list_new[1] - ib_list_old[1];
  
  if (file != NULL)
    fclose(file);

  // ib_xmit_byte_cntr

  file = fopen(ib_xmit_byte_cntr, "r");

  if (file == NULL) {
    fprintf(stderr, "Cannot open %s\n", ib_xmit_byte_cntr);
    return EXIT_FAILURE;
  }

  if (getline(&line_buf, &line_buf_size, file) <= 0) {
    fprintf(stderr, "Failed reading data from %s\n", ib_xmit_byte_cntr);
    return EXIT_FAILURE;
  }
  
  ib_list_new[2] = strtol(line_buf, NULL, 10);
  ib_list_diff[2] = ib_list_new[2] - ib_list_old[2];

  if (file != NULL)
    fclose(file);

  // ib_rcv_byte_cntr

  file = fopen(ib_rcv_byte_cntr, "r");

  if (file == NULL) {
    fprintf(stderr, "Cannot open %s\n", ib_rcv_byte_cntr);
    return EXIT_FAILURE;
  }

  if (getline(&line_buf, &line_buf_size, file) <= 0) {
    fclose(file);
    fprintf(stderr, "Failed reading data from %s\n", ib_rcv_byte_cntr);
    return EXIT_FAILURE;
  }
  
  ib_list_new[3] = strtol(line_buf, NULL, 10);
  ib_list_diff[3] = ib_list_new[3] - ib_list_old[3];

  if (file != NULL)
    fclose(file);

  // Update tmp file

  file = fopen("/dev/shm/remora_ib.tmp","w");
  fprintf(file, "%.6f ", time_new);

  for (int i = 0; i < 4; i++) {
    fprintf(file, "%llu", ib_list_new[i]);
    if (i <= 2)
      fprintf(file, " ");
    else
      fprintf(file, "\n");
  }
  fclose(file);

  // Write data

  if (is_tmp_file_exists) {
 
    char output_path[PATH_BUFFER_SIZE];
    snprintf(output_path, sizeof(output_path), "%s/ib_%s.txt", argv[3], argv[1]);
    file = fopen(output_path, "a+");

    fprintf(file, "%.3f ", 0.5 * (time_old + time_new));

    for (int i = 0; i < 4; i++) {
      fprintf(file, "%lld", ib_list_diff[i]);
      if (i <= 2)
        fprintf(file, " ");
      else
        fprintf(file, "\n");
    }
  }
  return EXIT_SUCCESS;
}

