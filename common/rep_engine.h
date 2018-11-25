#ifndef __REP_ENGINE_H__
#define __REP_ENGINE_H__


#include <stdint.h>
#include <cstdlib>
#include <vector>
#include <map>
#include <list>
#include <queue>



#define BUSY_WAIT_LOOP(runtime)                                        \
		double start_time, end_time, curr_time;                        \
        start_time = curr_time = ftimer_msec();                        \
        end_time = (runtime) + start_time;                             \
        while (curr_time < end_time) curr_time = ftimer_msec();        \
        
#define LABEL_SEP_CHR	'*'


namespace graph {

void compute_stats( std::vector<double>& arr, double& avg, double& stddev, double& median );

class Node;
class Graph;

//=================================

class Edge {
public:
    // Ctor
    Edge( Node* source, Node* target ) : _source( source ), _target( target ) {}
        
    Node* getSource() const { return _source; }
    Node* getTarget() const { return _target; }
    
private:
    Node* _source;
    Node* _target;
};        

//=================================

class Node {
public:
	
	friend class Graph;

    class AdjacencyIterator {
	public:	
		bool operator==( const AdjacencyIterator& other ) {
			return( _edge_iter == other._edge_iter );
		}
		bool operator!=( const AdjacencyIterator& other ) {
			return !( *this == other );
		}
		Node* operator*() {
			return( _entry_iter ? (*_edge_iter)->getSource() : (*_edge_iter)->getTarget() );
		} 
		AdjacencyIterator& operator++() {
			++_edge_iter;
			return *this;
		}
		
		static AdjacencyIterator beginAdjIter( Node* node, bool entry_iter ) {
			return AdjacencyIterator( node, false, entry_iter );
		}
		
		static AdjacencyIterator endAdjIter( Node* node, bool entry_iter ) {
			return AdjacencyIterator( node, true, entry_iter );
		}
	
	private:
		AdjacencyIterator( Node* node, bool end_iter, bool entry_iter ) 
		: _entry_iter( entry_iter ) {
			if( end_iter ) {
				_edge_iter = _entry_iter ? node->getEntries().end() : node->getExits().end();
			}
			else {
				_edge_iter = _entry_iter ? node->getEntries().begin() : node->getExits().begin();
			}
		}
		std::vector<Edge*>::const_iterator _edge_iter;
		bool _entry_iter;
	};
    
    // Ctor
    Node( int64_t id, double total_time ) 
        : _id( id ), _totalTime( total_time ), _visited( false ), _level( -1 ),
          _finishTime( 0 ), _pathLength( 0 ), _pathTime( 0 ), _prevCritical( NULL ),
          _isCritical( false ) {}
        
    int64_t getId() const 		{ return _id; }
    double getTotalTime() const { return _totalTime; }
    void setTotalTime( double t )	{ _totalTime = t; }
    bool& getVisited() 			{ return _visited; }
    int& getLevel() 			{ return _level; }
    double& finishTime() 		{ return _finishTime; }
    void setCritical() 			{ _isCritical = true; }
    bool isCritical() 			{ return _isCritical; }
    std::vector<Edge*>& getEntries() 	{ return _entryEdges; }
    std::vector<Edge*>& getExits() 		{ return _exitEdges; }
    bool isConnectedWith( Node* target ) const;
    Edge* getConnection( Node* target ) const;
    double maxPredFinishTime();

private:
    int64_t _id;
    double  _totalTime;
    bool    _visited;
    int     _level;
    double  _finishTime;
    double  _pathLength;
    double  _pathTime;
    Node*   _prevCritical;
    bool    _isCritical;
    std::vector<Edge*> _entryEdges;
    std::vector<Edge*> _exitEdges;
};

//=================================

class Graph {
public:

    Graph() {}
    
    ~Graph() {
        
        // TODO
        //for (std::set<Node*>::iterator it = _graphNodes.begin(); it != _graphNodes.end(); ++it) {
		//	Node* curr_node = *it;
        //}
    }
    
    void addNode( int64_t id, Node* node )		{ _graphNodes[id] = node; }
    Node*& getNode( int64_t id )				{ return _graphNodes[id]; }
    std::map<int64_t, Node*>& getGraphNodes()	{ return _graphNodes; }
    
    void printDotFile( const std::string& file_name );
    void topoSort( std::list<Node*>& topo_list );
    void computeCriticalPath();
    
    static void connectNodes (Node* source, Node* target);
    
private:
    void visitNode (Node* curr_node, std::list<Node*>& topo_list);

    std::map<int64_t, Node*> _graphNodes;
    typedef std::map<int64_t, Node*>::iterator NodesIterator;
    
    double _critical_path_time_len;
    unsigned int _critical_path_len;
};

}	// end namespace graph

#endif  // __REP_ENGINE_H__

