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
