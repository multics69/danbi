/*                                                                 --*- C++ -*-
  Copyright (C) 2013 Changwoo Min. All Rights Reserved.

  This file is part of DANBI project. 

  NOTICE: All information contained herein is, and remains the property 
  of Changwoo Min. The intellectual and technical concepts contained 
  herein are proprietary to Changwoo Min and may be covered  by patents 
  or patents in process, and are protected by trade secret or copyright law. 
  Dissemination of this information or reproduction of this material is 
  strictly forbidden unless prior written permission is obtained 
  from Changwoo Min(multics69@gmail.com). 

  MapReduce.h -- Abstract base class of danbi MapReduce application 
                 Used Curiously Recurring Template Pattern (CRTP)
 */
#ifndef DANBI_MAP_REDUCE_H
#define DANBI_MAP_REDUCE_H
#include "DanbiCPU.h"
#include START_OF_DEFAULT_OPTIMIZATION

namespace danbi {

template <typename UserApp, 
          typename DataElementT, 
          typename KeyT, 
          typename ValueT>
class MapReduce {
private:
  void operator=(const MapReduce<UserApp, DataElementT, KeyT, ValueT>&); // Do not implement
  MapReduce(const MapReduce<UserApp, DataElementT, KeyT, ValueT>&); // Do not implement

  enum {
    TicketIssuerQ, 
    InNumQ,     InDataQ, 
    InNumQ0,    InDataQ0,
    InNumQ1,    InDataQ1,
    OutNumQ,    OutDataQ, 
    PrevCmdNumQ, 
    CmdNumQ,
  }; 

  enum {
#if 1
    MergeUnitSize = 5, // 4096, 
#else
    MergeUnitSize = 4096, 
#endif
  };

  enum PlanMergeCmdTy {
    SRC_MASK = 0xF0, 
    S_ = 0x10, SS = 0x20, SM = 0x40, MM = 0x80,

    DST_MAST = 0x0F, 
    _M = 0x01, FM = 0x02,

    S_2FM = S_ | FM, 
    SS2FM = SS | FM, 
    SS2_M = SS | _M,
    SM2FM = SM | FM, 
    SM2_M = SM | _M, 
    MM2FM = MM | FM, 
    MM2_M = MM | _M,
  };
  
  static inline bool isFinalMerge(PlanMergeCmdTy PlanMergeCmd) {
    return (PlanMergeCmd & DST_MAST) == FM;
  }
  
  struct MergeCmd {
    long Size[2];
    bool IsFinalMerge;
  };

  struct Config {
    char* ProgramName;
    long Cores;
    void* Data;
    long DataLen;
    long SplitSize; 
    long ReduceBatchSize;
    long ProcessBatchSize; 
    long NumSplitter;
  };

protected:
  struct Tuple {
    KeyT Key; 
    ValueT Value; 
  };

private:
  static MapReduce<UserApp, DataElementT, KeyT, ValueT>* This; 
  Config Conf;  
  Runtime* Rtm;

private:
  /**
   * Kernel helper functions 
   */ 
  static int compareKeyHelper(const void* T1, const void* T2);

  static long binSplit(Tuple* val, long low, long high, ARG_INPUT_QUEUE(InDataQ));

  static void genPassThroughPlan(
    PlanMergeCmdTy PlanMergeCmd, ARG_INPUT_QUEUE(TicketIssuerQ), ARG_INPUT_QUEUE(PrevCmdNumQ),
    bool SumNum, ARG_INPUT_QUEUE(InNumQ), ARG_INPUT_QUEUE(InDataQ),
    ARG_OUTPUT_QUEUE(CmdNumQ), ARG_OUTPUT_QUEUE(MergeCmdQ), ARG_OUTPUT_QUEUE(OutDataQ));

  static void genSingleSrcPlan(
    PlanMergeCmdTy PlanMergeCmd, ARG_INPUT_QUEUE(TicketIssuerQ), ARG_INPUT_QUEUE(PrevCmdNumQ),
    bool SumNum, ARG_INPUT_QUEUE(InNumQ), ARG_INPUT_QUEUE(InDataQ), 
    ARG_OUTPUT_QUEUE(CmdNumQ), ARG_OUTPUT_QUEUE(MergeCmdQ), ARG_OUTPUT_QUEUE(OutDataQ));
  
  static void genTwoSrcPlan(
    PlanMergeCmdTy PlanMergeCmd, ARG_INPUT_QUEUE(TicketIssuerQ), ARG_INPUT_QUEUE(PrevCmdNumQ),
    ARG_INPUT_QUEUE(InNumQ0), ARG_INPUT_QUEUE(InDataQ0), 
    bool SumNum1, ARG_INPUT_QUEUE(InNumQ1), ARG_INPUT_QUEUE(InDataQ1), 
    ARG_OUTPUT_QUEUE(CmdNumQ), ARG_OUTPUT_QUEUE(MergeCmdQ), ARG_OUTPUT_QUEUE(OutDataQ));

  static void passThrough(ARG_INPUT_QUEUE(TicketIssuerQ), 
                          long InNum, ARG_INPUT_QUEUE(InDataQ), 
                          ARG_OUTPUT_QUEUE(OutNumQ), ARG_OUTPUT_QUEUE(OutDataQ));

  template <bool Dryrun>
  static long combineFromOneQ(ARG_INPUT_QUEUE(InDataQ), long Num0, long Num1, 
                              ARG_OUTPUT_QUEUE(OutDataQ));

  static void mergeFromOneQ(ARG_INPUT_QUEUE(TicketIssuerQ),
                            long Num0, long Num1, ARG_INPUT_QUEUE(InDataQ),
                            ARG_OUTPUT_QUEUE(OutNumQ), ARG_OUTPUT_QUEUE(OutDataQ));

  static inline bool haveCombine() {
    return &MapReduce<UserApp, DataElementT, KeyT, ValueT>::combine != &UserApp::combine;
  }

  static inline bool haveReduce(){
    return &MapReduce<UserApp, DataElementT, KeyT, ValueT>::reduce != &UserApp::reduce;
  }

  static inline bool haveProcess() {
    return &MapReduce<UserApp, DataElementT, KeyT, ValueT>::process != &UserApp::process;
  }

  /** 
   * Kernel functions
   */ 
  static __kernel void knlSplitSq(void);
  static __kernel void knlMapPr(void); 
  static __kernel void knlSortPr(void); 
  static __kernel void knlPlanMergePr(void); 
  static __kernel void knlMergePr(void); 
  static __kernel void knlReducePr(void);
  static __kernel void knlProcessSq(void);

protected:
  /** 
   * Support functions for the user defined functions 
   */ 
  typedef ReserveCommitQueueAccessor Handle;

  /** Followings are called from split function */ 
  inline int reserveDataElements(Handle &H, long Num); 
  inline DataElementT* getDataElementAt(Handle &H, long I);
  inline void copyMemToDataElements(Handle &H, long Start, long Num, DataElementT* Elements);
  inline void commitDataElements(Handle &H); 

  /** Following are called from map function */ 
  inline int reserveTuples(Handle &H, long Num); 
  inline Tuple* getTupleAt(Handle &H, long I);
  inline void copyMemToTuple(Handle &H, long Start, long Num, Tuple* Tuples);
  inline void commitTuples(Handle &H); 

protected:
  /**
   * User defined functions
   */ 
  inline int compareKey(Tuple* T1, Tuple* T2) {
    return static_cast<UserApp*>(this)->compareKey(T1, T2); 
  }

  inline void split(void* Data, long DataLen, long Start, long End) {
    static_cast<UserApp*>(this)->split(Data, DataLen, Start, End); 
  }

  inline long map(DataElementT* Elm) {
    return static_cast<UserApp*>(this)->map(Elm);
  }

  inline void combine(Tuple* T1, Tuple* T2, Tuple* Out) {
    assert( compareKey(T1, T2) == 0 ); 
     static_cast<UserApp*>(this)->combine(T1, T2, Out);
  }

  inline void reduce(Tuple* In, Tuple* Out) {
    static_cast<UserApp*>(this)->reduce(In, Out);
  }

  inline void process(Tuple* Result) {
    static_cast<UserApp*>(this)->process(Result);
  }

public:
  /// Constructor 
  MapReduce(const char* ProgramName_, long Cores_, 
            void* Data_, long DataLen_, 
            long SplitSize_ = (16*1024), 
            long ReduceBatchSize_ = 2048,  
            long ProcessBatchSize_ = 2048);

  /// Destructor 
  ~MapReduce();

  /// Initialize
  int initialize(void);

  /// Run
  int run(void);

  /// Generate log
  int log(void);
};
} // End of danbi namespace

#include "MapReduceImpl.h"
#include END_OF_DEFAULT_OPTIMIZATION
#endif
