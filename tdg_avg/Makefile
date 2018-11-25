CXX      = icpc
CC       = icc
FLAGS    = -g -Wall -O3 -fpic -std=c++11 -I$(HOME)/ompt/replay_engine/common -I${HOME}/install/include
LDFLAGS  = -L$(HOME)/install/lib -lcgraph
SRCS     = tdg_avg.cc ../common/graph_reader.cc ../common/rep_engine.cc
OBJS     = $(SRCS:.cc=.o)
EXEC     = tdg_avg


all: $(EXEC)


.cc.o:
	$(CXX) $(FLAGS) -c $< -o $@


$(EXEC): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^


clean:
	rm -f $(OBJS) *~ $(EXEC)

