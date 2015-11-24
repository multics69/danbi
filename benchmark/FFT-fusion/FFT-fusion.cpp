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

  FFT2.cpp -- Fast Fourier Transform
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
void CombineDFT(int n, DFTParams *DP, float *w, float *src, float *dst) {
  // Combine
  for (int i = 0; i < n; i += 2) {
    float y0_r = src[i];
    float y0_i = src[i+1];
    float y1_r = src[n+i];
    float y1_i = src[n+i+1];
    
    // load into temps to make sure it doesn't got loaded
    // separately for each load
    float weight_real = w[i];
    float weight_imag = w[i+1];
    float y1w_r = y1_r * weight_real - y1_i * weight_imag;
    float y1w_i = y1_r * weight_imag + y1_i * weight_real;
    
    dst[i] = y0_r + y1w_r;
    dst[i+1] = y0_i + y1w_i;
    dst[n+i] = y0_r - y1w_r;
    dst[n+i+1] = y0_i - y1w_i;
  }
}

__kernel void CombineDFTFusion(void) {
  DECLARE_INPUT_QUEUE(0, float); 
  DECLARE_OUTPUT_QUEUE(0, float);
  DECLARE_ROBX(0, int, 1); // fusion level 
  DECLARE_ROBX(1, int, *ROBX(0, int, 0)); // N[]
  DECLARE_ROBX(2, DFTParams, *ROBX(0, int, 0)); // DP[]
  DECLARE_ROBXY(3, float, *ROBX(1, int, 0), *ROBX(0, int, 0)); // w[][]

  // Get parameters
  int FusionLevel = *ROBX(0, int, 0); 
  int* Ns = ROBX(1, int, 0);
  int TotalData = 2 * std::max(Ns[0], Ns[FusionLevel-1]);

  BEGIN_KERNEL_BODY() {
    KSTART(); 

    // Define local memory 
    float buff[2][TotalData];
    float *src = buff[0], *dst = buff[1];

    // Reserve input and output data 
    RESERVE_POP(0, TotalData); 
    RESERVE_PUSH_TICKET_INQ(0, 0, TotalData); 

    // Load data from input queue
    COPY_TO_MEM(0, float, 0, TotalData, src); 

    // Combine
    // FFT simple ordering for each fused level 
    for (int i = 0; i < FusionLevel; ++i) {
      // combine for each fused level 
      int n = Ns[i];
      int totalData = 2 * n; 
      DFTParams* DP = ROBX(2, DFTParams, i); 
      float* w = ROBXY(3, float, 0, i);
      for (int j = 0; j < TotalData; j += totalData)
        CombineDFT(n, DP, w, &src[j], &dst[j]);

      // swap source and destination buffer
      float *tmp; 
      tmp = src; 
      src = dst; 
      dst = tmp; 
    }

    // Store data to output queue 
    COPY_FROM_MEM(0, float, 0, TotalData, src); 

    // Commit input and output data 
    COMMIT_PUSH(0); 
    COMMIT_POP(0);

    KEND();
  } END_KERNEL_BODY(); 
}
#include END_OF_MULTIPLE_ITER_OPTIMIZATION

/// FFTReorderSimple
#include START_OF_MULTIPLE_ITER_OPTIMIZATION
void FFTReorderSimple(int n, float *in, float *out) {
  // Reordering
  int totalData = 2 * n; 
  int o = 0; 
  for (int i = 0; i < totalData; i+=4) {
    out[o++] = in[i];
    out[o++] = in[i+1];
  }
  
  for (int i = 2; i < totalData; i+=4) {
    out[o++] = in[i];
    out[o++] = in[i+1];
  }
}

__kernel void FFTReorderSimpleFusion(void) {
  DECLARE_INPUT_QUEUE(0, float); 
  DECLARE_OUTPUT_QUEUE(0, float);
  DECLARE_ROBX(0, int, 1); // fusion level 
  DECLARE_ROBX(1, int, *ROBX(0, int, 0)); // N[]

  // Get parameters
  int FusionLevel = *ROBX(0, int, 0); 
  int* Ns = ROBX(1, int, 0);
  int TotalData = 2 * std::max(Ns[0], Ns[FusionLevel-1]);
  BEGIN_KERNEL_BODY() {
    KSTART(); 

    // Define local memory 
    float buff[2][TotalData];
    float *src = buff[0], *dst = buff[1];

    // Reserve input and output data 
    RESERVE_POP(0, TotalData); 
    RESERVE_PUSH_TICKET_INQ(0, 0, TotalData); 

    // Load data from input queue
    COPY_TO_MEM(0, float, 0, TotalData, src); 

    // FFT simple ordering for each fused level 
    for (int i = 0; i < FusionLevel; ++i) {
      // reordering for each fused level 
      int n = Ns[i];
      int totalData = 2 * n; 
      for (int j = 0; j < TotalData; j += totalData)
        FFTReorderSimple(n, &src[j], &dst[j]);

      // swap source and destination buffer
      float *tmp; 
      tmp = src; 
      src = dst; 
      dst = tmp; 
    }

    // Store data to output queue 
    COPY_FROM_MEM(0, float, 0, TotalData, src); 

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
  DECLARE_ROBX(0, int, 1);
  DECLARE_ROBX(1, int, 1);

  int Iter = *ROBX(0, int, 0); 
  int N = *ROBX(1, int, 0); 
  BEGIN_KERNEL_BODY() {
    KSTART(); 

    // Generate random values with garbage
    for(int i = 0; i < Iter; ++i) {
      int Num = (2*(N-2) + 4); 
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
  DECLARE_ROBX(0, int, 1);
  DECLARE_ROBX(1, int, 1);

  int Iter = *ROBX(0, int, 0); 
  int N = *ROBX(1, int, 0); 
  int Num = 2*(N-2) + 4; 
  BEGIN_KERNEL_BODY() {
    KSTART(); 

    // Iterate
    for (int i = 0; i < Iter; ++i) {
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

/// Benchmark FFT
int benchmarkFFTFusion(int Cores, int Iter, float QFactor, char* Log) {
  ProgramDescriptor PD("FFT-Fusion"); 

  // Code
  PD.CodeTable["TestSource"] = new CodeDescriptor(TestSource); 
  PD.CodeTable["FFTReorderSimpleFusion"] = new CodeDescriptor(FFTReorderSimpleFusion); 
  PD.CodeTable["CombineDFTFusion"] = new CodeDescriptor(CombineDFTFusion); 
  PD.CodeTable["TestSink"] = new CodeDescriptor(TestSink); 

  // ROB
  int I_[1] = {Iter}; 
  PD.ROBTable["I"] = new ReadOnlyBufferDescriptor(I_, sizeof(int), 1); 
  int N64_[1] = {64}; 
  PD.ROBTable["N64"] = new ReadOnlyBufferDescriptor(N64_, sizeof(int), 1); 
  int ReorderFusionLevel_[1] = {5}; 
  PD.ROBTable["ReorderFusionLevel"] = new ReadOnlyBufferDescriptor(
    ReorderFusionLevel_, sizeof(int), 1); 
  int ReorderN_[5] = {64, 32, 16, 8, 4};
  PD.ROBTable["ReorderN"] = new ReadOnlyBufferDescriptor(ReorderN_, sizeof(int), 5); 
  int CombineFusionLevel_[1] = {6}; 
  PD.ROBTable["CombineFusionLevel"] = new ReadOnlyBufferDescriptor(
    CombineFusionLevel_, sizeof(int), 1); 
  int CombineN_[6] = {2, 4, 8, 16, 32, 64};
  PD.ROBTable["CombineN"] = new ReadOnlyBufferDescriptor(CombineN_, sizeof(int), 6); 
  DFTParams CombineDP_[6]; 
  float CombineW_[6][64];
  for (int i = 0; i < 6; ++i) {
    initializeDFTParams(CombineN_[i], &CombineDP_[i], CombineW_[i]); 
  }
  PD.ROBTable["CombineDP"] = new ReadOnlyBufferDescriptor(CombineDP_, sizeof(DFTParams), 6); 
  PD.ROBTable["CombineW"] = new ReadOnlyBufferDescriptor(CombineW_, sizeof(float), 64, 6); 

  // Queue
  const int QSize = float(100 * 2000 * 64 * 2) * QFactor; 
  PD.QTable["TestSourceQ"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                 "01TestSourceK", 
                                                 "02Reorder64_32_16_8_4K", 
                                                 false, false, true, false);
  PD.QTable["Reorder64_32_16_8_4Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                          "02Reorder64_32_16_8_4K", 
                                                          "03Combine2_4_8_16_32_64K", 
                                                          false, true, true, false);
  PD.QTable["Combine2_4_8_16_32_64Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                             "03Combine2_4_8_16_32_64K", 
                                                             "04TestSinkK", 
                                                             false, true, false, false);
  // Construct Kernels
  PD.KernelTable["01TestSourceK"] = new KernelDescriptor("TestSource", true, false);
  PD.KernelTable["01TestSourceK"]->OutQ[0] = "TestSourceQ";
  PD.KernelTable["01TestSourceK"]->ROB[0] = "I";
  PD.KernelTable["01TestSourceK"]->ROB[1] = "N64";

  PD.KernelTable["02Reorder64_32_16_8_4K"] = new KernelDescriptor("FFTReorderSimpleFusion", 
                                                                  false, true);
  PD.KernelTable["02Reorder64_32_16_8_4K"]->InQ[0] = "TestSourceQ";
  PD.KernelTable["02Reorder64_32_16_8_4K"]->OutQ[0] = "Reorder64_32_16_8_4Q";
  PD.KernelTable["02Reorder64_32_16_8_4K"]->ROB[0] = "ReorderFusionLevel";
  PD.KernelTable["02Reorder64_32_16_8_4K"]->ROB[1] = "ReorderN";

  PD.KernelTable["03Combine2_4_8_16_32_64K"] = new KernelDescriptor("CombineDFTFusion", 
                                                                    false, true);
  PD.KernelTable["03Combine2_4_8_16_32_64K"]->InQ[0] = "Reorder64_32_16_8_4Q";
  PD.KernelTable["03Combine2_4_8_16_32_64K"]->OutQ[0] = "Combine2_4_8_16_32_64Q";
  PD.KernelTable["03Combine2_4_8_16_32_64K"]->ROB[0] = "CombineFusionLevel";
  PD.KernelTable["03Combine2_4_8_16_32_64K"]->ROB[1] = "CombineN";
  PD.KernelTable["03Combine2_4_8_16_32_64K"]->ROB[2] = "CombineDP";
  PD.KernelTable["03Combine2_4_8_16_32_64K"]->ROB[3] = "CombineW";

  PD.KernelTable["04TestSinkK"] = new KernelDescriptor("TestSink", false, false);
  PD.KernelTable["04TestSinkK"]->InQ[0] = "Combine2_4_8_16_32_64Q";
  PD.KernelTable["04TestSinkK"]->ROB[0] = "I";
  PD.KernelTable["04TestSinkK"]->ROB[1] = "N64";

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
            "DANBI FFT-fusion benchmark\n");
  ::fprintf(stderr, 
            "  Usage: FFT-fusionRun \n"); 
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
            "# DANBI fast fourier transform benchmark\n");
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
  *iterations = 60000000;
  *cores = 1; 
  *log = const_cast<char *>("FFT-fusion-Log");

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

  // Parse options 
  ret = parseOption(argc, argv, &iterations, &cores, &log);
  if (ret) return 1; 

  // Initialize PMCs
  for (int i = 0; i < PERF_COUNT_HW_MAX; ++i)
    PMCs[i].initialize(i, false, true); 

  // Run benchmark 
  ret = benchmarkFFTFusion(cores, iterations, qfactor, log);
  if (ret) return 2; 

  // Read PMCs
  for (int i = 0; i < PERF_COUNT_HW_MAX; ++i)
    PMCs[i].readCounter(HWCounters + i); 

  // Print out results
  showResult(cores, iterations, qfactor); 

  return 0; 
}
