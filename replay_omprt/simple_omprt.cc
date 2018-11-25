#include <cstdlib>
#include <iostream>
#include <vector>
#include <algorithm>

#include <kmp.h>
#include <timer.h>
#include <rep_engine.h>
#include <graph_reader.h>

#define NTASKS	10

//extern "C" int omp_get_thread_num (void);


static ident_t g_loc = {0, KMP_IDENT_KMPC, 0, 0, ";unknown;unknown;0;0;;"};
static int depend_vals[NTASKS];


int task_func( int gtid, void* task_ptr ) {
	
	//int tid = omp_get_thread_num();
	kmp_task_t* tt = (kmp_task_t*)task_ptr;
	std::cout << "Inside a task: gtid = " << gtid << ", i = " << *((double*)tt->shareds) << std::endl;
	
	return 0;
}

void forked_func( int* gtid, int* btid, ... ) {
	
	if( __kmpc_single( &g_loc, *gtid ) == 1 )	{	// 1 -- means this thread should execute the construct
		
		//create_omp_tasks();
		for( int i = 0; i < NTASKS; ++i ) {
			int task_alloc_flags = 1;	// ???
			size_t sizeof_shareds = 8;	// pointer to block of pointers to shared vars -- 1 double = task time
			kmp_task_t* n_task = __kmpc_omp_task_alloc( &g_loc, *gtid, task_alloc_flags,
														sizeof(kmp_task_t), sizeof_shareds,
														task_func );
			kmp_depend_info_t depend_info_arr[2];
			int num_deps = 1;
			depend_info_arr[0].base_addr = (long)&depend_vals[i];
			depend_info_arr[0].len = 1;	// Instead of sizeof(depend_vals[i]), 1 is enough
			depend_info_arr[0].flags.in = 1;
			depend_info_arr[0].flags.out = 1;
			if( i < 0 ) {	// In-dependency
				++num_deps;
				depend_info_arr[1].base_addr = (long)&depend_vals[i-1];
				depend_info_arr[1].len = 1;	// Instead of sizeof(depend_vals[i]), 1 is enough
				depend_info_arr[1].flags.in = 1;
				depend_info_arr[1].flags.out = 0;
			}
			*((double*)n_task->shareds) = double(i) + 0.5;
			//__kmpc_omp_task( &loc1, *gtid, n_task );
			__kmpc_omp_task_with_deps( &g_loc, *gtid, n_task, num_deps, depend_info_arr, 0, NULL );
		}
		
		__kmpc_end_single( &g_loc, *gtid );
	}
	
	__kmpc_barrier( &g_loc, *gtid );
}


int main( int argc, char** argv ) {
	
	int gtid;
	int num_fork_args = 0;
	//int arg = 0;

	//double start_time, total_time, total_task_time;
	//ftimer_init();

	// Print topo_sort
	//for_each( topo_list.begin(), topo_list.end(), [] (graph::Node* node) {
	//	std::cout << node->getId () << "  " << node->getTotalTime () << "  " << node->getLevel () << std::endl;
	//} )

	__kmpc_begin( &g_loc, 0 );

	//start_time = ftimer_msec();
	
	gtid = __kmpc_global_thread_num( &g_loc );
	__kmpc_fork_call( &g_loc, num_fork_args, forked_func );
	
	__kmpc_end( &g_loc );
	
	//total_time = ftimer_msec() - start_time;
	

	//
	// Free memory
	//
	//delete gr;
	
	
	return 0;
}
