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

  FMRadio.cpp -- Filter Bank 
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

#define PADDING_SIZE (128*5*2)
#define PI 3.14159265f

struct LowPassFilterParams {
  float rate;
  float cutoff;
  int taps;
  int decimation;
  float coeff[128];
}; 

void CalcLowPassFilterParams(LowPassFilterParams* P) {
    int i;
    float m = P->taps - 1;
    float w = 2 * PI * P->cutoff / P->rate;
    for (i = 0; i < P->taps; i++) {
      if (i - m/2 == 0)
        P->coeff[i] = w/PI;
      else
        P->coeff[i] = sin(w*(i-m/2)) / PI / (i-m/2) *
          (0.54 - 0.46 * cos(2*PI*i/m));
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
    RESERVE_PEEK_POP(0, P->taps, 1 + P->decimation); 
    RESERVE_PUSH_TICKET_INQ(0, 0, 1); 

    float sum = 0.0f;
    for (int i = 0; i < P->taps; i++)
      sum += *POP_ADDRESS_AT(0, float, i) * P->coeff[i];
    *PUSH_ADDRESS_AT(0, float, 0) = sum; 

    // Commit input and output data 
    COMMIT_PUSH(0); 
    COMMIT_PEEK_POP(0);

    KEND();
  } END_KERNEL_BODY(); 
}
#include END_OF_MULTIPLE_ITER_OPTIMIZATION

struct FMDemodulatorParams {
  float sampRate;
  float max;
  float bandwidth;
  float mGain;
};

void CalcFMDemodulatorParams(FMDemodulatorParams* P) {
  P->mGain = P->max*(P->sampRate/(P->bandwidth*PI));
}

#include START_OF_MULTIPLE_ITER_OPTIMIZATION
__kernel void FMDemodulatorDupTwelve(void) {
  DECLARE_INPUT_QUEUE(0, float); 
  DECLARE_OUTPUT_QUEUE(0, float);
  DECLARE_OUTPUT_QUEUE(1, float);
  DECLARE_OUTPUT_QUEUE(2, float);
  DECLARE_OUTPUT_QUEUE(3, float);
  DECLARE_OUTPUT_QUEUE(4, float);
  DECLARE_OUTPUT_QUEUE(5, float);
  DECLARE_OUTPUT_QUEUE(6, float);
  DECLARE_OUTPUT_QUEUE(7, float);
  DECLARE_OUTPUT_QUEUE(8, float);
  DECLARE_OUTPUT_QUEUE(9, float);
  DECLARE_OUTPUT_QUEUE(10, float);
  DECLARE_OUTPUT_QUEUE(11, float);
  DECLARE_ROBX(0, FMDemodulatorParams, 1);

  FMDemodulatorParams* P = ROBX(0, FMDemodulatorParams, 0); 
  BEGIN_KERNEL_BODY() {
    KSTART();

    RESERVE_PEEK_POP(0, 2, 1); 
    float temp = *POP_ADDRESS_AT(0, float, 0) * *POP_ADDRESS_AT(0, float, 1); 
    temp = (float)(P->mGain * atan(temp));
    COMMIT_PEEK_POP(0);

    RESERVE_PUSH_TICKET_INQ(0, 0, 1); 
    *PUSH_ADDRESS_AT(0, float, 0) = temp;
    COMMIT_PUSH(0);

    RESERVE_PUSH_TICKET_INQ(1, 0, 1); 
    *PUSH_ADDRESS_AT(1, float, 0) = temp;
    COMMIT_PUSH(1);

    RESERVE_PUSH_TICKET_INQ(2, 0, 1); 
    *PUSH_ADDRESS_AT(2, float, 0) = temp;
    COMMIT_PUSH(2);

    RESERVE_PUSH_TICKET_INQ(3, 0, 1); 
    *PUSH_ADDRESS_AT(3, float, 0) = temp;
    COMMIT_PUSH(3);

    RESERVE_PUSH_TICKET_INQ(4, 0, 1); 
    *PUSH_ADDRESS_AT(4, float, 0) = temp;
    COMMIT_PUSH(4);

    RESERVE_PUSH_TICKET_INQ(5, 0, 1); 
    *PUSH_ADDRESS_AT(5, float, 0) = temp;
    COMMIT_PUSH(5);

    RESERVE_PUSH_TICKET_INQ(6, 0, 1); 
    *PUSH_ADDRESS_AT(6, float, 0) = temp;
    COMMIT_PUSH(6);

    RESERVE_PUSH_TICKET_INQ(7, 0, 1); 
    *PUSH_ADDRESS_AT(7, float, 0) = temp;
    COMMIT_PUSH(7);

    RESERVE_PUSH_TICKET_INQ(8, 0, 1); 
    *PUSH_ADDRESS_AT(8, float, 0) = temp;
    COMMIT_PUSH(8);

    RESERVE_PUSH_TICKET_INQ(9, 0, 1); 
    *PUSH_ADDRESS_AT(9, float, 0) = temp;
    COMMIT_PUSH(9);

    RESERVE_PUSH_TICKET_INQ(10, 0, 1); 
    *PUSH_ADDRESS_AT(10, float, 0) = temp;
    COMMIT_PUSH(10);

    RESERVE_PUSH_TICKET_INQ(11, 0, 1); 
    *PUSH_ADDRESS_AT(11, float, 0) = temp;
    COMMIT_PUSH(11);
    
    KEND();
  } END_KERNEL_BODY(); 
}
#include END_OF_MULTIPLE_ITER_OPTIMIZATION

struct Subtracter_Amplify_AdderParams {
  float gains[7];
}; 

void CalcSubtracter_Amplify_AdderParams(Subtracter_Amplify_AdderParams* P) {
  int eqBands = 7;
  
  // first gain doesn't really correspond to a band
  P->gains[0] = 0.0f;
  for (int i=1; i<eqBands; i++) {
    // the gain grows linearly towards the center bands
    float val = (((float)(i-1))-(((float)(eqBands-2))/2.0f)) / 5.0f;
    P->gains[i] = (val > 0.0f) ? 2.0f-val : 2.0f+val;
  }
}

#include START_OF_MULTIPLE_ITER_OPTIMIZATION
__kernel void Subtracter_Amplify_Adder(void) {
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
  DECLARE_OUTPUT_QUEUE(0, float);
  DECLARE_ROBX(0, Subtracter_Amplify_AdderParams, 1);

  Subtracter_Amplify_AdderParams* P = 
    ROBX(0, Subtracter_Amplify_AdderParams, 0); 
  BEGIN_KERNEL_BODY() {
    KSTART();

    float sum = 0.0f; 

    RESERVE_POP(0, 1); 
    RESERVE_POP_TICKET_INQ(1, 0, 1); 
    sum += (*POP_ADDRESS_AT(1, float, 0) - *POP_ADDRESS_AT(0, float, 0)) * P->gains[1];
    COMMIT_POP(0);
    COMMIT_POP(1);

    RESERVE_POP_TICKET_INQ(2, 0, 1); 
    RESERVE_POP_TICKET_INQ(3, 0, 1); 
    sum += (*POP_ADDRESS_AT(3, float, 0) - *POP_ADDRESS_AT(2, float, 0)) * P->gains[2];
    COMMIT_POP(2);
    COMMIT_POP(3);

    RESERVE_POP_TICKET_INQ(4, 0, 1); 
    RESERVE_POP_TICKET_INQ(5, 0, 1); 
    sum += (*POP_ADDRESS_AT(5, float, 0) - *POP_ADDRESS_AT(4, float, 0)) * P->gains[3];
    COMMIT_POP(4);
    COMMIT_POP(5);

    RESERVE_POP_TICKET_INQ(6, 0, 1); 
    RESERVE_POP_TICKET_INQ(7, 0, 1); 
    sum += (*POP_ADDRESS_AT(7, float, 0) - *POP_ADDRESS_AT(6, float, 0)) * P->gains[4];
    COMMIT_POP(6);
    COMMIT_POP(7);

    RESERVE_POP_TICKET_INQ(8, 0, 1); 
    RESERVE_POP_TICKET_INQ(9, 0, 1); 
    sum += (*POP_ADDRESS_AT(9, float, 0) - *POP_ADDRESS_AT(8, float, 0)) * P->gains[5];
    COMMIT_POP(8);
    COMMIT_POP(9);

    RESERVE_POP_TICKET_INQ(10, 0, 1); 
    RESERVE_POP_TICKET_INQ(11, 0, 1); 
    sum += (*POP_ADDRESS_AT(11, float, 0) - *POP_ADDRESS_AT(11, float, 0)) * P->gains[6];
    COMMIT_POP(10);
    COMMIT_POP(11);

    RESERVE_PUSH_TICKET_INQ(0, 0, 1); 
    *PUSH_ADDRESS_AT(0, float, 0) = sum; 
    COMMIT_PUSH(0); 

    KEND();
  } END_KERNEL_BODY(); 
}
#include END_OF_MULTIPLE_ITER_OPTIMIZATION

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
int benchmarkFMRadio(int Cores, int Iter, float QFactor, char* Log) {
  /// Benchmark pipeline 
  // [01TestSource]--[[02LowPassFilter]]--
  //   [[03FMDemodulatorDupTwelve]]--
  //     [[04LowPassFilter01]]--
  //     [[05LowPassFilter02]]--
  //     [[06LowPassFilter03]]--
  //     [[07LowPassFilter04]]--
  //     [[08LowPassFilter05]]--
  //     [[09LowPassFilter06]]--
  //     [[10LowPassFilter07]]--
  //     [[11LowPassFilter08]]--
  //     [[12LowPassFilter09]]--
  //     [[13LowPassFilter10]]--
  //     [[14LowPassFilter11]]--
  //     [[15LowPassFilter12]]--
  //   [[16Subtracter_Amplify_Adder]]--
  // --[17TestSink]
  ProgramDescriptor PD("FMRadio"); 

  // Code
  PD.CodeTable["TestSource"] = new CodeDescriptor(TestSource); 
  PD.CodeTable["LowPassFilter"] = new CodeDescriptor(LowPassFilter); 
  PD.CodeTable["FMDemodulatorDupTwelve"] = new CodeDescriptor(FMDemodulatorDupTwelve); 
  PD.CodeTable["Subtracter_Amplify_Adder"] = new CodeDescriptor(Subtracter_Amplify_Adder); 
  PD.CodeTable["TestSink"] = new CodeDescriptor(TestSink); 

  // ROB
  int I_[1] = {1};
  PD.ROBTable["I"] = new ReadOnlyBufferDescriptor(I_, sizeof(int), 1);

  int N = Iter * 1280000; // For now, N should be multiple of 128. 
  int N_[1] = {N};
  PD.ROBTable["N"] = new ReadOnlyBufferDescriptor(N_, sizeof(int), 1);

  int eqBands = 7;
  float eqCutoff[eqBands];
  LowPassFilterParams LP0_[13];
  for (int i=0; i<eqBands; i++) {
    float low = 55.0f;
    float high = 1760.0f;
    // have exponentially spaced cutoffs
    eqCutoff[i] = (float)exp(i*(log(high)-log(low))/(eqBands-1) + log(low));
  }
 for (int i = 0; i < 13; ++i) {
    LP0_[i].rate = 250000000; // 250 MHz sampling rate is sensible
    LP0_[i].cutoff = (i == 0) ? 108000000.0f : eqCutoff[i/2];
    LP0_[i].taps = 128; 
    LP0_[i].decimation = (i == 0) ? 4.0f : 0.0f; 
    CalcLowPassFilterParams(LP0_ + i);
  }
  PD.ROBTable["LP02"] = new ReadOnlyBufferDescriptor(LP0_ + 0, sizeof(LowPassFilterParams), 1);
  PD.ROBTable["LP04"] = new ReadOnlyBufferDescriptor(LP0_ + 1, sizeof(LowPassFilterParams), 1);
  PD.ROBTable["LP05"] = new ReadOnlyBufferDescriptor(LP0_ + 2, sizeof(LowPassFilterParams), 1);
  PD.ROBTable["LP06"] = new ReadOnlyBufferDescriptor(LP0_ + 3, sizeof(LowPassFilterParams), 1);
  PD.ROBTable["LP07"] = new ReadOnlyBufferDescriptor(LP0_ + 4, sizeof(LowPassFilterParams), 1);
  PD.ROBTable["LP08"] = new ReadOnlyBufferDescriptor(LP0_ + 5, sizeof(LowPassFilterParams), 1);
  PD.ROBTable["LP09"] = new ReadOnlyBufferDescriptor(LP0_ + 6, sizeof(LowPassFilterParams), 1);
  PD.ROBTable["LP10"] = new ReadOnlyBufferDescriptor(LP0_ + 7, sizeof(LowPassFilterParams), 1);
  PD.ROBTable["LP11"] = new ReadOnlyBufferDescriptor(LP0_ + 8, sizeof(LowPassFilterParams), 1);
  PD.ROBTable["LP12"] = new ReadOnlyBufferDescriptor(LP0_ + 9, sizeof(LowPassFilterParams), 1);
  PD.ROBTable["LP13"] = new ReadOnlyBufferDescriptor(LP0_ + 10, sizeof(LowPassFilterParams), 1);
  PD.ROBTable["LP14"] = new ReadOnlyBufferDescriptor(LP0_ + 11, sizeof(LowPassFilterParams), 1);
  PD.ROBTable["LP15"] = new ReadOnlyBufferDescriptor(LP0_ + 12, sizeof(LowPassFilterParams), 1);

  FMDemodulatorParams FMP_[1]; 
  FMP_[0].sampRate = 250000000.0f; 
  FMP_[0].max = 27000.0f; 
  FMP_[0].bandwidth = 10000.0f;
  CalcFMDemodulatorParams(FMP_);
  PD.ROBTable["FMP"] = new ReadOnlyBufferDescriptor(FMP_, sizeof(FMDemodulatorParams), 1);

  Subtracter_Amplify_AdderParams SAAP_[1];
  CalcSubtracter_Amplify_AdderParams(SAAP_);
  PD.ROBTable["SAAP"] = new ReadOnlyBufferDescriptor(SAAP_, 
                                                     sizeof(Subtracter_Amplify_AdderParams), 1);

  // Queue
  const int RootQSize = N + PADDING_SIZE;
  const int QSize = float(std::min(DANBI_MULTIPLE_ITER_COUNT * Cores * 2 + PADDING_SIZE, 
                                   RootQSize)) * QFactor;
  PD.QTable["01TestSourceQ"] = new QueueDescriptor(RootQSize, sizeof(float), false, 
                                                   "01TestSourceK", 
                                                   "02LowPassFilterK", 
                                                   false, false, true, false);
  PD.QTable["02LowPassFilterQ"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                   "02LowPassFilterK", 
                                                   "03FMDemodulatorDupTwelveK", 
                                                   false, true, true, false);
  PD.QTable["03FMDemodulatorDupTwelve01Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                   "03FMDemodulatorDupTwelveK", 
                                                   "04LowPassFilter01K", 
                                                   false, true, true, false);
  PD.QTable["04LowPassFilter01Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                        "04LowPassFilter01K", 
                                                        "16Subtracter_Amplify_AdderK", 
                                                        false, true, true, false);
  PD.QTable["03FMDemodulatorDupTwelve02Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                                 "03FMDemodulatorDupTwelveK", 
                                                                 "05LowPassFilter02K", 
                                                                 false, true, true, false);
  PD.QTable["05LowPassFilter02Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                        "05LowPassFilter02K", 
                                                        "16Subtracter_Amplify_AdderK", 
                                                        false, true, false, true);
  PD.QTable["03FMDemodulatorDupTwelve03Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                                 "03FMDemodulatorDupTwelveK", 
                                                                 "06LowPassFilter03K", 
                                                                 false, true, true, false);
  PD.QTable["06LowPassFilter03Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                        "06LowPassFilter03K", 
                                                        "16Subtracter_Amplify_AdderK", 
                                                        false, true, false, true);
  PD.QTable["03FMDemodulatorDupTwelve04Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                                 "03FMDemodulatorDupTwelveK", 
                                                                 "07LowPassFilter04K", 
                                                                 false, true, true, false);
  PD.QTable["07LowPassFilter04Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                        "07LowPassFilter04K", 
                                                        "16Subtracter_Amplify_AdderK", 
                                                        false, true, false, true);
  PD.QTable["03FMDemodulatorDupTwelve05Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                                 "03FMDemodulatorDupTwelveK", 
                                                                 "08LowPassFilter05K", 
                                                                 false, true, true, false);
  PD.QTable["08LowPassFilter05Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                        "08LowPassFilter05K", 
                                                        "16Subtracter_Amplify_AdderK", 
                                                        false, true, false, true);
  PD.QTable["03FMDemodulatorDupTwelve06Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                                 "03FMDemodulatorDupTwelveK", 
                                                                 "09LowPassFilter06K", 
                                                                 false, true, true, false);
  PD.QTable["09LowPassFilter06Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                        "09LowPassFilter06K", 
                                                        "16Subtracter_Amplify_AdderK", 
                                                        false, true, false, true);
  PD.QTable["03FMDemodulatorDupTwelve07Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                                 "03FMDemodulatorDupTwelveK", 
                                                                 "10LowPassFilter07K", 
                                                                 false, true, true, false);
  PD.QTable["10LowPassFilter07Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                        "10LowPassFilter07K", 
                                                        "16Subtracter_Amplify_AdderK", 
                                                        false, true, false, true);
  PD.QTable["03FMDemodulatorDupTwelve08Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                                 "03FMDemodulatorDupTwelveK", 
                                                                 "11LowPassFilter08K", 
                                                                 false, true, true, false);
  PD.QTable["11LowPassFilter08Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                        "11LowPassFilter08K", 
                                                        "16Subtracter_Amplify_AdderK", 
                                                        false, true, false, true);
  PD.QTable["03FMDemodulatorDupTwelve09Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                                 "03FMDemodulatorDupTwelveK", 
                                                                 "12LowPassFilter09K", 
                                                                 false, true, true, false);
  PD.QTable["12LowPassFilter09Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                        "12LowPassFilter09K", 
                                                        "16Subtracter_Amplify_AdderK", 
                                                        false, true, false, true);
  PD.QTable["03FMDemodulatorDupTwelve10Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                                 "03FMDemodulatorDupTwelveK", 
                                                                 "13LowPassFilter10K", 
                                                                 false, true, true, false);
  PD.QTable["13LowPassFilter10Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                        "13LowPassFilter10K", 
                                                        "16Subtracter_Amplify_AdderK", 
                                                        false, true, false, true);
  PD.QTable["03FMDemodulatorDupTwelve11Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                                 "03FMDemodulatorDupTwelveK", 
                                                                 "14LowPassFilter11K", 
                                                                 false, true, true, false);
  PD.QTable["14LowPassFilter11Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                        "14LowPassFilter11K", 
                                                        "16Subtracter_Amplify_AdderK", 
                                                        false, true, false, true);
  PD.QTable["03FMDemodulatorDupTwelve12Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                                 "03FMDemodulatorDupTwelveK", 
                                                                 "15LowPassFilter12K", 
                                                                 false, true, true, false);
  PD.QTable["15LowPassFilter12Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                        "15LowPassFilter12K", 
                                                        "16Subtracter_Amplify_AdderK", 
                                                        false, true, false, true);
  PD.QTable["16Subtracter_Amplify_AdderQ"] = new QueueDescriptor(RootQSize, sizeof(float), false, 
                                                                 "16Subtracter_Amplify_AdderK", 
                                                                 "17TestSinkK",
                                                                 false, true, false, false);
  
  // Kernel 
  PD.KernelTable["01TestSourceK"] = new KernelDescriptor("TestSource", true, false); 
  PD.KernelTable["01TestSourceK"]->OutQ[0] = "01TestSourceQ";
  PD.KernelTable["01TestSourceK"]->ROB[0] = "I";
  PD.KernelTable["01TestSourceK"]->ROB[1] = "N";

  PD.KernelTable["02LowPassFilterK"] = new KernelDescriptor("LowPassFilter", false, true); 
  PD.KernelTable["02LowPassFilterK"]->InQ[0] = "01TestSourceQ";
  PD.KernelTable["02LowPassFilterK"]->OutQ[0] = "02LowPassFilterQ";
  PD.KernelTable["02LowPassFilterK"]->ROB[0] = "LP02";

  PD.KernelTable["03FMDemodulatorDupTwelveK"] = new KernelDescriptor("FMDemodulatorDupTwelve", false, true); 
  PD.KernelTable["03FMDemodulatorDupTwelveK"]->InQ[0] = "02LowPassFilterQ";
  PD.KernelTable["03FMDemodulatorDupTwelveK"]->OutQ[0] = "03FMDemodulatorDupTwelve01Q";
  PD.KernelTable["03FMDemodulatorDupTwelveK"]->OutQ[1] = "03FMDemodulatorDupTwelve02Q";
  PD.KernelTable["03FMDemodulatorDupTwelveK"]->OutQ[2] = "03FMDemodulatorDupTwelve03Q";
  PD.KernelTable["03FMDemodulatorDupTwelveK"]->OutQ[3] = "03FMDemodulatorDupTwelve04Q";
  PD.KernelTable["03FMDemodulatorDupTwelveK"]->OutQ[4] = "03FMDemodulatorDupTwelve05Q";
  PD.KernelTable["03FMDemodulatorDupTwelveK"]->OutQ[5] = "03FMDemodulatorDupTwelve06Q";
  PD.KernelTable["03FMDemodulatorDupTwelveK"]->OutQ[6] = "03FMDemodulatorDupTwelve07Q";
  PD.KernelTable["03FMDemodulatorDupTwelveK"]->OutQ[7] = "03FMDemodulatorDupTwelve08Q";
  PD.KernelTable["03FMDemodulatorDupTwelveK"]->OutQ[8] = "03FMDemodulatorDupTwelve09Q";
  PD.KernelTable["03FMDemodulatorDupTwelveK"]->OutQ[9] = "03FMDemodulatorDupTwelve10Q";
  PD.KernelTable["03FMDemodulatorDupTwelveK"]->OutQ[10] = "03FMDemodulatorDupTwelve11Q";
  PD.KernelTable["03FMDemodulatorDupTwelveK"]->OutQ[11] = "03FMDemodulatorDupTwelve12Q";
  PD.KernelTable["03FMDemodulatorDupTwelveK"]->ROB[0] = "FMP";

  PD.KernelTable["04LowPassFilter01K"] = new KernelDescriptor("LowPassFilter", false, true); 
  PD.KernelTable["04LowPassFilter01K"]->InQ[0] = "03FMDemodulatorDupTwelve01Q";
  PD.KernelTable["04LowPassFilter01K"]->OutQ[0] = "04LowPassFilter01Q";
  PD.KernelTable["04LowPassFilter01K"]->ROB[0] = "LP04";

  PD.KernelTable["05LowPassFilter02K"] = new KernelDescriptor("LowPassFilter", false, true); 
  PD.KernelTable["05LowPassFilter02K"]->InQ[0] = "03FMDemodulatorDupTwelve02Q";
  PD.KernelTable["05LowPassFilter02K"]->OutQ[0] = "05LowPassFilter02Q";
  PD.KernelTable["05LowPassFilter02K"]->ROB[0] = "LP05";

  PD.KernelTable["06LowPassFilter03K"] = new KernelDescriptor("LowPassFilter", false, true); 
  PD.KernelTable["06LowPassFilter03K"]->InQ[0] = "03FMDemodulatorDupTwelve03Q";
  PD.KernelTable["06LowPassFilter03K"]->OutQ[0] = "06LowPassFilter03Q";
  PD.KernelTable["06LowPassFilter03K"]->ROB[0] = "LP06";

  PD.KernelTable["07LowPassFilter04K"] = new KernelDescriptor("LowPassFilter", false, true); 
  PD.KernelTable["07LowPassFilter04K"]->InQ[0] = "03FMDemodulatorDupTwelve04Q";
  PD.KernelTable["07LowPassFilter04K"]->OutQ[0] = "07LowPassFilter04Q";
  PD.KernelTable["07LowPassFilter04K"]->ROB[0] = "LP07";

  PD.KernelTable["08LowPassFilter05K"] = new KernelDescriptor("LowPassFilter", false, true); 
  PD.KernelTable["08LowPassFilter05K"]->InQ[0] = "03FMDemodulatorDupTwelve05Q";
  PD.KernelTable["08LowPassFilter05K"]->OutQ[0] = "08LowPassFilter05Q";
  PD.KernelTable["08LowPassFilter05K"]->ROB[0] = "LP08";

  PD.KernelTable["09LowPassFilter06K"] = new KernelDescriptor("LowPassFilter", false, true); 
  PD.KernelTable["09LowPassFilter06K"]->InQ[0] = "03FMDemodulatorDupTwelve06Q";
  PD.KernelTable["09LowPassFilter06K"]->OutQ[0] = "09LowPassFilter06Q";
  PD.KernelTable["09LowPassFilter06K"]->ROB[0] = "LP09";

  PD.KernelTable["10LowPassFilter07K"] = new KernelDescriptor("LowPassFilter", false, true); 
  PD.KernelTable["10LowPassFilter07K"]->InQ[0] = "03FMDemodulatorDupTwelve07Q";
  PD.KernelTable["10LowPassFilter07K"]->OutQ[0] = "10LowPassFilter07Q";
  PD.KernelTable["10LowPassFilter07K"]->ROB[0] = "LP10";

  PD.KernelTable["11LowPassFilter08K"] = new KernelDescriptor("LowPassFilter", false, true); 
  PD.KernelTable["11LowPassFilter08K"]->InQ[0] = "03FMDemodulatorDupTwelve08Q";
  PD.KernelTable["11LowPassFilter08K"]->OutQ[0] = "11LowPassFilter08Q";
  PD.KernelTable["11LowPassFilter08K"]->ROB[0] = "LP11";

  PD.KernelTable["12LowPassFilter09K"] = new KernelDescriptor("LowPassFilter", false, true); 
  PD.KernelTable["12LowPassFilter09K"]->InQ[0] = "03FMDemodulatorDupTwelve09Q";
  PD.KernelTable["12LowPassFilter09K"]->OutQ[0] = "12LowPassFilter09Q";
  PD.KernelTable["12LowPassFilter09K"]->ROB[0] = "LP12";

  PD.KernelTable["13LowPassFilter10K"] = new KernelDescriptor("LowPassFilter", false, true); 
  PD.KernelTable["13LowPassFilter10K"]->InQ[0] = "03FMDemodulatorDupTwelve10Q";
  PD.KernelTable["13LowPassFilter10K"]->OutQ[0] = "13LowPassFilter10Q";
  PD.KernelTable["13LowPassFilter10K"]->ROB[0] = "LP13";

  PD.KernelTable["14LowPassFilter11K"] = new KernelDescriptor("LowPassFilter", false, true); 
  PD.KernelTable["14LowPassFilter11K"]->InQ[0] = "03FMDemodulatorDupTwelve11Q";
  PD.KernelTable["14LowPassFilter11K"]->OutQ[0] = "14LowPassFilter11Q";
  PD.KernelTable["14LowPassFilter11K"]->ROB[0] = "LP14";

  PD.KernelTable["15LowPassFilter12K"] = new KernelDescriptor("LowPassFilter", false, true); 
  PD.KernelTable["15LowPassFilter12K"]->InQ[0] = "03FMDemodulatorDupTwelve12Q";
  PD.KernelTable["15LowPassFilter12K"]->OutQ[0] = "15LowPassFilter12Q";
  PD.KernelTable["15LowPassFilter12K"]->ROB[0] = "LP15";

  PD.KernelTable["16Subtracter_Amplify_AdderK"] = new KernelDescriptor("Subtracter_Amplify_Adder", false, true); 
  PD.KernelTable["16Subtracter_Amplify_AdderK"]->InQ[0] = "04LowPassFilter01Q";
  PD.KernelTable["16Subtracter_Amplify_AdderK"]->InQ[1] = "05LowPassFilter02Q";
  PD.KernelTable["16Subtracter_Amplify_AdderK"]->InQ[2] = "06LowPassFilter03Q";
  PD.KernelTable["16Subtracter_Amplify_AdderK"]->InQ[3] = "07LowPassFilter04Q";
  PD.KernelTable["16Subtracter_Amplify_AdderK"]->InQ[4] = "08LowPassFilter05Q";
  PD.KernelTable["16Subtracter_Amplify_AdderK"]->InQ[5] = "09LowPassFilter06Q";
  PD.KernelTable["16Subtracter_Amplify_AdderK"]->InQ[6] = "10LowPassFilter07Q";
  PD.KernelTable["16Subtracter_Amplify_AdderK"]->InQ[7] = "11LowPassFilter08Q";
  PD.KernelTable["16Subtracter_Amplify_AdderK"]->InQ[8] = "12LowPassFilter09Q";
  PD.KernelTable["16Subtracter_Amplify_AdderK"]->InQ[9] = "13LowPassFilter10Q";
  PD.KernelTable["16Subtracter_Amplify_AdderK"]->InQ[10] = "14LowPassFilter11Q";
  PD.KernelTable["16Subtracter_Amplify_AdderK"]->InQ[11] = "15LowPassFilter12Q";
  PD.KernelTable["16Subtracter_Amplify_AdderK"]->OutQ[0] = "16Subtracter_Amplify_AdderQ";
  PD.KernelTable["16Subtracter_Amplify_AdderK"]->ROB[0] = "SAAP";

  PD.KernelTable["17TestSinkK"] = new KernelDescriptor("TestSink", false, false); 
  PD.KernelTable["17TestSinkK"]->InQ[0] = "16Subtracter_Amplify_AdderQ";
  PD.KernelTable["17TestSinkK"]->ROB[0] = "I";
  PD.KernelTable["17TestSinkK"]->ROB[1] = "N";
  
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
            "DANBI FM radio benchmark\n");
  ::fprintf(stderr, 
            "  Usage: FMRadioRun \n"); 
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
            "# DANBI FM radio benchmark\n");
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
  *iterations = 200; 
  *cores = 1; 
  *log = const_cast<char *>("FMRadio-Log");

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
  ret = benchmarkFMRadio(cores, iterations, qfactor, log);
  if (ret) return 2; 

  // Read PMCs
  for (int i = 0; i < PERF_COUNT_HW_MAX; ++i)
    PMCs[i].readCounter(HWCounters + i); 

  // Print out results
  showResult(cores, iterations, qfactor); 

  return 0; 
}

