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


#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <vector>
#include <algorithm>

#include <rep_engine.h>
#include <graph_reader.h>


int main( int argc, char** argv ) {
	
	if( argc < 4 ) {
		std::cout << "Usage: " << argv[0] << " <dot_files_prefix> <num files> <output file>" << std::endl;
		exit( -1 );
	}

	unsigned int num_files = atoi (argv[2]);

	char in_filename[256];
	std::vector<graph::Graph*> gr_v;
	
    for( unsigned int i = 1; i <= num_files; ++i ) {
        sprintf( in_filename, "%s%d.dot", argv[1], i );
        gr_v.push_back( reader::readGraph( in_filename, NULL ) );
    }
    
    auto gr_nodes = gr_v[0]->getGraphNodes();
    for( auto node_iter = gr_nodes.begin(); node_iter != gr_nodes.end(); ++node_iter ) {
		int64_t curr_id = node_iter->first;
		std::vector<double> exec_time_v;
		for_each( gr_v.begin(), gr_v.end(), [=, &exec_time_v] (graph::Graph* gr) { 
			exec_time_v.push_back( gr->getNode( curr_id )->getTotalTime() );
		} );
		double avg_time, stddev_time, median;
		graph::compute_stats( exec_time_v, avg_time, stddev_time, median );
		node_iter->second->setTotalTime( median );
	}
    
    gr_v[0]->printDotFile( std::string( argv[3] ));
    
	for_each( gr_v.begin(), gr_v.end(), [] (graph::Graph* gr) { delete gr; } );
	
	return 0;
}
