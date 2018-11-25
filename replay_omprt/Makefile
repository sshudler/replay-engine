CXX      = icpc
CC       = icc
FLAGS    = -g -Wall -O3 -fpic -std=c++11 -I$(HOME)/ompt/llvm-openmp-extended/runtime/src -I$(HOME)/ompt/llvm-openmp-extended/BUILD/runtime/src -I$(HOME)/ompt/libtdg-extended/timer -I$(HOME)/ompt/replay_engine/common -I${HOME}/install/include
LDFLAGS  = -L$(HOME)/ompt/llvm_omp_root/lib -L$(HOME)/install/lib -lftimer -lomp -lcgraph
SRCS     = rep_omprt.cc ../common/graph_reader.cc ../common/rep_engine.cc
OBJS     = $(SRCS:.cc=.o)
EXEC     = rep_omprt


all: $(EXEC)


.cc.o:
	$(CXX) $(FLAGS) -c $< -o $@


$(EXEC): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^


simple_omprt: simple_omprt.cc
	$(CXX) $(FLAGS) $(LDFLAGS) -o $@ $^


tasks: tasks.c
	$(CXX) $(FLAGS) -qopenmp $(LDFLAGS) -o $@ $^


clean:
	rm -f $(OBJS) *~ $(EXEC)

