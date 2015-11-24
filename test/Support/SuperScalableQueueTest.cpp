/*                                                                 --*- C++ -*-
  Copyright (C) 2012 Changwoo Min. All Rights Reserved.

  This file is part of DANBI project. 

  NOTICE: All information contained herein is, and remains the property 
  of Changwoo Min. The intellectual and technical concepts contained 
  herein are proprietary to Changwoo Min and may be covered  by patents 
  or patents in process, and are protected by trade secret or copyright law. 
  Dissemination of this information or reproduction of this material is 
  strictly forbidden unless prior written permission is obtained 
  from Changwoo Min(multics69@gmail.com). 

  SuperScalableQueueTest.cpp -- unit test cases for SuperScalableQueue class
 */
#include <unistd.h>
#include <cerrno>
#include <list>
#include "gtest/gtest.h"
#include "Support/SuperScalableQueue.h"
#include "Support/TaggedPointer.h" 
#include "Support/AbstractRunnable.h"
#include "Support/Thread.h"
#include "Support/Debug.h"

#define NUM_TEST_THREADS 10

using namespace danbi;
volatile bool RunningSSQPS; 
volatile bool StartSSQPS; 

class TestSSQPS {
  DANBI_SSQ_ITERABLE(TestSSQPS); 
public:
  int value; 
}; 

static TestSSQPS SSQNodes[32];

#define NUM_PRODUCER_CONSUMER_NODES 10000000
static TestSSQPS ProducerConsumerSSQNodes[NUM_PRODUCER_CONSUMER_NODES];

class TestSSQPSThread: public Thread, private AbstractRunnable {
private:
  SuperScalableQueue<TestSSQPS>& Queue; 

  void main() {
    TestSSQPS* Node; 
    do { 
      while ( Queue.pop(Node) ) ; 
      Queue.push(Node);  
    } while(RunningSSQPS); 
  }

public:
  TestSSQPSThread(SuperScalableQueue<TestSSQPS>& Queue_) 
    : Thread(static_cast<AbstractRunnable*>(this)[0]), Queue(Queue_) {}
};



class ProducerThread: public Thread, private AbstractRunnable {
private:
  int ID;                                               
  SuperScalableQueue<TestSSQPS>& Queue; 

  void main() {
    int i = ID * (NUM_PRODUCER_CONSUMER_NODES / 10); 
    int e = (ID+1) * (NUM_PRODUCER_CONSUMER_NODES / 10); 

    while (!StartSSQPS)
      Machine::mb(); 
    for (; i < e; ++i)
      Queue.push(ProducerConsumerSSQNodes+i);  
  }
public:
  ProducerThread(int ID_, SuperScalableQueue<TestSSQPS>& Queue_) 
    : Thread(static_cast<AbstractRunnable*>(this)[0]), ID(ID_), Queue(Queue_) {}
};

class ConsumerThread: public Thread, private AbstractRunnable {
private:
  int ID;                                               
  SuperScalableQueue<TestSSQPS>& Queue; 

  void main() {
    TestSSQPS* Node; 
    int e = NUM_PRODUCER_CONSUMER_NODES / 10;

    while (!StartSSQPS)
      Machine::mb(); 
    for (int i = 0; i < e; ++i)
      while( Queue.pop(Node) ) ; 
  }
public:
  ConsumerThread(int ID_, SuperScalableQueue<TestSSQPS>& Queue_) 
    : Thread(static_cast<AbstractRunnable*>(this)[0]), ID(ID_), Queue(Queue_) {}
};

TEST(SuperScalableQueueTest, SingleThreaded) {
  SuperScalableQueue<TestSSQPS>* Queue = new SuperScalableQueue<TestSSQPS>(); 
  EXPECT_TRUE(Queue != NULL); 

  for(int i=0; i<10; ++i)
    SSQNodes[i].value = i; 

  TestSSQPS* Node;
  EXPECT_EQ(-EAGAIN, Queue->pop(Node)); 

  Queue->push(SSQNodes+0); 
  EXPECT_EQ(0, Queue->pop(Node)); 
  EXPECT_EQ(0, Node->value); 
  EXPECT_EQ(-EAGAIN, Queue->pop(Node)); 
  
  Queue->push(SSQNodes+4); 
  Queue->push(SSQNodes+5); 
  EXPECT_EQ(0, Queue->pop(Node)); 
  EXPECT_EQ(4, Node->value); 
  EXPECT_EQ(0, Queue->pop(Node)); 
  EXPECT_EQ(5, Node->value); 
  EXPECT_EQ(-EAGAIN, Queue->pop(Node)); 

  Queue->push(SSQNodes+7); 
  Queue->push(SSQNodes+8); 
  Queue->push(SSQNodes+9); 
  EXPECT_EQ(0, Queue->pop(Node)); 
  EXPECT_EQ(7, Node->value); 
  EXPECT_EQ(0, Queue->pop(Node)); 
  EXPECT_EQ(8, Node->value); 
  EXPECT_EQ(0, Queue->pop(Node)); 
  EXPECT_EQ(9, Node->value); 
  EXPECT_EQ(-EAGAIN, Queue->pop(Node)); 

  delete Queue; 
}

TEST(SuperScalableQueueTest, MultiThreaded) {
  SuperScalableQueue<TestSSQPS>* Queue = new SuperScalableQueue<TestSSQPS>(); 
  EXPECT_TRUE(Queue != NULL); 

  for(int i=0; i<5; ++i) {
    SSQNodes[i].value = 12340000 + i; 
    Queue->push(SSQNodes+i);
  }
  
  RunningSSQPS = true; 
  Machine::mb(); 
  std::list<TestSSQPSThread*> Threads; 
  for(int i=0; i<NUM_TEST_THREADS; ++i) {
    TestSSQPSThread* Worker = new TestSSQPSThread(*Queue); 
    EXPECT_TRUE(Worker != NULL); 
    Threads.push_back(Worker); 
    EXPECT_EQ(0, Worker->start()); 
    ::usleep(100000);
  }
  ::usleep(100000);
  RunningSSQPS = false; 
  Machine::mb(); 

  for(std::list<TestSSQPSThread*>::iterator
        i = Threads.begin(), e = Threads.end(); 
      i != e; ++i) {
    EXPECT_EQ(0, (*i)->join()); 
  }

  delete Queue; 
}

TEST(SuperScalableQueueTest, CorrectnessConcurrencyLevelOne) {
  SuperScalableQueue<TestSSQPS>* Queue = new SuperScalableQueue<TestSSQPS>(); 
  EXPECT_TRUE(Queue != NULL); 

  // Initialize data for debugging
  for(int i=0; i<NUM_PRODUCER_CONSUMER_NODES; ++i)
    ProducerConsumerSSQNodes[i].value = i; 
  
  // Creating producer threads
  StartSSQPS = false; 
  Machine::mb();
  std::list<ProducerThread*> ProducerThreads; 
  for(int i=0; i<NUM_TEST_THREADS; ++i) {
    ProducerThread* Producer = new ProducerThread(i, *Queue); 
    EXPECT_TRUE(Producer != NULL); 
    ProducerThreads.push_back(Producer); 
    EXPECT_EQ(0, Producer->start()); 
  }
  StartSSQPS = true; 
  Machine::mb(); 

  // Waiting for producer
  for(std::list<ProducerThread*>::iterator
        i = ProducerThreads.begin(), e = ProducerThreads.end(); 
      i != e; ++i) {
    EXPECT_EQ(0, (*i)->join()); 
  }
  EXPECT_TRUE(true); 

  // Creating consumer threads
  StartSSQPS = false; 
  Machine::mb();
  std::list<ConsumerThread*> ConsumerThreads; 
  for(int i=0; i<NUM_TEST_THREADS; ++i) {
    ConsumerThread* Consumer = new ConsumerThread(i, *Queue); 
    EXPECT_TRUE(Consumer != NULL); 
    ConsumerThreads.push_back(Consumer); 
    EXPECT_EQ(0, Consumer->start()); 
  }
  StartSSQPS = true; 
  Machine::mb(); 
  
  // Waiting for consumer
  for(std::list<ConsumerThread*>::iterator
        i = ConsumerThreads.begin(), e = ConsumerThreads.end(); 
      i != e; ++i) {
    EXPECT_EQ(0, (*i)->join()); 
  }
  EXPECT_TRUE(true); 
#ifdef ENABLE_SSQ_DEBUG
  EXPECT_EQ(0, Queue->__NumElm);
#endif 

  delete Queue; 
}

TEST(SuperScalableQueueTest, CorrectnessConcurrencyLevelTwo) {
  SuperScalableQueue<TestSSQPS>* Queue = new SuperScalableQueue<TestSSQPS>(); 
  EXPECT_TRUE(Queue != NULL); 

  // Initialize data for debugging
  for(int i=0; i<NUM_PRODUCER_CONSUMER_NODES; ++i)
    ProducerConsumerSSQNodes[i].value = i; 
  
  // Creating producer threads
  StartSSQPS = false; 
  Machine::mb();
  std::list<ProducerThread*> ProducerThreads; 
  for(int i=0; i<NUM_TEST_THREADS; ++i) {
    ProducerThread* Producer = new ProducerThread(i, *Queue); 
    EXPECT_TRUE(Producer != NULL); 
    ProducerThreads.push_back(Producer); 
    EXPECT_EQ(0, Producer->start()); 
  }

  // Creating consumer threads
  std::list<ConsumerThread*> ConsumerThreads; 
  for(int i=0; i<NUM_TEST_THREADS; ++i) {
    ConsumerThread* Consumer = new ConsumerThread(i, *Queue); 
    EXPECT_TRUE(Consumer != NULL); 
    ConsumerThreads.push_back(Consumer); 
    EXPECT_EQ(0, Consumer->start()); 
  }
  
  // StartSSQPS
  StartSSQPS = true; 
  Machine::mb(); 

  // Waiting for producer
  for(std::list<ProducerThread*>::iterator
        i = ProducerThreads.begin(), e = ProducerThreads.end(); 
      i != e; ++i) {
    EXPECT_EQ(0, (*i)->join()); 
  }
  EXPECT_TRUE(true); 

  // Waiting for consumer
  for(std::list<ConsumerThread*>::iterator
        i = ConsumerThreads.begin(), e = ConsumerThreads.end(); 
      i != e; ++i) {
    EXPECT_EQ(0, (*i)->join()); 
  }
  EXPECT_TRUE(true); 
#ifdef ENABLE_SSQ_DEBUG
  EXPECT_EQ(0, Queue->__NumElm);
#endif 

  delete Queue; 
}

