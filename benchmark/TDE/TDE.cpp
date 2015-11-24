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

  TDE.cpp -- TDE
  Implementation is based on the code from StreamIt benchmark. 
 */
#include <unistd.h>
#include <getopt.h>
#include <algorithm>
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

/// Contract
#include START_OF_MULTIPLE_ITER_OPTIMIZATION
__kernel void Contract(void) {
  DECLARE_INPUT_QUEUE(0, float); 
  DECLARE_OUTPUT_QUEUE(0, float);
  DECLARE_ROBX(0, int, 1); // N
  DECLARE_ROBX(1, int, 1); // B

  // Get parameters
  int N = *ROBX(0, int, 0); 
  int B = *ROBX(1, int, 0); 
  int PopN = 2*N + (2*B - 2*N); 
  int PushN = 2*N; 
  BEGIN_KERNEL_BODY() {
    KSTART(); 

    RESERVE_POP(0, PopN); 
    RESERVE_PUSH_TICKET_INQ(0, 0, PushN); 

    COPY_QUEUE(0, float, 0, PushN, 0, 0);

    COMMIT_PUSH(0); 
    COMMIT_POP(0);

    KEND();
  } END_KERNEL_BODY(); 
}
#include END_OF_MULTIPLE_ITER_OPTIMIZATION

/// Expand
#include START_OF_MULTIPLE_ITER_OPTIMIZATION
__kernel void Expand(void) {
  DECLARE_INPUT_QUEUE(0, float); 
  DECLARE_OUTPUT_QUEUE(0, float);
  DECLARE_ROBX(0, int, 1); // N
  DECLARE_ROBX(1, int, 1); // B

  // Get parameters
  int N = *ROBX(0, int, 0); 
  int B = *ROBX(1, int, 0); 
  int PopN = 2*N; 
  int PushN = 2*N + (2*B - 2*N); 
  BEGIN_KERNEL_BODY() {
    KSTART(); 

    RESERVE_POP(0, PopN); 
    RESERVE_PUSH_TICKET_INQ(0, 0, PushN); 

    int o = PopN; 
    COPY_QUEUE(0, float, 0, PopN, 0, 0);
    for (int i = 2*N; i < 2*B; i++) 
      *PUSH_ADDRESS_AT(0, float, o++) = 0.0f;

    COMMIT_PUSH(0); 
    COMMIT_POP(0);

    KEND();
  } END_KERNEL_BODY(); 
}
#include END_OF_MULTIPLE_ITER_OPTIMIZATION

/// Simplified multiply by a constant.
#include START_OF_MULTIPLE_ITER_OPTIMIZATION
__kernel void Multiply_by_float(void) {
  DECLARE_INPUT_QUEUE(0, float); 
  DECLARE_OUTPUT_QUEUE(0, float);
  DECLARE_ROBX(0, int, 1); // B
  DECLARE_ROBX(1, float, 1); // m

  // Get parameters
  int B = *ROBX(0, int, 0); 
  float m = *ROBX(1, float, 0); 
  int PopN = 2*B; 
  int PushN = 2*B;
  BEGIN_KERNEL_BODY() {
    KSTART(); 

    RESERVE_POP(0, PopN); 
    RESERVE_PUSH_TICKET_INQ(0, 0, PushN); 

    int o = 0; 
    for (int j = 0; j < B; j++) {
      *PUSH_ADDRESS_AT(0, float, o) = *POP_ADDRESS_AT(0, float, o) * m; 
      ++o;
      *PUSH_ADDRESS_AT(0, float, o) = *POP_ADDRESS_AT(0, float, o) * m; 
      ++o;
    }

    COMMIT_PUSH(0); 
    COMMIT_POP(0);

    KEND();
  } END_KERNEL_BODY(); 
}
#include END_OF_MULTIPLE_ITER_OPTIMIZATION

/// Transpose written as a filter.
#include START_OF_MULTIPLE_ITER_OPTIMIZATION
__kernel void Transpose(void) {
  DECLARE_INPUT_QUEUE(0, float); 
  DECLARE_OUTPUT_QUEUE(0, float);
  DECLARE_ROBX(0, int, 1); // M
  DECLARE_ROBX(1, int, 1); // N

  // Get parameters
  int M = *ROBX(0, int, 0); 
  int N = *ROBX(1, int, 0); 
  int totalData = M * N * 2; 
  BEGIN_KERNEL_BODY() {
    KSTART(); 

    // Reserve input and output data 
    RESERVE_POP(0, totalData); 
    RESERVE_PUSH_TICKET_INQ(0, 0, totalData); 

    // Transpose
    int o = 0; 
    for(int i=0; i<M; i++) {
      for(int j=0; j<N; j++) {
        *PUSH_ADDRESS_AT(0, float, o++) 
          = *POP_ADDRESS_AT(0, float, i*N*2+j*2); 
        *PUSH_ADDRESS_AT(0, float, o++) 
          = *POP_ADDRESS_AT(0, float, i*N*2+j*2+1); 
      }
    }

    // Commit input and output data 
    COMMIT_PUSH(0); 
    COMMIT_POP(0);

    KEND();
  } END_KERNEL_BODY(); 
}
#include END_OF_MULTIPLE_ITER_OPTIMIZATION

struct DFTParams {
  float wn_r;
  float wn_i;
  float real;
  float imag;
  float next_real, next_imag;
};

static void initializeDFTParams(int n, DFTParams* DP, float* w) {
  DP->wn_r = (float)cos(2 * 3.141592654 / n);
  DP->wn_i = (float)sin(-2 * 3.141592654 / n);
  DP->real = 1;
  DP->imag = 0;
  for (int i=0; i<n; i+=2) {
    w[i] = DP->real;
    w[i+1] = DP->imag;
    DP->next_real = DP->real * DP->wn_r - DP->imag * DP->wn_i;
    DP->next_imag = DP->real * DP->wn_i + DP->imag * DP->wn_r;
    DP->real = DP->next_real;
    DP->imag = DP->next_imag;
  }
}

/// CombineDFT
#include START_OF_MULTIPLE_ITER_OPTIMIZATION
__kernel void CombineDFT(void) {
  DECLARE_INPUT_QUEUE(0, float); 
  DECLARE_OUTPUT_QUEUE(0, float);
  DECLARE_ROBX(0, int, 1); // n 
  DECLARE_ROBX(1, DFTParams, 1); // DP  
  DECLARE_ROBX(2, float, *ROBX(0, int, 0)); // w

  // Get parameters
  int n = *ROBX(0, int, 0); 
  int totalData = 2 * n; 
  DFTParams* DP = ROBX(1, DFTParams, 0); 
  float* w = ROBX(2, float, 0);
  BEGIN_KERNEL_BODY() {
    KSTART(); 

    // Reserve input and output data 
    RESERVE_POP(0, totalData); 
    RESERVE_PUSH_TICKET_INQ(0, 0, totalData); 

    // CombineDFT
    for (int i = 0; i < n; i += 2) {
      int i_plus_1 = i+1;

      float y0_r = *POP_ADDRESS_AT(0, float, i);
      float y0_i = *POP_ADDRESS_AT(0, float, i_plus_1);
            
      float y1_r = *POP_ADDRESS_AT(0, float, n + i);
      float y1_i = *POP_ADDRESS_AT(0, float, n + i_plus_1);

      // load into temps to make sure it doesn't got loaded
      // separately for each load
      float weight_real = w[i];
      float weight_imag = w[i_plus_1];

      float y1w_r = y1_r * weight_real - y1_i * weight_imag;
      float y1w_i = y1_r * weight_imag + y1_i * weight_real;

      *PUSH_ADDRESS_AT(0, float, i) = y0_r + y1w_r;
      *PUSH_ADDRESS_AT(0, float, i + 1) = y0_i + y1w_i;

      *PUSH_ADDRESS_AT(0, float, n + i) = y0_r - y1w_r;
      *PUSH_ADDRESS_AT(0, float, n + i + 1) = y0_i - y1w_i;
    }

    // Commit input and output data 
    COMMIT_PUSH(0); 
    COMMIT_POP(0);

    KEND();
  } END_KERNEL_BODY(); 
}
#include END_OF_MULTIPLE_ITER_OPTIMIZATION

/// CombineIDFT
#include START_OF_MULTIPLE_ITER_OPTIMIZATION
__kernel void CombineIDFT(void) {
  DECLARE_INPUT_QUEUE(0, float); 
  DECLARE_OUTPUT_QUEUE(0, float);
  DECLARE_ROBX(0, int, 1); // n 
  DECLARE_ROBX(1, DFTParams, 1); // DP  
  DECLARE_ROBX(2, float, *ROBX(0, int, 0)); // w

  // Get parameters
  int n = *ROBX(0, int, 0); 
  int totalData = 2 * n; 
  DFTParams* DP = ROBX(1, DFTParams, 0); 
  float* w = ROBX(2, float, 0);
  BEGIN_KERNEL_BODY() {
    KSTART(); 

    float results[totalData]; 

    // Reserve input and output data 
    RESERVE_POP(0, totalData); 
    RESERVE_PUSH_TICKET_INQ(0, 0, totalData); 

    // CombineIDFT
    for (int i = 0; i < n; i += 2) {
      int i_plus_1 = i+1;

      float y0_r = *POP_ADDRESS_AT(0, float, i);
      float y0_i = *POP_ADDRESS_AT(0, float, i_plus_1);
      
      float y1_r = *POP_ADDRESS_AT(0, float, n + i);
      float y1_i = *POP_ADDRESS_AT(0, float, n + i_plus_1);
      
      float weight_real = w[i];
      float weight_imag = w[i_plus_1];

      float y1w_r = y1_r * weight_real - y1_i * weight_imag;
      float y1w_i = y1_r * weight_imag + y1_i * weight_real;

      *PUSH_ADDRESS_AT(0, float, i) = y0_r + y1w_r;
      *PUSH_ADDRESS_AT(0, float, i + 1) = y0_i + y1w_i;

      *PUSH_ADDRESS_AT(0, float, n + i) = y0_r - y1w_r;
      *PUSH_ADDRESS_AT(0, float, n + i + 1) = y0_i - y1w_i;
    }

    // Commit input and output data 
    COMMIT_PUSH(0); 
    COMMIT_POP(0);

    KEND();
  } END_KERNEL_BODY(); 
}
#include END_OF_MULTIPLE_ITER_OPTIMIZATION

/// CombineIDFTFinal 
struct IDFTFinalParams {
  float wn_r;
  float wn_i;
  float n_recip;
  float real;
  float imag;
  float next_real, next_imag;
};

static void initializeIDFTFinalParams(int n, IDFTFinalParams* DP, float* w) {
  DP->wn_r = (float)cos(2 * 3.141592654 / n);
  DP->wn_i = (float)sin(2 * 3.141592654 / n);
  DP->n_recip = 1.0/((float)n);
  DP->real = DP->n_recip;
  DP->imag = 0;
  for (int i=0; i<n; i+=2) {
    w[i] = DP->real;
    w[i+1] = DP->imag;
    DP->next_real = DP->real * DP->wn_r - DP->imag * DP->wn_i;
    DP->next_imag = DP->real * DP->wn_i + DP->imag * DP->wn_r;
    DP->real = DP->next_real;
    DP->imag = DP->next_imag;
  }
}

#include START_OF_MULTIPLE_ITER_OPTIMIZATION
__kernel void CombineIDFTFinal(void) {
  DECLARE_INPUT_QUEUE(0, float); 
  DECLARE_OUTPUT_QUEUE(0, float);
  DECLARE_ROBX(0, int, 1); // n 
  DECLARE_ROBX(1, IDFTFinalParams, 1); // DP  
  DECLARE_ROBX(2, float, *ROBX(0, int, 0)); // w

  // Get parameters
  int n = *ROBX(0, int, 0); 
  int totalData = 2 * n; 
  IDFTFinalParams* DP = ROBX(1, IDFTFinalParams, 0); 
  float* w = ROBX(2, float, 0);
  BEGIN_KERNEL_BODY() {
    KSTART(); 

    float results[totalData]; 

    // Reserve input and output data 
    RESERVE_POP(0, totalData); 
    RESERVE_PUSH_TICKET_INQ(0, 0, totalData); 

    // CombineIDFT
    for (int i = 0; i < n; i += 2) {
      int i_plus_1 = i+1;

      float y0_r = DP->n_recip * *POP_ADDRESS_AT(0, float, i);
      float y0_i = DP->n_recip * *POP_ADDRESS_AT(0, float, i_plus_1);
      
      float y1_r = *POP_ADDRESS_AT(0, float, n + i);
      float y1_i = *POP_ADDRESS_AT(0, float, n + i_plus_1);
      
      float weight_real = w[i];
      float weight_imag = w[i_plus_1];

      float y1w_r = y1_r * weight_real - y1_i * weight_imag;
      float y1w_i = y1_r * weight_imag + y1_i * weight_real;

      *PUSH_ADDRESS_AT(0, float, i) = y0_r + y1w_r;
      *PUSH_ADDRESS_AT(0, float, i + 1) = y0_i + y1w_i;

      *PUSH_ADDRESS_AT(0, float, n + i) = y0_r - y1w_r;
      *PUSH_ADDRESS_AT(0, float, n + i + 1) = y0_i - y1w_i;
    }

    // Commit input and output data 
    COMMIT_PUSH(0); 
    COMMIT_POP(0);

    KEND();
  } END_KERNEL_BODY(); 
}
#include END_OF_MULTIPLE_ITER_OPTIMIZATION

/// FFTReorderSimple
#include START_OF_MULTIPLE_ITER_OPTIMIZATION
__kernel void FFTReorderSimple(void) {
  DECLARE_INPUT_QUEUE(0, float); 
  DECLARE_OUTPUT_QUEUE(0, float);
  DECLARE_ROBX(0, int, 1);

  // Get parameters
  int n = *ROBX(0, int, 0); 
  int totalData = 2 * n; 
  BEGIN_KERNEL_BODY() {
    KSTART(); 

    // Reserve input and output data 
    RESERVE_POP(0, totalData); 
    RESERVE_PUSH_TICKET_INQ(0, 0, totalData); 

    // Reordering
    int o = 0; 
    for (int i = 0; i < totalData; i+=4) {
      *PUSH_ADDRESS_AT(0, float, o++) = *POP_ADDRESS_AT(0, float, i); 
      *PUSH_ADDRESS_AT(0, float, o++) = *POP_ADDRESS_AT(0, float, i+1); 
    }

    for (int i = 2; i < totalData; i+=4) {
      *PUSH_ADDRESS_AT(0, float, o++) = *POP_ADDRESS_AT(0, float, i); 
      *PUSH_ADDRESS_AT(0, float, o++) = *POP_ADDRESS_AT(0, float, i+1); 
    }

    // Commit input and output data 
    COMMIT_PUSH(0); 
    COMMIT_POP(0);

    KEND();
  } END_KERNEL_BODY(); 
}
#include END_OF_MULTIPLE_ITER_OPTIMIZATION

/// Test source 
#include START_OF_DEFAULT_OPTIMIZATION
__kernel void TestSource(void) {
  DECLARE_OUTPUT_QUEUE(0, float);
  DECLARE_ROBX(0, int, 1); // Iter
  DECLARE_ROBX(1, int, 1); // m 

  int Iter = *ROBX(0, int, 0); 
  int m = *ROBX(1, int, 0); 
  int Num = 36 * 2 * m; 
  BEGIN_KERNEL_BODY() {
    KSTART(); 

    // Generate random values with garbage
    for(int i = 0; i < Iter; ++i) {
      RESERVE_PUSH(0, Num); 
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
  DECLARE_ROBX(0, int, 1); // Iter
  DECLARE_ROBX(1, int, 1); // m 

  int Iter = *ROBX(0, int, 0); 
  int m = *ROBX(1, int, 0); 
  int Num = 36 * 2 * m; 
  BEGIN_KERNEL_BODY() {
    KSTART(); 

    // Iterate
    for (int i = 0; i < Iter; ++i) {
      // Generate random values with garbage
      RESERVE_POP(0, Num); 
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

/// Benchmark TDE
int benchmarkTDE(int Cores, int Iter, char *Log) {
  /// Benchmark pipeline 
  // [TestSource(CH:6,M:15)]--
  //     [[Transpose(N:36, M:15)]]--[[Expand(N:36,B:64)]]--
  //
  //     [[Reorder64]]--[[Reorder32]]--[[Reorder16]]--[[Reorder8]]--[[Reorder4]]--
  //     [[Combine2]]--[[Combine4]]--[[Combine8]]--
  //     [[Combine16]]--[[Combine32]]--[[Combine64]]--
  //
  //     [[Multiply_by_float]]--
  //
  //     [[IReorder64]]--[[IReorder32]]--[[IReorder16]]--[[IReorder8]]--[[IReorder4]]--
  //     [[ICombine2]]--[[ICombine4]]--[[ICombine8]]--
  //     [[ICombine16]]--[[ICombine32]]--[[ICombineFinal]]--
  //
  //     [[Contract]]--[[ITranspose]]--
  // --[TestSink]
  ProgramDescriptor PD("TDE"); 

  // Code 
  PD.CodeTable["TestSource"] = new CodeDescriptor(TestSource); 
  PD.CodeTable["Transpose"] = new CodeDescriptor(Transpose); 
  PD.CodeTable["Expand"] = new CodeDescriptor(Expand); 
  PD.CodeTable["FFTReorderSimple"] = new CodeDescriptor(FFTReorderSimple); 
  PD.CodeTable["CombineDFT"] = new CodeDescriptor(CombineDFT); 
  PD.CodeTable["Multiply_by_float"] = new CodeDescriptor(Multiply_by_float); 
  PD.CodeTable["CombineIDFT"] = new CodeDescriptor(CombineIDFT); 
  PD.CodeTable["CombineIDFTFinal"] = new CodeDescriptor(CombineIDFTFinal); 
  PD.CodeTable["Contract"] = new CodeDescriptor(Contract); 
  PD.CodeTable["TestSink"] = new CodeDescriptor(TestSink); 

  // ROB
  int I[] = {Iter};
  PD.ROBTable["I"] = new ReadOnlyBufferDescriptor(I, sizeof(I), 1); 
  int CH6[] = {6};
  PD.ROBTable["CH6"] = new ReadOnlyBufferDescriptor(CH6, sizeof(CH6), 1); 
  int M15[]= {15};
  PD.ROBTable["M15"] = new ReadOnlyBufferDescriptor(M15, sizeof(M15), 1); 
  int m[]= {90};
  PD.ROBTable["m"] = new ReadOnlyBufferDescriptor(m, sizeof(m), 1); 
  int N36[] = {36};
  PD.ROBTable["N36"] = new ReadOnlyBufferDescriptor(N36, sizeof(N36), 1); 
  int B64[] = {64}; 
  PD.ROBTable["B64"] = new ReadOnlyBufferDescriptor(B64, sizeof(B64), 1); 
  float mult[] = {0.00390625}; 
  PD.ROBTable["mult"] = new ReadOnlyBufferDescriptor(mult, sizeof(mult), 1); 
  int N64[] = {64};
  PD.ROBTable["N64"] = new ReadOnlyBufferDescriptor(N64, sizeof(N64), 1); 
  int N32[] = {32}; 
  PD.ROBTable["N32"] = new ReadOnlyBufferDescriptor(N32, sizeof(N32), 1); 
  int N16[] = {16};
  PD.ROBTable["N16"] = new ReadOnlyBufferDescriptor(N16, sizeof(N16), 1); 
  int N8[] = {8};
  PD.ROBTable["N8"] = new ReadOnlyBufferDescriptor(N8, sizeof(N8), 1); 
  int N4[] = {4};
  PD.ROBTable["N4"] = new ReadOnlyBufferDescriptor(N4, sizeof(N4), 1); 
  int N2[] = {2}; 
  PD.ROBTable["N2"] = new ReadOnlyBufferDescriptor(N2, sizeof(N2), 1); 
  DFTParams DP2;   float W2[2];   initializeDFTParams(2, &DP2, W2); 
  PD.ROBTable["DP2"] = new ReadOnlyBufferDescriptor(&DP2, sizeof(DP2), 1); 
  PD.ROBTable["W2"] = new ReadOnlyBufferDescriptor(W2, sizeof(W2), 2); 
  DFTParams DP4;   float W4[4];   initializeDFTParams(4, &DP4, W4); 
  PD.ROBTable["DP4"] = new ReadOnlyBufferDescriptor(&DP4, sizeof(DP4), 1); 
  PD.ROBTable["W4"] = new ReadOnlyBufferDescriptor(W4, sizeof(W4), 4); 
  DFTParams DP8;   float W8[8];   initializeDFTParams(8, &DP8, W8); 
  PD.ROBTable["DP8"] = new ReadOnlyBufferDescriptor(&DP8, sizeof(DP8), 1); 
  PD.ROBTable["W8"] = new ReadOnlyBufferDescriptor(W8, sizeof(W8), 8); 
  DFTParams DP16;  float W16[16]; initializeDFTParams(16, &DP16, W16); 
  PD.ROBTable["DP16"] = new ReadOnlyBufferDescriptor(&DP16, sizeof(DP16), 1); 
  PD.ROBTable["W16"] = new ReadOnlyBufferDescriptor(W16, sizeof(W16), 16); 
  DFTParams DP32;  float W32[32]; initializeDFTParams(32, &DP32, W32); 
  PD.ROBTable["DP32"] = new ReadOnlyBufferDescriptor(&DP32, sizeof(DP32), 1); 
  PD.ROBTable["W32"] = new ReadOnlyBufferDescriptor(W32, sizeof(W32), 32); 
  DFTParams DP64;  float W64[64]; initializeDFTParams(64, &DP64, W64); 
  PD.ROBTable["DP64"] = new ReadOnlyBufferDescriptor(&DP64, sizeof(DP64), 1); 
  PD.ROBTable["W64"] = new ReadOnlyBufferDescriptor(W64, sizeof(W64), 64); 
  IDFTFinalParams IDPF64; float IW64[64];  initializeIDFTFinalParams(64, &IDPF64, IW64); 
  PD.ROBTable["IDPF64"] = new ReadOnlyBufferDescriptor(&IDPF64, sizeof(IDPF64), 1); 
  PD.ROBTable["IW64"] = new ReadOnlyBufferDescriptor(IW64, sizeof(IW64), 64); 

  // Queue
  const int QSize = 100 * 40 * 6 * 15 * 36 * 2; 
  PD.QTable["TestSourceQ"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                 "01TestSourceK", "02TransposeK", 
                                                 false, false, true, false); 
  PD.QTable["TransposeQ"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                "02TransposeK", "03ExpandK", 
                                                false, true, true, false); 
  PD.QTable["ExpandQ"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                             "03ExpandK", "04Reorder64K", 
                                             false, true, true, false); 
  PD.QTable["Reorder64Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                "04Reorder64K", "05Reorder32K", 
                                                false, true, true, false); 
  PD.QTable["Reorder32Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                "05Reorder32K", "06Reorder16K", 
                                                false, true, true, false); 
  PD.QTable["Reorder16Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                "06Reorder16K", "07Reorder8K", 
                                                false, true, true, false); 
  PD.QTable["Reorder8Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                               "07Reorder8K", "08Reorder4K", 
                                               false, true, true, false); 
  PD.QTable["Reorder4Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                               "08Reorder4K", "09Combine2K", 
                                               false, true, true, false); 
  PD.QTable["Combine2Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                               "09Combine2K", "10Combine4K", 
                                               false, true, true, false); 
  PD.QTable["Combine4Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                               "10Combine4K", "11Combine8K", 
                                               false, true, true, false); 
  PD.QTable["Combine8Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                               "11Combine8K", "12Combine16K", 
                                               false, true, true, false); 
  PD.QTable["Combine16Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                "12Combine16K", "13Combine32K", 
                                                false, true, true, false); 
  PD.QTable["Combine32Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                "13Combine32K", "14Combine64K", 
                                                false, true, true, false); 
  PD.QTable["Combine64Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                "14Combine64K", "15Multiply_by_floatK", 
                                                false, true, true, false); 
  PD.QTable["Multiply_by_floatQ"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                        "15Multiply_by_floatK", "16IReorder64K", 
                                                        false, true, true, false); 
  PD.QTable["IReorder64Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                 "16IReorder64K", "17IReorder32K", 
                                                 false, true, true, false); 
  PD.QTable["IReorder32Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                 "17IReorder32K", "18IReorder16K", 
                                                 false, true, true, false); 
  PD.QTable["IReorder16Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                 "18IReorder16K", "19IReorder8K", 
                                                 false, true, true, false); 
  PD.QTable["IReorder8Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                "19IReorder8K", "20IReorder4K", 
                                                false, true, true, false); 
  PD.QTable["IReorder4Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                "20IReorder4K", "21ICombine2K", 
                                                false, true, true, false); 
  PD.QTable["ICombine2Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                "21ICombine2K", "22ICombine4K", 
                                                false, true, true, false); 
  PD.QTable["ICombine4Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                "22ICombine4K", "23ICombine8K", 
                                                false, true, true, false); 
  PD.QTable["ICombine8Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                "23ICombine8K", "24ICombine16K", 
                                                false, true, true, false); 
  PD.QTable["ICombine16Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                 "24ICombine16K", "25ICombine32K", 
                                                 false, true, true, false); 
  PD.QTable["ICombine32Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                 "25ICombine32K", "26ICombineFinalK", 
                                                 false, true, true, false); 
  PD.QTable["ICombineFinalQ"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                    "26ICombineFinalK", "27ContractK", 
                                                    false, true, true, false); 
  PD.QTable["ContractQ"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                               "27ContractK", "28ITransposeK", 
                                               false, true, true, false); 
  PD.QTable["ITransposeQ"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                 "28ITransposeK", "29TestSinkK", 
                                                 false, true, false, false); 

  // Kernel 
  PD.KernelTable["01TestSourceK"] = new KernelDescriptor("TestSource", true, false); 
  PD.KernelTable["01TestSourceK"]->OutQ[0] = "TestSourceQ";
  PD.KernelTable["01TestSourceK"]->ROB[0] = "I";
  PD.KernelTable["01TestSourceK"]->ROB[1] = "m";

  PD.KernelTable["02TransposeK"] = new KernelDescriptor("Transpose", false, true); 
  PD.KernelTable["02TransposeK"]->InQ[0] = "TestSourceQ";
  PD.KernelTable["02TransposeK"]->OutQ[0] = "TransposeQ";
  PD.KernelTable["02TransposeK"]->ROB[0] = "N36";
  PD.KernelTable["02TransposeK"]->ROB[1] = "M15";

  PD.KernelTable["03ExpandK"] = new KernelDescriptor("Expand", false, true); 
  PD.KernelTable["03ExpandK"]->InQ[0] = "TransposeQ";
  PD.KernelTable["03ExpandK"]->OutQ[0] = "ExpandQ";
  PD.KernelTable["03ExpandK"]->ROB[0] = "N36";
  PD.KernelTable["03ExpandK"]->ROB[1] = "B64";

  PD.KernelTable["04Reorder64K"] = new KernelDescriptor("FFTReorderSimple", false, true); 
  PD.KernelTable["04Reorder64K"]->InQ[0] = "ExpandQ";
  PD.KernelTable["04Reorder64K"]->OutQ[0] = "Reorder64Q";
  PD.KernelTable["04Reorder64K"]->ROB[0] = "N64";

  PD.KernelTable["05Reorder32K"] = new KernelDescriptor("FFTReorderSimple", false, true); 
  PD.KernelTable["05Reorder32K"]->InQ[0] = "Reorder64Q";
  PD.KernelTable["05Reorder32K"]->OutQ[0] = "Reorder32Q";
  PD.KernelTable["05Reorder32K"]->ROB[0] = "N32";

  PD.KernelTable["06Reorder16K"] = new KernelDescriptor("FFTReorderSimple", false, true); 
  PD.KernelTable["06Reorder16K"]->InQ[0] = "Reorder32Q";
  PD.KernelTable["06Reorder16K"]->OutQ[0] = "Reorder16Q";
  PD.KernelTable["06Reorder16K"]->ROB[0] = "N16";

  PD.KernelTable["07Reorder8K"] = new KernelDescriptor("FFTReorderSimple", false, true); 
  PD.KernelTable["07Reorder8K"]->InQ[0] = "Reorder16Q";
  PD.KernelTable["07Reorder8K"]->OutQ[0] = "Reorder8Q";
  PD.KernelTable["07Reorder8K"]->ROB[0] = "N8";

  PD.KernelTable["08Reorder4K"] = new KernelDescriptor("FFTReorderSimple", false, true); 
  PD.KernelTable["08Reorder4K"]->InQ[0] = "Reorder8Q";
  PD.KernelTable["08Reorder4K"]->OutQ[0] = "Reorder4Q";
  PD.KernelTable["08Reorder4K"]->ROB[0] = "N4";

  PD.KernelTable["09Combine2K"] = new KernelDescriptor("CombineDFT", false, true); 
  PD.KernelTable["09Combine2K"]->InQ[0] = "Reorder4Q";
  PD.KernelTable["09Combine2K"]->OutQ[0] = "Combine2Q";
  PD.KernelTable["09Combine2K"]->ROB[0] = "N2";
  PD.KernelTable["09Combine2K"]->ROB[1] = "DP2";
  PD.KernelTable["09Combine2K"]->ROB[2] = "W2";

  PD.KernelTable["10Combine4K"] = new KernelDescriptor("CombineDFT", false, true); 
  PD.KernelTable["10Combine4K"]->InQ[0] = "Combine2Q";
  PD.KernelTable["10Combine4K"]->OutQ[0] = "Combine4Q";
  PD.KernelTable["10Combine4K"]->ROB[0] = "N4";
  PD.KernelTable["10Combine4K"]->ROB[1] = "DP4";
  PD.KernelTable["10Combine4K"]->ROB[2] = "W4";

  PD.KernelTable["11Combine8K"] = new KernelDescriptor("CombineDFT", false, true); 
  PD.KernelTable["11Combine8K"]->InQ[0] = "Combine4Q";
  PD.KernelTable["11Combine8K"]->OutQ[0] = "Combine8Q";
  PD.KernelTable["11Combine8K"]->ROB[0] = "N8";
  PD.KernelTable["11Combine8K"]->ROB[1] = "DP8";
  PD.KernelTable["11Combine8K"]->ROB[2] = "W8";

  PD.KernelTable["12Combine16K"] = new KernelDescriptor("CombineDFT", false, true); 
  PD.KernelTable["12Combine16K"]->InQ[0] = "Combine8Q";
  PD.KernelTable["12Combine16K"]->OutQ[0] = "Combine16Q";
  PD.KernelTable["12Combine16K"]->ROB[0] = "N16";
  PD.KernelTable["12Combine16K"]->ROB[1] = "DP16";
  PD.KernelTable["12Combine16K"]->ROB[2] = "W16";

  PD.KernelTable["13Combine32K"] = new KernelDescriptor("CombineDFT", false, true); 
  PD.KernelTable["13Combine32K"]->InQ[0] = "Combine16Q";
  PD.KernelTable["13Combine32K"]->OutQ[0] = "Combine32Q";
  PD.KernelTable["13Combine32K"]->ROB[0] = "N32";
  PD.KernelTable["13Combine32K"]->ROB[1] = "DP32";
  PD.KernelTable["13Combine32K"]->ROB[2] = "W32";

  PD.KernelTable["14Combine64K"] = new KernelDescriptor("CombineDFT", false, true); 
  PD.KernelTable["14Combine64K"]->InQ[0] = "Combine32Q";
  PD.KernelTable["14Combine64K"]->OutQ[0] = "Combine64Q";
  PD.KernelTable["14Combine64K"]->ROB[0] = "N64";
  PD.KernelTable["14Combine64K"]->ROB[1] = "DP64";
  PD.KernelTable["14Combine64K"]->ROB[2] = "W64";

  PD.KernelTable["15Multiply_by_floatK"] = new KernelDescriptor("Multiply_by_float", false, true); 
  PD.KernelTable["15Multiply_by_floatK"]->InQ[0] = "Combine64Q";
  PD.KernelTable["15Multiply_by_floatK"]->OutQ[0] = "Multiply_by_floatQ";
  PD.KernelTable["15Multiply_by_floatK"]->ROB[0] = "B64";
  PD.KernelTable["15Multiply_by_floatK"]->ROB[1] = "mult";

  PD.KernelTable["16IReorder64K"] = new KernelDescriptor("FFTReorderSimple", false, true); 
  PD.KernelTable["16IReorder64K"]->InQ[0] = "Multiply_by_floatQ";
  PD.KernelTable["16IReorder64K"]->OutQ[0] = "IReorder64Q";
  PD.KernelTable["16IReorder64K"]->ROB[0] = "N64";

  PD.KernelTable["17IReorder32K"] = new KernelDescriptor("FFTReorderSimple", false, true); 
  PD.KernelTable["17IReorder32K"]->InQ[0] = "IReorder64Q";
  PD.KernelTable["17IReorder32K"]->OutQ[0] = "IReorder32Q";
  PD.KernelTable["17IReorder32K"]->ROB[0] = "N32";

  PD.KernelTable["18IReorder16K"] = new KernelDescriptor("FFTReorderSimple", false, true); 
  PD.KernelTable["18IReorder16K"]->InQ[0] = "IReorder32Q";
  PD.KernelTable["18IReorder16K"]->OutQ[0] = "IReorder16Q";
  PD.KernelTable["18IReorder16K"]->ROB[0] = "N16";

  PD.KernelTable["19IReorder8K"] = new KernelDescriptor("FFTReorderSimple", false, true); 
  PD.KernelTable["19IReorder8K"]->InQ[0] = "IReorder16Q";
  PD.KernelTable["19IReorder8K"]->OutQ[0] = "IReorder8Q";
  PD.KernelTable["19IReorder8K"]->ROB[0] = "N8";

  PD.KernelTable["20IReorder4K"] = new KernelDescriptor("FFTReorderSimple", false, true); 
  PD.KernelTable["20IReorder4K"]->InQ[0] = "IReorder8Q";
  PD.KernelTable["20IReorder4K"]->OutQ[0] = "IReorder4Q";
  PD.KernelTable["20IReorder4K"]->ROB[0] = "N4";

  PD.KernelTable["21ICombine2K"] = new KernelDescriptor("CombineIDFT", false, true); 
  PD.KernelTable["21ICombine2K"]->InQ[0] = "IReorder4Q";
  PD.KernelTable["21ICombine2K"]->OutQ[0] = "ICombine2Q";
  PD.KernelTable["21ICombine2K"]->ROB[0] = "N2";
  PD.KernelTable["21ICombine2K"]->ROB[1] = "DP2";
  PD.KernelTable["21ICombine2K"]->ROB[2] = "W2";

  PD.KernelTable["22ICombine4K"] = new KernelDescriptor("CombineIDFT", false, true); 
  PD.KernelTable["22ICombine4K"]->InQ[0] = "ICombine2Q";
  PD.KernelTable["22ICombine4K"]->OutQ[0] = "ICombine4Q";
  PD.KernelTable["22ICombine4K"]->ROB[0] = "N4";
  PD.KernelTable["22ICombine4K"]->ROB[1] = "DP4";
  PD.KernelTable["22ICombine4K"]->ROB[2] = "W4";

  PD.KernelTable["23ICombine8K"] = new KernelDescriptor("CombineIDFT", false, true); 
  PD.KernelTable["23ICombine8K"]->InQ[0] = "ICombine4Q";
  PD.KernelTable["23ICombine8K"]->OutQ[0] = "ICombine8Q";
  PD.KernelTable["23ICombine8K"]->ROB[0] = "N8";
  PD.KernelTable["23ICombine8K"]->ROB[1] = "DP8";
  PD.KernelTable["23ICombine8K"]->ROB[2] = "W8";

  PD.KernelTable["24ICombine16K"] = new KernelDescriptor("CombineIDFT", false, true); 
  PD.KernelTable["24ICombine16K"]->InQ[0] = "ICombine8Q";
  PD.KernelTable["24ICombine16K"]->OutQ[0] = "ICombine16Q";
  PD.KernelTable["24ICombine16K"]->ROB[0] = "N16";
  PD.KernelTable["24ICombine16K"]->ROB[1] = "DP16";
  PD.KernelTable["24ICombine16K"]->ROB[2] = "W16";

  PD.KernelTable["25ICombine32K"] = new KernelDescriptor("CombineIDFT", false, true); 
  PD.KernelTable["25ICombine32K"]->InQ[0] = "ICombine16Q";
  PD.KernelTable["25ICombine32K"]->OutQ[0] = "ICombine32Q";
  PD.KernelTable["25ICombine32K"]->ROB[0] = "N32";
  PD.KernelTable["25ICombine32K"]->ROB[1] = "DP32";
  PD.KernelTable["25ICombine32K"]->ROB[2] = "W32";

  PD.KernelTable["26ICombineFinalK"] = new KernelDescriptor("CombineIDFTFinal", false, true); 
  PD.KernelTable["26ICombineFinalK"]->InQ[0] = "ICombine32Q";
  PD.KernelTable["26ICombineFinalK"]->OutQ[0] = "ICombineFinalQ";
  PD.KernelTable["26ICombineFinalK"]->ROB[0] = "N64";
  PD.KernelTable["26ICombineFinalK"]->ROB[1] = "IDPF64";
  PD.KernelTable["26ICombineFinalK"]->ROB[2] = "IW64";

  PD.KernelTable["27ContractK"] = new KernelDescriptor("Contract", false, true); 
  PD.KernelTable["27ContractK"]->InQ[0] = "ICombineFinalQ";
  PD.KernelTable["27ContractK"]->OutQ[0] = "ContractQ";
  PD.KernelTable["27ContractK"]->ROB[0] = "N36";
  PD.KernelTable["27ContractK"]->ROB[1] = "B64";

  PD.KernelTable["28ITransposeK"] = new KernelDescriptor("Transpose", false, true); 
  PD.KernelTable["28ITransposeK"]->InQ[0] = "ContractQ";
  PD.KernelTable["28ITransposeK"]->OutQ[0] = "ITransposeQ";
  PD.KernelTable["28ITransposeK"]->ROB[0] = "M15";
  PD.KernelTable["28ITransposeK"]->ROB[1] = "N36";

  PD.KernelTable["29TestSinkK"] = new KernelDescriptor("TestSink", false, false); 
  PD.KernelTable["29TestSinkK"]->InQ[0] = "ITransposeQ";
  PD.KernelTable["29TestSinkK"]->ROB[0] = "I";
  PD.KernelTable["29TestSinkK"]->ROB[1] = "m";

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
    SAFE_CALL_0( Rtm->start() );
    SAFE_CALL_0( Rtm->join() );
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
            "DANBI TDE benchmark\n");
  ::fprintf(stderr, 
            "  Usage: TDERun \n"); 
  ::fprintf(stderr, 
            "      [--repeat repeat_count]\n"); 
  ::fprintf(stderr, 
            "      [--core maximum_number_of_assignable_cores]\n");
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
            "# DANBI TDE benchmark\n");
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
int parseOption(int argc, char *argv[], int *iterations, int *cores, char **log) {
  struct option options[] = {
    {"repeat", required_argument, 0, 'r'}, 
    {"core",   required_argument, 0, 'c'}, 
    {"log",   required_argument, 0, 'l'}, 
    {0, 0, 0, 0}, 
  }; 

  // Set default values
  *iterations = 200000;
  *cores = 1; 
  *log = const_cast<char *>("TDE-Log");

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
  char *log; 
  int ret; 

  // Parse options 
  ret = parseOption(argc, argv, &iterations, &cores, &log);
  if (ret) return 1; 

  // Initialize PMCs
  for (int i = 0; i < PERF_COUNT_HW_MAX; ++i)
    PMCs[i].initialize(i, false, true); 

  // Run benchmark 
  ret = benchmarkTDE(cores, iterations, log);
  if (ret) return 2; 

  // Read PMCs
  for (int i = 0; i < PERF_COUNT_HW_MAX; ++i)
    PMCs[i].readCounter(HWCounters + i); 

  // Print out results
  showResult(cores, iterations); 

  return 0; 
}

