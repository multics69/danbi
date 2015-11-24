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

  MapReduceImpl.h -- member function implementation of MapReduce class
 */
#include <stdlib.h>
#include <cassert>
#include <algorithm>
#include "Support/BranchHint.h"
#include "Support/PerformanceCounter.h"
#include "DebugInfo/ProgramVisualizer.h"


using namespace danbi; 

#define DEBUG_MR_ON 1
#if DEBUG_MR_ON
#define DEBUG_MR(...) do {                      \
    printf("DANBI-%s:", __func__);              \
    printf(__VA_ARGS__);                        \
  } while(false);
#define DEBUG_MR_CLEAN(...) do {                \
    printf(__VA_ARGS__);                        \
  } while(false);
#define DEBUG_PLAN_MERGE_CMD(Src, Dst) do {     \
    printf("PlanMergeCmd: ");                   \
    switch (Src) {                              \
    case S_:                                    \
      printf("S_ ");                            \
      break;                                    \
    case SS:                                    \
      printf("SS ");                            \
      break;                                    \
    case SM:                                    \
      printf("SM ");                            \
      break;                                    \
    case MM:                                    \
      printf("MM ");                            \
      break;                                    \
    }                                           \
    switch (Dst) {                              \
    case _M:                                    \
      printf("_M\n");                           \
      break;                                    \
    case FM:                                    \
      printf("FM\n");                           \
      break;                                    \
    }                                           \
  }  while(false)
#else
#define DEBUG_MR(...) ;
#define DEBUG_MR_CLEAN(...) ;
#define DEBUG_PLAN_MERGE_CMD(...) ;
#endif

template <typename UserApp, typename DataElementT, typename KeyT, typename ValueT>
MapReduce<UserApp, DataElementT, KeyT, ValueT>* 
MapReduce<UserApp, DataElementT, KeyT, ValueT>::This; 

template <typename UserApp, 
          typename DataElementT, 
          typename KeyT, 
          typename ValueT>
MapReduce<UserApp, DataElementT, KeyT, ValueT>::MapReduce(const char* ProgramName_, long Cores_, 
                                                          void* Data_, long DataLen_, 
                                                          long SplitSize_,
                                                          long ReduceBatchSize_, 
                                                          long ProcessBatchSize_) {
  MapReduce<UserApp, DataElementT, KeyT, ValueT>::This = this; 
  Conf.ProgramName = ::strdup(ProgramName_);
  Conf.Cores = Cores_;
  Conf.Data = Data_; 
  Conf.DataLen = DataLen_; 
  Conf.SplitSize = (Conf.DataLen <= 0) ? 0 : SplitSize_;
  Conf.ReduceBatchSize = ReduceBatchSize_;
  Conf.ProcessBatchSize = ProcessBatchSize_;
  Conf.NumSplitter = (Conf.SplitSize == 0) ? 
    0 : ceil(float(Conf.DataLen)/float(Conf.SplitSize));
  Rtm = NULL;
}

template <typename UserApp, typename DataElementT, typename KeyT, typename ValueT>
MapReduce<UserApp, DataElementT, KeyT, ValueT>::~MapReduce() {
  if (Rtm) {
    delete Rtm;
    Rtm = NULL; 
  }

  if (Conf.ProgramName)
    ::free(Conf.ProgramName);
}

template <typename UserApp, typename DataElementT, typename KeyT, typename ValueT>
int MapReduce<UserApp, DataElementT, KeyT, ValueT>::compareKeyHelper(const void* T1, const void* T2) {
  return This->compareKey((Tuple*)T1, (Tuple*)T2);
}

template <typename UserApp, typename DataElementT, typename KeyT, typename ValueT>
__kernel void MapReduce<UserApp, DataElementT, KeyT, ValueT>::knlSplitSq(void) {
  enum { ConfROB = 0, 
         
         DataElementQ = 0, PlanMergeCmdQ = 1 };
  DECLARE_ROBX(ConfROB, Config, 1); 
  DECLARE_OUTPUT_QUEUE(DataElementQ, DataElementT);
  DECLARE_OUTPUT_QUEUE(PlanMergeCmdQ, PlanMergeCmdTy);

  Config* ConfInst = ROBX(ConfROB, Config, 0);

  DEBUG_MR("Start\n");
  BEGIN_KERNEL_BODY() {
    // Split
    for (long i = 0; i < ConfInst->NumSplitter; ++i) {
      long Start = ConfInst->SplitSize * i; 
      long End = std::min(Start + ConfInst->SplitSize, ConfInst->DataLen); 
      This->split(ConfInst->Data, ConfInst->DataLen, Start, End);
    }

    // Generate merge command
    long ChunkNum[2] = {
      ConfInst->NumSplitter, // the number of chunk from sort kernel 
      0 // the number of chunk from merge kernel 
    }; 
    PlanMergeCmdTy MergeCmd[ConfInst->NumSplitter];
    long MergeLevel[ConfInst->NumSplitter];
    long NumMerge = 0; 
    long Level = 0;
    PlanMergeCmdTy Src, Dst; 
    do {
      // Determine source 
      if (ChunkNum[0] >= 2 && 
          !(ChunkNum[1] >= 2 && MergeLevel[ChunkNum[1]-1] == MergeLevel[ChunkNum[1]-2]))
        Src = SS, ChunkNum[0] -= 2; 
      else if (ChunkNum[1] >= 2)
        Src = MM, ChunkNum[1] -= 2;
      else if (ChunkNum[0] == 1 && ChunkNum[1] > 0)
        Src = SM, --ChunkNum[0], --ChunkNum[1];
      else if (ChunkNum[0] == 1 && ChunkNum[1] == 0)
        Src = S_, --ChunkNum[0];
      else
        assert(0 && "Never be here!");
      assert(ChunkNum[0] >= 0 && ChunkNum[0] < ConfInst->NumSplitter);
    
      // Determine target
      if (ChunkNum[0] == 0 && ChunkNum[1] == 0)
        Dst = FM; 
      else {
        Level = (Src == MM) ? Level + 1 : 0; 
        MergeLevel[ChunkNum[1]++] = Level; 
        Dst = _M;
      }
      assert(ChunkNum[1] >= 0 && ChunkNum[1] < ConfInst->NumSplitter);

      // Generate the final command
      assert((Src | Dst) != (S_ | _M) && "Invalid merge command.");
      MergeCmd[NumMerge++] = static_cast<PlanMergeCmdTy>(Src | Dst); 
      DEBUG_PLAN_MERGE_CMD(Src, Dst); 
    } while(Dst != FM); 

    RESERVE_PUSH(PlanMergeCmdQ, NumMerge);
    COPY_FROM_MEM(PlanMergeCmdQ, Tuple, 0, NumMerge, MergeCmd);
    COMMIT_PUSH(PlanMergeCmdQ);
  } END_KERNEL_BODY(); 
  DEBUG_MR("End\n");
}

template <typename UserApp, typename DataElementT, typename KeyT, typename ValueT>
__kernel void MapReduce<UserApp, DataElementT, KeyT, ValueT>::knlMapPr(void) {
  enum { ConfROB = 0, 
         DataElementQ = 0, 
         MapOutNumQ = 0, MapOutDataQ = 1 };
  DECLARE_ROBX(ConfROB, Config, 1); 
  DECLARE_INPUT_QUEUE(DataElementQ, DataElementT);
  DECLARE_OUTPUT_QUEUE(MapOutNumQ, long);
  DECLARE_OUTPUT_QUEUE(MapOutDataQ, Tuple);

  Config* ConfInst = ROBX(ConfROB, Config, 0);
  DEBUG_MR("Start\n");
  BEGIN_KERNEL_BODY() {
    // NOTE: Ticket synch. between data MapOutNumQ and MapOutDataQ does not needed, 
    //       since the next sort() phase is commutative and associative. 

    // Do map() for a data element
    RESERVE_POP(DataElementQ, 1);
    long NumMapOutData = This->map(POP_ADDRESS_AT(DataElementQ, DataElementT, 0));
    COMMIT_POP(DataElementQ);

    // Push the number of generated tuples
    RESERVE_PUSH_TICKET_INQ(MapOutNumQ, DataElementQ, 1); 
    *PUSH_ADDRESS_AT(MapOutNumQ, long, 0) = NumMapOutData; 
    COMMIT_PUSH(MapOutNumQ);
    DEBUG_MR("MapSize: %ld\n", NumMapOutData);
  } END_KERNEL_BODY(); 
  DEBUG_MR("End\n");
}

template <typename UserApp, typename DataElementT, typename KeyT, typename ValueT>
__kernel void MapReduce<UserApp, DataElementT, KeyT, ValueT>::knlSortPr(void) {
  enum { ConfROB = 0, 
         MapOutNumQ = 0, MapOutDataQ = 1,
         SortOutNumQ = 0, SortOutDataQ = 1 };
  DECLARE_ROBX(ConfROB, Config, 1); 
  DECLARE_INPUT_QUEUE(MapOutNumQ, long);
  DECLARE_INPUT_QUEUE(MapOutDataQ, Tuple);
  DECLARE_OUTPUT_QUEUE(SortOutNumQ, long);
  DECLARE_OUTPUT_QUEUE(SortOutDataQ, Tuple);

  Config* ConfInst = ROBX(ConfROB, Config, 0);
  DEBUG_MR("Start: ");
  BEGIN_KERNEL_BODY() {
    // Get a sort size
    RESERVE_POP(MapOutNumQ, 1); 
    long SortSize = *POP_ADDRESS_AT(MapOutNumQ, long, 0);
    COMMIT_POP(MapOutNumQ);
    {
      Tuple CombinedTuple[SortSize];
      long Combined = 0; 

      if (likely(SortSize > 0)) {
        // Sort
        RESERVE_POP(MapOutDataQ, SortSize);
        COPY_TO_MEM(MapOutDataQ, Tuple, 0, SortSize, CombinedTuple);
        COMMIT_POP(MapOutDataQ);
        ::qsort(CombinedTuple, SortSize, sizeof(CombinedTuple[0]), compareKeyHelper);
    
        // Combine
        if (haveCombine()) {
          for (long s = 1; s < SortSize; ++s) {
            if (!This->compareKey(&CombinedTuple[Combined], &CombinedTuple[s]))
              This->combine(&CombinedTuple[Combined], &CombinedTuple[s], &CombinedTuple[Combined]);
            else {
              Combined++;
              if (Combined != s) 
                CombinedTuple[Combined] = CombinedTuple[s];
            }
          }
          Combined++; // Convert indext to size.
        }
        else
          Combined = SortSize;
      }

      // Generate the number of sorted data set 
      RESERVE_PUSH(SortOutNumQ, 1); 
      *PUSH_ADDRESS_AT(SortOutNumQ, long, 0) = Combined;
      COMMIT_PUSH(SortOutNumQ);
      DEBUG_MR("SortSize: %ld\n", Combined);
    
      if (likely(Combined > 0)) {
        // Copy the combined results to the output queue 
        RESERVE_PUSH_TICKET_OUTQ(SortOutDataQ, SortOutNumQ, Combined); 
        COPY_FROM_MEM(SortOutDataQ, Tuple, 0, Combined, CombinedTuple);
        COMMIT_PUSH(SortOutDataQ);
#if DEBUG_MR_ON
        DEBUG_MR("SORTED-COMBINED: ");
        for (long i = 0; i < Combined; ++i)
          DEBUG_MR_CLEAN("(%s, %ld) ", CombinedTuple[i].Key, CombinedTuple[i].Value);
        DEBUG_MR_CLEAN("\n");
#endif
      }
      else {
        // Consume the ticket of the output queue
        CONSUME_PUSH_TICKET_OUTQ(SortOutDataQ, SortOutNumQ); 
      }
    }
  } END_KERNEL_BODY(); 
  DEBUG_MR("End\n");
}

template <typename UserApp, typename DataElementT, typename KeyT, typename ValueT>
void MapReduce<UserApp, DataElementT, KeyT, ValueT>::genPassThroughPlan(
  PlanMergeCmdTy PlanMergeCmd, ARG_INPUT_QUEUE(TicketIssuerQ), ARG_INPUT_QUEUE(PrevCmdNumQ),
  bool SumNum, ARG_INPUT_QUEUE(InNumQ), ARG_INPUT_QUEUE(InDataQ),
  ARG_OUTPUT_QUEUE(CmdNumQ), ARG_OUTPUT_QUEUE(MergeCmdQ), ARG_OUTPUT_QUEUE(OutDataQ)) {
  BEGIN_KERNEL_BODY() {
    // Calc. pop count 
    long Num; 
    if (SumNum) {
      RESERVE_POP_TICKET_INQ(PrevCmdNumQ, TicketIssuerQ, 1); 
      long NumCount = *POP_ADDRESS_AT(PrevCmdNumQ, long, 0);
      COMMIT_POP(PrevCmdNumQ);

      Num = 0; 
      RESERVE_POP_TICKET_INQ(InNumQ, TicketIssuerQ, NumCount); 
      for (long i = 0; i < NumCount; ++i) 
        Num += *POP_ADDRESS_AT(InNumQ, long, i);
      COMMIT_POP(InNumQ);
    }
    else {
      CONSUME_POP_TICKET_INQ(PrevCmdNumQ, TicketIssuerQ); 
      RESERVE_POP_TICKET_INQ(InNumQ, TicketIssuerQ, 1); 
      Num = *POP_ADDRESS_AT(InNumQ, long, 0);
      COMMIT_POP(InNumQ);
    }
    
    // Generate a merge command
    RESERVE_PUSH_TICKET_INQ(CmdNumQ, TicketIssuerQ, 1); 
    *PUSH_ADDRESS_AT(CmdNumQ, long, 0) = 1;
    COMMIT_PUSH(CmdNumQ);

    RESERVE_PUSH_TICKET_INQ(MergeCmdQ, TicketIssuerQ, 1); 
    MergeCmd* Cmd = PUSH_ADDRESS_AT(MergeCmdQ, MergeCmd, 0); 
    Cmd->Size[0] = Num; 
    Cmd->Size[1] = 0; 
    Cmd->IsFinalMerge = isFinalMerge(PlanMergeCmd);
    COMMIT_PUSH(MergeCmdQ); 

    // Copy input data to output data
    if (likely(Num > 0)) {
      RESERVE_POP_TICKET_INQ(InDataQ, TicketIssuerQ, Num); 
      RESERVE_PUSH_TICKET_INQ(OutDataQ, TicketIssuerQ, Num); 
      COPY_QUEUE(OutDataQ, Tuple, 0, Num, InDataQ, 0);
      COMMIT_PUSH(OutDataQ); 
      COMMIT_POP(InDataQ);
    }
    else {
      CONSUME_POP_TICKET_INQ(InDataQ, TicketIssuerQ); 
      CONSUME_PUSH_TICKET_INQ(OutDataQ, TicketIssuerQ); 
    }
  } END_KERNEL_BODY(); 
}

template <typename UserApp, typename DataElementT, typename KeyT, typename ValueT>
long MapReduce<UserApp, DataElementT, KeyT, ValueT>::binSplit(
  Tuple* val, long low, long high, ARG_INPUT_QUEUE(InDataQ)) {
  long mid;

  /* check if range is valid */ 
  if (low > high) 
    return high; 

  /*
   * returns index which contains greatest element <= val.  If val is
   * less than all elements, returns low-1
   */
  while (low != high) {
    mid = low + ((high - low + 1) >> 1);
    if (compareKeyHelper(val, POP_ADDRESS_AT(InDataQ, Tuple, mid)) <= 0)
      high = mid - 1;
    else
      low = mid;
  }

  if (compareKeyHelper(POP_ADDRESS_AT(InDataQ, Tuple, low), val) > 0)
    return low - 1;
  else
    return low;
}

template <typename UserApp, typename DataElementT, typename KeyT, typename ValueT>
void MapReduce<UserApp, DataElementT, KeyT, ValueT>::genSingleSrcPlan(
  PlanMergeCmdTy PlanMergeCmd, ARG_INPUT_QUEUE(TicketIssuerQ), ARG_INPUT_QUEUE(PrevCmdNumQ),
  bool SumNum, ARG_INPUT_QUEUE(InNumQ), ARG_INPUT_QUEUE(InDataQ), 
  ARG_OUTPUT_QUEUE(CmdNumQ), ARG_OUTPUT_QUEUE(MergeCmdQ), ARG_OUTPUT_QUEUE(OutDataQ)) {
  BEGIN_KERNEL_BODY() {
    // Calc. pop count 
    long Num[2];
    if (SumNum) {
      long NumCount[2];
      RESERVE_POP_TICKET_INQ(PrevCmdNumQ, TicketIssuerQ, 2); 
      NumCount[0] = *POP_ADDRESS_AT(PrevCmdNumQ, long, 0);
      NumCount[1] = *POP_ADDRESS_AT(PrevCmdNumQ, long, 0);
      COMMIT_POP(PrevCmdNumQ);

      Num[0] = Num[1] = 0; 
      RESERVE_POP_TICKET_INQ(InNumQ, TicketIssuerQ, NumCount[0] + NumCount[1]); 
      for (long i = 0; i < NumCount[0]; ++i) 
        Num[0] += *POP_ADDRESS_AT(InNumQ, long, i);
      for (long i = NumCount[0]; i < (NumCount[0] + NumCount[1]); ++i) 
        Num[1] += *POP_ADDRESS_AT(InNumQ, long, i);
      COMMIT_POP(InNumQ);
    }
    else {
      CONSUME_POP_TICKET_INQ(PrevCmdNumQ, TicketIssuerQ); 
      RESERVE_POP_TICKET_INQ(InNumQ, TicketIssuerQ, 2); 
      Num[0] = *POP_ADDRESS_AT(InNumQ, long, 0);
      Num[1] = *POP_ADDRESS_AT(InNumQ, long, 1);
      COMMIT_POP(InNumQ);
    }
    long Total = Num[0] + Num[1];
  
    // Case 1: At leaset one of two is zero.
    if (unlikely(Num[0] == 0 || Num[1] == 0)) {
      // Case 1-1: Both are zero.
      if (unlikely(Total == 0)) {
        // Just consume tickets
        CONSUME_POP_TICKET_INQ(InDataQ, TicketIssuerQ); 
        CONSUME_PUSH_TICKET_INQ(OutDataQ, TicketIssuerQ); 
      }
      // Case 1-2: Only one of two is zero.
      else {
        // Generate a merge command 
        RESERVE_PUSH_TICKET_INQ(CmdNumQ, TicketIssuerQ, 1); 
        *PUSH_ADDRESS_AT(CmdNumQ, long, 0) = 1;
        COMMIT_PUSH(CmdNumQ);

        RESERVE_PUSH_TICKET_INQ(MergeCmdQ, TicketIssuerQ, 1); 
        MergeCmd* Cmd = PUSH_ADDRESS_AT(MergeCmdQ, MergeCmd, 0); 
        Cmd->Size[0] = Total; 
        Cmd->Size[1] = 0; 
        Cmd->IsFinalMerge = isFinalMerge(PlanMergeCmd);
        COMMIT_PUSH(MergeCmdQ); 

        // And then, copy the data in the input queue to the output queue
        RESERVE_POP_TICKET_INQ(InDataQ, TicketIssuerQ, Total); 
        RESERVE_PUSH_TICKET_INQ(OutDataQ, TicketIssuerQ, Total); 
        COPY_QUEUE(OutDataQ, Tuple, 0, Total, InDataQ, 0);
        COMMIT_PUSH(OutDataQ); 
        COMMIT_POP(InDataQ);
      }
    }
    // Case 2: Both are non-zero.
    else {
      int MaxMergeCmd = (Total / MergeUnitSize) + 1; 
      MergeCmd Cmd[MaxMergeCmd]; 
      long start[2] = {0, Num[0]}; 
      long end[2] = {Num[0] - 1, Total - 1}; 
      long low[2] = {start[0], start[1]};
      long high[2] = {std::min(low[0] + MergeUnitSize - 1, end[0]), 0}; 
      int o = 0;
      int mi; 
                     
      RESERVE_POP_TICKET_INQ(InDataQ, TicketIssuerQ, Total); 
      RESERVE_PUSH_TICKET_INQ(OutDataQ, TicketIssuerQ, Total); 
      for (mi = 0; mi < MaxMergeCmd; ++mi) {
        // Calculate high[1]
       calc_high1:
        if ((low[0] <= end[0]) && (high[0] < end[0]))
          high[1] = binSplit(POP_ADDRESS_AT(InDataQ, Tuple, high[0]), 
                             low[1], end[1], INPUT_QUEUE(InDataQ));
        else
          high[1] = end[1];

        // If the second chunk is empty, try to increase the first chunk 
        if ((low[1] > high[1]) && (high[0] < end[0])) {
          high[0] = std::min(high[0] + MergeUnitSize, end[0]); 
          goto calc_high1;
        }

        // Copy [low0, high0], [low1, high1]
        int CopyNum = high[0] - low[0] + 1; 
        if (CopyNum > 0) {
          COPY_QUEUE(OutDataQ, Tuple, o, CopyNum, InDataQ, low[0]);
          o += CopyNum;
        }
        CopyNum = high[1] - low[1] + 1; 
        if (CopyNum > 0) {
          COPY_QUEUE(OutDataQ, Tuple, o, CopyNum, InDataQ, low[1]);
          o += CopyNum;
        }

        // Push merge command
        MergeCmd* mc = &Cmd[mi];
        mc->Size[0] = std::max(high[0] - low[0] + 1, 0L); 
        mc->Size[1] = std::max(high[1] - low[1] + 1, 0L); 
        mc->IsFinalMerge = isFinalMerge(PlanMergeCmd);
        DEBUG_MR("MergeCmd(%ld, %ld, %d)\n", mc->Size[0], mc->Size[1], mc->IsFinalMerge); 

        // Update low and high 
        low[0] = high[0] + 1; 
        low[1] = high[1] + 1; 
        high[0] = std::min(low[0] + MergeUnitSize - 1, end[0]); 

        // Check the end 
        if ((low[0] > end[0]) && (low[1] > end[1])) {
          ++mi;
          break;
        }
      }
      COMMIT_PUSH(OutDataQ); 
      COMMIT_POP(InDataQ);
      
      // Push merge commands
      RESERVE_PUSH_TICKET_INQ(CmdNumQ, TicketIssuerQ, 1); 
      *PUSH_ADDRESS_AT(CmdNumQ, long, 0) = mi;
      COMMIT_PUSH(CmdNumQ);

      RESERVE_PUSH_TICKET_INQ(MergeCmdQ, TicketIssuerQ, mi); 
      COPY_FROM_MEM(MergeCmdQ, MergeCmd, 0, mi, Cmd);
      COMMIT_PUSH(MergeCmdQ);
    }
  } END_KERNEL_BODY(); 
}

template <typename UserApp, typename DataElementT, typename KeyT, typename ValueT>
void MapReduce<UserApp, DataElementT, KeyT, ValueT>::genTwoSrcPlan(
  PlanMergeCmdTy PlanMergeCmd, ARG_INPUT_QUEUE(TicketIssuerQ), ARG_INPUT_QUEUE(PrevCmdNumQ),
  ARG_INPUT_QUEUE(InNumQ0), ARG_INPUT_QUEUE(InDataQ0), 
  bool SumNum1, ARG_INPUT_QUEUE(InNumQ1), ARG_INPUT_QUEUE(InDataQ1), 
  ARG_OUTPUT_QUEUE(CmdNumQ), ARG_OUTPUT_QUEUE(MergeCmdQ), ARG_OUTPUT_QUEUE(OutDataQ)) {
  BEGIN_KERNEL_BODY() {
    // Calc. pop count
    long Num[2];
    RESERVE_POP_TICKET_INQ(InNumQ0, TicketIssuerQ, 1); 
    Num[0] = *POP_ADDRESS_AT(InNumQ0, long, 0);
    COMMIT_POP(InNumQ0);

    if (SumNum1) {
      RESERVE_POP_TICKET_INQ(PrevCmdNumQ, TicketIssuerQ, 1); 
      long NumCount = *POP_ADDRESS_AT(PrevCmdNumQ, long, 0);
      COMMIT_POP(PrevCmdNumQ);

      Num[1] = 0; 
      RESERVE_POP_TICKET_INQ(InNumQ1, TicketIssuerQ, NumCount); 
      for (long i = 0; i < NumCount; ++i) 
        Num[1] += *POP_ADDRESS_AT(InNumQ1, long, i);
      COMMIT_POP(InNumQ1);
    }
    else {
      CONSUME_POP_TICKET_INQ(PrevCmdNumQ, TicketIssuerQ); 
      RESERVE_POP_TICKET_INQ(InNumQ1, TicketIssuerQ, 1); 
      Num[1] = *POP_ADDRESS_AT(InNumQ1, long, 0);
      COMMIT_POP(InNumQ1);
    }
    long Total = Num[0] + Num[1];
  
    // Case 1: At leaset one of two is zero.
    if (unlikely(Num[0] == 0 || Num[1] == 0)) {
      // Case 1-1: Both are zero.
      if (unlikely(Total == 0)) {
        // Just consume tickets
        CONSUME_POP_TICKET_INQ(InDataQ0, TicketIssuerQ); 
        CONSUME_POP_TICKET_INQ(InDataQ1, TicketIssuerQ); 
        CONSUME_PUSH_TICKET_INQ(OutDataQ, TicketIssuerQ); 
      }
      // Case 1-2: Only one of two is zero.
      else {
        // Generate a merge command 
        RESERVE_PUSH_TICKET_INQ(CmdNumQ, TicketIssuerQ, 1); 
        *PUSH_ADDRESS_AT(CmdNumQ, long, 0) = 1;
        COMMIT_PUSH(CmdNumQ);

        RESERVE_PUSH_TICKET_INQ(MergeCmdQ, TicketIssuerQ, 1); 
        MergeCmd* Cmd = PUSH_ADDRESS_AT(MergeCmdQ, MergeCmd, 0); 
        Cmd->Size[0] = Total; 
        Cmd->Size[1] = 0; 
        Cmd->IsFinalMerge = isFinalMerge(PlanMergeCmd);
        COMMIT_PUSH(MergeCmdQ); 

        // And then, copy the data in the input queue to the output queue
        if (Num[0] > 0) {
          CONSUME_POP_TICKET_INQ(InDataQ1, TicketIssuerQ); 
          RESERVE_POP_TICKET_INQ(InDataQ0, TicketIssuerQ, Total); 
          RESERVE_PUSH_TICKET_INQ(OutDataQ, TicketIssuerQ, Total); 
          COPY_QUEUE(OutDataQ, Tuple, 0, Total, InDataQ0, 0);
          COMMIT_PUSH(OutDataQ); 
          COMMIT_POP(InDataQ0);
        }
        else {
          CONSUME_POP_TICKET_INQ(InDataQ0, TicketIssuerQ); 
          RESERVE_POP_TICKET_INQ(InDataQ1, TicketIssuerQ, Total); 
          RESERVE_PUSH_TICKET_INQ(OutDataQ, TicketIssuerQ, Total); 
          COPY_QUEUE(OutDataQ, Tuple, 0, Total, InDataQ1, 0);
          COMMIT_PUSH(OutDataQ); 
          COMMIT_POP(InDataQ1);
        }
      }
    }
    // Case 2: Both are non-zero.
    else {
      int MaxMergeCmd = (Total / MergeUnitSize) + 1; 
      MergeCmd Cmd[MaxMergeCmd]; 
      long start[2] = {0, 0}; 
      long end[2] = {Num[0] - 1, Num[1] - 1}; 
      long low[2] = {start[0], start[1]};
      long high[2] = {std::min(low[0] + MergeUnitSize - 1, end[0]), 0}; 
      int o = 0;
      int mi; 
                     
      RESERVE_POP_TICKET_INQ(InDataQ0, TicketIssuerQ, Num[0]); 
      RESERVE_POP_TICKET_INQ(InDataQ1, TicketIssuerQ, Num[1]); 
      RESERVE_PUSH_TICKET_INQ(OutDataQ, TicketIssuerQ, Total); 
      for (mi = 0; mi < MaxMergeCmd; ++mi) {
        // Calculate high[1]
       calc_high1:
        if ((low[0] <= end[0]) && (high[0] < end[0]))
          high[1] = binSplit(POP_ADDRESS_AT(InDataQ0, Tuple, high[0]), 
                             low[1], end[1], INPUT_QUEUE(InDataQ1));
        else
          high[1] = end[1];

        // If the second chunk is empty, try to increase the first chunk 
        if ((low[1] > high[1]) && (high[0] < end[0])) {
          high[0] = std::min(high[0] + MergeUnitSize, end[0]); 
          goto calc_high1;
        }

        // Copy [low0, high0], [low1, high1]
        int CopyNum = high[0] - low[0] + 1; 
        if (CopyNum > 0) {
          COPY_QUEUE(OutDataQ, Tuple, o, CopyNum, InDataQ0, low[0]);
          o += CopyNum;
        }
        CopyNum = high[1] - low[1] + 1; 
        if (CopyNum > 0) {
          COPY_QUEUE(OutDataQ, Tuple, o, CopyNum, InDataQ1, low[1]);
          o += CopyNum;
        }

        // Push merge command
        MergeCmd* mc = &Cmd[mi];
        mc->Size[0] = std::max(high[0] - low[0] + 1, 0L); 
        mc->Size[1] = std::max(high[1] - low[1] + 1, 0L); 
        mc->IsFinalMerge = isFinalMerge(PlanMergeCmd);
        DEBUG_MR("MergeCmd(%ld, %ld, %d)\n", mc->Size[0], mc->Size[1], mc->IsFinalMerge); 

        // Update low and high 
        low[0] = high[0] + 1; 
        low[1] = high[1] + 1; 
        high[0] = std::min(low[0] + MergeUnitSize - 1, end[0]); 

        // Check the end 
        if ((low[0] > end[0]) && (low[1] > end[1])) {
          ++mi;
          break;
        }
      }
      COMMIT_PUSH(OutDataQ); 
      COMMIT_POP(InDataQ1);
      COMMIT_POP(InDataQ0);
      
      // Push merge commands
      RESERVE_PUSH_TICKET_INQ(CmdNumQ, TicketIssuerQ, 1); 
      *PUSH_ADDRESS_AT(CmdNumQ, long, 0) = mi;
      COMMIT_PUSH(CmdNumQ);

      RESERVE_PUSH_TICKET_INQ(MergeCmdQ, TicketIssuerQ, mi); 
      COPY_FROM_MEM(MergeCmdQ, MergeCmd, 0, mi, Cmd);
      COMMIT_PUSH(MergeCmdQ);
    }
  } END_KERNEL_BODY(); 
}

template <typename UserApp, typename DataElementT, typename KeyT, typename ValueT>
__kernel void MapReduce<UserApp, DataElementT, KeyT, ValueT>::knlPlanMergePr(void) {
  enum { ConfROB = 0, 
         PlanMergeCmdQ = 0, SortOutNumQ = 1, SortOutDataQ = 2, PrevMergeCmdNumQ = 3, PrevMergeOutNumQ = 4, PrevMergeOutDataQ = 5, 
         MergeCmdNumQ = 0, MergeCmdQ = 1, PlanMergeOutDataQ = 2 };
  DECLARE_ROBX(ConfROB, Config, 1); 
  DECLARE_INPUT_QUEUE(PlanMergeCmdQ, PlanMergeCmdTy);
  DECLARE_INPUT_QUEUE(SortOutNumQ, long);
  DECLARE_INPUT_QUEUE(SortOutDataQ, Tuple);
  DECLARE_INPUT_QUEUE(PrevMergeCmdNumQ, long);
  DECLARE_INPUT_QUEUE(PrevMergeOutNumQ, long);
  DECLARE_INPUT_QUEUE(PrevMergeOutDataQ, Tuple);
  DECLARE_OUTPUT_QUEUE(MergeCmdNumQ, long);
  DECLARE_OUTPUT_QUEUE(MergeCmdQ, MergeCmd);
  DECLARE_OUTPUT_QUEUE(PlanMergeOutDataQ, Tuple);

  Config* ConfInst = ROBX(ConfROB, Config, 0);

  DEBUG_MR("Start\n");
  BEGIN_KERNEL_BODY() {
    PlanMergeCmdTy PlanMergeCmd;
    RESERVE_POP(PlanMergeCmdQ, 1); 
    PlanMergeCmd = *POP_ADDRESS_AT(PlanMergeCmdQ, PlanMergeCmdTy, 0); 
    COMMIT_POP(PlanMergeCmdQ); 
#if 1
    {
      static int count = 0; 
      count++; 
      printf("%s:%d cound = %d\n", __func__, __LINE__, count);
    }
#endif
    switch(PlanMergeCmd) {
    case S_2FM:
      // Consume unused queues
      CONSUME_POP_TICKET_INQ(PrevMergeOutNumQ, PlanMergeCmdQ); 
      CONSUME_POP_TICKET_INQ(PrevMergeOutDataQ, PlanMergeCmdQ);
      // Generate pass through plan
      genPassThroughPlan(
        PlanMergeCmd, INPUT_QUEUE(PlanMergeCmdQ), INPUT_QUEUE(PrevMergeCmdNumQ),
        false, INPUT_QUEUE(SortOutNumQ), INPUT_QUEUE(SortOutDataQ), 
        OUTPUT_QUEUE(MergeCmdNumQ), OUTPUT_QUEUE(MergeCmdQ), OUTPUT_QUEUE(PlanMergeOutDataQ));
      break;
    case SS2_M:
    case SS2FM:
      // Consume unused queues
      CONSUME_POP_TICKET_INQ(PrevMergeOutNumQ, PlanMergeCmdQ); 
      CONSUME_POP_TICKET_INQ(PrevMergeOutDataQ, PlanMergeCmdQ);
      // Generate single source plan
      genSingleSrcPlan(
        PlanMergeCmd, INPUT_QUEUE(PlanMergeCmdQ), INPUT_QUEUE(PrevMergeCmdNumQ),
        false, INPUT_QUEUE(SortOutNumQ), INPUT_QUEUE(SortOutDataQ), 
        OUTPUT_QUEUE(MergeCmdNumQ), OUTPUT_QUEUE(MergeCmdQ), OUTPUT_QUEUE(PlanMergeOutDataQ));
      break;
    case MM2_M:
    case MM2FM:
      // Consume unused queues
      CONSUME_POP_TICKET_INQ(SortOutNumQ, PlanMergeCmdQ); 
      CONSUME_POP_TICKET_INQ(SortOutDataQ, PlanMergeCmdQ);
      // Generate single source plan
      genSingleSrcPlan(
        PlanMergeCmd, INPUT_QUEUE(PlanMergeCmdQ), INPUT_QUEUE(PrevMergeCmdNumQ),
        true, INPUT_QUEUE(PrevMergeOutNumQ), INPUT_QUEUE(PrevMergeOutDataQ), 
        OUTPUT_QUEUE(MergeCmdNumQ), OUTPUT_QUEUE(MergeCmdQ), OUTPUT_QUEUE(PlanMergeOutDataQ));
      break;
    case SM2_M:
    case SM2FM:
      // Generate two sources plan
      genTwoSrcPlan(
        PlanMergeCmd, INPUT_QUEUE(PlanMergeCmdQ), INPUT_QUEUE(PrevMergeCmdNumQ),
        INPUT_QUEUE(SortOutNumQ), INPUT_QUEUE(SortOutDataQ), 
        true, INPUT_QUEUE(PrevMergeOutNumQ), INPUT_QUEUE(PrevMergeOutDataQ), 
        OUTPUT_QUEUE(MergeCmdNumQ), OUTPUT_QUEUE(MergeCmdQ), OUTPUT_QUEUE(PlanMergeOutDataQ));
      break;
    default:
      assert(0 && "Never be here!"); 
    }
  } END_KERNEL_BODY(); 
  DEBUG_MR("End\n");
}

template <typename UserApp, typename DataElementT, typename KeyT, typename ValueT>
void MapReduce<UserApp, DataElementT, KeyT, ValueT>::passThrough(
  ARG_INPUT_QUEUE(TicketIssuerQ),
  long InNum, ARG_INPUT_QUEUE(InDataQ), 
  ARG_OUTPUT_QUEUE(OutNumQ), ARG_OUTPUT_QUEUE(OutDataQ)) {
  BEGIN_KERNEL_BODY() {
    if (IS_VALID_OUTPUT_QUEUE(OutNumQ)) {
      RESERVE_PUSH_TICKET_INQ(OutNumQ, TicketIssuerQ, 1); 
      *PUSH_ADDRESS_AT(OutNumQ, long, 0) = InNum; 
      COMMIT_PUSH(OutNumQ); 
    }

    if (likely(InNum > 0)) {
      RESERVE_POP_TICKET_INQ(InDataQ, TicketIssuerQ, InNum); 
      RESERVE_PUSH_TICKET_INQ(OutDataQ, TicketIssuerQ, InNum); 
      COPY_QUEUE(OutDataQ, Tuple, 0, InNum, InDataQ, 0);
      COMMIT_PUSH(OutDataQ); 
      COMMIT_POP(InDataQ);
    }
    else {
      CONSUME_POP_TICKET_INQ(InDataQ, TicketIssuerQ); 
      CONSUME_PUSH_TICKET_INQ(OutDataQ, TicketIssuerQ); 
    }
  } END_KERNEL_BODY(); 
}

template <typename UserApp, typename DataElementT, typename KeyT, typename ValueT>
template <bool Dryrun>
long MapReduce<UserApp, DataElementT, KeyT, ValueT>::combineFromOneQ(
  ARG_INPUT_QUEUE(InDataQ), long Num0, long Num1, ARG_OUTPUT_QUEUE(OutDataQ)) {
  // Check if one of two is zero
  if (Dryrun) {
    if (!haveCombine() || Num0 == 0 || Num1 == 0) 
      return Num0 + Num1; 
  }

  // We assume that there are no duplicates in each chunk. 
  // Merge the overlapped
  long Combined = 0; 
  Tuple* C = NULL;
  long i0 = 0, i1 = 0;
  do {
    Tuple* T0 = POP_ADDRESS_AT(InDataQ, Tuple, i0); 
    Tuple* T1 = POP_ADDRESS_AT(InDataQ, Tuple, Num0 + i1);
    long *I0 = &i0, *I1 = &i1;
    int Comp = This->compareKey(T0, T1);

    // T0 is always smaller than T1. 
    if (Comp > 0) {
      Tuple* X = T0;
      T0 = T1; 
      T1 = X;
      I0 = &i1; 
      I1 = &i0; 
    }

    // Merge the smaller
    if (haveCombine() && unlikely(C != NULL) && !This->compareKey(C, T0)) {
      if (!Dryrun)
        This->combine(T0, C, C); 
    }
    else {
#if 1
      static int count = 0; 
      count++;
#endif
      if (!Dryrun)
        *(C = PUSH_ADDRESS_AT(OutDataQ, Tuple, Combined)) = *T0; 
      else
        C = T0; 
      Combined++;
    }
    I0[0]++; 

    // When T0 is the same as T1, combine. 
    if (haveCombine() && Comp == 0) {
      if (!Dryrun) {
        assert(C != NULL); 
        This->combine(T1, C, C); 
      }
      I1[0]++;
    }
  } while(i0 < Num0 && i1 < Num1);

  // Copy the rest
  long Delta = 0;
  if (i0 < Num0) {
    Delta = Num0 - i0; 
    if (!Dryrun)
      COPY_QUEUE(OutDataQ, Tuple, Combined, Delta, InDataQ, i0);
  }
  else if (i1 < Num1) {
    Delta = Num1 - i1; 
    if (!Dryrun)
      COPY_QUEUE(OutDataQ, Tuple, Combined, Delta, InDataQ, Num0 + i1);
  }
  Combined += Delta; 

#if 0
#if DEBUG_MR_ON
  if (!Dryrun) {
    DEBUG_MR("MERGED[%ld]: ", Combined);
    for (long i = 0; i < Combined; ++i) {
      Tuple* C = PUSH_ADDRESS_AT(OutDataQ, Tuple, i);
      DEBUG_MR_CLEAN("(%s, %ld) ", C->Key, C->Value);
    }
    DEBUG_MR("\n");
  }
  else 
    DEBUG_MR("MERGED-COUNT[%ld]: ", Combined);
#endif
#endif
  return Combined; 
}

template <typename UserApp, typename DataElementT, typename KeyT, typename ValueT>
void MapReduce<UserApp, DataElementT, KeyT, ValueT>::mergeFromOneQ(
  ARG_INPUT_QUEUE(TicketIssuerQ),
  long Num0, long Num1, ARG_INPUT_QUEUE(InDataQ), 
  ARG_OUTPUT_QUEUE(OutNumQ), ARG_OUTPUT_QUEUE(OutDataQ)) {
  BEGIN_KERNEL_BODY() {
#if 1
    static int count = 0; 
    count++;
#endif
    if (likely((Num0 + Num1) > 0)) {
      RESERVE_POP_TICKET_INQ(InDataQ, TicketIssuerQ, Num0 + Num1); 
      {
        long CombinedNum = combineFromOneQ<true>(INPUT_QUEUE(InDataQ), 
                                                 Num0, Num1, OUTPUT_QUEUE(OutDataQ));

        if (IS_VALID_OUTPUT_QUEUE(OutNumQ)) {
          RESERVE_PUSH_TICKET_INQ(OutNumQ, TicketIssuerQ, 1); 
          *PUSH_ADDRESS_AT(OutNumQ, long, 0) = CombinedNum; 
          COMMIT_PUSH(OutNumQ); 
        }

        if (likely(CombinedNum > 0)) {
          RESERVE_PUSH_TICKET_INQ(OutDataQ, TicketIssuerQ, CombinedNum); 
          long Num = combineFromOneQ<false>(INPUT_QUEUE(InDataQ), 
                                            Num0, Num1, OUTPUT_QUEUE(OutDataQ));
          assert(Num == CombinedNum);
          COMMIT_PUSH(OutDataQ); 
        }
        else
          CONSUME_PUSH_TICKET_INQ(OutDataQ, TicketIssuerQ); 
      }
      COMMIT_POP(InDataQ);
    }
    else {
      if (IS_VALID_OUTPUT_QUEUE(OutNumQ)) {
        assert((Num0 + Num1) == 0);
        RESERVE_PUSH_TICKET_INQ(OutNumQ, TicketIssuerQ, 1); 
        *PUSH_ADDRESS_AT(OutNumQ, long, 0) = 0; 
        COMMIT_PUSH(OutNumQ); 
      }
      CONSUME_POP_TICKET_INQ(InDataQ, TicketIssuerQ); 
      CONSUME_PUSH_TICKET_INQ(OutDataQ, TicketIssuerQ); 
    }
  } END_KERNEL_BODY(); 
}

template <typename UserApp, typename DataElementT, typename KeyT, typename ValueT>
__kernel void MapReduce<UserApp, DataElementT, KeyT, ValueT>::knlMergePr(void) {
  enum { ConfROB = 0, 
         MergeCmdQ = 0, PlanMergeOutDataQ = 1, 
         MergeOutNumQ = 0, MergeOutDataQ = 1, FinalMergeOutNumQ = -2, FinalMergeOutDataQ = 3 };
  DECLARE_ROBX(ConfROB, Config, 1); 
  DECLARE_INPUT_QUEUE(MergeCmdQ, MergeCmd);
  DECLARE_INPUT_QUEUE(PlanMergeOutDataQ, Tuple);
  DECLARE_OUTPUT_QUEUE(MergeOutNumQ, long);
  DECLARE_OUTPUT_QUEUE(MergeOutDataQ, Tuple);
  DECLARE_OUTPUT_QUEUE(FinalMergeOutNumQ, long); // No ticket ordering since it is the final.
  DECLARE_OUTPUT_QUEUE(FinalMergeOutDataQ, Tuple); // No ticket ordering since it is the final.

  Config* ConfInst = ROBX(ConfROB, Config, 0);

  DEBUG_MR("Start\n");
  BEGIN_KERNEL_BODY() {
    // Get a merge command
    RESERVE_POP(MergeCmdQ, 1); 
    MergeCmd mc = *POP_ADDRESS_AT(MergeCmdQ, MergeCmd, 0); 
    COMMIT_POP(MergeCmdQ);

    // Execute the merge command
    // Case 1. It is a final merge. 
    if (unlikely(mc.IsFinalMerge)) {
      // Case 1-1. Only one chunk. 
      if (unlikely(mc.Size[1] == 0)) {
        // Consume unused queues
        CONSUME_PUSH_TICKET_INQ(MergeOutNumQ, MergeCmdQ);
        CONSUME_PUSH_TICKET_INQ(MergeOutDataQ, MergeCmdQ);
        // Pass thorugh SortOutDataQ to Final MergeOutDataQ
        passThrough(INPUT_QUEUE(MergeCmdQ), 
                    mc.Size[0], INPUT_QUEUE(PlanMergeOutDataQ), 
                    OUTPUT_QUEUE(FinalMergeOutNumQ), OUTPUT_QUEUE(FinalMergeOutDataQ));
      }
      // Case 1-2. Two chunks
      else {
        // Consume unused queues
        CONSUME_PUSH_TICKET_INQ(MergeOutNumQ, MergeCmdQ);
        CONSUME_PUSH_TICKET_INQ(MergeOutDataQ, MergeCmdQ);
        // Merge two chunks from SortOutDataQ to MergeOutDataQ
        mergeFromOneQ(INPUT_QUEUE(MergeCmdQ),
                      mc.Size[0], mc.Size[1], INPUT_QUEUE(PlanMergeOutDataQ),
                      OUTPUT_QUEUE(FinalMergeOutNumQ), OUTPUT_QUEUE(FinalMergeOutDataQ));
      }
    }
    // Case 2. It is not a final merge. 
    else {
      // Case 1-1. Only one chunk. 
      if (unlikely(mc.Size[1] == 0)) {
        // Consume unused queues
        CONSUME_PUSH_TICKET_INQ(FinalMergeOutDataQ, MergeCmdQ);
        // Pass thorugh SortOutDataQ to Final MergeOutDataQ
        passThrough(INPUT_QUEUE(MergeCmdQ), 
                    mc.Size[0], INPUT_QUEUE(PlanMergeOutDataQ), 
                    OUTPUT_QUEUE(MergeOutNumQ), OUTPUT_QUEUE(MergeOutDataQ));
      }
      // Case 1-2. Two chunks
      else {
        // Consume unused queues
        CONSUME_PUSH_TICKET_INQ(FinalMergeOutDataQ, MergeCmdQ);
        // Merge two chunks from SortOutDataQ to MergeOutDataQ
        mergeFromOneQ(INPUT_QUEUE(MergeCmdQ),
                      mc.Size[0], mc.Size[1], INPUT_QUEUE(PlanMergeOutDataQ),
                      OUTPUT_QUEUE(MergeOutNumQ), OUTPUT_QUEUE(MergeOutDataQ));
      }
    }
  } END_KERNEL_BODY(); 
  DEBUG_MR("End\n");
}

template <typename UserApp, typename DataElementT, typename KeyT, typename ValueT>
__kernel void MapReduce<UserApp, DataElementT, KeyT, ValueT>::knlReducePr(void) {
  enum { ConfROB = 0, 
         FinalMergeOutNumQ = 0, FinalMergeOutDataQ = 1,
         ReduceOutQ = 0};
  DECLARE_ROBX(ConfROB, Config, 1); 
  DECLARE_INPUT_QUEUE(FinalMergeOutNumQ, long); // Do not use
  DECLARE_INPUT_QUEUE(FinalMergeOutDataQ, Tuple);
  DECLARE_OUTPUT_QUEUE(ReduceOutQ, Tuple);

  Config* ConfInst = ROBX(ConfROB, Config, 0);
  DEBUG_MR("Start\n");
  BEGIN_KERNEL_BODY() {
    long Num = ConfInst->ReduceBatchSize;

    TRY_RESERVE_POP(FinalMergeOutDataQ, 1, Num); 
    if (likely(Num > 0)) {
      RESERVE_PUSH_TICKET_INQ(ReduceOutQ, FinalMergeOutDataQ, Num); 
      if (haveReduce()) {
        // Since we already combined all tuples with the same key, all tuples are unique. 
        for (long i = 0; i < Num; ++i)
          This->reduce(POP_ADDRESS_AT(FinalMergeOutDataQ, Tuple, i), 
                       PUSH_ADDRESS_AT(ReduceOutQ, Tuple, i));
      }
      else 
        COPY_QUEUE(ReduceOutQ, Tuple, 0, Num, FinalMergeOutDataQ, 0);
      COMMIT_PUSH(ReduceOutQ);
    }
    COMMIT_POP(FinalMergeOutDataQ); 
    DEBUG_MR("Reduce-Number: %ld\n", Num);
  } END_KERNEL_BODY(); 
  DEBUG_MR("End\n");
}

template <typename UserApp, typename DataElementT, typename KeyT, typename ValueT>
__kernel void MapReduce<UserApp, DataElementT, KeyT, ValueT>::knlProcessSq(void) {
  enum { ConfROB = 0, 
         ReduceOutQ = 0};
  DECLARE_ROBX(ConfROB, Config, 1); 
  DECLARE_INPUT_QUEUE(ReduceOutQ, Tuple);

  Config* ConfInst = ROBX(ConfROB, Config, 0);
  DEBUG_MR("Start\n");
  BEGIN_KERNEL_BODY() {
    long Num = ConfInst->ProcessBatchSize;

    TRY_RESERVE_POP(ReduceOutQ, 1, Num); 
    if (haveProcess()) {
      for (long i = 0; i < Num; ++i)
        This->process(POP_ADDRESS_AT(ReduceOutQ, Tuple, i));
    }
    COMMIT_POP(ReduceOutQ);
    DEBUG_MR("Process-Number: %ld\n", Num);
  } END_KERNEL_BODY(); 
  DEBUG_MR("End\n");
}

template <typename UserApp, typename DataElementT, typename KeyT, typename ValueT>
int MapReduce<UserApp, DataElementT, KeyT, ValueT>::reserveDataElements(Handle &H, long Num) {
  enum { DataElementQ = 0 };
  return __reserve_push(DataElementQ, &H, Num, 0, NULL);
}

template <typename UserApp, typename DataElementT, typename KeyT, typename ValueT>
DataElementT* MapReduce<UserApp, DataElementT, KeyT, ValueT>::getDataElementAt(Handle &H, long I) {
  return (DataElementT*)H.getElementAt(sizeof(DataElementT), I);
}

template <typename UserApp, typename DataElementT, typename KeyT, typename ValueT>
void MapReduce<UserApp, DataElementT, KeyT, ValueT>::copyMemToDataElements(
  Handle &H, long Start, long Num, DataElementT* Elements) {
  H.copyFromMem(sizeof(*Elements), Start, Num, Elements);
}

template <typename UserApp, typename DataElementT, typename KeyT, typename ValueT>
void MapReduce<UserApp, DataElementT, KeyT, ValueT>::commitDataElements(Handle &H) {
  enum { DataElementQ = 0 };
  __commit_push(DataElementQ, &H);
}

template <typename UserApp, typename DataElementT, typename KeyT, typename ValueT>
int MapReduce<UserApp, DataElementT, KeyT, ValueT>::reserveTuples(Handle &H, long Num) {
  enum { MapOutDataQ = 1 };
  return __reserve_push(MapOutDataQ, &H, Num, 0, NULL);
}

template <typename UserApp, typename DataElementT, typename KeyT, typename ValueT>
typename MapReduce<UserApp, DataElementT, KeyT, ValueT>::Tuple* 
MapReduce<UserApp, DataElementT, KeyT, ValueT>::getTupleAt(Handle &H, long I) {
  return (Tuple*)H.getElementAt(sizeof(Tuple), I);
}

template <typename UserApp, typename DataElementT, typename KeyT, typename ValueT>
void MapReduce<UserApp, DataElementT, KeyT, ValueT>::copyMemToTuple(
  Handle &H, long Start, long Num, Tuple* Tuples) {
  H.copyFromMem(sizeof(*Tuples), Start, Num, Tuples);
}

template <typename UserApp, typename DataElementT, typename KeyT, typename ValueT>
void MapReduce<UserApp, DataElementT, KeyT, ValueT>::commitTuples(Handle &H) {
  enum { MapOutDataQ = 1 };
  __commit_push(MapOutDataQ, &H);
}

template <typename UserApp, typename DataElementT, typename KeyT, typename ValueT>
int MapReduce<UserApp, DataElementT, KeyT, ValueT>::initialize(void) {
  ProgramDescriptor PD(Conf.ProgramName); 

  // Cleanup already created Runtime 
  if (Rtm) {
    delete Rtm;
    Rtm = NULL; 
  }

  // Code
  PD.CodeTable["knlSplitSq"] = new CodeDescriptor(knlSplitSq); 
  PD.CodeTable["knlMapPr"] = new CodeDescriptor(knlMapPr); 
  PD.CodeTable["knlSortPr"] = new CodeDescriptor(knlSortPr); 
  PD.CodeTable["knlPlanMergePr"] = new CodeDescriptor(knlPlanMergePr); 
  PD.CodeTable["knlMergePr"] = new CodeDescriptor(knlMergePr); 
  PD.CodeTable["knlReducePr"] = new CodeDescriptor(knlReducePr); 
  PD.CodeTable["knlProcessSq"] = new CodeDescriptor(knlProcessSq); 

  // ROB
  PD.ROBTable["ConfROB"] = new ReadOnlyBufferDescriptor(&Conf, sizeof(Conf), 1);

  // Queue
  PD.QTable["DataElementQ"] = new QueueDescriptor(
    -1, sizeof(DataElementT), false, "01knlSplitSqK", "02knlMapPrK",
    false, false, false, false); 
  PD.QTable["PlanMergeCmdQ"] = new QueueDescriptor(
    -1, sizeof(PlanMergeCmdTy), false, "01knlSplitSqK", "04knlPlanMergePrK", 
    false, false, true, false); 
  PD.QTable["MapOutNumQ"] = new QueueDescriptor(
    -1, sizeof(long), false, "02knlMapPrK", "03knlSortPrK",
    false, false, false, false); 
  PD.QTable["MapOutDataQ"] = new QueueDescriptor(
    -1, sizeof(Tuple), false, "02knlMapPrK", "03knlSortPrK",
    false, false, false, false); 
  PD.QTable["SortOutNumQ"] = new QueueDescriptor(
    -1, sizeof(long), false, "03knlSortPrK", "04knlPlanMergePrK",
    true, false, false, true); 
  PD.QTable["SortOutDataQ"] = new QueueDescriptor(
    -1, sizeof(Tuple), false, "03knlSortPrK", "04knlPlanMergePrK",
    false, true, false, true); 
  PD.QTable["MergeCmdNumQ"] = new QueueDescriptor(
    -1, sizeof(long), true, "04knlPlanMergePrK", "04knlPlanMergePrK",
    false, true, false, true); 
  PD.QTable["MergeCmdQ"] = new QueueDescriptor(
    -1, sizeof(MergeCmd), false, "04knlPlanMergePrK", "05knlMergePrK", 
    false, true, true, false); 
  PD.QTable["PlanMergeOutDataQ"] = new QueueDescriptor(
    -1, sizeof(Tuple), false, "04knlPlanMergePrK", "05knlMergePrK", 
    false, true, false, true); 
  PD.QTable["MergeOutNumQ"] = new QueueDescriptor(
    -1, sizeof(long), true, "05knlMergePrK", "04knlPlanMergePrK",
    false, true, false, true); 
  PD.QTable["MergeOutDataQ"] = new QueueDescriptor(
    -1, sizeof(Tuple), true, "05knlMergePrK", "04knlPlanMergePrK",
    false, true, false, true); 
  PD.QTable["FinalMergeOutNumQ"] = new QueueDescriptor(
    -1, sizeof(long), false, "05knlMergePrK", "06knlReducePrK",
    false, true, true, false); 
  PD.QTable["FinalMergeOutDataQ"] = new QueueDescriptor(
    -1, sizeof(Tuple), false, "05knlMergePrK", "06knlReducePrK",
    false, true, true, false); 
  PD.QTable["ReduceOutQ"] = new QueueDescriptor(
    -1, sizeof(Tuple), false, "06knlReducePrK", "07knlProcessSqK", 
    false, true, false, false); 

  // Kernel 
  PD.KernelTable["01knlSplitSqK"] = new KernelDescriptor("knlSplitSq", true, false); 
  PD.KernelTable["01knlSplitSqK"]->ROB[0] = "ConfROB";
  PD.KernelTable["01knlSplitSqK"]->OutQ[0] = "DataElementQ";
  PD.KernelTable["01knlSplitSqK"]->OutQ[1] = "PlanMergeCmdQ";

  PD.KernelTable["02knlMapPrK"] = new KernelDescriptor("knlMapPr", false, true); 
  PD.KernelTable["02knlMapPrK"]->ROB[0] = "ConfROB";
  PD.KernelTable["02knlMapPrK"]->InQ[0] = "DataElementQ";
  PD.KernelTable["02knlMapPrK"]->OutQ[0] = "MapOutNumQ";
  PD.KernelTable["02knlMapPrK"]->OutQ[1] = "MapOutDataQ";

  PD.KernelTable["03knlSortPrK"] = new KernelDescriptor("knlSortPr", false, true); 
  PD.KernelTable["03knlSortPrK"]->ROB[0] = "ConfROB";
  PD.KernelTable["03knlSortPrK"]->InQ[0] = "MapOutNumQ";
  PD.KernelTable["03knlSortPrK"]->InQ[1] = "MapOutDataQ";
  PD.KernelTable["03knlSortPrK"]->OutQ[0] = "SortOutNumQ";
  PD.KernelTable["03knlSortPrK"]->OutQ[1] = "SortOutDataQ";

  PD.KernelTable["04knlPlanMergePrK"] = new KernelDescriptor("knlPlanMergePr", false, true); 
  PD.KernelTable["04knlPlanMergePrK"]->ROB[0] = "ConfROB";
  PD.KernelTable["04knlPlanMergePrK"]->InQ[0] = "PlanMergeCmdQ";
  PD.KernelTable["04knlPlanMergePrK"]->InQ[1] = "SortOutNumQ";
  PD.KernelTable["04knlPlanMergePrK"]->InQ[2] = "SortOutDataQ";
  PD.KernelTable["04knlPlanMergePrK"]->InQ[3] = "MergeCmdNumQ";
  PD.KernelTable["04knlPlanMergePrK"]->InQ[4] = "MergeOutNumQ";
  PD.KernelTable["04knlPlanMergePrK"]->InQ[5] = "MergeOutDataQ";
  PD.KernelTable["04knlPlanMergePrK"]->OutQ[0] = "MergeCmdNumQ";
  PD.KernelTable["04knlPlanMergePrK"]->OutQ[1] = "MergeCmdQ";
  PD.KernelTable["04knlPlanMergePrK"]->OutQ[2] = "PlanMergeOutDataQ";

  PD.KernelTable["05knlMergePrK"] = new KernelDescriptor("knlMergePr", false, true); 
  PD.KernelTable["05knlMergePrK"]->ROB[0] = "ConfROB";
  PD.KernelTable["05knlMergePrK"]->InQ[0] = "MergeCmdQ";
  PD.KernelTable["05knlMergePrK"]->InQ[1] = "PlanMergeOutDataQ";
  PD.KernelTable["05knlMergePrK"]->OutQ[0] = "MergeOutNumQ";
  PD.KernelTable["05knlMergePrK"]->OutQ[1] = "MergeOutDataQ";
  PD.KernelTable["05knlMergePrK"]->OutQ[2] = "FinalMergeOutNumQ";
  PD.KernelTable["05knlMergePrK"]->OutQ[3] = "FinalMergeOutDataQ";

  PD.KernelTable["06knlReducePrK"] = new KernelDescriptor("knlReducePr", false, true); 
  PD.KernelTable["06knlReducePrK"]->ROB[0] = "ConfROB";
  PD.KernelTable["06knlReducePrK"]->InQ[0] = "FinalMergeOutNumQ";
  PD.KernelTable["06knlReducePrK"]->InQ[1] = "FinalMergeOutDataQ";
  PD.KernelTable["06knlReducePrK"]->OutQ[0] = "ReduceOutQ";

  PD.KernelTable["07knlProcessSqK"] = new KernelDescriptor("knlProcessSq", false, false); 
  PD.KernelTable["07knlProcessSqK"]->ROB[0] = "ConfROB";
  PD.KernelTable["07knlProcessSqK"]->InQ[0] = "ReduceOutQ";


  // Create a program using the descriptor
  ProgramFactory PF; 
  Program* Pgm = PF.newProgram(PD); 
  
  // Create a runtime 
  Rtm = RuntimeFactory::newRuntime(Pgm, Conf.Cores); 

  // Execute the program on the runtime 
  Rtm->prepare();

  // Draw the pipeline
  ProgramVisualizer PV(PD); 
  PV.generateGraph(std::string(Conf.ProgramName) + ".dot"); 
  return 0; 
}

template <typename UserApp, typename DataElementT, typename KeyT, typename ValueT>
int MapReduce<UserApp, DataElementT, KeyT, ValueT>::run(void) {
  Rtm->start();
  Rtm->join();
  return 0; 
}

template <typename UserApp, typename DataElementT, typename KeyT, typename ValueT>
int MapReduce<UserApp, DataElementT, KeyT, ValueT>::log(void) {
  Rtm->generateLog(std::string(Conf.ProgramName) + std::string("-Simple.gpl"), 
                   std::string(Conf.ProgramName) + std::string("-Full.gpl"));
  return 0;
}
