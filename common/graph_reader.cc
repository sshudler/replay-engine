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


#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <graphviz/cgraph.h>

#include <graph_reader.h>



namespace reader {


static std::map<int64_t, double> g_task_times;


void extractNodeInfo( Agnode_t* dot_node, int64_t& internal_id, double& node_time ) {
    
    std::string label = agget( dot_node, "label" );
    size_t sep_idx1 = label.find_first_of( LABEL_SEP_CHR );
    size_t sep_idx2 = label.find_first_of( LABEL_SEP_CHR );
    node_time = atof( label.substr( 0, sep_idx1 - 1 ).c_str() );
    internal_id = atoi( label.substr( sep_idx1 + 1, sep_idx2 - 1 ).c_str() );
    
    // Get alternative time, if it exists
    std::map<int64_t, double>::iterator iter = g_task_times.find( internal_id );
    if( iter != g_task_times.end() )
        node_time = iter->second;
}

graph::Node* readNode( graph::Graph* n_graph, Agnode_t* dot_node, Agraph_t *dot_gr ) {
    
    int64_t internal_id;
    double node_time;
    extractNodeInfo( dot_node, internal_id, node_time );
    graph::Node*& curr_node = n_graph->getNode( internal_id );
    
    if( curr_node == NULL ) {
		Agedge_t* dot_edge = NULL;
        curr_node = new graph::Node( internal_id, node_time );
        for( dot_edge = agfstout( dot_gr, dot_node ); dot_edge; dot_edge = agnxtout( dot_gr, dot_edge ) ) {
            graph::Node* target_node = readNode( n_graph, dot_edge->node, dot_gr );
            graph::Graph::connectNodes( curr_node, target_node );
        }
    }
    return curr_node;
}

graph::Graph* readGraph( const char* dot_filename, const char* times_file ) {

    if( times_file ) {
        std::ifstream timesf( times_file );
        std::string in_str;
        
        timesf >> in_str;
        unsigned int num_nodes = atoi( in_str.c_str() );
        
        for( unsigned int i = 0; i < num_nodes; i++ ) {
            timesf >> in_str; timesf >> in_str; 
            int64_t wd_id = atoi( in_str.c_str() );
            timesf >> in_str; timesf >> in_str; 
            double task_time = atof( in_str.c_str() );
            g_task_times[wd_id] = task_time;
        }
        
        timesf.close();
    }

    FILE *dotfile = fopen( dot_filename, "r" );
    
    Agraph_t *dot_gr = agread( dotfile, 0 );
    fclose( dotfile );
    
    graph::Graph* n_graph = new graph::Graph;
    
    Agnode_t* dot_node = NULL;
    
    for( dot_node = agfstnode (dot_gr); dot_node; dot_node = agnxtnode(dot_gr, dot_node) ) {
        readNode( n_graph, dot_node, dot_gr );
    }
    
    agclose( dot_gr );
    
    return n_graph;
}

}   // namespace reader

//////////// For testing purposes:
//int main( int argc, char** argv ) {
	
	//if( argc < 2 ) {
		//std::cout << "Usage: treader <dot file>" << std::endl;
		//exit( -1 );
	//}

	//graph::Graph* gr = reader::readGraph (argv[1], NULL);
	//gr->computeCriticalPath ();
	//gr->printDotFile (std::string ("out.dot"));

	//return 0;
//}
////////////
