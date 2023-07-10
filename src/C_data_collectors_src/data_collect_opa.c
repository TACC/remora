/*
  File: collect_opa.c
*/

#include <dirent.h>
#include <malloc.h>
#include <stdbool.h>
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

//const char *dev_adapter_0="/sys/class/infiniband/hfi1_0/ports/1/counters";
  
  char *dev_mlx5_0="/sys/class/infiniband/mlx5_0/ports/1/counters";
  char *dev_hfi1_0="/sys/class/infiniband/hfi1_0/ports/1/counters";
  char *dev_adapter_0;

  DIR *dir = opendir(dev_mlx5_0);
  if (dir){ dev_adapter_0=dev_mlx5_0; closedir(dir); }
  else    { dev_adapter_0=dev_hfi1_0;                }

  char opa_xmit_pack_cntr[PATH_BUFFER_SIZE];
  char opa_rcv_pack_cntr[PATH_BUFFER_SIZE];
  char opa_xmit_byte_cntr[PATH_BUFFER_SIZE];
  char opa_rcv_byte_cntr[PATH_BUFFER_SIZE];

  bool found_opa = false;

  // dev_adapter_0
  if (!found_opa) {
    dir = opendir(dev_adapter_0);
    if (dir) {
      snprintf(opa_xmit_pack_cntr, sizeof(opa_xmit_pack_cntr), "%s/%s", dev_adapter_0, "port_xmit_packets");
      snprintf(opa_rcv_pack_cntr,  sizeof(opa_rcv_pack_cntr),  "%s/%s", dev_adapter_0, "port_rcv_packets");
      snprintf(opa_xmit_byte_cntr, sizeof(opa_xmit_byte_cntr), "%s/%s", dev_adapter_0, "port_xmit_data");
      snprintf(opa_rcv_byte_cntr,  sizeof(opa_rcv_byte_cntr),  "%s/%s", dev_adapter_0, "port_rcv_data");
      found_opa = true;
    }
    if (dir != NULL)
      closedir(dir);
  }

  if (!found_opa) {
    printf("opa support is not available\n");
    return 1;
  }

  unsigned long long * opa_list_old = malloc(sizeof(unsigned long long int) * 4);
  unsigned long long * opa_list_new = malloc(sizeof(unsigned long long int) * 4);
  long long int * opa_list_diff = malloc(sizeof(long long int) * 4);

  // Previous data

  char *line_buf = NULL;
  size_t line_buf_size = 0;

  char *rest;
  char *time_str;
  double time_old;

  bool is_tmp_file_exists = false;

  FILE *file = NULL;
  file = fopen("/dev/shm/remora_opa.tmp","r");

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
        opa_list_old[idx] = val;
        idx++;
      }
      rest = end;
    }
    fclose(file);
  }

  struct timespec now;

  clock_gettime(CLOCK_REALTIME, &now);

  double time_new = ((double)now.tv_sec*1e9 + now.tv_nsec)/1000000000;

  file = NULL;

  // opa_xmit_pack_cntr

  file = fopen(opa_xmit_pack_cntr, "r");

  if (file == NULL) {
    fprintf(stderr, "Cannot open %s\n", opa_xmit_pack_cntr);
    return EXIT_FAILURE;
  }

  if (getline(&line_buf, &line_buf_size, file) <= 0) {
    fprintf(stderr, "Failed reading data from %s\n", opa_xmit_pack_cntr);
    fclose(file);
    return EXIT_FAILURE;
  }

  opa_list_new[0] = strtol(line_buf, NULL, 10);
  opa_list_diff[0] = opa_list_new[0] - opa_list_old[0];

  if (file != NULL)
    fclose(file);

  // opa_rcv_pack_cntr 

  file = fopen(opa_rcv_pack_cntr, "r");

  if (file == NULL) {
    fprintf(stderr, "Cannot open %s\n", opa_rcv_pack_cntr);
    return EXIT_FAILURE;
  }

  if (getline(&line_buf, &line_buf_size, file) <= 0) {
    fprintf(stderr, "Failed reading data from %s\n", opa_rcv_pack_cntr);
    fclose(file);
    return EXIT_FAILURE;
  }
  
  opa_list_new[1] = strtol(line_buf, NULL, 10);
  opa_list_diff[1] = opa_list_new[1] - opa_list_old[1];
  
  if (file != NULL)
    fclose(file);

  // opa_xmit_byte_cntr

  file = fopen(opa_xmit_byte_cntr, "r");

  if (file == NULL) {
    fprintf(stderr, "Cannot open %s\n", opa_xmit_byte_cntr);
    return EXIT_FAILURE;
  }

  if (getline(&line_buf, &line_buf_size, file) <= 0) {
    fprintf(stderr, "Failed reading data from %s\n", opa_xmit_byte_cntr);
    fclose(file);
    return EXIT_FAILURE;
  }
  
  opa_list_new[2] = strtol(line_buf, NULL, 10);
  opa_list_diff[2] = opa_list_new[2] - opa_list_old[2];

  if (file != NULL)
    fclose(file);

  // opa_rcv_byte_cntr

  file = fopen(opa_rcv_byte_cntr, "r");

  if (file == NULL) {
    fprintf(stderr, "Cannot open %s\n", opa_rcv_byte_cntr);
    return EXIT_FAILURE;
  }

  if (getline(&line_buf, &line_buf_size, file) <= 0) {
    fprintf(stderr, "Failed reading data from %s\n", opa_rcv_byte_cntr);
    fclose(file);
    return EXIT_FAILURE;
  }
  
  opa_list_new[3] = strtol(line_buf, NULL, 10);
  opa_list_diff[3] = opa_list_new[3] - opa_list_old[3];

  if (file != NULL)
    fclose(file);

  // Update tmp file

  file = fopen("/dev/shm/remora_opa.tmp","w");
  fprintf(file, "%.6f ", time_new);

  for (int i = 0; i < 4; i++) {
    fprintf(file, "%llu", opa_list_new[i]);
    if (i <= 2)
      fprintf(file, " ");
    else
      fprintf(file, "\n");
  }
  fclose(file);

  // write data

  if (is_tmp_file_exists) {
 
    char output_path[PATH_BUFFER_SIZE];
  //snprintf(output_path, sizeof(output_path), "%s/opa_%s.txt", argv[3], argv[1]);
    snprintf(output_path, sizeof(output_path), "%s/opa_packets-%s.txt", argv[3], argv[1]);
    file = fopen(output_path, "a+");
    fprintf(file, "%.3f ", 0.5 * (time_new + time_old));

    for (int i = 0; i < 4; i++) {
      fprintf(file, "%lld", opa_list_diff[i]);
      if (i <= 2)
        fprintf(file, " ");
      else
        fprintf(file, "\n");
    }
    fclose(file);
  }
  return EXIT_SUCCESS;
}

