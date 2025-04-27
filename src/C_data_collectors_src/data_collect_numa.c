#include  <stdio.h>
#include <string.h>
#include <stdlib.h>
#include  <stdio.h>
#include   <time.h>


# define N 1024
# define PATH_BUFFER_SIZE 360

# define M(i,j) numaStatM[i][j]
# define C(i,j) numaStatC[i][j]

int main(int argc, char *argv[])
{


  FILE *file;                 // file handle
  char line[N], str[N];       // input line & line wo return (more than ample size)
  int row, i;                 // data array row and column (i for index) values
  int lineno, node_count;     // input line number, number of numa nodes
  long long numaStatC[ 3][8]; // Data for numastat -c
  float     numaStatM[26][8]; // Data for numastat -m

  if (argc != 4) {
    fprintf(stderr, "Usage: %s %s %s %s\n", argv[0], "node", "outdir", "tmpdir");
    return EXIT_FAILURE;
  }
  const char *node;
  char path[PATH_BUFFER_SIZE];

  struct timespec now;
  clock_gettime(CLOCK_REALTIME, &now);
  double time_new = ((double)now.tv_sec*1e9 + now.tv_nsec) / 1000000000;

  // get number of non-zero NODES with "numastat -z" by
  // counting the number of words "Node 0 Node 1 ... Total" inline 3.
  int nNodes, nNodesP1;
  if (!(file = popen("numastat -z  | sed -n 3p  | wc -w", "r"))) { exit(1); }
  fgets ( line, sizeof line, file );
  nNodes=atoi(line); nNodes=(nNodes-1)/2; nNodesP1= nNodes + 1;
  pclose(file);

  // Now form numastat command which removed all lines before "----- ------ ..." line
  // which separates data from initial warning.
  //                 numastat -c | awk -v n=3 '/-------/{flag=1} flag {for(i=1;i<=n;i++) printf "%s ", $i; printf "%s", $NF; print ""}'

  char numastat_m[256];
  char numastat_c[256];

  sprintf(numastat_m, "%s%d %s", \
          "numastat -m | awk -v n=", nNodesP1, \
          "'/-------/{flag=1} flag {for(i=1;i<=n;i++) printf \"%s \", $i; printf \"%s\", $NF; print \"\"}'");
  sprintf(numastat_c, "%s%d %s", \
          "numastat -c | awk -v n=", nNodesP1, \
          "'/-------/{flag=1} flag {for(i=1;i<=n;i++) printf \"%s \", $i; printf \"%s\", $NF; print \"\"}'");

//  Extract numasat -c data

  if (!(file = popen( numastat_m,   "r"))) { exit(1); }

  lineno=0; row=0;
  while ( fgets ( line, sizeof line, file ) != NULL ){
    if(lineno>1 && lineno<34){                          //printf("read: %d %s",lineno,line);

      char * token = strtok(line, " "); // tokenize
      token = strtok(NULL, " ");        // skip 1st token, identifier keyword
      i=0;

      while( token != NULL ) { 
        sscanf(token,"%f",  &numaStatM[row][i]);
        token = strtok(NULL, " ");
        i++;
      }   
      row++; 
    } // end selecting line 1-32
    lineno++;

  } // end while getting line  

  pclose(file);


//  Extract numasat -c data

  if (!(file = popen( numastat_c,   "r"))) { exit(1); }

  lineno=0; row=0;
  while ( fgets ( line, sizeof line, file ) != NULL ){
    if(lineno>0 && lineno<4){                            //printf("read: %s\n",line);

      char * token = strtok(line, " "); // tokenize
      token = strtok(NULL, " ");        // skip 1st token, identifier keyword
      i=0;

      while( token != NULL ) { 
        sscanf(token,"%ld",  &numaStatC[row][i]);
        token = strtok(NULL, " ");
        i++;
      }   
      if(row == 0) node_count=i-1;  // #nodes=#values less 1, last value  is a total).
      row++; 

    } // end selecting line 1-3
    lineno++;

  } // end while getting line   

  pclose(file);


// Write Data to file

  char output_path[PATH_BUFFER_SIZE];
  snprintf(output_path, sizeof(output_path), "%s/numa_stats_%s.txt", argv[3], argv[1]);
  file = fopen(output_path, "a+");
  
  fprintf(file, " \
%10.3f %2d \
%10.2f %10.2f %10.2f %10.2f %10.2f %10.2f %10.2f %10.2f %10.2f \
%15ld %15ld %15ld %15ld %15ld %15ld %15ld %15ld %15ld \
%10.2f %10.2f %10.2f %10.2f %10.2f %10.2f \n", \
    time_new,node_count, \
    M(12,0),M(12,1),M(12,2), M(14,0),M(14,1),M(14,2), M(24,0),M(24,1),M(24,2), \
    C(0,0),C(0,1),C(0,2), C(1,0),C(1,1),C(1,2), C(2,0),C(2,1),C(2,2), \
    M(0,0),M(0,1),M(0,2), M(1,0),M(1,1),M(1,2) );

  return EXIT_SUCCESS;
}
