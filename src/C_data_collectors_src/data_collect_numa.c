/*
  File: collect_numa.c
*/

#include <dirent.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_LINE 1000
#define PATH_BUFFER_SIZE 360

struct NUMA_STATS
{
    int node_idx;
    unsigned long long int numa_foreign;
    unsigned long long int local_node;
    unsigned long long int other_node;
    unsigned long long int mem_free;
    unsigned long long int mem_used;
};

int main(int argc, char *argv[])
{
  if (argc != 4) {
    fprintf(stderr, "Usage: %s %s %s %s\n", argv[0], "node", "outdir", "tmpdir");
    return EXIT_FAILURE;
  }

  const char *node;
  char path[PATH_BUFFER_SIZE];
  char line[MAX_LINE];

  struct timespec now;

  clock_gettime(CLOCK_REALTIME, &now);

  double time_new = ((double)now.tv_sec*1e9 + now.tv_nsec) / 1000000000;

  // n_nodes

  DIR *dir = NULL;
  const char *dir_path = "/sys/devices/system/node";

  dir = opendir(dir_path);
  if (dir == NULL) {
    fprintf(stderr, "Cannot open %s\n", dir_path);
    return EXIT_FAILURE;
  }

  int n_nodes = 0;
  struct dirent *ent;

  while ((ent = readdir(dir)) != NULL) {

    if (strncmp(ent->d_name, "node", 4) != 0)
      continue;

    n_nodes++;
  }

  if (dir != NULL)
    closedir(dir);

  struct NUMA_STATS **stats;

  stats = malloc(sizeof(struct NUMA_STATS*)*n_nodes);

  for (int i = 0; i < n_nodes; i++)  {
    stats[i] = malloc(sizeof(struct NUMA_STATS));
  }

  FILE *file;

  for (int node = 0; node < n_nodes; node++)  {

    // numa_foreign/local_node/other_node : /NODE/NUMASTAT

    snprintf(path, sizeof(path), "%s/node%d/numastat", dir_path, node);

    file = NULL;
  
    file = fopen(path, "r");
    if (file == NULL) {
      fprintf(stderr, "Cannot open %s\n", path);
      return EXIT_FAILURE;
    }

    unsigned long long int numa_foreign;
    unsigned long long int local_node;
    unsigned long long int other_node;

    bool match_numa_foreign = false;
    bool match_local_node = false;
    bool match_other_node = false;

    while (!match_numa_foreign || !match_local_node || !match_other_node) {
      if (fgets(line, MAX_LINE, file) == NULL)
        break;

      if (strncmp(line, "numa_foreign", 12) == 0) {
        numa_foreign = strtol(line + 12, NULL, 10);
        match_numa_foreign = true;
        stats[node]->numa_foreign = numa_foreign;
      }
      if (strncmp(line, "local_node", 10) == 0) {
        local_node = strtol(line + 10, NULL, 10);
        match_local_node = true;
        stats[node]->local_node = local_node;
      }
      if (strncmp(line, "other_node", 10) == 0) {
        other_node = strtol(line + 10, NULL, 10);
        match_other_node = true;
        stats[node]->other_node = other_node;
      }
    }
    if (file != NULL)
      fclose(file);

    // MemFree/MemUsed: /NODE/MEMINFO

    snprintf(path, sizeof(path), "%s/node%d/meminfo", dir_path, node);

    file = NULL;

    file = fopen(path, "r");
    if (file == NULL) {
      fprintf(stderr, "Cannot open %s\n", path);
      return EXIT_FAILURE;
    }

    unsigned long long int mem_free;
    unsigned long long int mem_used;

    bool match_mem_free = false;
    bool match_mem_used = false;

    char mem_free_str[100];
    char mem_used_str[100];

    snprintf(mem_free_str, sizeof(mem_free_str), "Node %d MemFree:", node);
    snprintf(mem_used_str, sizeof(mem_used_str), "Node %d MemUsed:", node);

    while (!match_mem_free || !match_mem_used) {
      if (fgets(line, MAX_LINE, file) == NULL)
        break;

      if (strncmp(line, mem_free_str, strlen(mem_free_str)) == 0) {
        mem_free = strtol(line + strlen(mem_free_str), NULL, 10);
        match_mem_free = true;
        stats[node]->mem_free = mem_free;
      }
      if (strncmp(line, mem_used_str, strlen(mem_used_str)) == 0) {
        mem_used = strtol(line + strlen(mem_used_str), NULL, 10);
        match_mem_used = true;
        stats[node]->mem_used = mem_used;
      }
    }
    if (file != NULL)
      fclose(file);
  }

  // AnonPages/AnonHugePages: /PROC/MEMINFO

  file = NULL;
  file = fopen("/proc/meminfo", "r");
  if (file == NULL) {
    fprintf(stderr, "Failed opening /proc/meminfo\n");
    return EXIT_FAILURE;
  }

  unsigned long long int anon_pages;
  unsigned long long int anon_huge_pages;

  bool match_anon_pages = false;
  bool match_anon_huge_pages = false;

  while (!match_anon_pages || !match_anon_huge_pages) {
    if (fgets(line, MAX_LINE, file) == NULL)
      break;

    if (strncmp(line, "AnonPages:", 10) == 0) {
      anon_pages = strtol(line + 10, NULL, 10);
      match_anon_pages = true;
    }
    if (strncmp(line, "AnonHugePages:", 14) == 0) {
      anon_huge_pages = strtol(line + 14, NULL, 10);
      match_anon_huge_pages = true;
    }
  }

  // Write data

  char output_path[PATH_BUFFER_SIZE];
  snprintf(output_path, sizeof(output_path), "%s/numa_stats_%s.txt", argv[3], argv[1]);
  file = fopen(output_path, "a+");

  fprintf(file, "%.3f ", time_new);

  fprintf(file, "%llu %llu ", anon_pages, anon_huge_pages);

  for (int i = 0 ; i < n_nodes; i++)
    fprintf(file, "%llu ", stats[i]->numa_foreign);

  for (int i = 0 ; i < n_nodes; i++)
    fprintf(file, "%.4lf ", stats[i]->local_node/256.0);

  for (int i = 0 ; i < n_nodes; i++)
    fprintf(file, "%.4lf ", stats[i]->other_node/256.0);

  for (int i = 0 ; i < n_nodes; i++)
    fprintf(file, "%.4lf ", stats[i]->mem_free/1024.0);

  for (int i = 0 ; i < n_nodes; i++)
    fprintf(file, "%.4lf ", stats[i]->mem_used/1024.0);

  fprintf(file, "\n");

  fclose(file);

  for (int i = 0 ; i < n_nodes; i++)
    free(stats[i]);
  free(stats);

  return EXIT_SUCCESS;
}

