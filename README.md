# replay-engine
Replays task dependency graphs using either LLVM OpenMP runtime or pthreads. Assumes that the input graphs are
represented using GraphViz DOT files and that each node is a task with a given runtime. The strucure of the directories 
is as follows:

* `common/` -- common files to read a task graph from GraphViz DOT files (https://graphviz.gitlab.io/documentation/).

* `replay_omprt/` -- replays a graph using the LLVM OpenMP runtime API (`kmp.h`). Creates an explicit task 
   for each node in the graph and each edge is translated into a task dependency (`__kmpc_omp_task_with_deps`). 

* `replay_posix/` -- replays a graph using pthreads. Each thread maintains a queue of tasks for execution. Before executing
   the root task, the engine produces a schedule using a simple heuristics based on topological sort of the graph.

* `tdg_avg/` -- averages a given number of task graphs with same shape into one task graph. Basically, averages the 
   runtimes in each node of the graph.

* `test_inputs/` -- a number of test inputs.
