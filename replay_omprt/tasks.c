#include <stdio.h>
#include <math.h>
#include <omp.h>


#define NTASKS     10

static int depend_vals[NTASKS];

void task_func( int val )
{
	printf( "Task: %d\n", val );
}

int main( int argc, char** argv )
{
  #pragma omp parallel
  {
    #pragma omp single
    {
		for( int i = 0; i < NTASKS; ++i ) 
		{
			if( i > 1 ) 
			{
				#pragma omp task depend(in:depend_vals[i-1]) depend(out:depend_vals[i]) firstprivate(i)
				{
					task_func( i );
				}
			}
			else
			{
				int* out_ptr = &depend_vals[i];
				#pragma omp task depend(out:depend_vals[i]) firstprivate(i)
				{
					task_func( i );
				}
			}
		}
    }	// end of single
  }	// end of parallel

  return 0;
}
