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

  MergeSort2.cpp -- parallel mergesort
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
# define DBG_EXEC_SPLIT(x) x
# define DBG_DATA(x) x
# define KILO 2
# define QUICKSIZE (2*KILO)
# define MERGESIZE (2*KILO)
#else 
# define DBG_CMD(x) 
# define DBG_EXEC(x)
# define DBG_EXEC_SPLIT(x)
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

/// Merge
#include START_OF_DEFAULT_OPTIMIZATION
__kernel void InitialMerge(void) {
  DECLARE_INPUT_QUEUE(0, Command); // command - chunk size
  DECLARE_INPUT_QUEUE(1, int); // data
  DECLARE_OUTPUT_QUEUE(0, int); // merged data for split kernel
  DECLARE_ROBX(0, int, 1); // elements
#ifdef DBG_ACCOUNT_MERGE_ELM
  int rmg = -1, pmg = -1; 
#endif

  DBG_EXEC( printf("[0x%x] { Run Initial Merge Cmd\n", Thread::getID()) );
  Command c = {-1, -1, -1, -1}; 
  BEGIN_KERNEL_BODY() {
    // Get the chunk size for merge
    RESERVE_POP(0, 1); 
    c = *POP_ADDRESS_AT(0, Command, 0); 
    COMMIT_POP(0); 
    assert(c.Chunk1 > 0); 
    DBG_EXEC( printf("[0x%x] (%d/%d) === Run Initial Merge Cmd (%d, %d) q %d\n", 
                     Thread::getID(), c.ID, c.ID2, c.Chunk1, c.Chunk2, c.Where) ); 

    // When merging whole data, push the merged data to test sink 
    int Size = c.Chunk1 + c.Chunk2;  

    // Reserve input and output
    RESERVE_POP_TICKET_INQ(1, 0, Size); 
    RESERVE_PUSH_TICKET_INQ(0, 0, Size); 

    // Merge
    int start1 = 0, start2 = c.Chunk1; 
    int end1 = c.Chunk1 - 1, end2 = Size - 1; 
    int i1 = start1, i2 = start2, oi = 0; 
    DBG_DATA( printf("Initial Merged Data %d [", c.Where) ); 
    if (c.Chunk2 == 0) {
      for (; i1 <= end1; ++i1, ++oi) {
        *PUSH_ADDRESS_AT(0, int, oi) = *POP_ADDRESS_AT(1, int, i1);
        DBG_DATA( printf("%d ", *PUSH_ADDRESS_AT(0, int, oi)) );
      }
    }
    else {
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
    }
    assert( oi == Size && "Whole data are not merged."); 
    DBG_DATA( printf("]\n") ); 
 
    // Commit input and output
    COMMIT_POP(1); 
    COMMIT_PUSH(0); 

#ifdef DBG_ACCOUNT_MERGE_ELM
    Machine::rmb(); 
    pmg = Machine::atomicIntAddReturn<volatile int>(&ProcessedMergeElm, Size); 
    rmg = RequestedMergeElm; 
#endif    
  } END_KERNEL_BODY(); 
#ifdef DBG_ACCOUNT_MERGE_ELM
  DBG_EXEC( printf("[0x%x] (%d/%d) } Run Initial Merge Cmd (%d, %d) q %d pmg/rmg(%d/%d)\n", 
                   Thread::getID(), c.ID, c.ID2, c.Chunk1, c.Chunk2, c.Where, 
                   pmg, rmg) ); 
#else
  DBG_EXEC( printf("[0x%x] (%d/%d) } Run Initial Merge Cmd (%d, %d) q %d\n", 
                   Thread::getID(), c.ID, c.ID2, c.Chunk1, c.Chunk2, c.Where) ); 
#endif
}
#include END_OF_DEFAULT_OPTIMIZATION

/// Split
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

__kernel void Split(void) {
  DECLARE_INPUT_QUEUE(0, Command); // command - split size 
  DECLARE_INPUT_QUEUE(1, int); // data from initial merge kernel 
  DECLARE_INPUT_QUEUE(2, int); // data from merge kernel 
  DECLARE_OUTPUT_QUEUE(0, Command); // command for merge kernel 
  DECLARE_OUTPUT_QUEUE(1, int); // splitd data 
  DECLARE_ROBX(0, int, 1); // elements
  int N = *ROBX(0, int, 0); 
#ifdef DBG_ACCOUNT_MERGE_ELM
  int rmg = -1; 
#endif

  DBG_EXEC_SPLIT( printf("[0x%x] { Run Split Cmd\n", Thread::getID()) ); 
  DBG_EXEC_SPLIT( fflush(stdout) );
  Command c = {-1, -1, -1, -1};
  BEGIN_KERNEL_BODY() {
    // Get the number of data for this
    RESERVE_POP(0, 1); 
    c = *POP_ADDRESS_AT(0, Command, 0); 
    COMMIT_POP(0); 
    DBG_EXEC_SPLIT( printf("[0x%x] (%d) === Run Split Cmd (%d, %d) q %d\n", 
                             Thread::getID(), c.ID, c.Chunk1, c.Chunk2, c.Where) ); 
    DBG_EXEC_SPLIT( fflush(stdout) );

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

    // Split
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
        if (low1 <= end1) {
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
      assert(Num == MergeSum && "Incorrect merge command generation in split kernel.");

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
  DBG_EXEC_SPLIT( printf("[0x%x] (%d) } Run Split Cmd (%d, %d) q %d rmg: %d\n", 
                           Thread::getID(), c.ID, c.Chunk1, c.Chunk2, c.Where, rmg) ); 
#else 
  DBG_EXEC_SPLIT( printf("[0x%x] (%d) } Run Split Cmd (%d, %d) q %d\n", 
                           Thread::getID(), c.ID, c.Chunk1, c.Chunk2, c.Where) ); 
#endif 
  DBG_EXEC_SPLIT( fflush(stdout) );
}
#include END_OF_DEFAULT_OPTIMIZATION

/// Merge
#include START_OF_DEFAULT_OPTIMIZATION
__kernel void Merge(void) {
  DECLARE_INPUT_QUEUE(0, Command); // command - chunk size
  DECLARE_INPUT_QUEUE(1, int); // data
  DECLARE_OUTPUT_QUEUE(0, int); // merged data for split kernel
  DECLARE_OUTPUT_QUEUE(1, int); // merged data for test sink
  DECLARE_ROBX(0, int, 1); // elements
  int N = *ROBX(0, int, 0); 
#ifdef DBG_ACCOUNT_MERGE_ELM
  int rmg = -1, pmg = -1; 
#endif

  DBG_EXEC( printf("[0x%x] { Run Merge Cmd\n", Thread::getID()) );
  Command c = {-1, -1, -1, -1}; 
  BEGIN_KERNEL_BODY() {
    // Get the chunk size for merge
    DBG_EXEC( printf("[0x%x] ?1 Run Merge Cmd\n", Thread::getID()) );
    RESERVE_POP(0, 1); 
    DBG_EXEC( printf("[0x%x] ?2 Run Merge Cmd\n", Thread::getID()) );
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

/// Test source 
int random(int x) {
  return 1103515245 * x + 12345; 
}

#include START_OF_DEFAULT_OPTIMIZATION
__kernel void TestSource(void) {
  DECLARE_OUTPUT_QUEUE(0, int); // raw data
  DECLARE_OUTPUT_QUEUE(1, int); // sort command
  DECLARE_OUTPUT_QUEUE(2, Command); // initial merge command
  DECLARE_OUTPUT_QUEUE(3, Command); // split command
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
      // Generate data
      RESERVE_PUSH(0, N);
      COMMIT_PUSH(0); 

      // Generate sort command
      int NumSortCmd = std::max(N / QUICKSIZE, 1); 
      if ( (NumSortCmd % 2) && (NumSortCmd > 2) ) 
        --NumSortCmd; 
      assert( (NumSortCmd >= 1) && ((NumSortCmd == 1) || (NumSortCmd%2 == 0)) );
      int Cmd[NumSortCmd]; 
      DBG_CMD( int SUM = 0; );
      RESERVE_PUSH(1, NumSortCmd); 
      for (int c = 0; c < NumSortCmd ; ++c) {
        int SortSize = ( (c == (NumSortCmd - 1)) ? 
                         (N - (c * QUICKSIZE)) : QUICKSIZE ); 
        *PUSH_ADDRESS_AT(1, int, c) = Cmd[c] = SortSize; 
        DBG_CMD( SUM += SortSize; ); 
        DBG_CMD( printf("[SequentialSortCmd] s %d S %d\n", SortSize, SUM) ); 
      }
      DBG_CMD( assert(SUM == N) ); 
      COMMIT_PUSH(1);

      // Generate initial merge command 
      int NumInitialMergeCmd = 0;
      int pi = 0, ci = 0; 
      Command* cc;
      for (int c = 0; c < NumSortCmd ; c += 2) {
        RESERVE_PUSH(2, 1); 
        cc = PUSH_ADDRESS_AT(2, Command, 0); 
        cc->Chunk1 = Cmd[ci%NumSortCmd]; ci++; 
        if ((NumSortCmd > 1) && (cc->Chunk1 < N)) {
          cc->Chunk2 = Cmd[ci%NumSortCmd]; ci++; 
        }
        else {
          cc->Chunk2 = 0; 
        }
        Cmd[pi%NumSortCmd] = cc->Chunk1 + cc->Chunk2; pi++; 
        cc->Where = -1;
        cc->ID = id; id++; NumInitialMergeCmd++;
        DBG_CMD( printf("[InitialMergeCmd] %d (%d, %d) q %d\n", 
                        cc->ID, cc->Chunk1, cc->Chunk2, cc->Where) ); 
        COMMIT_PUSH(2);
      } 
      
      // Generate split command 
      int si = 0; 
      do {
        RESERVE_PUSH(3, 1); 
        cc = PUSH_ADDRESS_AT(3, Command, 0); 
        cc->Chunk1 = Cmd[ci%NumSortCmd]; ci++; si++;
        if ( (NumInitialMergeCmd % 2) && (si == NumInitialMergeCmd) ) {
          cc->Chunk2 = 0; 
        }
        else {
          if ((NumSortCmd > 1) && (cc->Chunk1 < N)) {
            cc->Chunk2 = Cmd[ci%NumSortCmd]; ci++; si++;
          }
          else 
            cc->Chunk2 = 0; 
        }
        Cmd[pi%NumSortCmd] = cc->Chunk1 + cc->Chunk2; pi++; 
        cc->Where = ((si <= NumInitialMergeCmd) ? 1 : 2); 
        cc->ID = id; id++; 
        DBG_CMD( printf("[SplitCmd] %d (%d, %d) q %d\n", 
                        cc->ID, cc->Chunk1, cc->Chunk2, cc->Where) ); 
        COMMIT_PUSH(3);
      } while((cc->Chunk1 + cc->Chunk2) < N);
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
int benchmarkMergeSort2(int Cores, int Iter, int Elements, char *Log) {
  /// Benchmark pipeline 
  // [TestSource]--
  //      [[SeuqntialSort]]--[[InitialMerge]]---[[Split]]--[[Merge]]-+--
  //                                         /|\                     |
  //                                          +----------------------+
  // --[TestSink]
  ProgramDescriptor PD("MergeSort2"); 

  // Code
  PD.CodeTable["TestSource"] = new CodeDescriptor(TestSource);
  PD.CodeTable["SequentialSort"] = new CodeDescriptor(SequentialSort);
  PD.CodeTable["InitialMerge"] = new CodeDescriptor(InitialMerge);
  PD.CodeTable["Split"] = new CodeDescriptor(Split);
  PD.CodeTable["Merge"] = new CodeDescriptor(Merge);
  PD.CodeTable["TestSink"] = new CodeDescriptor(TestSink);

  // ROB
  int I_[1] = {Iter}; 
  PD.ROBTable["I"] = new ReadOnlyBufferDescriptor(I_, sizeof(int), 1); 
  int N_[1] = {Elements}; 
  PD.ROBTable["N"] = new ReadOnlyBufferDescriptor(N_, sizeof(int), 1); 

  // Queue
  const int QSize = Elements; 
  const int CmdQSize = ((Elements/QUICKSIZE) + 1);
  const int RootQSize = QSize * 2;
  const int RootCmdQSize = CmdQSize * 2;
  PD.QTable["RawDataQ"] = new QueueDescriptor(RootQSize, sizeof(int), false, 
                                              "01TestSourceK", "02SequentialSortK",
                                              false, false, false, true);
  PD.QTable["SequentialSortCmdQ"] = new QueueDescriptor(RootCmdQSize, sizeof(int), false, 
                                                        "01TestSourceK", "02SequentialSortK",
                                                        false, false, true, false);
  PD.QTable["SortedDataQ"] = new QueueDescriptor(RootQSize, sizeof(int), false, 
                                                 "02SequentialSortK", "03InitialMergeK",
                                                 false, true, false, true);
  PD.QTable["InitialMergeCmdQ"] = new QueueDescriptor(RootQSize, sizeof(Command), false, 
                                                      "01TestSourceK", "03InitialMergeK",
                                                      false, false, true, false);
  PD.QTable["InitialMergedDataQ"] = new QueueDescriptor(RootQSize, sizeof(int), false, 
                                                        "03InitialMergeK", "04SplitK",
                                                        false, true, false, true);
  PD.QTable["SplitCmdQ"] = new QueueDescriptor(RootQSize, sizeof(Command), false, 
                                                 "01TestSourceK", "04SplitK",
                                                 false, false, true, false);
  PD.QTable["SplitdDataQ"] = new QueueDescriptor(QSize, sizeof(int), false, 
                                                   "04SplitK", "05MergeK",
                                                   false, true, false, true);
  PD.QTable["MergeCmdQ"] = new QueueDescriptor(CmdQSize, sizeof(Command), false, 
                                               "04SplitK", "05MergeK",
                                               false, true, true, false);
  PD.QTable["MergedDataQ"] = new QueueDescriptor(QSize, sizeof(int), true, 
                                                 "05MergeK", "04SplitK",
                                                 false, true, false, true);
  PD.QTable["FinalMergedDataQ"] = new QueueDescriptor(QSize, sizeof(int), false, 
                                                      "05MergeK", "06TestSinkK",
                                                      false, true, false, false);

  // Kernel
  PD.KernelTable["01TestSourceK"] = new KernelDescriptor("TestSource", true, false); 
  PD.KernelTable["01TestSourceK"]->OutQ[0] = "RawDataQ";
  PD.KernelTable["01TestSourceK"]->OutQ[1] = "SequentialSortCmdQ";
  PD.KernelTable["01TestSourceK"]->OutQ[2] = "InitialMergeCmdQ";
  PD.KernelTable["01TestSourceK"]->OutQ[3] = "SplitCmdQ";
  PD.KernelTable["01TestSourceK"]->ROB[0] = "I";
  PD.KernelTable["01TestSourceK"]->ROB[1] = "N";

  PD.KernelTable["02SequentialSortK"] = new KernelDescriptor("SequentialSort", false, true); 
  PD.KernelTable["02SequentialSortK"]->InQ[0] = "SequentialSortCmdQ";
  PD.KernelTable["02SequentialSortK"]->InQ[1] = "RawDataQ";
  PD.KernelTable["02SequentialSortK"]->OutQ[0] = "SortedDataQ";

  PD.KernelTable["03InitialMergeK"] = new KernelDescriptor("InitialMerge", false, true); 
  PD.KernelTable["03InitialMergeK"]->InQ[0] = "InitialMergeCmdQ";
  PD.KernelTable["03InitialMergeK"]->InQ[1] = "SortedDataQ";
  PD.KernelTable["03InitialMergeK"]->OutQ[0] = "InitialMergedDataQ";

  PD.KernelTable["04SplitK"] = new KernelDescriptor("Split", false, true); 
  PD.KernelTable["04SplitK"]->InQ[0] = "SplitCmdQ";
  PD.KernelTable["04SplitK"]->InQ[1] = "InitialMergedDataQ";
  PD.KernelTable["04SplitK"]->InQ[2] = "MergedDataQ";
  PD.KernelTable["04SplitK"]->OutQ[0] = "MergeCmdQ";
  PD.KernelTable["04SplitK"]->OutQ[1] = "SplitdDataQ";
  PD.KernelTable["04SplitK"]->ROB[0] = "N";

  PD.KernelTable["05MergeK"] = new KernelDescriptor("Merge", false, true); 
  PD.KernelTable["05MergeK"]->InQ[0] = "MergeCmdQ";
  PD.KernelTable["05MergeK"]->InQ[1] = "SplitdDataQ";
  PD.KernelTable["05MergeK"]->OutQ[0] = "MergedDataQ";
  PD.KernelTable["05MergeK"]->OutQ[1] = "FinalMergedDataQ";
  PD.KernelTable["05MergeK"]->ROB[0] = "N";

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
            "  Usage: MergeSortRun \n"); 
  ::fprintf(stderr, 
            "      [--repeat repeat_count]\n"); 
  ::fprintf(stderr, 
            "      [--core maximum_number_of_assignable_cores]\n");
  ::fprintf(stderr, 
            "      [--element number_of_data_elements]\n");
}

static
void showResult(int cores, int iterations) {
  double InstrPerCycle = (double)HWCounters[PERF_COUNT_HW_INSTRUCTIONS]/
    (double)HWCounters[PERF_COUNT_HW_CPU_CYCLES]; 

  double CyclesPerInstr = (double)HWCounters[PERF_COUNT_HW_CPU_CYCLES]/
    (double)HWCounters[PERF_COUNT_HW_INSTRUCTIONS];

  double StalledCyclesPerInstr = (double)(
    HWCounters[PERF_COUNT_HW_STALLED_CYCLES_FRONTEND] +
    HWCounters[PERF_COUNT_HW_STALLED_CYCLES_BACKEND]) / 
    (double)HWCounters[PERF_COUNT_HW_INSTRUCTIONS]; 

  ::fprintf(stdout, 
            "# DANBI merge sort benchmark\n");
  ::fprintf(stdout, 
            "# cores(1) iterations(2) microsec(3)"
            " InstrPerCycle(4) CyclesPerInstr(5) StalledCyclesPerInstr(6)"
            " HW_CPU_CYCLES(7) HW_INSTRUCTIONS(8) CACHE_REFERENCES(9) HW_CACHE_MISSES(10)"
            " HW_BRANCH_INSTRUCTIONS(11) HW_BRANCH_MISSES(12) HW_BUS_CYCLES(13)"
            " HW_STALLED_CYCLES_FRONTEND(14) HW_STALLED_CYCLES_BACKEND(15)\n");
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
            " %lld %lld\n", 
            HWCounters[PERF_COUNT_HW_STALLED_CYCLES_FRONTEND], 
            HWCounters[PERF_COUNT_HW_STALLED_CYCLES_BACKEND]); 
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
  *iterations = 150; 
  *cores = 1; 
  *log = const_cast<char *>("MergeSort2-Log");

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
  ret = benchmarkMergeSort2(cores, iterations, 10000000, log);
  if (ret) return 2; 

  // Read PMCs
  for (int i = 0; i < PERF_COUNT_HW_MAX; ++i)
    PMCs[i].readCounter(HWCounters + i); 

  // Print out results
  showResult(cores, iterations); 

  return 0; 
}

