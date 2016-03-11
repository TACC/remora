//========================================================================
// HEADER
//========================================================================
// DESCRIPTION
// mic_affinity
//
// Sets affinity of parent launching process to last core on 61 core KNC
//========================================================================
//- IMPLEMENTATION
//-      version     REMORA 1.6
//-      authors     Carlos Rosales (carlos@tacc.utexas.edu)
//-                  Antonio Gomez  (agomez@tacc.utexas.edu)
//-      license     MIT
//========================================================================

#define _GNU_SOURCE
#include <stdio.h>
#include <sched.h>

int main(int argc, char *argv[])
{	
	cpu_set_t set;

	CPU_ZERO( &set );
	CPU_SET( 243, &set );
	CPU_SET( 242, &set );
	CPU_SET( 241, &set );
	CPU_SET( 0, &set );
	
	if( sched_setaffinity(getppid(), sizeof(cpu_set_t), &set) == -1 ) exit(1);

	return 0;
}
