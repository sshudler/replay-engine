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

static bool g_print_debug = false;

static ident_t g_loc = {0, KMP_IDENT_KMPC, 0, 0, ";unknown;unknown;0;0;;"};
static int depend_vals[NTASKS];

static std::list<graph::Node*> g_topo_list;


static void execute_task (void* ptr) {

	double* task_time = (double*)ptr;

    //std::cout << "Inside a task, work time: " << *task_time << std::endl;

    double start_time, end_time, curr_time;

    start_time = curr_time = ftimer_msec();
    end_time = *task_time + start_time;
    while (curr_time < end_time)
		curr_time = ftimer_msec();

	//std::cout << "Task finished, elapsed time: " << curr_time - start_time << std::endl;
}

double create_omp_tasks( ) {

	double total_task_time = 0.0;

	for( auto l_iter = g_topo_list.begin(); l_iter != g_topo_list.end (); ++l_iter ) {
		graph::Node* curr_node = *l_iter;

		int task_alloc_flags = 1;	// ???
		size_t sizeof_shareds = 8;	// pointer to block of pointers to shared vars -- 1 double = task time
		kmp_task_t* n_task = __kmpc_omp_task_alloc( &g_loc, *gtid, task_alloc_flags,
													sizeof(kmp_task_t), sizeof_shareds,
													task_func );
		
		unsigned int num_deps = curr_node->getEntries().size() + curr_node->getExits().size();
		unsigned int cnt = 0;
		kmp_depend_info_t* depend_info_arr = new kmp_depend_info_t[num_deps];
		// Iterate over entry edges
		for_each( curr_node->getEntries().begin(), curr_node->getEntries().end(), [&] (Edge* edge) {
			depend_info_arr[cnt].base_addr = (long)edge;
			depend_info_arr[cnt].len = 1;	// Instead of sizeof(depend_vals[i]), 1 is enough
			depend_info_arr[cnt].flags.in = true;
			depend_info_arr[cnt].flags.out = false;
			++cnt;
		} );
		// Iterate over exit edges
		for_each( curr_node->getExits().begin(), curr_node->getExits().end(), [&] (Edge* edge) {
			depend_info_arr[cnt].base_addr = (long)edge;
			depend_info_arr[cnt].len = 1;	// Instead of sizeof(depend_vals[i]), 1 is enough
			depend_info_arr[cnt].flags.in = false;
			depend_info_arr[cnt].flags.out = true;
			++cnt;
		} );
		
		
	}

	return total_task_time;
}

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
			depend_info_arr[0].flags.in = false;
			depend_info_arr[0].flags.out = true;
			if( i > 0 ) {	// In-dependency
				++num_deps;
				depend_info_arr[1].base_addr = (long)&depend_vals[i-1];
				depend_info_arr[1].len = 1;	// Instead of sizeof(depend_vals[i]), 1 is enough
				depend_info_arr[1].flags.in = true;
				depend_info_arr[1].flags.out = false;
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
	int arg = 0;
	
	if( argc < 3 ) {
		std::cout << "Usage: replay <dot file> <num iters> [debug] [times file]" << std::endl;
		exit( -1 );
	}

	unsigned int num_iters = atoi (argv[2]);

	g_print_debug = argc >= 4 ? (atoi (argv[3]) == 1) : false;
	char* times_file = argc >= 5 ? argv[4] : NULL;

	graph::Graph* gr = reader::readGraph (argv[1], times_file);
	//gr->printDotFile (std::string ("out.dot"));

	gr->topoSort( g_topo_list );

	double start_time, total_time, total_task_time;
	ftimer_init();

	// Print topo_sort
	//for_each( topo_list.begin(), topo_list.end(), [] (graph::Node* node) {
	//	std::cout << node->getId () << "  " << node->getTotalTime () << "  " << node->getLevel () << std::endl;
	//} )

	std::vector<double> exec_time_v;
	std::cout << "Num iters: " << num_iters << std::endl;

	for( unsigned int i = 0; i < num_iters; ++i ) {

		start_time = ftimer_msec();

		// __kmpc_begin
	
		gtid = __kmpc_global_thread_num( &g_loc );
		__kmpc_fork_call( &g_loc, num_fork_args, forked_func );
	
		// __kmpc_end
	
		total_time = ftimer_msec() - start_time;

		exec_time_v.push_back( total_time );
	}

	double avg_time, stddev_time, median;
	graph::compute_stats( exec_time_v, avg_time, stddev_time, median );

	std::cout << "[TOTAL TASK TIME] " << total_task_time << std::endl;
	std::cout << "[AVG EXEC TIME] " << avg_time << std::endl;
	std::cout << "[STDDEV EXEC TIME] " << stddev_time << std::endl;
	std::cout << "[MEDIAN EXEC TIME] " << median << std::endl;
	std::cout << "[NUM SAMPLES] " << exec_time_v.size() << std::endl;

	//
	// Free memory
	//
	delete gr;
	
	
	return 0;
}
