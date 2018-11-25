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
#include <time.h>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <algorithm>

#include <rep_engine.h>


namespace graph {

//========================== Node ======================================

bool Node::isConnectedWith (Node* target) const {

	bool res = false;
	for( auto it = _exitEdges.begin (); it != _exitEdges.end (); ++it ) {
		if ((*it)->getTarget () == target) {
			res = true;
			break;
		}
	}
	return res;
}

Edge* Node::getConnection (Node* target) const {

	Edge* result = NULL;
	for( auto it = _exitEdges.begin (); it != _exitEdges.end (); ++it ) {
		if ((*it)->getTarget () == target) {
			result = *it;
			break;
		}
	}
	return result;
}

double Node::maxPredFinishTime () {

	double max_finish_time = 0.0;
	Node::AdjacencyIterator adj_iter =
			Node::AdjacencyIterator::beginAdjIter (this, true);    // Iterate over entry edges
	Node::AdjacencyIterator adj_iter_end =
			Node::AdjacencyIterator::endAdjIter (this, true);      // Iterate over entry edges
	while (adj_iter != adj_iter_end) {
		Node* source_node = *adj_iter;
		max_finish_time = std::max (source_node->finishTime (), max_finish_time);
		++adj_iter;
	}

	return max_finish_time;
}

//========================= Graph ======================================

void Graph::visitNode (Node* curr_node, std::list<Node*>& topo_list) {

	curr_node->getVisited () = true;

	Node::AdjacencyIterator adj_iter =
			Node::AdjacencyIterator::beginAdjIter (curr_node, false);   // Iterate over exit edges
	Node::AdjacencyIterator adj_iter_end =
			Node::AdjacencyIterator::endAdjIter (curr_node, false);     // Iterate over exit edges
	while (adj_iter != adj_iter_end) {
		Node* adj_node = *adj_iter;
		if (!adj_node->getVisited ())
			visitNode (adj_node, topo_list);
		++adj_iter;
	}

	topo_list.push_front (curr_node);
}

void Graph::topoSort (std::list<Node*>& topo_list) {

	for (Graph::NodesIterator it = _graphNodes.begin(); it != _graphNodes.end(); ++it) {
		Node* curr_node = it->second;
		if (!curr_node->getVisited ())  // Unmarked node
			visitNode (curr_node, topo_list);
	}

	int max_level = -1;
	for( auto l_iter = topo_list.begin(); l_iter != topo_list.end(); ++l_iter ) {
		Node* curr_node = *l_iter;

		int max_curr_level = 0;
		Node::AdjacencyIterator adj_iter =
				Node::AdjacencyIterator::beginAdjIter (curr_node, true);    // Iterate over entry edges
		Node::AdjacencyIterator adj_iter_end =
				Node::AdjacencyIterator::endAdjIter (curr_node, true);      // Iterate over entry edges
		while (adj_iter != adj_iter_end) {
			Node* source_node = *adj_iter;
			max_curr_level = std::max (source_node->getLevel () + 1, max_curr_level);
			++adj_iter;
		}
		curr_node->getLevel () = max_curr_level;
		max_level = std::max (max_level, max_curr_level);
	}

	std::list<Node*>::iterator start_iter = topo_list.begin ();
	for (int i = 0; i < max_level; ++i) {
		while (start_iter != topo_list.end () && (*start_iter)->getLevel () == i)
			++start_iter;
		if (start_iter == topo_list.end ())
			break;
		std::list<Node*>::iterator l_iter = start_iter;
		++l_iter;
		while (l_iter != topo_list.end ()) {
			Node* curr_node = *l_iter;
			if (curr_node->getLevel () == i) {
				std::list<Node*>::iterator next_iter = l_iter;
				++next_iter;
				topo_list.erase (l_iter);
				topo_list.insert (start_iter, curr_node);
				l_iter = next_iter;
			}
			else {
				++l_iter;
			}
		}
	}
}

void Graph::computeCriticalPath () {
		
		_critical_path_time_len = 0.0;
		_critical_path_len = 0;
		std::list<Node*> topo_list;
		topoSort (topo_list);
		
        Node* last_critical_node = NULL;
        
		//std::cerr << "CriticalPathMetric - scanning nodes:" << std::endl;
		for( auto l_iter = topo_list.begin(); l_iter != topo_list.end(); ++l_iter ) {
			Node* curr_node = *l_iter;
			double max_curr_path_time = curr_node->_totalTime;
			int max_curr_path_len = 0;
			Node::AdjacencyIterator adj_iter = 
				Node::AdjacencyIterator::beginAdjIter (curr_node, true);     // Iterate over entry edges
			Node::AdjacencyIterator adj_iter_end = 
				Node::AdjacencyIterator::endAdjIter (curr_node, true);   // Iterate over entry edges
			while (adj_iter != adj_iter_end) {
				Node* source_node = *adj_iter;
                if (source_node->_pathLength + 1 > max_curr_path_len) {
                    max_curr_path_len = source_node->_pathLength + 1;
                    //curr_node->_prev_critical = source_node;
                }
                if (source_node->_pathTime + curr_node->_totalTime > max_curr_path_time) {
                    max_curr_path_time = source_node->_pathTime + curr_node->_totalTime;
                    curr_node->_prevCritical = source_node;
                }
				++adj_iter;
			}
			curr_node->_pathLength = max_curr_path_len;
			curr_node->_pathTime = max_curr_path_time;
			
			//std::cerr << "Curr node: " << curr_node->get_wd_id () << ", path length: " << curr_node->_path_length << std::endl;
			//std::cerr << max_curr_path_len << "   " << max_curr_path_time << std::endl;
			
            if (max_curr_path_len > _critical_path_len) {
                _critical_path_len = max_curr_path_len;
                //last_critical_node = curr_node;
            }
            if (max_curr_path_time > _critical_path_time_len) {
                _critical_path_time_len = max_curr_path_time;
                last_critical_node = curr_node;
            }
		}
		
        double total_time_on_criticial_path = 0.0;
        while (last_critical_node) {
            last_critical_node->setCritical();
            total_time_on_criticial_path += last_critical_node->_totalTime;
            last_critical_node = last_critical_node->_prevCritical;
        }
	}
	
void Graph::connectNodes (Node* source, Node* target) {

	if (!source->isConnectedWith (target)) {
		Edge* new_edge = new Edge (source, target);
		source->getExits ().push_back (new_edge);
		target->getEntries ().push_back (new_edge);
	}
}

void Graph::printDotFile (const std::string& file_name) {

	std::stringstream str_stream;

	str_stream << "digraph {" << std::endl;
	for (Graph::NodesIterator it = _graphNodes.begin(); it != _graphNodes.end(); ++it) {
		Node* curr_node = it->second;
		if (curr_node->isCritical()) {
			str_stream << curr_node->getId () << " [style=filled, fillcolor=orange, label=\"" << curr_node->getTotalTime ()
                		   << " " << LABEL_SEP_CHR << " " << curr_node->getId () << "\"];" << std::endl;
		}
		else {
			str_stream << curr_node->getId () << " [label=\"" << curr_node->getTotalTime ()
							<< " " << LABEL_SEP_CHR << " " << curr_node->getId () << "\"];" << std::endl;
		}

		Node::AdjacencyIterator adj_iter =
				Node::AdjacencyIterator::beginAdjIter (curr_node, false);   // Iterate over exit edges
		Node::AdjacencyIterator adj_iter_end =
				Node::AdjacencyIterator::endAdjIter (curr_node, false);     // Iterate over exit edges
		while (adj_iter != adj_iter_end) {
			Node* adj_node = *adj_iter;
			str_stream << curr_node->getId () << " -> " << adj_node->getId () << ";" << std::endl;
			++adj_iter;
		}
	}

	str_stream << "}" << std::endl;

	std::ofstream dot_file;
	dot_file.open (file_name.c_str());
	if (dot_file.is_open()) {
		dot_file << str_stream.str ();
		dot_file.close();
	}
}

void graph::compute_stats( std::vector<double>& arr, double& avg, double& stddev, double& median ) {

	avg = 0.0;
	for_each( arr.begin(), arr.end(), [&avg] (double val) { avg += val; } );
	avg /= arr.size();

	double variance = 0.0;
	for_each( arr.begin(), arr.end(), [=, &variance] (double val) { variance += (val - avg) * (val - avg); } );
	variance /= arr.size();

	stddev = std::sqrt (variance);
	
	std::sort( arr.begin(), arr.end() );
	median = arr[arr.size() / 2];
	if ( arr.size() % 2 == 0 ) {
		median = (median + arr[arr.size() / 2 - 1]) / 2.0;
	}
}

}	// end namespace graph
