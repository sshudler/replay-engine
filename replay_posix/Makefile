CXX      = icpc
CC       = icc
FLAGS    = -g -Wall -O3 -fpic -std=c++11 -I$(HOME)/ompt/libtdg-extended/timer -I$(HOME)/ompt/replay_engine/common -I${HOME}/install/include
LDFLAGS  = -L$(HOME)/ompt/llvm_omp_root/lib -L$(HOME)/install/lib -lftimer -lcgraph -lpthread
SRCS     = rep_posix.cc ../common/graph_reader.cc ../common/rep_engine.cc
OBJS     = $(SRCS:.cc=.o)
EXEC     = rep_posix


all: $(EXEC)


.cc.o:
	$(CXX) $(FLAGS) -c $< -o $@


$(EXEC): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^


clean:
	rm -f $(OBJS) *~ $(EXEC)

