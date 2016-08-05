
/**
Author : lufyzhang
Date   : 2016/8/5
*/
#ifndef THREAD_SIMPLE_H
#define  THREAD_SIMPLE_H
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <queue>
#include <assert.h>
#include <functional>
#include <stdint.h>

typedef std::function<void(void)> CallBack;

template<typename T>
class Queue{// index same is empty and less 1 is full
  //not safe and shouldn't use directly,which can't ensure access invalid element
private:
   T* m_pData;
   uint32_t m_nSize;
   uint32_t m_nReadIndex;
   uint32_t m_nWriteIndex;
 public:
   Queue(int nSize):m_nSize(nSize+1){
     m_pData = new T[m_nSize];
     m_nReadIndex = m_nWriteIndex = 0;
   }
   ~Queue(){
     delete []m_pData;
   }

   int Enqueue(T item){//if queue full it would be failed and result is not expected!
     m_pData[m_nWriteIndex] = item;
     m_nWriteIndex = (m_nWriteIndex + 1) % m_nSize;
     return 0;
   }

   T Dequeue(){
      T ret = m_pData[m_nReadIndex];
      m_nReadIndex = (m_nReadIndex + 1) % m_nSize;
      return ret;
   }

   bool Full(){
     return ((m_nWriteIndex + 1) % m_nSize == m_nReadIndex);
   }

   bool Empty(){
     return (m_nWriteIndex == m_nReadIndex);
   }

   bool Size(){
     return m_nSize - 1;//one space for check empty or full
   }
};

class ThreadSimple{
pthread_mutex_t m_mutex;
pthread_cond_t m_notFull;
pthread_cond_t m_notEmpty;
pthread_cond_t m_idle;
Queue<CallBack> _queue;
int m_nSize;
int m_nWorker;
pthread_t *_threads;
bool m_bQuit;

public:
  explicit ThreadSimple(int nSize,int nWorker=4):m_nSize(nSize),m_nWorker(nWorker),_queue(nSize){
    pthread_mutex_init(&m_mutex,NULL);
    pthread_cond_init(&m_notFull,NULL);
    pthread_cond_init(&m_notEmpty,NULL);
    pthread_cond_init(&m_idle,NULL);
    m_bQuit = false;
  }

  ~ThreadSimple(){
    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_notFull);
    pthread_cond_destroy(&m_notEmpty);
    pthread_cond_destroy(&m_idle);
  }

  void Start(){
    _threads = new pthread_t[m_nWorker];
    for(int i = 0;i<m_nWorker;++i){
      pthread_create(&_threads[i],NULL,&ThreadSimple::RunLoop,this);
    }
  }

  void Wait(){
      pthread_mutex_lock(&m_mutex);
      while(!_queue.Empty()){
        pthread_cond_wait(&m_idle,&m_mutex);
      }
      m_bQuit = true;
      pthread_cond_broadcast(&m_notEmpty);
      for(int i =0;i<m_nWorker;++i){
        pthread_join(_threads[i],NULL);
      }
  }

  //abort still non excute queue task and exit
  void Stop(){
      m_bQuit = true;
      pthread_cond_broadcast(&m_notEmpty);
      for(int i =0;i<m_nWorker;++i){
        pthread_join(_threads[i],NULL);
      }
  }

  static void* RunLoop(void * arg){
      ThreadSimple *p = (ThreadSimple*)arg;
      p->Run();
      return NULL;
  }

  void Run(){
      while(!m_bQuit){
        pthread_mutex_lock(&m_mutex);
        while((!m_bQuit) && _queue.Empty()){
          pthread_cond_wait(&m_notEmpty,&m_mutex);
        }
        CallBack cb;
        if(!_queue.Empty()){
           cb = _queue.Dequeue();
        }
        if(_queue.Empty()){
          pthread_cond_signal(&m_idle);
        }
        pthread_mutex_unlock(&m_mutex);
        pthread_cond_signal(&m_notFull);
        if(cb)
          cb();
    }
  }

  void Add(CallBack cb){
    pthread_mutex_lock(&m_mutex);
    while(_queue.Full()){
      pthread_cond_wait(&m_notFull,&m_mutex);
    }
    assert(!_queue.Full());
    _queue.Enqueue(cb);
    pthread_cond_signal(&m_notEmpty);
    pthread_mutex_unlock(&m_mutex);
  }
};

#endif
