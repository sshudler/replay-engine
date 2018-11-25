#include <cstdlib>
#include <iostream>
#include <vector>
#include <algorithm>

#include <kmp.h>
#include <timer.h>
#include <rep_engine.h>
#include <graph_reader.h>




static bool g_print_debug = false;

static ident_t g_loc = {0, KMP_IDENT_KMPC, 0, 0, ";unknown;unknown;0;0;;"};
static std::list<graph::Node*> g_topo_list;


int task_func( int gtid, void* task_ptr ) {
	
	kmp_task_t* tt = (kmp_task_t*)task_ptr;
	double task_time = *(double*)(tt->shareds+0);	
	int64_t task_id = *(int64_t*)(tt->shareds+sizeof(double));	
	
	//std::cout << "Exec task, task id = " << task_id << ", tid = " << gtid << std::endl;
    //std::cout << "Inside a task, work time: " << *task_time << std::endl;

	BUSY_WAIT_LOOP(task_time);

	//std::cout << "Task finished, elapsed time: " << curr_time - start_time << std::endl;
	
	return 0;
}


double create_omp_tasks( int gtid ) {

	double total_task_time = 0.0;

	for( auto l_iter = g_topo_list.begin(); l_iter != g_topo_list.end (); ++l_iter ) {
		graph::Node* curr_node = *l_iter;

		int task_alloc_flags = 1;	// ???
		// pointer to block of pointers to shared vars (1 dbl + 1 long = task time, id):
		size_t sizeof_shareds = sizeof(double) + sizeof(int64_t);	
		kmp_task_t* n_task = __kmpc_omp_task_alloc( &g_loc, gtid, task_alloc_flags,
													sizeof(kmp_task_t), sizeof_shareds,
													task_func );
		
		unsigned int num_deps = curr_node->getEntries().size() + curr_node->getExits().size();
		unsigned int cnt = 0;
		kmp_depend_info_t* depend_info_arr = new kmp_depend_info_t[num_deps];
		// Iterate over entry edges
		for_each( curr_node->getEntries().begin(), curr_node->getEntries().end(), [&] (graph::Edge* edge) {
			depend_info_arr[cnt].base_addr = (long)edge;
			depend_info_arr[cnt].len = 1;	// Instead of sizeof(depend_vals[i]), 1 is enough
			depend_info_arr[cnt].flags.in = true;
			depend_info_arr[cnt].flags.out = false;
			++cnt;
		} );
		// Iterate over exit edges
		for_each( curr_node->getExits().begin(), curr_node->getExits().end(), [&] (graph::Edge* edge) {
			depend_info_arr[cnt].base_addr = (long)edge;
			depend_info_arr[cnt].len = 1;	// Instead of sizeof(depend_vals[i]), 1 is enough
			depend_info_arr[cnt].flags.in = true;	// out and inout have the same meaning here
			depend_info_arr[cnt].flags.out = true;
			++cnt;
		} );
		
		*((double*)(n_task->shareds+0)) = curr_node->getTotalTime();
		*((int64_t*)(n_task->shareds+sizeof(double))) = curr_node->getId();
		
		__kmpc_omp_task_with_deps( &g_loc, gtid, n_task, num_deps, depend_info_arr, 0, NULL );
	}

	return total_task_time;
}


void forked_func( int* gtid, int* btid, ... ) {
	
	if( __kmpc_single( &g_loc, *gtid ) == 1 )	{	// 1 -- means this thread should execute the construct
		
		create_omp_tasks( *gtid );
		
		__kmpc_end_single( &g_loc, *gtid );
	}
	
	//__kmpc_barrier( &g_loc, *gtid );  ===> no barrier equivalent to 'single nowait'
}


int main( int argc, char** argv ) {
	
	int gtid;
	int num_fork_args = 0;
	double start_time, total_time, total_task_time = 0;
	
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

	// Print topo_sort
	if (g_print_debug) {
		for_each( g_topo_list.begin(), g_topo_list.end(), [] (graph::Node* n) {
			std::cout << n->getId () << "  " << n->getTotalTime () << "  " << n->getLevel () << std::endl;
		} );
	}

	ftimer_init();

	std::vector<double> exec_time_v;
	std::cout << "Num iters: " << num_iters << std::endl;
	
	__kmpc_begin( &g_loc, 0 );

	for( unsigned int i = 0; i < num_iters; ++i ) {

		start_time = ftimer_msec();
	
		gtid = __kmpc_global_thread_num( &g_loc );
		__kmpc_fork_call( &g_loc, num_fork_args, forked_func );
	
		total_time = ftimer_msec() - start_time;

		exec_time_v.push_back( total_time );
	}
	
	__kmpc_end( &g_loc );

	for_each( g_topo_list.begin(), g_topo_list.end(), [&total_task_time] (graph::Node* n) { 
		total_task_time += n->getTotalTime(); 
	} );

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
