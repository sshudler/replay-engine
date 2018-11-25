#ifndef __GRAPH_READER_H__
#define __GRAPH_READER_H__

#include "rep_engine.h"


namespace reader {
    
graph::Graph* readGraph (const char* dot_filename, const char* times_file = NULL);
    
    
}   // namespace reader




#endif  // __GRAPH_READER_H__

