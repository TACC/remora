/*
  File: collect_temperature.c
*/

#include <dirent.h>
#include <malloc.h>
#include <stdarg.h>
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

  // Determine n_cpus

  DIR *dir = NULL;
  const char *dir_path = "/sys/devices/platform";

  dir = opendir(dir_path);
  if (dir == NULL) {
    fprintf(stderr, "Cannot open %s\n", dir_path);
    return EXIT_FAILURE;
  }

  int n_cpus = 0;
  int n_cores = 0;

  char *line_buf = NULL;
  size_t line_buf_size = 0;

  int * idx_list;
  int core_idx;
  unsigned long * temp_list;
  unsigned long long temp;

  struct timespec now;

  clock_gettime(CLOCK_REALTIME, &now);

  double time_now = ((double)now.tv_sec*1e9 + now.tv_nsec)/1000000;

  FILE *file;

  struct dirent *ent;

  while ((ent = readdir(dir)) != NULL) {

    if (ent->d_type == DT_DIR && strncmp(ent->d_name, "coretemp.", 9) == 0) {

      DIR *tmp_dir = NULL;
      char tmp_dir_path[PATH_BUFFER_SIZE];
      struct dirent *ent_tmp;

      snprintf(tmp_dir_path, sizeof(tmp_dir_path), "%s/%s/hwmon", dir_path, ent->d_name);
      tmp_dir = opendir(tmp_dir_path);

      while ((ent_tmp = readdir(tmp_dir)) != NULL) {
        if (ent_tmp->d_type == DT_DIR && strncmp(ent_tmp->d_name, "hwmon", 5) == 0) {
 
          DIR *temp_dir = NULL;
          char temp_dir_path[PATH_BUFFER_SIZE];
          struct dirent *ent_temp;

          snprintf(temp_dir_path, sizeof(temp_dir_path), "%s/%s/hwmon/%s", dir_path, ent->d_name, ent_tmp->d_name);

          temp_dir = opendir(temp_dir_path);

          while ((ent_temp = readdir(temp_dir)) != NULL) {
            if (ent_temp->d_type != DT_DIR && strncmp(ent_temp->d_name, "temp", 4) == 0 && strstr(ent_temp->d_name, "_input") != NULL)
              n_cores++;   
          }
          rewinddir(temp_dir);

          idx_list = malloc(sizeof(int)*n_cores);
          temp_list = malloc(sizeof(unsigned long)*n_cores);
          n_cores = 0;

          while ((ent_temp = readdir(temp_dir)) != NULL) {
            if (ent_temp->d_type != DT_DIR && strncmp(ent_temp->d_name, "temp", 4) == 0 && strstr(ent_temp->d_name, "_input") != NULL) {
          
              char temp_file_path[PATH_BUFFER_SIZE];

              sscanf(ent_temp->d_name, "temp%d_input", &core_idx);
              snprintf(temp_file_path, sizeof(temp_file_path), "%s/%s", temp_dir_path, ent_temp->d_name);

              file = NULL;
              file = fopen(temp_file_path, "r");

              if (file == NULL) {
                fprintf(stderr, "Cannot open %s\n", temp_file_path);
                return EXIT_FAILURE;
              }

              if (getline(&line_buf, &line_buf_size, file) <= 0) {
                fprintf(stderr, "Failed reading file %s\n", temp_file_path);
                fclose(file);
                return EXIT_FAILURE;
              }

              temp = strtoul(line_buf, NULL, 10);

              idx_list[n_cores] = core_idx;
              temp_list[n_cores] = temp;
              n_cores++;
            }
          }
          if (temp_dir != NULL)
            closedir(temp_dir);
        }
      }

      int swap_idx;
      unsigned long swap_temp;

      for (int i = 0 ; i < n_cores - 1; i++)  {
        for (int j = 0 ; j < n_cores - i - 1; j++)  {
          if (idx_list[j] > idx_list[j+1])  {
            swap_idx = idx_list[j];
            idx_list[j] = idx_list[j+1];
            idx_list[j+1] = swap_idx;
            swap_temp = temp_list[j];
            temp_list[j] = temp_list[j+1];
            temp_list[j+1] = swap_temp;
          }
        }
      }

      char output_path[PATH_BUFFER_SIZE];
      snprintf(output_path, sizeof(output_path), "%s/temperature_cpu%d_%s.txt", argv[3], n_cpus, argv[1]);
      file = fopen(output_path, "a+");

      fprintf(file, "%.3f ", time_now);

      for (int i = 0; i < n_cores; i++) {
        fprintf(file, "%lu ", temp_list[i]);
        if (i == n_cores - 1)
          fprintf(file, "\n");
        else
          fprintf(file, " ");
      }
      fclose(file);

      free(idx_list);
      free(temp_list);
      n_cpus++;

      if (tmp_dir != NULL)
        closedir(tmp_dir);
    }
  }

  if (dir != NULL)
    closedir(dir);

  free(line_buf);

  return EXIT_SUCCESS;
}

