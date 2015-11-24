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

  FilterBank.cpp -- Filter Bank 
  Implementation is based on the code from StreamIt benchmark. 
 */
#include <unistd.h>
#include <getopt.h>
#include <algorithm>
#include <math.h>
#include "DanbiCPU.h"
#include "Support/PerformanceCounter.h"
#include "DebugInfo/ProgramVisualizer.h"
static unsigned long long HWCounters[PERF_COUNT_HW_MAX];
static unsigned long long ExecTimeInMicroSec; 

#undef TRACE_KERNEL_EXEC
#ifdef TRACE_KERNEL_EXEC
#define KSTART() printf("  <%s:%d\n", __func__, __LINE__)
#define KMID()   printf("  -%s:%d\n", __func__, __LINE__)
#define KEND()   printf("  >%s:%d\n", __func__, __LINE__)
#else
#define KSTART()
#define KMID()  
#define KEND()  
#endif 

#define PADDING_SIZE (128*3-1)
#define PI 3.14159265f

/** 
 * Simple FIR low pass filter with gain=g, wc=cutoffFreq(in radians) and N samples.
 * Eg:
 *                 ^ H(e^jw)
 *                 |
 *          ---------------
 *          |      |      |
 *          |      |      |
 *    <-------------------------> w
 *         -wc            wc
 *
 * This implementation is a FIR filter is a rectangularly windowed sinc function 
 * (eg sin(x)/x), which is the optimal FIR low pass filter in 
 * mean square error terms.
 *
 * Specifically, h[n] has N samples from n=0 to (N-1)
 * such that h[n] = sin(cutoffFreq*pi*(n-N/2))/(pi*(n-N/2)).
 * and the field h holds h[-n].
 */
struct LowPassFilterParams {
  float g; 
  float cutoffFreq; 
  int N;
  float h[128];
}; 

void CalcLowPassFilterParams(LowPassFilterParams* P) {
  /* since the impulse response is symmetric, I don't worry about reversing h[n]. */
  int OFFSET = P->N/2;
  for (int i=0; i<P->N; i++) {
    int idx = i + 1;
    // generate real part
    if (idx == OFFSET) 
      /* take care of div by 0 error (lim x->oo of sin(x)/x actually equals 1)*/
      P->h[i] = P->g * P->cutoffFreq / PI; 
    else 
      P->h[i] = P->g * sin(P->cutoffFreq * (idx-OFFSET)) / (PI*(idx-OFFSET));
  }
}

#include START_OF_MULTIPLE_ITER_OPTIMIZATION
__kernel void LowPassFilter(void) {
  DECLARE_INPUT_QUEUE(0, float); 
  DECLARE_OUTPUT_QUEUE(0, float);
  DECLARE_ROBX(0, LowPassFilterParams, 1);

  LowPassFilterParams* P = ROBX(0, LowPassFilterParams, 0); 
  BEGIN_KERNEL_BODY() {
    KSTART();

    // Reserve input and output data
    RESERVE_PEEK_POP(0, P->N, 1); 
    RESERVE_PUSH_TICKET_INQ(0, 0, 1); 

    // Implement the FIR filtering operation as the convolution sum.
    float sum = 0.0f;
    for (int i = 0; i < P->N; i++)
      sum += P->h[i] * *POP_ADDRESS_AT(0, float, i);
    *PUSH_ADDRESS_AT(0, float, 0) = sum; 

    // Commit input and output data 
    COMMIT_PUSH(0); 
    COMMIT_PEEK_POP(0);

    KEND();
  } END_KERNEL_BODY(); 
}
#include END_OF_MULTIPLE_ITER_OPTIMIZATION

/** 
 * Simple FIR high pass filter with gain=g, stopband ws(in radians) and N samples.
 *
 * Eg
 *                 ^ H(e^jw)
 *                 |
 *     --------    |    -------
 *     |      |    |    |     |
 *     |      |    |    |     |
 *    <-------------------------> w
 *                   pi-wc pi pi+wc
 *
 *
 * This implementation is a FIR filter is a rectangularly windowed sinc function 
 * (eg sin(x)/x) multiplied by e^(j*pi*n)=(-1)^n, which is the optimal FIR high pass filter in 
 * mean square error terms.
 *
 * Specifically, h[n] has N samples from n=0 to (N-1)
 * such that h[n] = (-1)^(n-N/2) * sin(cutoffFreq*pi*(n-N/2))/(pi*(n-N/2)).
 * where cutoffFreq is pi-ws
 * and the field h holds h[-n].
 */
struct HighPassFilterParams {
  float g; 
  float ws; 
  int N;
  float h[128];
}; 

void CalcHighPassFilterParams(HighPassFilterParams* P) {
  /* since the impulse response is symmetric, I don't worry about reversing h[n]. */
  int OFFSET = P->N/2;
  float cutoffFreq = PI - P->ws;
  for (int i = 0; i < P->N; i++) {
    int idx = i + 1;
    /* flip signs every other sample (done this way so that it gets array destroyed) */
    int sign = ((i%2) == 0) ? 1 : -1;
    // generate real part
    if (idx == OFFSET) 
      /* take care of div by 0 error (lim x->oo of sin(x)/x actually equals 1)*/
      P->h[i] = sign * P->g * cutoffFreq / PI; 
    else 
      P->h[i] = sign * P->g * sin(cutoffFreq * (idx-OFFSET)) / (PI*(idx-OFFSET));
  }
}

#include START_OF_MULTIPLE_ITER_OPTIMIZATION
__kernel void HighPassFilter(void) {
  DECLARE_INPUT_QUEUE(0, float); 
  DECLARE_OUTPUT_QUEUE(0, float);
  DECLARE_ROBX(0, HighPassFilterParams, 1);

  HighPassFilterParams* P = ROBX(0, HighPassFilterParams, 0); 
  BEGIN_KERNEL_BODY() {
    KSTART();

    // Reserve input and output data
    RESERVE_PEEK_POP(0, P->N, 1); 
    RESERVE_PUSH_TICKET_INQ(0, 0, 1); 

    // Implement the FIR filtering operation as the convolution sum.
    float sum = 0.0f;
    for (int i = 0; i < P->N; i++)
      sum += P->h[i] * *POP_ADDRESS_AT(0, float, i);
    *PUSH_ADDRESS_AT(0, float, 0) = sum; 

    // Commit input and output data 
    COMMIT_PUSH(0); 
    COMMIT_PEEK_POP(0);

    KEND();
  } END_KERNEL_BODY(); 
}
#include END_OF_MULTIPLE_ITER_OPTIMIZATION

struct Compressor_ProcessFilter_ExpanderParams {
  int M; // for Compressor
  int order; // for ProcessFiler 
  int L; // for Expander
}; 

#include START_OF_MULTIPLE_ITER_OPTIMIZATION
__kernel void Compressor_ProcessFilter_Expander(void) {
  DECLARE_INPUT_QUEUE(0, float); 
  DECLARE_OUTPUT_QUEUE(0, float);
  DECLARE_OUTPUT_QUEUE(1, float);
  DECLARE_ROBX(0, Compressor_ProcessFilter_ExpanderParams, 1);

  Compressor_ProcessFilter_ExpanderParams* P = 
    ROBX(0, Compressor_ProcessFilter_ExpanderParams, 0); 
  BEGIN_KERNEL_BODY() {
    KSTART();

    // Reserve input and output data
    RESERVE_POP(0, P->M); 
    RESERVE_PUSH_TICKET_INQ(0, 0, P->L); 
    RESERVE_PUSH_TICKET_INQ(1, 0, P->L); 

    // Compressor
    // - This filter compresses the signal at its input by a factor M.
    // - Eg it inputs M samples, and only outputs the first sample.
    *PUSH_ADDRESS_AT(0, float, 0) = 
      *PUSH_ADDRESS_AT(1, float, 0) = *POP_ADDRESS_AT(0, float, 0);
    
    // ProcessFilter
    // - this is the filter that we are processing the sub bands with.
    // Do nothing

    // Expander
    // - This filter expands the input by a factor L. Eg in takes in one
    // - sample and outputs L samples. The first sample is the input
    // - and the rest of the samples are zeros. 
    for (int i = 1; i < P->L; ++i) 
      *PUSH_ADDRESS_AT(0, float, i) = *PUSH_ADDRESS_AT(1, float, i) = 0.0f; 

    // Commit input and output data 
    COMMIT_PUSH(1); 
    COMMIT_PUSH(0); 
    COMMIT_POP(0);

    KEND();
  } END_KERNEL_BODY(); 
}
#include END_OF_MULTIPLE_ITER_OPTIMIZATION

#include START_OF_MULTIPLE_ITER_OPTIMIZATION
__kernel void AddSixteenQueues(void) {
  DECLARE_INPUT_QUEUE(0, float); 
  DECLARE_INPUT_QUEUE(1, float); 
  DECLARE_INPUT_QUEUE(2, float); 
  DECLARE_INPUT_QUEUE(3, float); 
  DECLARE_INPUT_QUEUE(4, float); 
  DECLARE_INPUT_QUEUE(5, float); 
  DECLARE_INPUT_QUEUE(6, float); 
  DECLARE_INPUT_QUEUE(7, float); 
  DECLARE_INPUT_QUEUE(8, float); 
  DECLARE_INPUT_QUEUE(9, float); 
  DECLARE_INPUT_QUEUE(10, float); 
  DECLARE_INPUT_QUEUE(11, float); 
  DECLARE_INPUT_QUEUE(12, float); 
  DECLARE_INPUT_QUEUE(13, float); 
  DECLARE_INPUT_QUEUE(14, float); 
  DECLARE_INPUT_QUEUE(15, float); 
  DECLARE_OUTPUT_QUEUE(0, float);

  BEGIN_KERNEL_BODY() {
   KSTART();

   float sum = 0.0f; 
 
   RESERVE_POP(0, 1); 
   sum += *POP_ADDRESS_AT(0, float, 0);
   COMMIT_POP(0);
 
   RESERVE_POP_TICKET_INQ(1, 0, 1); 
   sum += *POP_ADDRESS_AT(1, float, 0);
   COMMIT_POP(1);
 
   RESERVE_POP_TICKET_INQ(2, 0, 1); 
   sum += *POP_ADDRESS_AT(2, float, 0);
   COMMIT_POP(2);
 
   RESERVE_POP_TICKET_INQ(3, 0, 1); 
   sum += *POP_ADDRESS_AT(3, float, 0);
   COMMIT_POP(3);
 
   RESERVE_POP_TICKET_INQ(4, 0, 1); 
   sum += *POP_ADDRESS_AT(4, float, 0);
   COMMIT_POP(4);
 
   RESERVE_POP_TICKET_INQ(5, 0, 1); 
   sum += *POP_ADDRESS_AT(5, float, 0);
   COMMIT_POP(5);
 
   RESERVE_POP_TICKET_INQ(6, 0, 1); 
   sum += *POP_ADDRESS_AT(6, float, 0);
   COMMIT_POP(6);
 
   RESERVE_POP_TICKET_INQ(7, 0, 1); 
   sum += *POP_ADDRESS_AT(7, float, 0);
   COMMIT_POP(7);
 
   RESERVE_POP_TICKET_INQ(8, 0, 1); 
   sum += *POP_ADDRESS_AT(8, float, 0);
   COMMIT_POP(8);
 
   RESERVE_POP_TICKET_INQ(9, 0, 1); 
   sum += *POP_ADDRESS_AT(9, float, 0);
   COMMIT_POP(9);
 
   RESERVE_POP_TICKET_INQ(10, 0, 1); 
   sum += *POP_ADDRESS_AT(10, float, 0);
   COMMIT_POP(10);
 
   RESERVE_POP_TICKET_INQ(11, 0, 1); 
   sum += *POP_ADDRESS_AT(11, float, 0);
   COMMIT_POP(11);
 
   RESERVE_POP_TICKET_INQ(12, 0, 1); 
   sum += *POP_ADDRESS_AT(12, float, 0);
   COMMIT_POP(12);
 
   RESERVE_POP_TICKET_INQ(13, 0, 1); 
   sum += *POP_ADDRESS_AT(13, float, 0);
   COMMIT_POP(13);
 
   RESERVE_POP_TICKET_INQ(14, 0, 1); 
   sum += *POP_ADDRESS_AT(14, float, 0);
   COMMIT_POP(14);
 
   RESERVE_POP_TICKET_INQ(15, 0, 1); 
   sum += *POP_ADDRESS_AT(15, float, 0);
   COMMIT_POP(15);
 
   RESERVE_PUSH_TICKET_INQ(0, 0, 1); 
   *PUSH_ADDRESS_AT(0, float, 0) = sum; 
   COMMIT_PUSH(0); 

   KEND();
  } END_KERNEL_BODY(); 
}
#include END_OF_MULTIPLE_ITER_OPTIMIZATION

#include START_OF_DEFAULT_OPTIMIZATION
__kernel void DupEight(void) {
  DECLARE_INPUT_QUEUE(0, float); 
  DECLARE_OUTPUT_QUEUE(0, float);
  DECLARE_OUTPUT_QUEUE(1, float);
  DECLARE_OUTPUT_QUEUE(2, float);
  DECLARE_OUTPUT_QUEUE(3, float);
  DECLARE_OUTPUT_QUEUE(4, float);
  DECLARE_OUTPUT_QUEUE(5, float);
  DECLARE_OUTPUT_QUEUE(6, float);
  DECLARE_OUTPUT_QUEUE(7, float);
  DECLARE_ROBX(0, int, 1);

  int N = *ROBX(0, int, 0); 
  int Total = N + PADDING_SIZE;
  int BaseDupSize = std::min(128 * DANBI_MULTIPLE_ITER_COUNT, Total);
  int DupSize = BaseDupSize;
  BEGIN_KERNEL_BODY() {
    KSTART();
    RESERVE_POP(0, DupSize); 
    
    do {
      // Copy to eight output queues
      RESERVE_PUSH_TICKET_INQ(0, 0, DupSize);  
      COPY_QUEUE(0, float, 0, DupSize, 0, 0);
      COMMIT_PUSH(0); 
      
      RESERVE_PUSH_TICKET_INQ(1, 0, DupSize);  
      COPY_QUEUE(1, float, 0, DupSize, 0, 0);
      COMMIT_PUSH(1); 
      
      RESERVE_PUSH_TICKET_INQ(2, 0, DupSize);  
      COPY_QUEUE(2, float, 0, DupSize, 0, 0);
      COMMIT_PUSH(2); 

      RESERVE_PUSH_TICKET_INQ(3, 0, DupSize);  
      COPY_QUEUE(3, float, 0, DupSize, 0, 0);
      COMMIT_PUSH(3); 

      RESERVE_PUSH_TICKET_INQ(4, 0, DupSize);  
      COPY_QUEUE(4, float, 0, DupSize, 0, 0);
      COMMIT_PUSH(4); 

      RESERVE_PUSH_TICKET_INQ(5, 0, DupSize);  
      COPY_QUEUE(5, float, 0, DupSize, 0, 0);
      COMMIT_PUSH(5); 

      RESERVE_PUSH_TICKET_INQ(6, 0, DupSize);  
      COPY_QUEUE(6, float, 0, DupSize, 0, 0);
      COMMIT_PUSH(6); 

      RESERVE_PUSH_TICKET_INQ(7, 0, DupSize);  
      COPY_QUEUE(7, float, 0, DupSize, 0, 0);
      COMMIT_PUSH(7); 

      // If it is the last, copy all remainings
      int DupOrder = GET_TICKET_INQ(0) + 1; 
      if ( DupOrder == (Total/BaseDupSize) ) {
        DupSize = Total - (DupOrder * BaseDupSize);
        continue; 
      }
    } while(false);
    COMMIT_POP(0);
    KEND();
  } END_KERNEL_BODY(); 
}
#include END_OF_DEFAULT_OPTIMIZATION

/// Test source 
#include START_OF_DEFAULT_OPTIMIZATION
__kernel void TestSource(void) {
  DECLARE_OUTPUT_QUEUE(0, float);
  DECLARE_ROBX(0, int, 1);
  DECLARE_ROBX(1, int, 1);

  int Iter = *ROBX(0, int, 0); 
  int N = *ROBX(1, int, 0); 
  BEGIN_KERNEL_BODY() {
    KSTART();
    
    // Iterate
    for (int i = 0; i < Iter; ++i) {
      // Generate random test data with garbage values
      RESERVE_PUSH(0, N + PADDING_SIZE); 
      for (int i = 0; i < PADDING_SIZE; ++i) 
        *PUSH_ADDRESS_AT(0, float, N + i) = 0.0f; 
      COMMIT_PUSH(0); 
    }

    KEND();
  } END_KERNEL_BODY(); 
}
#include END_OF_DEFAULT_OPTIMIZATION

/// Test sink 
#include START_OF_DEFAULT_OPTIMIZATION
__kernel void TestSink(void) {
  DECLARE_INPUT_QUEUE(0, float);
  DECLARE_ROBX(0, int, 1);
  DECLARE_ROBX(1, int, 1);

  int Iter = *ROBX(0, int, 0); 
  int N = *ROBX(1, int, 0); 
  BEGIN_KERNEL_BODY() {
    KSTART();

    // Iterate
    for (int i = 0; i < Iter; ++i) {
      RESERVE_POP(0, N); 
      COMMIT_POP(0); 
    }

    KEND();
  } END_KERNEL_BODY(); 
}
#include END_OF_DEFAULT_OPTIMIZATION


/// Utility macro 
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

/// Benchmark recursive gaussian
int benchmarkFilterBank(int Cores, int Iter, float QFactor, char* Log) {
  /// Benchmark pipeline 
  // [01TestSource]--
  //     [[02DupEight]]--
  //       [[03LowPassFilter10]]--[[11HighPassFilter10]]--[[19Compressor_ProcessFilter_Expander10]]--[[27LowPassFilter11, 28HighPassFilter11]]--
  //       [[04LowPassFilter20]]--[[12HighPassFilter20]]--[[20Compressor_ProcessFilter_Expander20]]--[[29LowPassFilter21, 30HighPassFilter21]]--
  //       [[05LowPassFilter30]]--[[13HighPassFilter30]]--[[21Compressor_ProcessFilter_Expander30]]--[[31LowPassFilter31, 32HighPassFilter31]]--
  //       [[06LowPassFilter40]]--[[14HighPassFilter40]]--[[22Compressor_ProcessFilter_Expander40]]--[[33LowPassFilter41, 34HighPassFilter41]]--
  //       [[07LowPassFilter50]]--[[15HighPassFilter50]]--[[23Compressor_ProcessFilter_Expander50]]--[[35LowPassFilter51, 36HighPassFilter51]]--
  //       [[08LowPassFilter60]]--[[16HighPassFilter60]]--[[24Compressor_ProcessFilter_Expander60]]--[[37LowPassFilter61, 38HighPassFilter61]]--
  //       [[09LowPassFilter70]]--[[17HighPassFilter70]]--[[25Compressor_ProcessFilter_Expander70]]--[[39LowPassFilter71, 40HighPassFilter71]]--
  //       [[10LowPassFilter80]]--[[18HighPassFilter80]]--[[26Compressor_ProcessFilter_Expander80]]--[[41LowPassFilter81, 42HighPassFilter81]]--
  //     [[43AddSixteenQueues]]--
  // --[44TestSink]
  ProgramDescriptor PD("FilterBank"); 

  // Code
  PD.CodeTable["TestSource"] = new CodeDescriptor(TestSource); 
  PD.CodeTable["DupEight"] = new CodeDescriptor(DupEight); 
  PD.CodeTable["LowPassFilter"] = new CodeDescriptor(LowPassFilter); 
  PD.CodeTable["HighPassFilter"] = new CodeDescriptor(HighPassFilter); 
  PD.CodeTable["Compressor_ProcessFilter_Expander"] = 
    new CodeDescriptor(Compressor_ProcessFilter_Expander); 
  PD.CodeTable["AddSixteenQueues"] = new CodeDescriptor(AddSixteenQueues); 
  PD.CodeTable["TestSink"] = new CodeDescriptor(TestSink); 

  // ROB
  int I_[1] = {1};
  PD.ROBTable["I"] = new ReadOnlyBufferDescriptor(I_, sizeof(int), 1);
  int N = Iter * 1280000; // For now, N should be multiple of 128. 
  int N_[1] = {N};
  PD.ROBTable["N"] = new ReadOnlyBufferDescriptor(N_, sizeof(int), 1);
  LowPassFilterParams LP0_[8];
  for (int i = 0; i < 8; ++i) {
    LP0_[i].g = 1.0f; 
    LP0_[i].cutoffFreq = (((float)i + 1.0f) * PI) / 8.0f;
    LP0_[i].N = 128;
    CalcLowPassFilterParams(LP0_ + i);
  }
  PD.ROBTable["LP10"] = new ReadOnlyBufferDescriptor(LP0_ + 0, sizeof(LowPassFilterParams), 1);
  PD.ROBTable["LP20"] = new ReadOnlyBufferDescriptor(LP0_ + 1, sizeof(LowPassFilterParams), 1);
  PD.ROBTable["LP30"] = new ReadOnlyBufferDescriptor(LP0_ + 2, sizeof(LowPassFilterParams), 1);
  PD.ROBTable["LP40"] = new ReadOnlyBufferDescriptor(LP0_ + 3, sizeof(LowPassFilterParams), 1);
  PD.ROBTable["LP50"] = new ReadOnlyBufferDescriptor(LP0_ + 4, sizeof(LowPassFilterParams), 1);
  PD.ROBTable["LP60"] = new ReadOnlyBufferDescriptor(LP0_ + 5, sizeof(LowPassFilterParams), 1);
  PD.ROBTable["LP70"] = new ReadOnlyBufferDescriptor(LP0_ + 6, sizeof(LowPassFilterParams), 1);
  PD.ROBTable["LP80"] = new ReadOnlyBufferDescriptor(LP0_ + 7, sizeof(LowPassFilterParams), 1);
  HighPassFilterParams HP0_[8];
  for (int i = 0; i < 8; ++i) {
    HP0_[i].g = 1.0f; 
    HP0_[i].ws = ((float)i * PI) / 8.0f;
    HP0_[i].N = 128;
    CalcHighPassFilterParams(HP0_ + i);
  }
  PD.ROBTable["HP10"] = new ReadOnlyBufferDescriptor(HP0_ + 0, sizeof(HighPassFilterParams), 1);
  PD.ROBTable["HP20"] = new ReadOnlyBufferDescriptor(HP0_ + 1, sizeof(HighPassFilterParams), 1);
  PD.ROBTable["HP30"] = new ReadOnlyBufferDescriptor(HP0_ + 2, sizeof(HighPassFilterParams), 1);
  PD.ROBTable["HP40"] = new ReadOnlyBufferDescriptor(HP0_ + 3, sizeof(HighPassFilterParams), 1);
  PD.ROBTable["HP50"] = new ReadOnlyBufferDescriptor(HP0_ + 4, sizeof(HighPassFilterParams), 1);
  PD.ROBTable["HP60"] = new ReadOnlyBufferDescriptor(HP0_ + 5, sizeof(HighPassFilterParams), 1);
  PD.ROBTable["HP70"] = new ReadOnlyBufferDescriptor(HP0_ + 6, sizeof(HighPassFilterParams), 1);
  PD.ROBTable["HP80"] = new ReadOnlyBufferDescriptor(HP0_ + 7, sizeof(HighPassFilterParams), 1);
  Compressor_ProcessFilter_ExpanderParams CPE_[8];
  for (int i = 0; i < 8; ++i) {
    CPE_[i].M = 8; 
    CPE_[i].order = i;
    CPE_[i].L = 8; 
  }
  PD.ROBTable["CPE10"] = new ReadOnlyBufferDescriptor(
    CPE_ + 0, sizeof(Compressor_ProcessFilter_ExpanderParams), 1);
  PD.ROBTable["CPE20"] = new ReadOnlyBufferDescriptor(
    CPE_ + 1, sizeof(Compressor_ProcessFilter_ExpanderParams), 1);
  PD.ROBTable["CPE30"] = new ReadOnlyBufferDescriptor(
    CPE_ + 2, sizeof(Compressor_ProcessFilter_ExpanderParams), 1);
  PD.ROBTable["CPE40"] = new ReadOnlyBufferDescriptor(
    CPE_ + 3, sizeof(Compressor_ProcessFilter_ExpanderParams), 1);
  PD.ROBTable["CPE50"] = new ReadOnlyBufferDescriptor(
    CPE_ + 4, sizeof(Compressor_ProcessFilter_ExpanderParams), 1);
  PD.ROBTable["CPE60"] = new ReadOnlyBufferDescriptor(
    CPE_ + 5, sizeof(Compressor_ProcessFilter_ExpanderParams), 1);
  PD.ROBTable["CPE70"] = new ReadOnlyBufferDescriptor(
    CPE_ + 6, sizeof(Compressor_ProcessFilter_ExpanderParams), 1);
  PD.ROBTable["CPE80"] = new ReadOnlyBufferDescriptor(
    CPE_ + 7, sizeof(Compressor_ProcessFilter_ExpanderParams), 1);
  LowPassFilterParams LP1_[8];
  for (int i = 0; i < 8; ++i) {
    LP1_[i].g = 8.0f; 
    LP1_[i].cutoffFreq = (((float)i + 1.0f) * PI) / 8.0f;
    LP1_[i].N = 128;
    CalcLowPassFilterParams(LP1_ + i);
  }
  PD.ROBTable["LP11"] = new ReadOnlyBufferDescriptor(LP1_ + 0, sizeof(LowPassFilterParams), 1);
  PD.ROBTable["LP21"] = new ReadOnlyBufferDescriptor(LP1_ + 1, sizeof(LowPassFilterParams), 1);
  PD.ROBTable["LP31"] = new ReadOnlyBufferDescriptor(LP1_ + 2, sizeof(LowPassFilterParams), 1);
  PD.ROBTable["LP41"] = new ReadOnlyBufferDescriptor(LP1_ + 3, sizeof(LowPassFilterParams), 1);
  PD.ROBTable["LP51"] = new ReadOnlyBufferDescriptor(LP1_ + 4, sizeof(LowPassFilterParams), 1);
  PD.ROBTable["LP61"] = new ReadOnlyBufferDescriptor(LP1_ + 5, sizeof(LowPassFilterParams), 1);
  PD.ROBTable["LP71"] = new ReadOnlyBufferDescriptor(LP1_ + 6, sizeof(LowPassFilterParams), 1);
  PD.ROBTable["LP81"] = new ReadOnlyBufferDescriptor(LP1_ + 7, sizeof(LowPassFilterParams), 1);
  HighPassFilterParams HP1_[8];
  for (int i = 0; i < 8; ++i) {
    HP1_[i].g = 8.0f; 
    HP1_[i].ws = ((float)i * PI) / 8.0f;
    HP1_[i].N = 128;
    CalcHighPassFilterParams(HP1_ + i);
  }
  PD.ROBTable["HP11"] = new ReadOnlyBufferDescriptor(HP1_ + 0, sizeof(HighPassFilterParams), 1);
  PD.ROBTable["HP21"] = new ReadOnlyBufferDescriptor(HP1_ + 1, sizeof(HighPassFilterParams), 1);
  PD.ROBTable["HP31"] = new ReadOnlyBufferDescriptor(HP1_ + 2, sizeof(HighPassFilterParams), 1);
  PD.ROBTable["HP41"] = new ReadOnlyBufferDescriptor(HP1_ + 3, sizeof(HighPassFilterParams), 1);
  PD.ROBTable["HP51"] = new ReadOnlyBufferDescriptor(HP1_ + 4, sizeof(HighPassFilterParams), 1);
  PD.ROBTable["HP61"] = new ReadOnlyBufferDescriptor(HP1_ + 5, sizeof(HighPassFilterParams), 1);
  PD.ROBTable["HP71"] = new ReadOnlyBufferDescriptor(HP1_ + 6, sizeof(HighPassFilterParams), 1);
  PD.ROBTable["HP81"] = new ReadOnlyBufferDescriptor(HP1_ + 7, sizeof(HighPassFilterParams), 1);

  // Queue
  const int RootQSize = N + PADDING_SIZE;
  const int QSize = float(std::min(DANBI_MULTIPLE_ITER_COUNT * Cores * 8 * 2 + PADDING_SIZE, 
                                   RootQSize)) * QFactor;
  PD.QTable["01TestSourceQ"] = new QueueDescriptor(RootQSize, sizeof(float), false, 
                                                   "01TestSourceK", 
                                                   "02DupEightK", 
                                                   false, false, true, false);
  PD.QTable["02DupEight10Q"] = new QueueDescriptor(RootQSize, sizeof(float), false, 
                                                   "02DupEightK", 
                                                   "03LowPassFilter10K", 
                                                   false, true, true, false);
  PD.QTable["02DupEight20Q"] = new QueueDescriptor(RootQSize, sizeof(float), false, 
                                                   "02DupEightK", 
                                                   "04LowPassFilter20K", 
                                                   false, true, true, false);
  PD.QTable["02DupEight30Q"] = new QueueDescriptor(RootQSize, sizeof(float), false, 
                                                   "02DupEightK", 
                                                   "05LowPassFilter30K", 
                                                   false, true, true, false);
  PD.QTable["02DupEight40Q"] = new QueueDescriptor(RootQSize, sizeof(float), false, 
                                                   "02DupEightK", 
                                                   "06LowPassFilter40K", 
                                                   false, true, true, false);
  PD.QTable["02DupEight50Q"] = new QueueDescriptor(RootQSize, sizeof(float), false, 
                                                   "02DupEightK", 
                                                   "07LowPassFilter50K", 
                                                   false, true, true, false);
  PD.QTable["02DupEight60Q"] = new QueueDescriptor(RootQSize, sizeof(float), false, 
                                                   "02DupEightK", 
                                                   "08LowPassFilter60K", 
                                                   false, true, true, false);
  PD.QTable["02DupEight70Q"] = new QueueDescriptor(RootQSize, sizeof(float), false, 
                                                   "02DupEightK", 
                                                   "09LowPassFilter70K", 
                                                   false, true, true, false);
  PD.QTable["02DupEight80Q"] = new QueueDescriptor(RootQSize, sizeof(float), false, 
                                                   "02DupEightK", 
                                                   "10LowPassFilter80K", 
                                                   false, true, true, false);
  PD.QTable["03LowPassFilter10Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                        "03LowPassFilter10K", 
                                                        "11HighPassFilter10K",
                                                        false, true, true, false);
  PD.QTable["04LowPassFilter20Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                         "04LowPassFilter20K", 
                                                         "12HighPassFilter20K",
                                                         false, true, true, false);
  PD.QTable["05LowPassFilter30Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                         "05LowPassFilter30K", 
                                                         "13HighPassFilter30K",
                                                         false, true, true, false);
  PD.QTable["06LowPassFilter40Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                         "06LowPassFilter40K", 
                                                         "14HighPassFilter40K",
                                                         false, true, true, false);
  PD.QTable["07LowPassFilter50Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                         "07LowPassFilter50K", 
                                                         "15HighPassFilter50K",
                                                         false, true, true, false);
  PD.QTable["08LowPassFilter60Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                         "08LowPassFilter60K", 
                                                         "16HighPassFilter60K",
                                                         false, true, true, false);
  PD.QTable["09LowPassFilter70Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                         "09LowPassFilter70K", 
                                                         "17HighPassFilter70K",
                                                         false, true, true, false);
  PD.QTable["10LowPassFilter80Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                         "10LowPassFilter80K", 
                                                         "18HighPassFilter80K",
                                                        false, true, true, false);
  PD.QTable["11HighPassFilter10Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                      "11HighPassFilter10K", 
                                                      "19Compressor_ProcessFilter_Expander10K", 
                                                      false, true, true, false);
  PD.QTable["12HighPassFilter20Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                      "12HighPassFilter20K", 
                                                      "20Compressor_ProcessFilter_Expander20K", 
                                                      false, true, true, false);
  PD.QTable["13HighPassFilter30Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                      "13HighPassFilter30K", 
                                                      "21Compressor_ProcessFilter_Expander30K", 
                                                      false, true, true, false);
  PD.QTable["14HighPassFilter40Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                      "14HighPassFilter40K", 
                                                      "22Compressor_ProcessFilter_Expander40K", 
                                                      false, true, true, false);
  PD.QTable["15HighPassFilter50Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                      "15HighPassFilter50K", 
                                                      "23Compressor_ProcessFilter_Expander50K", 
                                                      false, true, true, false);
  PD.QTable["16HighPassFilter60Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                      "16HighPassFilter60K", 
                                                      "24Compressor_ProcessFilter_Expander60K", 
                                                      false, true, true, false);
  PD.QTable["17HighPassFilter70Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                      "17HighPassFilter70K", 
                                                      "25Compressor_ProcessFilter_Expander70K", 
                                                      false, true, true, false);
  PD.QTable["18HighPassFilter80Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                         "18HighPassFilter80K", 
                                                         "26Compressor_ProcessFilter_Expander80K", 
                                                         false, true, true, false);
  PD.QTable["19Compressor_ProcessFilter_Expander10LQ"] = 
    new QueueDescriptor(QSize, sizeof(float), false, 
                        "19Compressor_ProcessFilter_Expander10K", "27LowPassFilter11K", 
                        false, true, true, false);
  PD.QTable["20Compressor_ProcessFilter_Expander20LQ"] = 
    new QueueDescriptor(QSize, sizeof(float), false, 
                        "20Compressor_ProcessFilter_Expander20K", "29LowPassFilter21K", 
                        false, true, true, false);
  PD.QTable["21Compressor_ProcessFilter_Expander30LQ"] = 
    new QueueDescriptor(QSize, sizeof(float), false, 
                        "21Compressor_ProcessFilter_Expander30K", "31LowPassFilter31K", 
                        false, true, true, false);
  PD.QTable["22Compressor_ProcessFilter_Expander40LQ"] = 
    new QueueDescriptor(QSize, sizeof(float), false, 
                        "22Compressor_ProcessFilter_Expander40K", "33LowPassFilter41K", 
                        false, true, true, false);
  PD.QTable["23Compressor_ProcessFilter_Expander50LQ"] = 
    new QueueDescriptor(QSize, sizeof(float), false, 
                        "23Compressor_ProcessFilter_Expander50K", "35LowPassFilter51K", 
                        false, true, true, false);
  PD.QTable["24Compressor_ProcessFilter_Expander60LQ"] = 
    new QueueDescriptor(QSize, sizeof(float), false, 
                        "24Compressor_ProcessFilter_Expander60K", "37LowPassFilter61K", 
                        false, true, true, false);
  PD.QTable["25Compressor_ProcessFilter_Expander70LQ"] = 
    new QueueDescriptor(QSize, sizeof(float), false, 
                        "25Compressor_ProcessFilter_Expander70K", "39LowPassFilter71K", 
                        false, true, true, false);
  PD.QTable["26Compressor_ProcessFilter_Expander80LQ"] = 
    new QueueDescriptor(QSize, sizeof(float), false, 
                        "26Compressor_ProcessFilter_Expander80K", "41LowPassFilter81K", 
                        false, true, true, false);
  PD.QTable["19Compressor_ProcessFilter_Expander10HQ"] = 
    new QueueDescriptor(QSize, sizeof(float), false, 
                        "19Compressor_ProcessFilter_Expander10K", "28HighPassFilter11K", 
                        false, true, true, false);
  PD.QTable["20Compressor_ProcessFilter_Expander20HQ"] = 
    new QueueDescriptor(QSize, sizeof(float), false, 
                        "20Compressor_ProcessFilter_Expander20K", "30HighPassFilter21K", 
                        false, true, true, false);
  PD.QTable["21Compressor_ProcessFilter_Expander30HQ"] = 
    new QueueDescriptor(QSize, sizeof(float), false, 
                        "21Compressor_ProcessFilter_Expander30K", "32HighPassFilter31K", 
                        false, true, true, false);
  PD.QTable["22Compressor_ProcessFilter_Expander40HQ"] = 
    new QueueDescriptor(QSize, sizeof(float), false, 
                        "22Compressor_ProcessFilter_Expander40K", "34HighPassFilter41K", 
                        false, true, true, false);
  PD.QTable["23Compressor_ProcessFilter_Expander50HQ"] = 
    new QueueDescriptor(QSize, sizeof(float), false, 
                        "23Compressor_ProcessFilter_Expander50K", "36HighPassFilter51K", 
                        false, true, true, false);
  PD.QTable["24Compressor_ProcessFilter_Expander60HQ"] = 
    new QueueDescriptor(QSize, sizeof(float), false, 
                        "24Compressor_ProcessFilter_Expander60K", "38HighPassFilter61K", 
                        false, true, true, false);
  PD.QTable["25Compressor_ProcessFilter_Expander70HQ"] = 
    new QueueDescriptor(QSize, sizeof(float), false, 
                        "25Compressor_ProcessFilter_Expander70K", "40HighPassFilter71K", 
                        false, true, true, false);
  PD.QTable["26Compressor_ProcessFilter_Expander80HQ"] = 
    new QueueDescriptor(QSize, sizeof(float), false, 
                        "26Compressor_ProcessFilter_Expander80K", "42HighPassFilter81K", 
                        false, true, true, false);
  PD.QTable["27LowPassFilter11Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                      "27LowPassFilter11K", 
                                                      "43AddSixteenQueuesK", 
                                                      false, true, true, false);
  PD.QTable["29LowPassFilter21Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                      "29LowPassFilter21K", 
                                                      "43AddSixteenQueuesK", 
                                                        false, true, false, true);
  PD.QTable["31LowPassFilter31Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                      "31LowPassFilter31K", 
                                                      "43AddSixteenQueuesK", 
                                                        false, true, false, true);
  PD.QTable["33LowPassFilter41Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                      "33LowPassFilter41K", 
                                                      "43AddSixteenQueuesK", 
                                                        false, true, false, true);
  PD.QTable["35LowPassFilter51Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                      "35LowPassFilter51K", 
                                                      "43AddSixteenQueuesK", 
                                                        false, true, false, true);
  PD.QTable["37LowPassFilter61Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                      "37LowPassFilter61K", 
                                                      "43AddSixteenQueuesK", 
                                                        false, true, false, true);
  PD.QTable["39LowPassFilter71Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                      "39LowPassFilter71K", 
                                                      "43AddSixteenQueuesK", 
                                                        false, true, false, true);
  PD.QTable["41LowPassFilter81Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                      "41LowPassFilter81K", 
                                                      "43AddSixteenQueuesK", 
                                                        false, true, false, true);
  PD.QTable["28HighPassFilter11Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                      "28HighPassFilter11K", 
                                                      "43AddSixteenQueuesK", 
                                                         false, true, false, true);
  PD.QTable["30HighPassFilter21Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                      "30HighPassFilter21K", 
                                                      "43AddSixteenQueuesK", 
                                                         false, true, false, true);
  PD.QTable["32HighPassFilter31Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                      "32HighPassFilter31K", 
                                                      "43AddSixteenQueuesK", 
                                                         false, true, false, true);
  PD.QTable["34HighPassFilter41Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                      "34HighPassFilter41K", 
                                                      "43AddSixteenQueuesK", 
                                                         false, true, false, true);
  PD.QTable["36HighPassFilter51Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                      "36HighPassFilter51K", 
                                                      "43AddSixteenQueuesK", 
                                                         false, true, false, true);
  PD.QTable["38HighPassFilter61Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                      "38HighPassFilter61K", 
                                                      "43AddSixteenQueuesK", 
                                                         false, true, false, true);
  PD.QTable["40HighPassFilter71Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                      "40HighPassFilter71K", 
                                                      "43AddSixteenQueuesK", 
                                                         false, true, false, true);
  PD.QTable["42HighPassFilter81Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                      "42HighPassFilter81K", 
                                                      "43AddSixteenQueuesK", 
                                                         false, true, false, true);
  PD.QTable["43AddSixteenQueuesQ"] = new QueueDescriptor(RootQSize, sizeof(float), false, 
                                                         "43AddSixteenQueuesK", 
                                                         "44TestSinkK",
                                                         false, true, false, false);
  
  // Kernel 
  PD.KernelTable["01TestSourceK"] = new KernelDescriptor("TestSource", true, false); 
  PD.KernelTable["01TestSourceK"]->OutQ[0] = "01TestSourceQ";
  PD.KernelTable["01TestSourceK"]->ROB[0] = "I";
  PD.KernelTable["01TestSourceK"]->ROB[1] = "N";

  PD.KernelTable["02DupEightK"] = 
    new KernelDescriptor("DupEight", false, true);  
  PD.KernelTable["02DupEightK"]->InQ[0] = "01TestSourceQ";
  PD.KernelTable["02DupEightK"]->OutQ[0] = "02DupEight10Q";
  PD.KernelTable["02DupEightK"]->OutQ[1] = "02DupEight20Q";
  PD.KernelTable["02DupEightK"]->OutQ[2] = "02DupEight30Q";
  PD.KernelTable["02DupEightK"]->OutQ[3] = "02DupEight40Q";
  PD.KernelTable["02DupEightK"]->OutQ[4] = "02DupEight50Q";
  PD.KernelTable["02DupEightK"]->OutQ[5] = "02DupEight60Q";
  PD.KernelTable["02DupEightK"]->OutQ[6] = "02DupEight70Q";
  PD.KernelTable["02DupEightK"]->OutQ[7] = "02DupEight80Q";
  PD.KernelTable["02DupEightK"]->ROB[0] = "N";

  PD.KernelTable["03LowPassFilter10K"] = new KernelDescriptor("LowPassFilter", false, true); 
  PD.KernelTable["03LowPassFilter10K"]->InQ[0] = "02DupEight10Q";
  PD.KernelTable["03LowPassFilter10K"]->OutQ[0] = "03LowPassFilter10Q";
  PD.KernelTable["03LowPassFilter10K"]->ROB[0] = "LP10";

  PD.KernelTable["04LowPassFilter20K"] = new KernelDescriptor("LowPassFilter", false, true); 
  PD.KernelTable["04LowPassFilter20K"]->InQ[0] = "02DupEight20Q";
  PD.KernelTable["04LowPassFilter20K"]->OutQ[0] = "04LowPassFilter20Q";
  PD.KernelTable["04LowPassFilter20K"]->ROB[0] = "LP20";

  PD.KernelTable["05LowPassFilter30K"] = new KernelDescriptor("LowPassFilter", false, true); 
  PD.KernelTable["05LowPassFilter30K"]->InQ[0] = "02DupEight30Q";
  PD.KernelTable["05LowPassFilter30K"]->OutQ[0] = "05LowPassFilter30Q";
  PD.KernelTable["05LowPassFilter30K"]->ROB[0] = "LP30";

  PD.KernelTable["06LowPassFilter40K"] = new KernelDescriptor("LowPassFilter", false, true); 
  PD.KernelTable["06LowPassFilter40K"]->InQ[0] = "02DupEight40Q";
  PD.KernelTable["06LowPassFilter40K"]->OutQ[0] = "06LowPassFilter40Q";
  PD.KernelTable["06LowPassFilter40K"]->ROB[0] = "LP40";

  PD.KernelTable["07LowPassFilter50K"] = new KernelDescriptor("LowPassFilter", false, true); 
  PD.KernelTable["07LowPassFilter50K"]->InQ[0] = "02DupEight50Q";
  PD.KernelTable["07LowPassFilter50K"]->OutQ[0] = "07LowPassFilter50Q";
  PD.KernelTable["07LowPassFilter50K"]->ROB[0] = "LP50";

  PD.KernelTable["08LowPassFilter60K"] = new KernelDescriptor("LowPassFilter", false, true); 
  PD.KernelTable["08LowPassFilter60K"]->InQ[0] = "02DupEight60Q";
  PD.KernelTable["08LowPassFilter60K"]->OutQ[0] = "08LowPassFilter60Q";
  PD.KernelTable["08LowPassFilter60K"]->ROB[0] = "LP60";

  PD.KernelTable["09LowPassFilter70K"] = new KernelDescriptor("LowPassFilter", false, true); 
  PD.KernelTable["09LowPassFilter70K"]->InQ[0] = "02DupEight70Q";
  PD.KernelTable["09LowPassFilter70K"]->OutQ[0] = "09LowPassFilter70Q";
  PD.KernelTable["09LowPassFilter70K"]->ROB[0] = "LP70";

  PD.KernelTable["10LowPassFilter80K"] = new KernelDescriptor("LowPassFilter", false, true); 
  PD.KernelTable["10LowPassFilter80K"]->InQ[0] = "02DupEight80Q";
  PD.KernelTable["10LowPassFilter80K"]->OutQ[0] = "10LowPassFilter80Q";
  PD.KernelTable["10LowPassFilter80K"]->ROB[0] = "LP80";

  PD.KernelTable["11HighPassFilter10K"] = new KernelDescriptor("HighPassFilter", false, true); 
  PD.KernelTable["11HighPassFilter10K"]->InQ[0] = "03LowPassFilter10Q";
  PD.KernelTable["11HighPassFilter10K"]->OutQ[0] = "11HighPassFilter10Q";
  PD.KernelTable["11HighPassFilter10K"]->ROB[0] = "HP10";

  PD.KernelTable["12HighPassFilter20K"] = new KernelDescriptor("HighPassFilter", false, true); 
  PD.KernelTable["12HighPassFilter20K"]->InQ[0] = "04LowPassFilter20Q";
  PD.KernelTable["12HighPassFilter20K"]->OutQ[0] = "12HighPassFilter20Q";
  PD.KernelTable["12HighPassFilter20K"]->ROB[0] = "HP20";

  PD.KernelTable["13HighPassFilter30K"] = new KernelDescriptor("HighPassFilter", false, true); 
  PD.KernelTable["13HighPassFilter30K"]->InQ[0] = "05LowPassFilter30Q";
  PD.KernelTable["13HighPassFilter30K"]->OutQ[0] = "13HighPassFilter30Q";
  PD.KernelTable["13HighPassFilter30K"]->ROB[0] = "HP30";

  PD.KernelTable["14HighPassFilter40K"] = new KernelDescriptor("HighPassFilter", false, true); 
  PD.KernelTable["14HighPassFilter40K"]->InQ[0] = "06LowPassFilter40Q";
  PD.KernelTable["14HighPassFilter40K"]->OutQ[0] = "14HighPassFilter40Q";
  PD.KernelTable["14HighPassFilter40K"]->ROB[0] = "HP40";

  PD.KernelTable["15HighPassFilter50K"] = new KernelDescriptor("HighPassFilter", false, true); 
  PD.KernelTable["15HighPassFilter50K"]->InQ[0] = "07LowPassFilter50Q";
  PD.KernelTable["15HighPassFilter50K"]->OutQ[0] = "15HighPassFilter50Q";
  PD.KernelTable["15HighPassFilter50K"]->ROB[0] = "HP50";

  PD.KernelTable["16HighPassFilter60K"] = new KernelDescriptor("HighPassFilter", false, true); 
  PD.KernelTable["16HighPassFilter60K"]->InQ[0] = "08LowPassFilter60Q";
  PD.KernelTable["16HighPassFilter60K"]->OutQ[0] = "16HighPassFilter60Q";
  PD.KernelTable["16HighPassFilter60K"]->ROB[0] = "HP60";

  PD.KernelTable["17HighPassFilter70K"] = new KernelDescriptor("HighPassFilter", false, true); 
  PD.KernelTable["17HighPassFilter70K"]->InQ[0] = "09LowPassFilter70Q";
  PD.KernelTable["17HighPassFilter70K"]->OutQ[0] = "17HighPassFilter70Q";
  PD.KernelTable["17HighPassFilter70K"]->ROB[0] = "HP70";

  PD.KernelTable["18HighPassFilter80K"] = new KernelDescriptor("HighPassFilter", false, true); 
  PD.KernelTable["18HighPassFilter80K"]->InQ[0] = "10LowPassFilter80Q";
  PD.KernelTable["18HighPassFilter80K"]->OutQ[0] = "18HighPassFilter80Q";
  PD.KernelTable["18HighPassFilter80K"]->ROB[0] = "HP80";

  PD.KernelTable["19Compressor_ProcessFilter_Expander10K"] = 
    new KernelDescriptor("Compressor_ProcessFilter_Expander", false, true); 
  PD.KernelTable["19Compressor_ProcessFilter_Expander10K"]->InQ[0] = "11HighPassFilter10Q";
  PD.KernelTable["19Compressor_ProcessFilter_Expander10K"]->OutQ[0] = 
    "19Compressor_ProcessFilter_Expander10LQ";
  PD.KernelTable["19Compressor_ProcessFilter_Expander10K"]->OutQ[1] = 
    "19Compressor_ProcessFilter_Expander10HQ";
  PD.KernelTable["19Compressor_ProcessFilter_Expander10K"]->ROB[0] = "CPE10";

  PD.KernelTable["20Compressor_ProcessFilter_Expander20K"] = 
    new KernelDescriptor("Compressor_ProcessFilter_Expander", false, true); 
  PD.KernelTable["20Compressor_ProcessFilter_Expander20K"]->InQ[0] = "12HighPassFilter20Q";
  PD.KernelTable["20Compressor_ProcessFilter_Expander20K"]->OutQ[0] = 
    "20Compressor_ProcessFilter_Expander20LQ";
  PD.KernelTable["20Compressor_ProcessFilter_Expander20K"]->OutQ[1] = 
    "20Compressor_ProcessFilter_Expander20HQ";
  PD.KernelTable["20Compressor_ProcessFilter_Expander20K"]->ROB[0] = "CPE20";

  PD.KernelTable["21Compressor_ProcessFilter_Expander30K"] = 
    new KernelDescriptor("Compressor_ProcessFilter_Expander", false, true); 
  PD.KernelTable["21Compressor_ProcessFilter_Expander30K"]->InQ[0] = "13HighPassFilter30Q";
  PD.KernelTable["21Compressor_ProcessFilter_Expander30K"]->OutQ[0] = 
    "21Compressor_ProcessFilter_Expander30LQ";
  PD.KernelTable["21Compressor_ProcessFilter_Expander30K"]->OutQ[1] = 
    "21Compressor_ProcessFilter_Expander30HQ";
  PD.KernelTable["21Compressor_ProcessFilter_Expander30K"]->ROB[0] = "CPE30";

  PD.KernelTable["22Compressor_ProcessFilter_Expander40K"] = 
    new KernelDescriptor("Compressor_ProcessFilter_Expander", false, true); 
  PD.KernelTable["22Compressor_ProcessFilter_Expander40K"]->InQ[0] = "14HighPassFilter40Q";
  PD.KernelTable["22Compressor_ProcessFilter_Expander40K"]->OutQ[0] = 
    "22Compressor_ProcessFilter_Expander40LQ";
  PD.KernelTable["22Compressor_ProcessFilter_Expander40K"]->OutQ[1] = 
    "22Compressor_ProcessFilter_Expander40HQ";
  PD.KernelTable["22Compressor_ProcessFilter_Expander40K"]->ROB[0] = "CPE40";

  PD.KernelTable["23Compressor_ProcessFilter_Expander50K"] = 
    new KernelDescriptor("Compressor_ProcessFilter_Expander", false, true); 
  PD.KernelTable["23Compressor_ProcessFilter_Expander50K"]->InQ[0] = "15HighPassFilter50Q";
  PD.KernelTable["23Compressor_ProcessFilter_Expander50K"]->OutQ[0] = 
    "23Compressor_ProcessFilter_Expander50LQ";
  PD.KernelTable["23Compressor_ProcessFilter_Expander50K"]->OutQ[1] = 
    "23Compressor_ProcessFilter_Expander50HQ";
  PD.KernelTable["23Compressor_ProcessFilter_Expander50K"]->ROB[0] = "CPE50";

  PD.KernelTable["24Compressor_ProcessFilter_Expander60K"] = 
    new KernelDescriptor("Compressor_ProcessFilter_Expander", false, true); 
  PD.KernelTable["24Compressor_ProcessFilter_Expander60K"]->InQ[0] = "16HighPassFilter60Q";
  PD.KernelTable["24Compressor_ProcessFilter_Expander60K"]->OutQ[0] = 
    "24Compressor_ProcessFilter_Expander60LQ";
  PD.KernelTable["24Compressor_ProcessFilter_Expander60K"]->OutQ[1] = 
    "24Compressor_ProcessFilter_Expander60HQ";
  PD.KernelTable["24Compressor_ProcessFilter_Expander60K"]->ROB[0] = "CPE60";

  PD.KernelTable["25Compressor_ProcessFilter_Expander70K"] = 
    new KernelDescriptor("Compressor_ProcessFilter_Expander", false, true); 
  PD.KernelTable["25Compressor_ProcessFilter_Expander70K"]->InQ[0] = "17HighPassFilter70Q";
  PD.KernelTable["25Compressor_ProcessFilter_Expander70K"]->OutQ[0] = 
    "25Compressor_ProcessFilter_Expander70LQ";
  PD.KernelTable["25Compressor_ProcessFilter_Expander70K"]->OutQ[1] = 
    "25Compressor_ProcessFilter_Expander70HQ";
  PD.KernelTable["25Compressor_ProcessFilter_Expander70K"]->ROB[0] = "CPE70";

  PD.KernelTable["26Compressor_ProcessFilter_Expander80K"] = 
    new KernelDescriptor("Compressor_ProcessFilter_Expander", false, true); 
  PD.KernelTable["26Compressor_ProcessFilter_Expander80K"]->InQ[0] = "18HighPassFilter80Q";
  PD.KernelTable["26Compressor_ProcessFilter_Expander80K"]->OutQ[0] = 
    "26Compressor_ProcessFilter_Expander80LQ";
  PD.KernelTable["26Compressor_ProcessFilter_Expander80K"]->OutQ[1] = 
    "26Compressor_ProcessFilter_Expander80HQ";
  PD.KernelTable["26Compressor_ProcessFilter_Expander80K"]->ROB[0] = "CPE80";

  PD.KernelTable["27LowPassFilter11K"] = new KernelDescriptor("LowPassFilter", false, true); 
  PD.KernelTable["27LowPassFilter11K"]->InQ[0] = "19Compressor_ProcessFilter_Expander10LQ";
  PD.KernelTable["27LowPassFilter11K"]->OutQ[0] = "27LowPassFilter11Q";
  PD.KernelTable["27LowPassFilter11K"]->ROB[0] = "LP11";

  PD.KernelTable["29LowPassFilter21K"] = new KernelDescriptor("LowPassFilter", false, true); 
  PD.KernelTable["29LowPassFilter21K"]->InQ[0] = "20Compressor_ProcessFilter_Expander20LQ";
  PD.KernelTable["29LowPassFilter21K"]->OutQ[0] = "29LowPassFilter21Q";
  PD.KernelTable["29LowPassFilter21K"]->ROB[0] = "LP21";

  PD.KernelTable["31LowPassFilter31K"] = new KernelDescriptor("LowPassFilter", false, true); 
  PD.KernelTable["31LowPassFilter31K"]->InQ[0] = "21Compressor_ProcessFilter_Expander30LQ";
  PD.KernelTable["31LowPassFilter31K"]->OutQ[0] = "31LowPassFilter31Q";
  PD.KernelTable["31LowPassFilter31K"]->ROB[0] = "LP31";

  PD.KernelTable["33LowPassFilter41K"] = new KernelDescriptor("LowPassFilter", false, true); 
  PD.KernelTable["33LowPassFilter41K"]->InQ[0] = "22Compressor_ProcessFilter_Expander40LQ";
  PD.KernelTable["33LowPassFilter41K"]->OutQ[0] = "33LowPassFilter41Q";
  PD.KernelTable["33LowPassFilter41K"]->ROB[0] = "LP41";

  PD.KernelTable["35LowPassFilter51K"] = new KernelDescriptor("LowPassFilter", false, true); 
  PD.KernelTable["35LowPassFilter51K"]->InQ[0] = "23Compressor_ProcessFilter_Expander50LQ";
  PD.KernelTable["35LowPassFilter51K"]->OutQ[0] = "35LowPassFilter51Q";
  PD.KernelTable["35LowPassFilter51K"]->ROB[0] = "LP51";

  PD.KernelTable["37LowPassFilter61K"] = new KernelDescriptor("LowPassFilter", false, true); 
  PD.KernelTable["37LowPassFilter61K"]->InQ[0] = "24Compressor_ProcessFilter_Expander60LQ";
  PD.KernelTable["37LowPassFilter61K"]->OutQ[0] = "37LowPassFilter61Q";
  PD.KernelTable["37LowPassFilter61K"]->ROB[0] = "LP61";

  PD.KernelTable["39LowPassFilter71K"] = new KernelDescriptor("LowPassFilter", false, true); 
  PD.KernelTable["39LowPassFilter71K"]->InQ[0] = "25Compressor_ProcessFilter_Expander70LQ";
  PD.KernelTable["39LowPassFilter71K"]->OutQ[0] = "39LowPassFilter71Q";
  PD.KernelTable["39LowPassFilter71K"]->ROB[0] = "LP71";

  PD.KernelTable["41LowPassFilter81K"] = new KernelDescriptor("LowPassFilter", false, true); 
  PD.KernelTable["41LowPassFilter81K"]->InQ[0] = "26Compressor_ProcessFilter_Expander80LQ";
  PD.KernelTable["41LowPassFilter81K"]->OutQ[0] = "41LowPassFilter81Q";
  PD.KernelTable["41LowPassFilter81K"]->ROB[0] = "LP81";

  PD.KernelTable["28HighPassFilter11K"] = new KernelDescriptor("HighPassFilter", false, true); 
  PD.KernelTable["28HighPassFilter11K"]->InQ[0] = "19Compressor_ProcessFilter_Expander10HQ";
  PD.KernelTable["28HighPassFilter11K"]->OutQ[0] = "28HighPassFilter11Q";
  PD.KernelTable["28HighPassFilter11K"]->ROB[0] = "HP11";

  PD.KernelTable["30HighPassFilter21K"] = new KernelDescriptor("HighPassFilter", false, true); 
  PD.KernelTable["30HighPassFilter21K"]->InQ[0] = "20Compressor_ProcessFilter_Expander20HQ";
  PD.KernelTable["30HighPassFilter21K"]->OutQ[0] = "30HighPassFilter21Q";
  PD.KernelTable["30HighPassFilter21K"]->ROB[0] = "HP21";

  PD.KernelTable["32HighPassFilter31K"] = new KernelDescriptor("HighPassFilter", false, true); 
  PD.KernelTable["32HighPassFilter31K"]->InQ[0] = "21Compressor_ProcessFilter_Expander30HQ";
  PD.KernelTable["32HighPassFilter31K"]->OutQ[0] = "32HighPassFilter31Q";
  PD.KernelTable["32HighPassFilter31K"]->ROB[0] = "HP31";

  PD.KernelTable["34HighPassFilter41K"] = new KernelDescriptor("HighPassFilter", false, true); 
  PD.KernelTable["34HighPassFilter41K"]->InQ[0] = "22Compressor_ProcessFilter_Expander40HQ";
  PD.KernelTable["34HighPassFilter41K"]->OutQ[0] = "34HighPassFilter41Q";
  PD.KernelTable["34HighPassFilter41K"]->ROB[0] = "HP41";

  PD.KernelTable["36HighPassFilter51K"] = new KernelDescriptor("HighPassFilter", false, true); 
  PD.KernelTable["36HighPassFilter51K"]->InQ[0] = "23Compressor_ProcessFilter_Expander50HQ";
  PD.KernelTable["36HighPassFilter51K"]->OutQ[0] = "36HighPassFilter51Q";
  PD.KernelTable["36HighPassFilter51K"]->ROB[0] = "HP51";

  PD.KernelTable["38HighPassFilter61K"] = new KernelDescriptor("HighPassFilter", false, true); 
  PD.KernelTable["38HighPassFilter61K"]->InQ[0] = "24Compressor_ProcessFilter_Expander60HQ";
  PD.KernelTable["38HighPassFilter61K"]->OutQ[0] = "38HighPassFilter61Q";
  PD.KernelTable["38HighPassFilter61K"]->ROB[0] = "HP61";

  PD.KernelTable["40HighPassFilter71K"] = new KernelDescriptor("HighPassFilter", false, true); 
  PD.KernelTable["40HighPassFilter71K"]->InQ[0] = "25Compressor_ProcessFilter_Expander70HQ";
  PD.KernelTable["40HighPassFilter71K"]->OutQ[0] = "40HighPassFilter71Q";
  PD.KernelTable["40HighPassFilter71K"]->ROB[0] = "HP71";

  PD.KernelTable["42HighPassFilter81K"] = new KernelDescriptor("HighPassFilter", false, true); 
  PD.KernelTable["42HighPassFilter81K"]->InQ[0] = "26Compressor_ProcessFilter_Expander80HQ";
  PD.KernelTable["42HighPassFilter81K"]->OutQ[0] = "42HighPassFilter81Q";
  PD.KernelTable["42HighPassFilter81K"]->ROB[0] = "HP81";

  PD.KernelTable["43AddSixteenQueuesK"] = new KernelDescriptor("AddSixteenQueues", false, true); 
  PD.KernelTable["43AddSixteenQueuesK"]->InQ[0] = "27LowPassFilter11Q";
  PD.KernelTable["43AddSixteenQueuesK"]->InQ[1] = "28HighPassFilter11Q";
  PD.KernelTable["43AddSixteenQueuesK"]->InQ[2] = "29LowPassFilter21Q";
  PD.KernelTable["43AddSixteenQueuesK"]->InQ[3] = "30HighPassFilter21Q";
  PD.KernelTable["43AddSixteenQueuesK"]->InQ[4] = "31LowPassFilter31Q";
  PD.KernelTable["43AddSixteenQueuesK"]->InQ[5] = "32HighPassFilter31Q";
  PD.KernelTable["43AddSixteenQueuesK"]->InQ[6] = "33LowPassFilter41Q";
  PD.KernelTable["43AddSixteenQueuesK"]->InQ[7] = "34HighPassFilter41Q";
  PD.KernelTable["43AddSixteenQueuesK"]->InQ[8] = "35LowPassFilter51Q";
  PD.KernelTable["43AddSixteenQueuesK"]->InQ[9] = "36HighPassFilter51Q";
  PD.KernelTable["43AddSixteenQueuesK"]->InQ[10] = "37LowPassFilter61Q";
  PD.KernelTable["43AddSixteenQueuesK"]->InQ[11] = "38HighPassFilter61Q";
  PD.KernelTable["43AddSixteenQueuesK"]->InQ[12] = "39LowPassFilter71Q";
  PD.KernelTable["43AddSixteenQueuesK"]->InQ[13] = "40HighPassFilter71Q";
  PD.KernelTable["43AddSixteenQueuesK"]->InQ[14] = "41LowPassFilter81Q";
  PD.KernelTable["43AddSixteenQueuesK"]->InQ[15] = "42HighPassFilter81Q";
  PD.KernelTable["43AddSixteenQueuesK"]->OutQ[0] = "43AddSixteenQueuesQ";

  PD.KernelTable["44TestSinkK"] = new KernelDescriptor("TestSink", false, false); 
  PD.KernelTable["44TestSinkK"]->InQ[0] = "43AddSixteenQueuesQ";
  PD.KernelTable["44TestSinkK"]->ROB[0] = "I";
  PD.KernelTable["44TestSinkK"]->ROB[1] = "N";
  
  // Generate program graph 
  ProgramVisualizer PV(PD); 
  PV.generateGraph(PD.Name + ".dot"); 

  // Create a program using the descriptor
  ProgramFactory PF; 
  Program* Pgm = PF.newProgram(PD); 
  
  // Create a runtime 
  Runtime* Rtm = RuntimeFactory::newRuntime(Pgm, Cores); 

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
            "DANBI filter bank benchmark\n");
  ::fprintf(stderr, 
            "  Usage: FilterBankRun \n"); 
  ::fprintf(stderr, 
            "      [--repeat repeat_count]\n"); 
  ::fprintf(stderr, 
            "      [--core maximum_number_of_assignable_cores]\n");
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
            "# DANBI filter bank benchmark\n");
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
int parseOption(int argc, char *argv[], int *iterations, int *cores, char **log) {
  struct option options[] = {
    {"repeat", required_argument, 0, 'r'}, 
    {"core",   required_argument, 0, 'c'}, 
    {"log",   required_argument, 0, 'l'}, 
    {0, 0, 0, 0}, 
  }; 

  // Set default values
  *iterations = 20; 
  *cores = 1; 
  *log = const_cast<char *>("FilterBank-Log");

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
  float qfactor = 3.0f / 3.0f;

  // Parse options 
  ret = parseOption(argc, argv, &iterations, &cores, &log);
  if (ret) return 1; 

  // Initialize PMCs
  for (int i = 0; i < PERF_COUNT_HW_MAX; ++i)
    PMCs[i].initialize(i, false, true); 

  // Run benchmark 
  ret = benchmarkFilterBank(cores, iterations, qfactor, log);
  if (ret) return 2; 

  // Read PMCs
  for (int i = 0; i < PERF_COUNT_HW_MAX; ++i)
    PMCs[i].readCounter(HWCounters + i); 

  // Print out results
  showResult(cores, iterations, qfactor); 

  return 0; 
}

