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


#include <pthread.h>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <algorithm>

#include <timer.h>
#include <rep_engine.h>
#include <graph_reader.h>

#include "rep_posix.h"


#define EPSILON     0.00001


static bool g_print_debug = false;

//========================= Thread =====================================

void Thread::printTaskQ () {

	std::queue<Task*> tmp_q;

	while (!_taskQ.empty ()) {
		Task* top_task = _taskQ.front ();
		///
		std::cout << "(" << top_task->getId () << ", " << top_task->getTaskTime () << ")" << std::endl;
		///
		tmp_q.push (top_task);
		_taskQ.pop ();
	}

	_taskQ = tmp_q;
}

void Thread::run () {

	while (!_taskQ.empty ()) {
		Task* top_task = _taskQ.front ();
		///
		//std::cout << "Executing task: " << top_task->getId () << " time: " << top_task->getTaskTime () << std::endl;
		///
		top_task->run ();
		delete top_task;
		_taskQ.pop ();
	}
	pthread_exit(NULL);
}

int Thread::create () {

	pthread_attr_t attr;
	pthread_attr_init (&attr);
	pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_JOINABLE);
	int ret_code = pthread_create (&_threadHandle, &attr, Thread::runFunc, (void*)this);
	pthread_attr_destroy (&attr);
	return ret_code;
}

//========================= WorkTask ===================================

void WorkTask::run () {

	//__asm volatile
	//(
	//"movq %0, %%rcx  \n\t"
	//"looper:         \n\t"
	//"nop             \n\t"
	//"loop looper     \n\t "
	//:                   /* output */
	//: "r"(_nopIters)    /* input */
	//: "rcx"             /* clobbered register */
	//);
	//for (unsigned int i = 0; i < _nopIters; ++i)
	//    __asm volatile ("nop");

	BUSY_WAIT_LOOP(_taskTime);

	//double total_time = curr_time - start_time;
	//std::cout << "Task: " << _id << " ran for: " << total_time << std::endl;
}

//========================= WaitTask ===================================

void WaitTask::run () {

	//double start_time, total_time;

	//start_time = ftimer_msec();

	// Get curr time
	struct timespec tv;
	clock_gettime(CLOCK_MONOTONIC, &tv);

	// Add the time to sleep
	tv.tv_sec += _timeSec;
	tv.tv_nsec += _timeNanoSec;
	tv.tv_sec += tv.tv_nsec / 1000000000;
	tv.tv_nsec = tv.tv_nsec % 1000000000;

	// Sleep
	while (clock_nanosleep (CLOCK_MONOTONIC, TIMER_ABSTIME, &tv, NULL) != 0);

	//total_time = ftimer_msec() - start_time;
	//std::cout << "Task: " << _id << " ran for: " << total_time << std::endl;
}


//========================= functions ==================================

void find_min_max_finish_threads (std::vector<Thread*> thread_vec, Thread*& min_thread, Thread*& max_thread) {

	for (unsigned int i = 0; i < thread_vec.size (); i++) {
		if (thread_vec[i]->getFinishTime () < min_thread->getFinishTime ())
			min_thread = thread_vec[i];
		if (thread_vec[i]->getFinishTime () > max_thread->getFinishTime ())
			max_thread = thread_vec[i];
	}
}

double schedule_tasks_1_thread (const std::list<graph::Node*>& topo_list, std::vector<Thread*> thread_vec) {

	double total_task_time = 0.0;

	//
	// Simple case of just 1 thread
	//
	for( auto l_iter = topo_list.begin (); l_iter != topo_list.end (); ++l_iter ) {
		graph::Node* curr_node = *l_iter;
		thread_vec[0]->addTask (new WorkTask (curr_node->getId (), curr_node->getTotalTime ()));
		total_task_time += curr_node->getTotalTime ();
	}

	return total_task_time;
}

double schedule_tasks_N_thread (std::list<graph::Node*>& topo_list, std::vector<Thread*> thread_vec) {

	double total_task_time = 0.0;

	//
	// N threads
	//
	Thread* min_thread = thread_vec[0];
	Thread* max_thread = thread_vec[0];
	for( auto l_iter = topo_list.begin (); l_iter != topo_list.end (); ++l_iter ) {
		graph::Node* curr_node = *l_iter;
		total_task_time += curr_node->getTotalTime ();
		double max_finish_time = curr_node->maxPredFinishTime ();

		if (max_finish_time < max_thread->getFinishTime ()) {
			// Schedule task on earliest finish time thread
			double time_diff = max_finish_time - min_thread->getFinishTime ();
			if (time_diff > EPSILON)
				min_thread->addTask (new WorkTask (-1, time_diff));
			min_thread->addTask (new WorkTask (curr_node->getId (), curr_node->getTotalTime ()));
			curr_node->finishTime () = min_thread->getFinishTime ();

			find_min_max_finish_threads (thread_vec, min_thread, max_thread);
		}
		else {
			// Schedule task on latest finish time thread
			double time_diff = max_finish_time - max_thread->getFinishTime ();
			if (time_diff > EPSILON)
				max_thread->addTask (new WorkTask (-1, time_diff));
			max_thread->addTask (new WorkTask (curr_node->getId (), curr_node->getTotalTime ()));
			curr_node->finishTime () = max_thread->getFinishTime ();

			if (min_thread == max_thread) {
				find_min_max_finish_threads (thread_vec, min_thread, max_thread);
			}
		}
	}

	return total_task_time;
}



//========================= main ======================================

int main (int argc, char** argv) {

	if (argc < 4) {
		std::cout << "Usage: replay <dot file> <num_threads> <num iters> [debug] [times file]" << std::endl;
		exit (-1);
	}

	const unsigned int WARMUP_ITERS = 5;
	unsigned int num_threads = atoi (argv[2]);
	unsigned int num_iters = atoi (argv[3]);
	g_print_debug = argc >= 5 ? (atoi (argv[4]) == 1) : false;
	char* times_file = argc >= 6 ? argv[5] : NULL;

	graph::Graph* gr = reader::readGraph (argv[1], times_file);
	//gr->printDotFile (std::string ("out.dot"));
	std::list<graph::Node*> topo_list;
	gr->topoSort (topo_list);

	//////
	//gr->computeCriticalPath ();
	//gr->printDotFile("test.dot");
	//////

	double start_time, total_time, total_task_time;
	ftimer_init();

	// Print topo_sort
	if (g_print_debug) {
		for_each( topo_list.begin(), topo_list.end(), [] (graph::Node* node) {
			std::cout << node->getId () << "  " << node->getTotalTime () << "  " << node->getLevel () << std::endl;
		} );
	}

	std::vector<Thread*> thread_vec;
	std::vector<double> exec_time_v;

	for (unsigned int j = 0; j < num_threads; ++j)
		thread_vec.push_back (new Thread);

	std::cout << "Num threads: " << num_threads << std::endl;
	std::cout << "Num iters: " << num_iters << std::endl;

	for (unsigned int i = 0; i < num_iters + WARMUP_ITERS; ++i) {
		
		for_each( topo_list.begin(), topo_list.end(), [] (graph::Node* n) { n->finishTime() = -1.0; } );
		
		start_time = ftimer_msec();

		if (num_threads > 1) {
			total_task_time = schedule_tasks_N_thread (topo_list, thread_vec);
			if (g_print_debug) {
				for (unsigned int i = 0; i < num_threads; i++) {
					std::cout << "Thread " << i << " task Q:" << std::endl;
					thread_vec[i]->printTaskQ ();
				}
			}
		}
		else {
			total_task_time = schedule_tasks_1_thread (topo_list, thread_vec);
		}

		for_each( thread_vec.begin(), thread_vec.end(), [] (Thread* th) { th->create(); } );
		for_each( thread_vec.begin(), thread_vec.end(), [] (Thread* th) { th->waitForCompletion(); } );

		total_time = ftimer_msec() - start_time;

		for_each( thread_vec.begin(), thread_vec.end(), [] (Thread* th) { th->reset(); } );

		if (i < WARMUP_ITERS)
			continue;

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
	for_each( thread_vec.begin(), thread_vec.end(), [] (Thread* th) { delete th; } );
	delete gr;


	return 0;
}
