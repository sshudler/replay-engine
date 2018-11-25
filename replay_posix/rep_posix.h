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


#ifndef __REP_POSIX_H__
#define __REP_POSIX_H__


#include <stdint.h>
#include <pthread.h>
#include <cstdlib>
#include <vector>
#include <map>
#include <list>
#include <queue>



//=================================

class Task {
public:
    Task (int64_t id, double task_time) : _id(id), _taskTime (task_time) {}
    virtual void run () = 0;
    
    double getTaskTime () const { return _taskTime; }
    
    int64_t getId () const { return _id; }
    
protected:
    double      _taskTime;   // in ms
    int64_t     _id;
};

//=================================

class WorkTask : public Task {
public:
    WorkTask (int64_t id, double task_time) : Task (id, task_time), _nopIters( 0 ) {
        //_nopIters = (uint64_t)((_taskTime * 0.001) * (double)timing::get_cycles_per_sec ());
    }
    
    virtual void run ();
    
private:
    uint64_t _nopIters;
};

//=================================

class WaitTask : public Task {
public:
    WaitTask (int64_t id, double task_time) : Task (id, task_time) {
        _timeSec = (unsigned int)(_taskTime / 1000.0); 
        _timeNanoSec = (unsigned int)((_taskTime - _timeSec) * 1000000.0);
    }
    
    virtual void run ();
    
private:
    unsigned int _timeSec;
    unsigned int _timeNanoSec;
};

//=================================

class Thread {
public:
    Thread () : _threadHandle(NULL), _finishTime (0.0) {}
    
    ~Thread () {}
    
    void addTask (Task* task) { 
        _taskQ.push (task); 
        _finishTime += task->getTaskTime ();
    }
    
    double getFinishTime () { return _finishTime; }
    
    void printTaskQ ();
    
    int create ();
    
    void run ();
    
    void* waitForCompletion () {
        
        void* status;
        pthread_join (_threadHandle, &status);
        return status;
    }
    
    void reset () {
    	_threadHandle = NULL;
    	_finishTime = 0.0;
    }

    static void* runFunc (void* ptr) {
        
        ((Thread*)ptr)->run ();
        return NULL;
    }
    
private:
    pthread_t _threadHandle;
    std::queue<Task*> _taskQ;
    double _finishTime;
};







#endif  // __REP_POSIX_H__

