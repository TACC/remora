/*
  File: collect_cpu.c
*/

#include <ctype.h>
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

  // Number of CPUs

  DIR *dir = NULL;

  const char *dir_path = "/sys/devices/system/cpu";

  dir = opendir(dir_path);
  if (dir == NULL) {
    fprintf(stderr, "cannot open %s\n", dir_path);
    return EXIT_FAILURE;
  }

  struct dirent *ent;
  int n_cpus = 0;

  while ((ent = readdir(dir)) != NULL) {
    if (ent->d_type == DT_DIR && strncmp(ent->d_name, "cpu", 3) == 0 && isdigit(*(ent->d_name+3)))
      n_cpus++;
  }

  if (dir != NULL)
    closedir(dir);

  // Previous data

  double time_old;
  char *line_buf = NULL;
  size_t line_buf_size = 0;

  unsigned long long * time_usr_old;
  unsigned long long * time_sys_old;
  unsigned long long * time_idle_old;
  unsigned long long * time_tot_old;

  bool is_tmp_file_exists = false;

  FILE *file = NULL;
  file = fopen("/dev/shm/remora_cpu.tmp","r");

  if (file != NULL)  {

    is_tmp_file_exists = true;

    time_usr_old  = malloc(sizeof(unsigned long long int) * n_cpus);
    time_sys_old  = malloc(sizeof(unsigned long long int) * n_cpus);
    time_idle_old = malloc(sizeof(unsigned long long int) * n_cpus);
    time_tot_old  = malloc(sizeof(unsigned long long int) * n_cpus);

    int idx = 0;

    getline(&line_buf, &line_buf_size, file);
    time_old = atof(line_buf);

    while (getline(&line_buf, &line_buf_size, file) >= 0) {
      sscanf(line_buf, "%llu %llu %llu %llu", time_usr_old+idx, time_sys_old+idx, time_idle_old+idx, time_tot_old+idx);
      idx++;
    }
  }

  // New data

  unsigned long long * time_usr_new  = malloc(sizeof(unsigned long long int) * n_cpus);
  unsigned long long * time_sys_new  = malloc(sizeof(unsigned long long int) * n_cpus);
  unsigned long long * time_idle_new = malloc(sizeof(unsigned long long int) * n_cpus);
  unsigned long long * time_tot_new  = malloc(sizeof(unsigned long long int) * n_cpus);

  struct timespec now;

  clock_gettime(CLOCK_REALTIME, &now);

  double time_new = ((double)now.tv_sec*1e9 + now.tv_nsec) / 1000000000;

  const char *path = "/proc/stat";

  file = fopen(path, "r");

  if (file == NULL) {
    fprintf(stderr, "cannot open %s\n", path);
    return EXIT_FAILURE;
  }

  int idx = 0;
  unsigned long long val, val_tot;

  while (getline(&line_buf, &line_buf_size, file) >= 0) {

    char *rest = line_buf;
    char *cpu = strsep(&rest, " \t\n\v\f\r");

    if (cpu == NULL || rest == NULL)
      continue;

    if (strncmp(cpu, "cpu", 3) != 0)
      continue;

    cpu += 3;

    if (!isdigit(*cpu))
      continue;

    char *str;
    char *end = NULL;

    val_tot = 0;

    for (int i = 0; i < 10; i++)  {
      str = strsep(&rest, " \t\n\v\f\r");
      val = strtoull(str, NULL, 0);
      val_tot += val;

      if (i == 0)
        time_usr_new[idx] = val;

      if (i == 2)
        time_sys_new[idx] = val;

      if (i == 3)
        time_idle_new[idx] = val;
    }
    time_tot_new[idx] = val_tot;
    idx++;
  }

  // Update tmp_file

  file = fopen("/dev/shm/remora_cpu.tmp","w");

  fprintf(file, "%.9f\n", time_new);

  for (int i = 0; i < n_cpus; i++)
    fprintf(file, "%llu %llu %llu %llu\n", time_usr_new[i], time_sys_new[i], time_idle_new[i], time_tot_new[i]);

  fclose(file);

  double frac;
  double avg_frac_usr = 0;
  double avg_frac_sys = 0;
  double avg_frac_idle = 0;

  if (is_tmp_file_exists) {

    for (int i = 0; i < n_cpus; i++) {

      frac = ((time_tot_new[i] - time_tot_old[i])-(time_idle_new[i] - time_idle_old[i])) * 100.0 / (time_tot_new[i] - time_tot_old[i]);
      avg_frac_usr += frac;

      frac = (time_usr_new[i] - time_usr_old[i]) * 100.0 / (time_tot_new[i] - time_tot_old[i]);
      avg_frac_usr += frac;

      frac = (time_sys_new[i] - time_sys_old[i]) * 100.0 / (time_tot_new[i] - time_tot_old[i]);
      avg_frac_sys += frac;

      frac = (time_idle_new[i] - time_idle_old[i]) * 100.0 / (time_tot_new[i] - time_tot_old[i]);
      avg_frac_idle += frac;
    }
    avg_frac_usr /= n_cpus;
    avg_frac_sys /= n_cpus;
    avg_frac_idle /= n_cpus;

    // Write data

    char output_path[PATH_BUFFER_SIZE];
    snprintf(output_path, sizeof(output_path), "%s/cpu_%s.txt", argv[3], argv[1]);
    file = fopen(output_path, "a+");

    fprintf(file, "%.3f ", 0.5 * (time_new + time_old));
    fprintf(file, "%.3f ", avg_frac_usr);

    for (int i = 0; i < n_cpus; i++) {
      frac = (time_usr_new[i] - time_usr_old[i]) * 100.0 / (time_tot_new[i] - time_tot_old[i]);
      fprintf(file, "%.3f ", frac);
    }

    fprintf(file, "%.3f ", avg_frac_sys);

    for (int i = 0; i < n_cpus; i++) {
      frac = (time_sys_new[i] - time_sys_old[i]) * 100.0 / (time_tot_new[i] - time_tot_old[i]);
      fprintf(file, "%.3f ", frac);
    }
   
    fprintf(file, "%.3f ", avg_frac_idle);

    for (int i = 0; i < n_cpus; i++) {
      frac = (time_idle_new[i] - time_idle_old[i]) * 100.0 / (time_tot_new[i] - time_tot_old[i]);
      if (i < n_cpus - 1)
        fprintf(file, "%.3f ", frac);
      else
        fprintf(file, "%.3f\n", frac);
    }
    fclose(file);
  }

  free(line_buf);

  if (is_tmp_file_exists) {
    free(time_usr_old);
    free(time_sys_old);
    free(time_idle_old);
    free(time_tot_old);
  }
  free(time_usr_new);
  free(time_sys_new);
  free(time_idle_new);
  free(time_tot_new);

  return EXIT_SUCCESS;
}

