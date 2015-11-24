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

  MergeSort3.cpp -- parallel mergesort
  Implementation is based on the code from Cilk. 
 */
#include <unistd.h>
#include <getopt.h>
#include <algorithm>
#include "DanbiCPU.h"
#include "Support/AbstractReserveCommitQueue.h"
#include "Support/PerformanceCounter.h"
#include "Support/Debug.h"
#include "Support/Thread.h"
#include "DebugInfo/ProgramVisualizer.h"
#include "Support/Machine.h"

#undef MONITORING
#ifdef MONITORING
# define DBG_CMD(x) x
# define DBG_EXEC(x) x
# define DBG_EXEC_COMBINE(x) x
# define DBG_DATA(x) x
# define KILO 2
# define QUICKSIZE (2*KILO)
# define MERGESIZE (2*KILO)
#else 
# define DBG_CMD(x) 
# define DBG_EXEC(x) 
# define DBG_EXEC_COMBINE(x)
# define DBG_DATA(x) 
# define KILO 1000
# define QUICKSIZE (10*KILO)
# define MERGESIZE (10*KILO)
#endif 

#undef DBG_ACCOUNT_MERGE_ELM
#ifdef DBG_ACCOUNT_MERGE_ELM
static volatile int RequestedMergeElm = 0; 
static volatile int ProcessedMergeElm = 0; 
#endif 

#define INSERTIONSIZE 20
struct Command {
  int Chunk1; 
  int Chunk2; 
  int Where; 

  int ID; /* for debugging */ 
  int ID2; /* for debugging */ 
};

/// Sequential Sort
#include START_OF_DEFAULT_OPTIMIZATION
static inline int med3(int a, int b, int c) {
  if (a < b) {
    if (b < c) {
      return b;
    } else {
      if (a < c)
        return c;
      else
        return a;
    }
  } else {
    if (b > c) {
      return b;
    } else {
      if (a > c)
        return c;
      else
        return a;
    }
  }
}

static inline int choose_pivot(int low, int high, ARG_OUTPUT_QUEUE(0)) {
  return med3(*PUSH_ADDRESS_AT(0, int, low), 
              *PUSH_ADDRESS_AT(0, int, high), 
              *PUSH_ADDRESS_AT(0, int, low + ((high - low) / 2))); 
}

static int seqpart(int low, int high, ARG_OUTPUT_QUEUE(0)) {
  int pivot;
  int h, l;
  int curr_low = low;
  int curr_high = high;

  pivot = choose_pivot(low, high, OUTPUT_QUEUE(0));

  while (1) {
    while ((h = *PUSH_ADDRESS_AT(0, int, curr_high)) > pivot)
      curr_high--;

    while ((l = *PUSH_ADDRESS_AT(0, int, curr_low)) < pivot)
      curr_low++;

    if (curr_low >= curr_high)
      break;

    *PUSH_ADDRESS_AT(0, int, curr_high) = l; 
    curr_high--; 
    *PUSH_ADDRESS_AT(0, int, curr_low) = h; 
    curr_low++; 
  }

  if (curr_high < high)
    return curr_high;
  else
    return curr_high - 1;
}

static void insertion_sort(int low, int high, ARG_OUTPUT_QUEUE(0)) {
  int p, q;
  int a, b;
  for (q = low + 1; q <= high; ++q) {
    a = *PUSH_ADDRESS_AT(0, int, q); 
    for (p = q - 1; p >= low && (b = *PUSH_ADDRESS_AT(0, int, p)) > a; p--)
      *PUSH_ADDRESS_AT(0, int, p+1) = b; 
    *PUSH_ADDRESS_AT(0, int, p+1) = a; 
  }
}

static void seqquick(int low, int high, ARG_OUTPUT_QUEUE(0)) {
  int p; 
  while (high - low >= INSERTIONSIZE) {
    p = seqpart(low, high, OUTPUT_QUEUE(0));
    seqquick(low, p, OUTPUT_QUEUE(0));
    low = p + 1;
  }
  insertion_sort(low, high, OUTPUT_QUEUE(0));
}

__kernel void SequentialSort(void) {
  DECLARE_INPUT_QUEUE(0, int); // command - number of element
  DECLARE_INPUT_QUEUE(1, int); // data
  DECLARE_OUTPUT_QUEUE(0, int); // sorted data 

  DBG_EXEC( printf("[0x%x] { Run Sequential Sort Cmd\n", Thread::getID()) ); 
  int Num; 
  BEGIN_KERNEL_BODY() {
    // Get the number of data for this
    RESERVE_POP(0, 1); 
    Num = *POP_ADDRESS_AT(0, int, 0); 
    COMMIT_POP(0); 

    // Reserve input and output
    RESERVE_POP_TICKET_INQ(1, 0, Num); 
    RESERVE_PUSH_TICKET_INQ(0, 0, Num); 

    // Copy input to output
    COPY_QUEUE(0, int, 0, Num, 1, 0); 

    // Sequential Sort
    seqquick(0, Num - 1, OUTPUT_QUEUE(0));

    // Log sequential sorted result
    DBG_DATA( printf("SequentialSort [") ); 
    for (int i = 0; i < Num; ++i) {
      DBG_DATA( printf("%d ", *PUSH_ADDRESS_AT(0, int, i)) ); 
    }
    DBG_DATA( printf("]\n") ); 

    // Commit input and output
    COMMIT_PUSH(0); 
    COMMIT_POP(1); 
  } END_KERNEL_BODY(); 
  DBG_EXEC( printf("[0x%x] } Run Sequential Sort Cmd: %d\n", Thread::getID(), Num) ); 
}
#include END_OF_DEFAULT_OPTIMIZATION

/// Combine
#include START_OF_DEFAULT_OPTIMIZATION
int binsplit(int val, int low, int high, ARG_INPUT_QUEUE(0)) {
  int mid;

  /* check if range is valid */ 
  if (low > high) 
    return high; 

  /*
   * returns index which contains greatest element <= val.  If val is
   * less than all elements, returns low-1
   */
  while (low != high) {
    mid = low + ((high - low + 1) >> 1);
    if (val <= *POP_ADDRESS_AT(0, int, mid))
      high = mid - 1;
    else
      low = mid;
  }

  if (*POP_ADDRESS_AT(0, int, low) > val)
    return low - 1;
  else
    return low;
}

__kernel void Combine(void) {
  DECLARE_INPUT_QUEUE(0, Command); // command - combine size 
  DECLARE_INPUT_QUEUE(1, int); // data from sequential sort kernel 
  DECLARE_INPUT_QUEUE(2, int); // data from merge kernel 
  DECLARE_OUTPUT_QUEUE(0, Command); // command for merge kernel 
  DECLARE_OUTPUT_QUEUE(1, int); // combined data 
  DECLARE_ROBX(0, int, 1); // elements
  int N = *ROBX(0, int, 0); 
#ifdef DBG_ACCOUNT_MERGE_ELM
  int rmg = -1; 
#endif

  DBG_EXEC( printf("[0x%x] { Run Combine Cmd\n", Thread::getID()) ); 
  DBG_EXEC( fflush(stdout) );
  Command c = {-1, -1, -1, -1};
  BEGIN_KERNEL_BODY() {
    // Get the number of data for this
    RESERVE_POP(0, 1); 
    c = *POP_ADDRESS_AT(0, Command, 0); 
    COMMIT_POP(0); 
    DBG_EXEC_COMBINE( printf("[0x%x] (%d) === Run Combine Cmd (%d, %d) q %d\n", 
                             Thread::getID(), c.ID, c.Chunk1, c.Chunk2, c.Where) ); 
    DBG_EXEC_COMBINE( fflush(stdout) );

    int Num = c.Chunk1 + c.Chunk2; 

    // Reserve input and output
    switch(c.Where) {
    case 1:
      RESERVE_POP_TICKET_INQ(1, 0, Num); 
      break; 
    case 2:
      RESERVE_POP_TICKET_INQ(2, 0, Num); 
      break; 
    default:
      assert(0); 
    }; 
    RESERVE_PUSH_TICKET_INQ(1, 0, Num); 

    // Combine
    int MergeSum = 0; 
    int MaxMergeCmd = ((c.Chunk1 + c.Chunk2) / (MERGESIZE/2)) + 1; 
    if (c.Chunk2 == 0) {
      // Copy chunk1
        switch(c.Where) {
        case 1:
          COPY_QUEUE(1, int, 0, c.Chunk1, 1, 0); 
          break; 
        case 2:
          COPY_QUEUE(1, int, 0, c.Chunk1, 2, 0); 
          break; 
        }; 

        // Push merge command 
	RESERVE_PUSH_TICKET_INQ(0, 0, 1);
        Command* mc = PUSH_ADDRESS_AT(0, Command, 0); 
        mc->Chunk1 = c.Chunk1;
        mc->Chunk2 = 0;
        mc->Where = ((Num == N) ? 1 : 0);
        MergeSum += (mc->Chunk1 + mc->Chunk2);
        mc->ID = MergeSum; 
        mc->ID2 = Num; 
        DBG_CMD( printf("[MergeCmd] (%d, %d) q %d [%d/%d] \n", 
                        mc->Chunk1, mc->Chunk2, mc->Where, 
                        mc->ID, mc->ID2) ); 
    }
    else {
      // Partial ordering two chunk 
      assert( (c.Chunk1 > 0) && (c.Chunk2 > 0) ); 
      Command MergeCmd[MaxMergeCmd]; 
      int o = 0; 
      int start1 = 0, start2 = c.Chunk1; 
      int end1 = c.Chunk1 - 1, end2 = Num - 1; 
      int low1 = start1, low2 = start2; 
      int high1 = std::min(low1 + MERGESIZE/2 - 1, end1); 
      int high2; 
      int CopyNum;

      int mi; 
      for (mi = 0; mi < MaxMergeCmd; ++mi) {
        // Calculate high2
       calc_high2:
        if ((low1 <= end1) && (high1 < end1)) {
          switch(c.Where) {
          case 1:
            high2 = binsplit(*POP_ADDRESS_AT(1, int, high1), low2, end2, 
                             INPUT_QUEUE(1));
            break; 
          case 2:
            high2 = binsplit(*POP_ADDRESS_AT(2, int, high1), low2, end2, 
                             INPUT_QUEUE(2));
            break; 
          }; 
        }
        else {
          high2 = end2; 
        }

        // If chunk2 is empty, try to increase chunk1
        if ( (low2 > high2) && (high1 < end1) ) {
          high1 = std::min(high1 + MERGESIZE/2, end1); 
          goto calc_high2; 
        }

        // Copy [low1, high1], [low2, high2]
        switch(c.Where) {
        case 1:
          CopyNum = high1 - low1 + 1;
          if (CopyNum > 0) {
            COPY_QUEUE(1, int, o, CopyNum, 1, low1); 
            o += CopyNum;
          }
          CopyNum = high2 - low2 + 1;
          if (CopyNum > 0) {
            COPY_QUEUE(1, int, o, CopyNum, 1, low2); 
            o += CopyNum;
          }
          break; 
        case 2:
          CopyNum = high1 - low1 + 1;
          if (CopyNum > 0) {
            COPY_QUEUE(1, int, o, CopyNum, 2, low1); 
            o += CopyNum;
          }
          CopyNum = high2 - low2 + 1;
          if (CopyNum > 0) {
            COPY_QUEUE(1, int, o, CopyNum, 2, low2); 
            o += CopyNum;
          }
          break; 
        }; 
      
        // Push merge command 
        Command* mc = &MergeCmd[mi];
        mc->Chunk1 = std::max(high1 - low1 + 1, 0); 
        mc->Chunk2 = std::max(high2 - low2 + 1, 0);
        if (mc->Chunk1 == 0) {
          mc->Chunk1 = mc->Chunk2; 
          mc->Chunk2 = 0; 
        }
        mc->Where = ((Num == N) ? 1 : 0);
        MergeSum += (mc->Chunk1 + mc->Chunk2);
        mc->ID = MergeSum; 
        mc->ID2 = Num; 
        DBG_CMD( printf("[MergeCmd] (%d, %d) q %d [%d/%d] \n", 
                        mc->Chunk1, mc->Chunk2, mc->Where, 
                        mc->ID, mc->ID2) ); 

        // Update low and high
        low1 = high1 + 1; 
        low2 = high2 + 1; 
        high1 = std::min(low1 + MERGESIZE/2 - 1, end1); 

        // Check the end 
        if ( (low1 > end1) && (low2 > end2) ) {
          ++mi; 
          break; 
        }
      }
      assert(Num == MergeSum && "Incorrect merge command generation in combine kernel.");

      // Push merge commands
      RESERVE_PUSH_TICKET_INQ(0, 0, mi);
      COPY_FROM_MEM(0, Command, 0, mi, MergeCmd); 
    }

    // Commit input and output
    COMMIT_PUSH(0); 
    COMMIT_PUSH(1); 
    switch(c.Where) {
    case 1:
      COMMIT_POP(1); 
      CONSUME_POP_TICKET_INQ(2, 0); 
      break; 
    case 2:
      COMMIT_POP(2); 
      CONSUME_POP_TICKET_INQ(1, 0); 
      break; 
    }; 

#ifdef DBG_ACCOUNT_MERGE_ELM
    rmg =  Machine::atomicIntAddReturn<volatile int>(&RequestedMergeElm, MergeSum); 
#endif    
  } END_KERNEL_BODY(); 
#ifdef DBG_ACCOUNT_MERGE_ELM
  DBG_EXEC( printf("[0x%x] (%d) } Run Combine Cmd (%d, %d) q %d rmg: %d\n", 
                   Thread::getID(), c.ID, c.Chunk1, c.Chunk2, c.Where, rmg) ); 
#else 
  DBG_EXEC( printf("[0x%x] (%d) } Run Combine Cmd (%d, %d) q %d\n", 
                   Thread::getID(), c.ID, c.Chunk1, c.Chunk2, c.Where) ); 
#endif 
  DBG_EXEC_COMBINE( fflush(stdout) );
}
#include END_OF_DEFAULT_OPTIMIZATION

/// Merge
#include START_OF_DEFAULT_OPTIMIZATION
__kernel void Merge(void) {
  DECLARE_INPUT_QUEUE(0, Command); // command - chunk size
  DECLARE_INPUT_QUEUE(1, int); // data
  DECLARE_OUTPUT_QUEUE(0, int); // merged data for combine kernel
  DECLARE_OUTPUT_QUEUE(1, int); // merged data for test sink
  DECLARE_OUTPUT_QUEUE(2, int); // merged num for combine kernel
  DECLARE_ROBX(0, int, 1); // elements
  int N = *ROBX(0, int, 0); 
#ifdef DBG_ACCOUNT_MERGE_ELM
  int rmg = -1, pmg = -1; 
#endif

  DBG_EXEC( printf("[0x%x] { Run Merge Cmd\n", Thread::getID()) );
  Command c = {-1, -1, -1, -1}; 
  BEGIN_KERNEL_BODY() {
    // Get the chunk size for merge
    RESERVE_POP(0, 1); 
    c = *POP_ADDRESS_AT(0, Command, 0); 
    COMMIT_POP(0); 
    assert(c.Chunk1 > 0); 
    DBG_EXEC( printf("[0x%x] (%d/%d) === Run Merge Cmd (%d, %d) q %d\n", 
                     Thread::getID(), c.ID, c.ID2, c.Chunk1, c.Chunk2, c.Where) ); 

    // When merging whole data, push the merged data to test sink 
    int Size = c.Chunk1 + c.Chunk2;  

    // Reserve input and output
    RESERVE_POP_TICKET_INQ(1, 0, Size); 
    switch (c.Where) {
    case 0:
      RESERVE_PUSH_TICKET_INQ(0, 0, Size); 
      // For feedback output, generate the number
      RESERVE_PUSH_TICKET_INQ(2, 0, 1); 
      *PUSH_ADDRESS_AT(2, int, 0) = Size;
      COMMIT_PUSH(2);
#if 1
      printf("%s:%d Size: %d\n", __func__, __LINE__, Size);
#endif      
      break; 
    case 1:
      RESERVE_PUSH_TICKET_INQ(1, 0, Size); 
      break; 
    default:
      assert(0);
    }

    // Merge
    int start1 = 0, start2 = c.Chunk1; 
    int end1 = c.Chunk1 - 1, end2 = Size - 1; 
    int i1 = start1, i2 = start2, oi = 0; 
    DBG_DATA( printf("Merged Data %d [", c.Where) ); 
    switch (c.Where) {
    case 0:
      if (c.Chunk2 == 0) {
        for (; i1 <= end1; ++i1, ++oi) {
          *PUSH_ADDRESS_AT(0, int, oi) = *POP_ADDRESS_AT(1, int, i1);
          DBG_DATA( printf("%d ", *PUSH_ADDRESS_AT(0, int, oi)) );
        }
        break;
      }
      do {
        int* v1 = POP_ADDRESS_AT(1, int, i1); 
        int* v2 = POP_ADDRESS_AT(1, int, i2); 
        if (*v1 <= *v2) {
          *PUSH_ADDRESS_AT(0, int, oi) = *v1; 
          DBG_DATA( printf("%d ", *PUSH_ADDRESS_AT(0, int, oi)) );
          ++i1; 
          ++oi;
          if (i1 > end1) {
            for (; i2 <= end2; ++i2, ++oi) {
              *PUSH_ADDRESS_AT(0, int, oi) = *POP_ADDRESS_AT(1, int, i2);
              DBG_DATA( printf("%d ", *PUSH_ADDRESS_AT(0, int, oi)) );
            }
            break; 
          }
        }
        else {
          *PUSH_ADDRESS_AT(0, int, oi) = *v2; 
          DBG_DATA( printf("%d ", *PUSH_ADDRESS_AT(0, int, oi)) );
          ++i2; 
          ++oi;
          if (i2 > end2) {
            for (; i1 <= end1; ++i1, ++oi) {
              *PUSH_ADDRESS_AT(0, int, oi) = *POP_ADDRESS_AT(1, int, i1);
              DBG_DATA( printf("%d ", *PUSH_ADDRESS_AT(0, int, oi)) );
            }
            break; 
          }
        }
      } while(1); 
      break; 
    case 1:
      if (c.Chunk2 == 0) {
        for (; i1 <= end1; ++i1, ++oi) {
          *PUSH_ADDRESS_AT(1, int, oi) = *POP_ADDRESS_AT(1, int, i1);
          DBG_DATA( printf("%d ", *PUSH_ADDRESS_AT(1, int, oi)) );
        }
        break;
      }
      do {
        int* v1 = POP_ADDRESS_AT(1, int, i1); 
        int* v2 = POP_ADDRESS_AT(1, int, i2); 
        if (*v1 <= *v2) {
          *PUSH_ADDRESS_AT(1, int, oi) = *v1; 
          DBG_DATA( printf("%d ", *PUSH_ADDRESS_AT(1, int, oi)) );
          ++i1; 
          ++oi;
          if (i1 > end1) {
            for (; i2 <= end2; ++i2, ++oi) {
              *PUSH_ADDRESS_AT(1, int, oi) = *POP_ADDRESS_AT(1, int, i2);
              DBG_DATA( printf("%d ", *PUSH_ADDRESS_AT(1, int, oi)) );
            }
            break; 
          }
        }
        else {
          *PUSH_ADDRESS_AT(1, int, oi) = *v2; 
          DBG_DATA( printf("%d ", *PUSH_ADDRESS_AT(1, int, oi)) );
          ++i2; 
          ++oi;
          if (i2 > end2) {
            for (; i1 <= end1; ++i1, ++oi) {
              *PUSH_ADDRESS_AT(1, int, oi) = *POP_ADDRESS_AT(1, int, i1);
              DBG_DATA( printf("%d ", *PUSH_ADDRESS_AT(1, int, oi)) );
            }
            break; 
          }
        }
      } while(1); 
      break; 
    }; 
    assert( oi == Size && "Whole data are not merged."); 
    DBG_DATA( printf("]\n") ); 
 
    // Commit input and output
    switch(c.Where) {
    case 0:
      COMMIT_PUSH(0);
      CONSUME_PUSH_TICKET_INQ(1, 0); 
      break; 
    case 1:
      COMMIT_PUSH(1); 
      CONSUME_PUSH_TICKET_INQ(0, 0); 
      break; 
    }
    COMMIT_POP(1); 

#ifdef DBG_ACCOUNT_MERGE_ELM
    Machine::rmb(); 
    pmg = Machine::atomicIntAddReturn<volatile int>(&ProcessedMergeElm, Size); 
    rmg = RequestedMergeElm; 
#endif    
  } END_KERNEL_BODY(); 
#ifdef DBG_ACCOUNT_MERGE_ELM
  DBG_EXEC( printf("[0x%x] (%d/%d) } Run Merge Cmd (%d, %d) q %d pmg/rmg(%d/%d)\n", 
                   Thread::getID(), c.ID, c.ID2, c.Chunk1, c.Chunk2, c.Where, 
                   pmg, rmg) ); 
#else
  DBG_EXEC( printf("[0x%x] (%d/%d) } Run Merge Cmd (%d, %d) q %d\n", 
                   Thread::getID(), c.ID, c.ID2, c.Chunk1, c.Chunk2, c.Where) ); 
#endif
}
#include END_OF_DEFAULT_OPTIMIZATION

/// Copy
#include START_OF_DEFAULT_OPTIMIZATION
__kernel void Copy(void) {
  DECLARE_INPUT_QUEUE(0, int); // copy size
  DECLARE_INPUT_QUEUE(1, int); // source data
  DECLARE_OUTPUT_QUEUE(0, int); // target data

  DBG_EXEC( printf("[0x%x] { Run Copy Kernel\n", Thread::getID()) ); 
  int Num; 
  BEGIN_KERNEL_BODY() {
    // Get the number of data for this
    RESERVE_POP(0, 1); 
    Num = *POP_ADDRESS_AT(0, int, 0); 
    COMMIT_POP(0); 

    // Reserve input and output
    RESERVE_POP_TICKET_INQ(1, 0, Num); 
    RESERVE_PUSH_TICKET_INQ(0, 0, Num); 

    // Copy input to output
    COPY_QUEUE(0, int, 0, Num, 1, 0); 

    // Commit input and output
    COMMIT_PUSH(0); 
    COMMIT_POP(1); 
  } END_KERNEL_BODY(); 
  DBG_EXEC( printf("[0x%x] } Run Copy Kernel: %d\n", Thread::getID(), Num) ); 
}
#include END_OF_DEFAULT_OPTIMIZATION


/// Test source 
int random(int x) {
  return 1103515245 * x + 12345; 
}

#include START_OF_DEFAULT_OPTIMIZATION
__kernel void TestSource(void) {
  DECLARE_OUTPUT_QUEUE(0, int); // sort command
  DECLARE_OUTPUT_QUEUE(1, int); // raw data
  DECLARE_OUTPUT_QUEUE(2, Command); // combine command
  DECLARE_ROBX(0, int, 1); // iteration
  DECLARE_ROBX(1, int, 1); // elements

  int Iter = *ROBX(0, int, 0); 
  int N = *ROBX(1, int, 0); 
  int r = 0; 
  DBG_EXEC( printf("[0x%x] { Run TestSource\n", Thread::getID()) ); 
  BEGIN_KERNEL_BODY() {
    // Iterate
    int id = 0;
    for (int i = 0; i < Iter; ++i) {
      // Generate sort command
      int NumSortCmd = std::max(N / QUICKSIZE, 1); 
      if ( (NumSortCmd % 2) && (NumSortCmd > 2) ) 
        --NumSortCmd; 
      assert( (NumSortCmd >= 1) && ((NumSortCmd == 1) || (NumSortCmd%2 == 0)) );
      int CombineCmd[NumSortCmd]; 
      DBG_CMD( int SUM = 0; );
      RESERVE_PUSH(0, NumSortCmd); 
      for (int c = 0; c < NumSortCmd ; ++c) {
        int SortSize = ( (c == (NumSortCmd - 1)) ? 
                         (N - (c * QUICKSIZE)) : QUICKSIZE ); 
        *PUSH_ADDRESS_AT(0, int, c) = CombineCmd[c] = SortSize; 
        DBG_CMD( SUM += SortSize; ); 
        DBG_CMD( printf("[SequentialSortCmd] s %d S %d\n", SortSize, SUM) ); 
      }
      DBG_CMD( assert(SUM == N) ); 
      COMMIT_PUSH(0); 

      // Generate data
      RESERVE_PUSH(1, N);
      COMMIT_PUSH(1); 

      // Generate combine command command
      int pi = 0, ci = 0; 
      Command* cc;
      do {
        RESERVE_PUSH(2, 1); 
        cc = PUSH_ADDRESS_AT(2, Command, 0); 
        cc->Chunk1 = CombineCmd[ci%NumSortCmd]; ci++; 
        if (NumSortCmd > 1) {
          cc->Chunk2 = CombineCmd[ci%NumSortCmd]; ci++; 
        }
        else {
          cc->Chunk2 = 0; 
        }
        CombineCmd[pi%NumSortCmd] = cc->Chunk1 + cc->Chunk2; pi++; 
        cc->Where = ((ci <= NumSortCmd) ? 1 : 2); 
        cc->ID = id; id++; 
        DBG_CMD( printf("[CombineCmd] %d (%d, %d) q %d\n", 
                        cc->ID, cc->Chunk1, cc->Chunk2, cc->Where) ); 
        COMMIT_PUSH(2);
      } while((cc->Chunk1 + cc->Chunk2) != N);
    }
  } END_KERNEL_BODY(); 
  DBG_EXEC( printf("[0x%x] } Run TestSource\n", Thread::getID()) ); 
}
#include END_OF_DEFAULT_OPTIMIZATION

/// Test sink 
#include START_OF_DEFAULT_OPTIMIZATION
__kernel void TestSink(void) {
  DECLARE_INPUT_QUEUE(0, int);
  DECLARE_ROBX(0, int, 1); // iteration
  DECLARE_ROBX(1, int, 1); // elements

  int Iter = *ROBX(0, int, 0); 
  int N = *ROBX(1, int, 0); 
  DBG_EXEC( printf("[0x%x] { Run TestSink\n", Thread::getID()) ); 
  BEGIN_KERNEL_BODY() {
    for (int i = 0; i < Iter; ++i) {
      RESERVE_POP(0, N); {
        DBG_DATA( printf("Sorted Data [") );
        for (int c = 0; c < N; ++c) {
          DBG_DATA( printf("%d ", *POP_ADDRESS_AT(0, int, c)) );
        }
        DBG_DATA( printf("]\n") ); 
      } COMMIT_POP(0); 
    }
  } END_KERNEL_BODY(); 
  DBG_EXEC( printf("[0x%x] } Run TestSink\n", Thread::getID()) ); 
}
#include END_OF_DEFAULT_OPTIMIZATION

/// Utility macro 
static unsigned long long HWCounters[PERF_COUNT_HW_MAX];
static unsigned long long ExecTimeInMicroSec; 

static int last_ret, error_line; 
static const char* error_func; 
#define SAFE_CALL_0(x) do {                                             \
    last_ret = x;                                                       \
    if (last_ret != 0) {                                                \
      error_line = __LINE__;                                            \
      error_func = __func__;                                            \
      return -__LINE__;                                                 \
    }                                                                   \
  } while(0); 

#define LOG_SAFE_CALL_ERROR() ::fprintf(stderr,                         \
    "Runtime error at %s:%d with %d\n", error_func, error_line, last_ret)

/// Benchmark MergeSort
int benchmarkMergeSort(int Cores, int Iter, float QFactor, int Elements, char *Log) {
  /// Benchmark pipeline 
  // [TestSource]--
  //      [[SeuqntialSort]]---[[Combine]]--[[Merge]]-+--
  //                       /|\                       |
  //                        +------------------------+
  // --[TestSink]
  ProgramDescriptor PD("MergeSort3"); 

  // Code
  PD.CodeTable["TestSource"] = new CodeDescriptor(TestSource);
  PD.CodeTable["SequentialSort"] = new CodeDescriptor(SequentialSort);
  PD.CodeTable["Combine"] = new CodeDescriptor(Combine);
  PD.CodeTable["Merge"] = new CodeDescriptor(Merge);
  PD.CodeTable["Copy"] = new CodeDescriptor(Copy);
  PD.CodeTable["TestSink"] = new CodeDescriptor(TestSink);

  // ROB
  int I_[1] = {Iter}; 
  PD.ROBTable["I"] = new ReadOnlyBufferDescriptor(I_, sizeof(int), 1); 
  int N_[1] = {Elements}; 
  PD.ROBTable["N"] = new ReadOnlyBufferDescriptor(N_, sizeof(int), 1); 

  // Queue
  const int QSize = float(Elements) * QFactor; 
  const int CmdQSize = float((Elements/QUICKSIZE) + 1) * QFactor;
  const int RootQSize = QSize * 1;
  const int RootCmdQSize = CmdQSize * 1;
  PD.QTable["SequentialSortCmdQ"] = new QueueDescriptor(RootCmdQSize, sizeof(int), false, 
                                                        "01TestSourceK", "02SequentialSortK",
                                                        false, false, true, false);
  PD.QTable["RawDataQ"] = new QueueDescriptor(RootQSize, sizeof(int), false, 
                                              "01TestSourceK", "02SequentialSortK",
                                              false, false, false, true);
  PD.QTable["CombineCmdQ"] = new QueueDescriptor(RootCmdQSize, sizeof(Command), false, 
                                                 "01TestSourceK", "03CombineK",
                                                 false, false, true, false);
  PD.QTable["SortedDataQ"] = new QueueDescriptor(RootQSize, sizeof(int), false, 
                                                 "02SequentialSortK", "03CombineK",
                                                 false, true, false, true);
  PD.QTable["CombinedDataQ"] = new QueueDescriptor(QSize, sizeof(int), false, 
                                                   "03CombineK", "04MergeK",
                                                   false, true, false, true);
  PD.QTable["MergeCmdQ"] = new QueueDescriptor(CmdQSize, sizeof(Command), false, 
                                               "03CombineK", "04MergeK",
                                               false, true, true, false);
  PD.QTable["MergedDataQ"] = new QueueDescriptor(QSize, sizeof(int), true, 
                                                 "04MergeK", "05CopyK",
                                                 false, true, false, true);
  PD.QTable["FinalMergedDataQ"] = new QueueDescriptor(QSize, sizeof(int), false, 
                                                      "04MergeK", "06TestSinkK",
                                                      false, true, false, false);
  PD.QTable["MergedNumQ"] = new QueueDescriptor(QSize, sizeof(int), true, 
                                                 "04MergeK", "05CopyK",
                                                false, true, true, false);
  PD.QTable["CopiedMergedDataQ"] = new QueueDescriptor(QSize, sizeof(int), true, 
                                                       "05CopyK", "03CombineK", 
                                                       false, true, false, true);

  // Kernel
  PD.KernelTable["01TestSourceK"] = new KernelDescriptor("TestSource", true, false); 
  PD.KernelTable["01TestSourceK"]->OutQ[0] = "SequentialSortCmdQ";
  PD.KernelTable["01TestSourceK"]->OutQ[1] = "RawDataQ";
  PD.KernelTable["01TestSourceK"]->OutQ[2] = "CombineCmdQ";
  PD.KernelTable["01TestSourceK"]->ROB[0] = "I";
  PD.KernelTable["01TestSourceK"]->ROB[1] = "N";

  PD.KernelTable["02SequentialSortK"] = new KernelDescriptor("SequentialSort", false, true); 
  PD.KernelTable["02SequentialSortK"]->InQ[0] = "SequentialSortCmdQ";
  PD.KernelTable["02SequentialSortK"]->InQ[1] = "RawDataQ";
  PD.KernelTable["02SequentialSortK"]->OutQ[0] = "SortedDataQ";

  PD.KernelTable["03CombineK"] = new KernelDescriptor("Combine", false, true); 
  PD.KernelTable["03CombineK"]->InQ[0] = "CombineCmdQ";
  PD.KernelTable["03CombineK"]->InQ[1] = "SortedDataQ";
  PD.KernelTable["03CombineK"]->InQ[2] = "CopiedMergedDataQ";
  PD.KernelTable["03CombineK"]->OutQ[0] = "MergeCmdQ";
  PD.KernelTable["03CombineK"]->OutQ[1] = "CombinedDataQ";
  PD.KernelTable["03CombineK"]->ROB[0] = "N";

  PD.KernelTable["04MergeK"] = new KernelDescriptor("Merge", false, true); 
  PD.KernelTable["04MergeK"]->InQ[0] = "MergeCmdQ";
  PD.KernelTable["04MergeK"]->InQ[1] = "CombinedDataQ";
  PD.KernelTable["04MergeK"]->OutQ[0] = "MergedDataQ";
  PD.KernelTable["04MergeK"]->OutQ[1] = "FinalMergedDataQ";
  PD.KernelTable["04MergeK"]->OutQ[2] = "MergedNumQ";
  PD.KernelTable["04MergeK"]->ROB[0] = "N";

  PD.KernelTable["05CopyK"] = new KernelDescriptor("Copy", false, true); 
  PD.KernelTable["05CopyK"]->InQ[0] = "MergedNumQ";
  PD.KernelTable["05CopyK"]->InQ[1] = "MergedDataQ";
  PD.KernelTable["05CopyK"]->OutQ[0] = "CopiedMergedDataQ";

  PD.KernelTable["06TestSinkK"] = new KernelDescriptor("TestSink", false, false); 
  PD.KernelTable["06TestSinkK"]->InQ[0] = "FinalMergedDataQ";
  PD.KernelTable["06TestSinkK"]->ROB[0] = "I";
  PD.KernelTable["06TestSinkK"]->ROB[1] = "N";

  // Generate program graph 
  ProgramVisualizer PV(PD); 
  PV.generateGraph(PD.Name + ".dot"); 

  // Create a program using the descriptor
  ProgramFactory PF; 
  Program* Pgm = PF.newProgram(PD); 
  
  // Create a runtime 
  Runtime* Rtm = RuntimeFactory::newRuntime(Pgm, Cores); 

  // Generate random data
  AbstractReserveCommitQueue* RawDataQ = PD.QTable["RawDataQ"]->Instance; 
  ReserveCommitQueueAccessor RCQA; 
  int r = 0; 
  int* RawData = reinterpret_cast<int *>(RawDataQ->__hookUpBuffer());
  for (int i = 0; i < QSize; ++i)
    RawData[i] = r = random(r);

  // Execute the program on the runtime 
  Rtm->prepare();
  PerformanceCounter::start();
  {
    Rtm->start();
    Rtm->join();
  }
  ExecTimeInMicroSec = PerformanceCounter::stop(); 
  Rtm->generateLog(std::string(Log) + std::string("-Simple.gpl"), 
                   std::string(Log) + std::string("-Full.gpl"));

  delete Rtm; 
  return 0; 
}

static 
void showUsage(void) {
  ::fprintf(stderr, 
            "DANBI merge sort benchmark\n");
  ::fprintf(stderr, 
            "  Usage: MergeSort3Run \n"); 
  ::fprintf(stderr, 
            "      [--repeat repeat_count]\n"); 
  ::fprintf(stderr, 
            "      [--core maximum_number_of_assignable_cores]\n");
  ::fprintf(stderr, 
            "      [--element number_of_data_elements]\n");
}

static
void showResult(int cores, int iterations, float qfactor) {
  double InstrPerCycle = (double)HWCounters[PERF_COUNT_HW_INSTRUCTIONS]/
    (double)HWCounters[PERF_COUNT_HW_CPU_CYCLES]; 

  double CyclesPerInstr = (double)HWCounters[PERF_COUNT_HW_CPU_CYCLES]/
    (double)HWCounters[PERF_COUNT_HW_INSTRUCTIONS];

  double StalledCyclesPerInstr = (double)(
    HWCounters[PERF_COUNT_HW_STALLED_CYCLES_FRONTEND] +
    HWCounters[PERF_COUNT_HW_STALLED_CYCLES_BACKEND]) / 
    (double)HWCounters[PERF_COUNT_HW_INSTRUCTIONS]; 

  ::fprintf(stdout, 
            "# DANBI merge sort3 benchmark\n");
  ::fprintf(stdout, 
            "# cores(1) iterations(2) microsec(3)"
            " InstrPerCycle(4) CyclesPerInstr(5) StalledCyclesPerInstr(6)"
            " HW_CPU_CYCLES(7) HW_INSTRUCTIONS(8) CACHE_REFERENCES(9) HW_CACHE_MISSES(10)"
            " HW_BRANCH_INSTRUCTIONS(11) HW_BRANCH_MISSES(12) HW_BUS_CYCLES(13)"
            " HW_STALLED_CYCLES_FRONTEND(14) HW_STALLED_CYCLES_BACKEND(15) QFactor(16)\n");
  ::fprintf(stdout, 
            "%d %d %lld %f %f %f", 
            cores, iterations, ExecTimeInMicroSec, 
            InstrPerCycle, CyclesPerInstr, StalledCyclesPerInstr); 
  ::fprintf(stdout, 
            " %lld %lld %lld %lld", 
            HWCounters[PERF_COUNT_HW_CPU_CYCLES], 
            HWCounters[PERF_COUNT_HW_INSTRUCTIONS], 
            HWCounters[PERF_COUNT_HW_CACHE_REFERENCES], 
            HWCounters[PERF_COUNT_HW_CACHE_MISSES]); 
  ::fprintf(stdout, 
            " %lld %lld %lld", 
            HWCounters[PERF_COUNT_HW_BRANCH_INSTRUCTIONS], 
            HWCounters[PERF_COUNT_HW_BRANCH_MISSES],
            HWCounters[PERF_COUNT_HW_BUS_CYCLES]); 
  ::fprintf(stdout, 
            " %lld %lld %f\n", 
            HWCounters[PERF_COUNT_HW_STALLED_CYCLES_FRONTEND], 
            HWCounters[PERF_COUNT_HW_STALLED_CYCLES_BACKEND], 
            qfactor); 
}

static 
int parseOption(int argc, char *argv[], int *iterations, int *cores, char ** log) {
  struct option options[] = {
    {"repeat", required_argument, 0, 'r'}, 
    {"core",   required_argument, 0, 'c'}, 
    {"log",   required_argument, 0, 'l'}, 
    {0, 0, 0, 0}, 
  }; 

  // Set default values
  *iterations = 100; 
  *cores = 1; 
  *log = const_cast<char *>("MergeSort3-Log");

  // Parse options
  do {
    int opt = getopt_long(argc, argv, "", options, NULL); 
    if (opt == -1) return 0; 
    switch(opt) {
    case 'r':
      *iterations = atoi(optarg); 
      break;
    case 'c':
      *cores = atoi(optarg); 
      break;
    case 'l':
      *log = optarg; 
       break;
    case '?':
    default:
      showUsage();
      return 1; 
    };
  } while(1); 
  return 0; 
}


int main(int argc, char* argv[]) {
  PerformanceCounter PMCs[PERF_COUNT_HW_MAX];
  int cores, iterations; 
  char* log; 
  int ret; 
  float qfactor = 1.0f;
  
#ifdef DANBI_DO_NOT_TICKET
  // Merge sort cannot finish without ticketing.
  return 0; 
#endif 

  // Parse options 
  ret = parseOption(argc, argv, &iterations, &cores, &log);
  if (ret) return 1; 

  // Initialize PMCs
  for (int i = 0; i < PERF_COUNT_HW_MAX; ++i)
    PMCs[i].initialize(i, false, true); 

  // Run benchmark 
  ret = benchmarkMergeSort(cores, iterations, qfactor, 10000000, log);
  if (ret) return 2; 

  // Read PMCs
  for (int i = 0; i < PERF_COUNT_HW_MAX; ++i)
    PMCs[i].readCounter(HWCounters + i); 

  // Print out results
  showResult(cores, iterations, qfactor); 

  return 0; 
}

