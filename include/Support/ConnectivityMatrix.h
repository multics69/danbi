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

  ConnectivityMatrix.h -- Connectivity matrix
 */
#ifndef DANBI_CONNECTIVITY_MATRIX_H
#define DANBI_CONNECTIVITY_MATRIX_H
#include <cerrno>
#include <vector>
#include <map>
#include <cassert>
#include <algorithm>

namespace danbi {
typedef std::multimap<int,int>::iterator ConnectivityIterator;
typedef std::vector<int>::iterator NodeIterator; 

template <typename ConnTy>
class ConnectivityMatrix {
private:
  void operator=(const ConnectivityMatrix&); // Do not implement
  ConnectivityMatrix(const ConnectivityMatrix&); // Do not implement

protected:
  std::vector<int>& StartNodes; 
  std::vector<ConnTy>& ConnectionTable; 
  std::vector<ConnTy>& FeedbackConnections; 

  std::vector<int> AllNodes; 
  std::multimap<int,int> Forwards;  // <producer, consumer> 
  std::multimap<int,int> Backwards; // <consumer, producer>
  std::multimap<int,int> ForwardsNoFeedback;  // <producer, consumer> 
  std::multimap<int,int> BackwardsNoFeedback; // <consumer, producer>

public:
  /// Constructor
  ConnectivityMatrix(std::vector<int>& StartNodes_, 
                     std::vector<ConnTy>& ConnectionTable_, 
                     std::vector<ConnTy>& FeedbackConnections_);

  /// Destructor
  virtual ~ConnectivityMatrix(); 
  
  /// Build matrix
  void build(); 

  // forward connection 
  bool forwardNext(int Producer, 
                   ConnectivityIterator& Start, 
                   ConnectivityIterator& End); 
  
  // backward connection
  bool backwardNext(int Consumer, 
                   ConnectivityIterator& Start, 
                   ConnectivityIterator& End); 

  // forward connection without feedback loop
  bool forwardNoFeedbackNext(int Producer, 
                             ConnectivityIterator& Start, 
                             ConnectivityIterator& End); 
  
  // backward connection without feedback loop
  bool backwardNoFeedbackNext(int Consumer, 
                              ConnectivityIterator& Start, 
                              ConnectivityIterator& End); 
  // get start nodes
  void getStartNodes(NodeIterator& Start, NodeIterator& End); 

  // get all nodes
  void getAllNodes(NodeIterator& Start, NodeIterator& End); 
};

template <typename ConnTy>
ConnectivityMatrix<ConnTy>::ConnectivityMatrix(std::vector<int>& StartNodes_, 
                                               std::vector<ConnTy>& ConnectionTable_, 
                                               std::vector<ConnTy>& FeedbackConnections_)
  : StartNodes(StartNodes_), 
    ConnectionTable(ConnectionTable_), 
    FeedbackConnections(FeedbackConnections_) {}

template <typename ConnTy>
ConnectivityMatrix<ConnTy>::~ConnectivityMatrix() {
  // do nothing 
}

template <typename ConnTy>
void ConnectivityMatrix<ConnTy>::build() {
  // For each connection 
  for (typename std::vector<ConnTy>::iterator 
         i = ConnectionTable.begin(), e = ConnectionTable.end(); 
       i != e; ++i ) {
    ConnTy Conn = i[0]; 
    int Producer = Conn->first; 
    int Consumer = Conn->second; 
    
    // Build up forward and backward connection matrix 
    Forwards.insert( std::pair<int,int>(Producer, Consumer) ); 
    Backwards.insert( std::pair<int,int>(Consumer, Producer) ); 

    // Build up forward and backward with no feedback loop matrix
    typename std::vector<ConnTy>::iterator FeedbackIter = 
      std::find(FeedbackConnections.begin(), FeedbackConnections.end(), Conn); 
    if ( FeedbackIter == FeedbackConnections.end() ) {
      ForwardsNoFeedback.insert( std::pair<int,int>(Producer, Consumer) ); 
      BackwardsNoFeedback.insert( std::pair<int,int>(Consumer, Producer) ); 
    }

    // Build up all nodes 
    std::vector<int>::iterator Begin, End; 
    Begin = AllNodes.begin(); 
    End = AllNodes.end(); 
    if ( std::find(Begin, End, Producer) == End )
      AllNodes.push_back(Producer); 

    Begin = AllNodes.begin(); 
    End = AllNodes.end(); 
    if ( std::find(Begin, End, Consumer) == End )
      AllNodes.push_back(Consumer); 
  }
}

template <typename ConnTy>
bool ConnectivityMatrix<ConnTy>::forwardNext(int Producer, 
                                             ConnectivityIterator& Start, 
                                             ConnectivityIterator& End) {
  Start = Forwards.find(Producer); 
  if (Start == Forwards.end())
    return false; 
  End = Forwards.upper_bound(Producer); 
  return true; 
}

template <typename ConnTy>
bool ConnectivityMatrix<ConnTy>::backwardNext(int Consumer, 
                                              ConnectivityIterator& Start, 
                                              ConnectivityIterator& End) {
  Start = Backwards.find(Consumer); 
  if (Start == Backwards.end())
    return false; 
  End = Backwards.upper_bound(Consumer); 
  return true; 
}

template <typename ConnTy>
bool ConnectivityMatrix<ConnTy>::forwardNoFeedbackNext(int Producer, 
                                                       ConnectivityIterator& Start, 
                                                       ConnectivityIterator& End) {
  Start = ForwardsNoFeedback.find(Producer); 
  if (Start == ForwardsNoFeedback.end())
    return false; 
  End = ForwardsNoFeedback.upper_bound(Producer); 
  return true; 
}
  
template <typename ConnTy>
bool ConnectivityMatrix<ConnTy>::backwardNoFeedbackNext(int Consumer, 
                                                        ConnectivityIterator& Start, 
                                                        ConnectivityIterator& End) {
  Start = BackwardsNoFeedback.find(Consumer); 
  if (Start == BackwardsNoFeedback.end())
    return false; 
  End = BackwardsNoFeedback.upper_bound(Consumer); 
  return true; 
}

template <typename ConnTy>
void ConnectivityMatrix<ConnTy>::getStartNodes(NodeIterator& Start, 
                                               NodeIterator& End) {
  Start = StartNodes.begin(); 
  End = StartNodes.end(); 
}

template <typename ConnTy>
void ConnectivityMatrix<ConnTy>::getAllNodes(NodeIterator& Start, 
                                             NodeIterator& End) {
  Start = AllNodes.begin(); 
  End = AllNodes.end(); 
}
} // End of danbi namespace



#endif 
