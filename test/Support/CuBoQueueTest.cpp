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

  CuBoQueueTest.cpp -- unit test cases for CuBoQueue class
 */
#include <unistd.h>
#include <cerrno>
#include <list>
#include "gtest/gtest.h"
#include "Support/CuBoQueue.h"
#include "Support/TaggedPointer.h" 
#include "Support/AbstractRunnable.h"
#include "Support/Thread.h"
#include "Support/Debug.h"

#define NUM_TEST_THREADS 10

using namespace danbi;
volatile bool RunningCuBoQ; 
volatile bool StartCuBoQ; 

class TestCuBoQ {
  DANBI_CUBOQ_ITERABLE(TestCuBoQ); 
public:
  int value; 
}; 

static TestCuBoQ CuBoQNodes[32];

#define NUM_PRODUCER_CONSUMER_NODES 10000000
static TestCuBoQ ProducerConsumerCuBoQNodes[NUM_PRODUCER_CONSUMER_NODES];

class TestCuBoQThread: public Thread, private AbstractRunnable {
private:
  CuBoQueue<TestCuBoQ>& Queue; 

  void main() {
    int id = Thread::getID();
    Queue.registerThread(id);
    do { 
      TestCuBoQ* Node; 
      while ( Queue.pop(Node, id) ) ; 
      Queue.push(Node, id);  
    } while(RunningCuBoQ); 
  }

public:
  TestCuBoQThread(CuBoQueue<TestCuBoQ>& Queue_) 
    : Thread(static_cast<AbstractRunnable*>(this)[0]), Queue(Queue_) {}
};



class ProducerThread: public Thread, private AbstractRunnable {
private:
  int ID;                                               
  CuBoQueue<TestCuBoQ>& Queue; 

  void main() {
    int id = Thread::getID();
    int i = ID * (NUM_PRODUCER_CONSUMER_NODES / 10); 
    int e = (ID+1) * (NUM_PRODUCER_CONSUMER_NODES / 10); 

    Queue.registerThread(id);
    while (!StartCuBoQ)
      Machine::mb(); 
    for (; i < e; ++i)
      Queue.push(ProducerConsumerCuBoQNodes+i, id);  
  }
public:
  ProducerThread(int ID_, CuBoQueue<TestCuBoQ>& Queue_) 
    : Thread(static_cast<AbstractRunnable*>(this)[0]), ID(ID_), Queue(Queue_) {}
};

class ConsumerThread: public Thread, private AbstractRunnable {
private:
  int ID;                                               
  CuBoQueue<TestCuBoQ>& Queue; 

  void main() {
    int id = Thread::getID();
    TestCuBoQ* Node; 
    int e = NUM_PRODUCER_CONSUMER_NODES / 10;

    Queue.registerThread(id);
    while (!StartCuBoQ)
      Machine::mb(); 
    for (int i = 0; i < e; ++i)
      while( Queue.pop(Node, id) ) ; 
  }
public:
  ConsumerThread(int ID_, CuBoQueue<TestCuBoQ>& Queue_) 
    : Thread(static_cast<AbstractRunnable*>(this)[0]), ID(ID_), Queue(Queue_) {}
};

TEST(CuBoQueueTest, SingleThreaded) {
  int id = Thread::getID();
  CuBoQueue<TestCuBoQ>* Queue = new CuBoQueue<TestCuBoQ>(); 
  EXPECT_TRUE(Queue != NULL); 

  Queue->registerThread(id);
  for(int i=0; i<10; ++i)
    CuBoQNodes[i].value = i; 

  TestCuBoQ* Node;
  EXPECT_EQ(-EAGAIN, Queue->pop(Node, id)); 

  Queue->push(CuBoQNodes+0, id); 
  EXPECT_EQ(0, Queue->pop(Node, id)); 
  EXPECT_EQ(0, Node->value); 
  EXPECT_EQ(-EAGAIN, Queue->pop(Node, id)); 
  
  Queue->push(CuBoQNodes+4, id); 
  Queue->push(CuBoQNodes+5, id); 
  EXPECT_EQ(0, Queue->pop(Node, id)); 
  EXPECT_EQ(4, Node->value); 
  EXPECT_EQ(0, Queue->pop(Node, id)); 
  EXPECT_EQ(5, Node->value); 
  EXPECT_EQ(-EAGAIN, Queue->pop(Node, id)); 

  Queue->push(CuBoQNodes+7, id); 
  Queue->push(CuBoQNodes+8, id); 
  Queue->push(CuBoQNodes+9, id); 
  EXPECT_EQ(0, Queue->pop(Node, id)); 
  EXPECT_EQ(7, Node->value); 
  EXPECT_EQ(0, Queue->pop(Node, id)); 
  EXPECT_EQ(8, Node->value); 
  EXPECT_EQ(0, Queue->pop(Node, id)); 
  EXPECT_EQ(9, Node->value); 
  EXPECT_EQ(-EAGAIN, Queue->pop(Node, id)); 

  delete Queue; 
}

TEST(CuBoQueueTest, MultiThreaded) {
  int id = Thread::getID();
  CuBoQueue<TestCuBoQ>* Queue = new CuBoQueue<TestCuBoQ>(); 
  EXPECT_TRUE(Queue != NULL); 

  Queue->registerThread(id);
  for(int i=0; i<5; ++i) {
    CuBoQNodes[i].value = 12340000 + i; 
    Queue->push(CuBoQNodes+i, id);
  }
  
  RunningCuBoQ = true; 
  Machine::mb(); 
  std::list<TestCuBoQThread*> Threads; 
  for(int i=0; i<NUM_TEST_THREADS; ++i) {
    TestCuBoQThread* Worker = new TestCuBoQThread(*Queue); 
    EXPECT_TRUE(Worker != NULL); 
    Threads.push_back(Worker); 
    EXPECT_EQ(0, Worker->start()); 
    ::usleep(100000);
  }
  ::usleep(100000);
  RunningCuBoQ = false; 
  Machine::mb(); 

  for(std::list<TestCuBoQThread*>::iterator
        i = Threads.begin(), e = Threads.end(); 
      i != e; ++i) {
    EXPECT_EQ(0, (*i)->join()); 
  }

  delete Queue; 
}

TEST(CuBoQueueTest, CorrectnessConcurrencyLevelOne) {
  CuBoQueue<TestCuBoQ>* Queue = new CuBoQueue<TestCuBoQ>(); 
  EXPECT_TRUE(Queue != NULL); 

  // Initialize data for debugging
  for(int i=0; i<NUM_PRODUCER_CONSUMER_NODES; ++i)
    ProducerConsumerCuBoQNodes[i].value = i; 
  
  // Creating producer threads
  StartCuBoQ = false; 
  Machine::mb();
  std::list<ProducerThread*> ProducerThreads; 
  for(int i=0; i<NUM_TEST_THREADS; ++i) {
    ProducerThread* Producer = new ProducerThread(i, *Queue); 
    EXPECT_TRUE(Producer != NULL); 
    ProducerThreads.push_back(Producer); 
    EXPECT_EQ(0, Producer->start()); 
  }
  StartCuBoQ = true; 
  Machine::mb(); 

  // Waiting for producer
  for(std::list<ProducerThread*>::iterator
        i = ProducerThreads.begin(), e = ProducerThreads.end(); 
      i != e; ++i) {
    EXPECT_EQ(0, (*i)->join()); 
  }
  EXPECT_TRUE(true); 

  // Creating consumer threads
  StartCuBoQ = false; 
  Machine::mb();
  std::list<ConsumerThread*> ConsumerThreads; 
  for(int i=0; i<NUM_TEST_THREADS; ++i) {
    ConsumerThread* Consumer = new ConsumerThread(i, *Queue); 
    EXPECT_TRUE(Consumer != NULL); 
    ConsumerThreads.push_back(Consumer); 
    EXPECT_EQ(0, Consumer->start()); 
  }
  StartCuBoQ = true; 
  Machine::mb(); 
  
  // Waiting for consumer
  for(std::list<ConsumerThread*>::iterator
        i = ConsumerThreads.begin(), e = ConsumerThreads.end(); 
      i != e; ++i) {
    EXPECT_EQ(0, (*i)->join()); 
  }
  EXPECT_TRUE(true); 

  delete Queue; 
}

TEST(CuBoQueueTest, CorrectnessConcurrencyLevelTwo) {
  CuBoQueue<TestCuBoQ>* Queue = new CuBoQueue<TestCuBoQ>(); 
  EXPECT_TRUE(Queue != NULL); 

  // Initialize data for debugging
  for(int i=0; i<NUM_PRODUCER_CONSUMER_NODES; ++i)
    ProducerConsumerCuBoQNodes[i].value = i; 
  
  // Creating producer threads
  StartCuBoQ = false; 
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
  
  // StartCuBoQ
  StartCuBoQ = true; 
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

  delete Queue; 
}

