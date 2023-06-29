/*
  File: collect_memory.c
*/

#define _XOPEN_SOURCE_EXTENDED 1

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/statvfs.h>

#define MAX_LINE 1000
#define PATH_BUFFER_SIZE 360

int main(int argc, char *argv[])
{
  if (argc != 4) {
    fprintf(stderr, "Usage: %s %s %s %s\n", argv[0], "node", "outdir", "tmpdir");
    return EXIT_FAILURE;
  }

  FILE *fp;
  char line[MAX_LINE];

  float vmem_max = 0;
  float rmem_max = 0;
  float tmem_max = 0;

  // Previous data

  char *line_buf = NULL;
  size_t line_buf_size = 0;

  char *rest;
  char *time_str;
  double time_old;

  bool is_tmp_file_exists = false;

  FILE *file = NULL;
  file = fopen("/dev/shm/remora_memory.tmp","r");

  if (file != NULL)  {

    is_tmp_file_exists = true;

    while (getline(&line_buf, &line_buf_size, file) >= 0) {
      rest = line_buf;
      time_str = strsep(&rest, " \t\n\v\f\r");
      time_old = atof(time_str);
    }
    sscanf(rest, "%f %f %f", &vmem_max, &rmem_max, &tmem_max);
    fclose(file);
  }

  // New data

  struct timespec now;

  clock_gettime(CLOCK_REALTIME, &now);

  double time_new = ((double)now.tv_sec*1e9 + now.tv_nsec) / 1000000000;

  pid_t current_pid = getpid();

  char *user_name = getlogin();
  struct passwd *pw = getpwnam(user_name);

  if (!pw)  {
    fprintf(stderr, "Error: getpwnam failed for user %s\n", user_name);
    return EXIT_FAILURE;
  }

  uid_t user_id = pw->pw_uid;

  DIR *dir = opendir("/proc");
  if (dir == NULL){
    fprintf(stderr, "Error: opendir failed for dir /proc\n");
    return EXIT_FAILURE;
  }

  /* Scan entries under /proc directory */

  unsigned long long int vmrss, vmrss_sum;
  unsigned long long int vmsize, vmsize_sum;

  vmrss_sum = 0;
  vmsize_sum = 0;

  while (true) {
    errno = 0;
    struct dirent *dp = readdir(dir);
    if (dp == NULL) {
      if (errno != 0)
        return EXIT_FAILURE;
      else
        break;
    }

    if (dp->d_type != DT_DIR || !isdigit((unsigned char) dp->d_name[0]))
      continue;

    char path[PATH_MAX];
    snprintf(path, PATH_MAX, "/proc/%s/status", dp->d_name);

    file = fopen(path, "r");
    if (file == NULL)
      continue;

    uid_t uid;
    pid_t pid;
    bool match_pid = false;
    bool match_uid = false;
    bool match_vmsize = false;
    bool match_vmrss = false;
    char cmd[MAX_LINE];
    char *str, *vm_str;

    while (!match_uid || !match_vmsize || !match_vmrss) {

      if (fgets(line, MAX_LINE, file) == NULL)
        break;

      if (strncmp(line, "Uid:", 4) == 0) {
        uid = strtol(line + 4, NULL, 10);
        match_uid = (uid == user_id) ? true : false; 
      }

      if (strncmp(line, "Pid:", 4) == 0) {
        pid = strtol(line + 4, NULL, 10);
        match_pid = true; 
      }

      // Virtual memory size (KBs)
      if (match_pid && pid != current_pid && match_uid && strncmp(line, "VmSize:", 7) == 0) {
        str = strdup(line+7);
        while (!isdigit(*str))
          str++;
        vm_str = strsep(&str," ");
        vmsize = strtoull (vm_str, NULL, 10);
        //printf("pid %lu, vmsize = %lf\n", pid, vmsize*1.0);
        vmsize_sum += vmsize;
        match_vmsize = true;
      }

      // Resident set size (KBs)
      if (match_pid && pid != current_pid && match_uid && strncmp(line, "VmRSS:", 6) == 0) {
        str = strdup(line+6);
        while (!isdigit(*str))
          str++;
        vm_str = strsep(&str," ");
        vmrss = strtoull (vm_str, NULL, 10);
        vmrss_sum += vmrss;
        match_vmrss = true;
      }
    }
    fclose(file);
    match_pid = false;
  }

  if (dir != NULL)
    closedir(dir);

  // MEM_FREE : MEMINFO

  file = fopen("/proc/meminfo", "r");
  if (file == NULL) {
    fprintf(stderr, "Failed opening/proc/meminfo\n");
    return EXIT_FAILURE;
  }

  unsigned long long int mem_free;
  bool match_memfree = false;

  while (!match_memfree) {
    if (fgets(line, MAX_LINE, file) == NULL)
      break;

    if (strncmp(line, "MemFree:", 8) == 0) {
      mem_free = strtol(line + 8, NULL, 10);
      match_memfree = true;
    }
  }

  // SHM MEMORY (Bytes)

  struct statvfs buf; 
  statvfs("/dev/shm", &buf);
  unsigned long long int mem_shm = buf.f_frsize * (buf.f_blocks - buf.f_bfree);

  double vmsize_gb = vmsize_sum / (1024.0 * 1024.0);
  double vmrss_gb = vmrss_sum / (1024.0 * 1024.0);
  double mem_shm_gb = mem_shm / (1024.0 * 1024.0 * 1024.0);
  double mem_free_gb = mem_free / (1024.0 * 1024.0);
  double mem_t_gb = vmrss_gb + mem_shm_gb;

  // Update tmp_file

  vmem_max = (vmsize_gb > vmem_max) ? vmsize_gb : vmem_max;
  rmem_max = (vmrss_gb  > rmem_max) ? vmrss_gb  : rmem_max;
  tmem_max = (mem_t_gb  > tmem_max) ? mem_t_gb  : tmem_max;

  file = fopen("/dev/shm/remora_memory.tmp","w");
  fprintf(file, "%.6f ",  time_new);
  fprintf(file, "%.6f ",  vmem_max);
  fprintf(file, "%.6f ",  rmem_max);
  fprintf(file, "%.6f\n", tmem_max);
  fclose(file);

  // Write data

  char output_path[PATH_BUFFER_SIZE];
  snprintf(output_path, sizeof(output_path), "%s/memory_stats_%s.txt", argv[3], argv[1]);
  file = fopen(output_path, "a+");

  fprintf(file, "%.3f ", time_new);
  fprintf(file, "%.6f %.6f %.6f %.6f %.6f %.6f %.6f\n", vmem_max, vmsize_gb, rmem_max, vmrss_gb, mem_shm_gb, mem_free_gb, tmem_max);
  fclose(file);

  return EXIT_SUCCESS;
}
