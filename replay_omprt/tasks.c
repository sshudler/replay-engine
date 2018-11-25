// Copyright (c) 2018 Sergei Shudler
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


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
