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
__kernel void CombineDFT(void) {
  DECLARE_INPUT_QUEUE(0, float); 
  DECLARE_OUTPUT_QUEUE(0, float);
  DECLARE_ROBX(0, int, 1);
  DECLARE_ROBX(1, DFTParams, 1); 
  DECLARE_ROBX(2, float, *ROBX(0, int, 0)); 

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

    // Combine
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

/// Benchmark FFT2
int benchmarkFFT2_64(int Cores, int Iter, char* Log) {
  /// Benchmark pipeline 
  // [TestSource]--
  //     [[Reorder64]]--[[Reorder32]]--[[Reorder16]]--[[Reorder8]]--[[Reorder4]]--
  //     [[Combine2]]--[[Combine4]]--[[Combine8]]--
  //     [[Combine16]]--[[Combine32]]--[[Combine64]]--
  // --[TestSink]
  ProgramDescriptor PD("FFT2"); 

  // Code
  PD.CodeTable["TestSource"] = new CodeDescriptor(TestSource); 
  PD.CodeTable["FFTReorderSimple"] = new CodeDescriptor(FFTReorderSimple); 
  PD.CodeTable["CombineDFT"] = new CodeDescriptor(CombineDFT); 
  PD.CodeTable["TestSink"] = new CodeDescriptor(TestSink); 

  // ROB
  int I_[1] = {Iter}; 
  PD.ROBTable["I"] = new ReadOnlyBufferDescriptor(I_, sizeof(int), 1); 
  int N64_[1] = {64}; 
  PD.ROBTable["N64"] = new ReadOnlyBufferDescriptor(N64_, sizeof(int), 1); 
  int N32_[1] = {32}; 
  PD.ROBTable["N32"] = new ReadOnlyBufferDescriptor(N32_, sizeof(int), 1); 
  int N16_[1] = {16}; 
  PD.ROBTable["N16"] = new ReadOnlyBufferDescriptor(N16_, sizeof(int), 1); 
  int N8_[1] = {8}; 
  PD.ROBTable["N8"] = new ReadOnlyBufferDescriptor(N8_, sizeof(int), 1); 
  int N4_[1] = {4}; 
  PD.ROBTable["N4"] = new ReadOnlyBufferDescriptor(N4_, sizeof(int), 1); 
  int N2_[1] = {2}; 
  PD.ROBTable["N2"] = new ReadOnlyBufferDescriptor(N2_, sizeof(int), 1); 
  DFTParams DP2_;   float W2_[2]; 
  initializeDFTParams(2, &DP2_, W2_); 
  PD.ROBTable["DP2"] = new ReadOnlyBufferDescriptor(&DP2_, sizeof(DP2_), 1); 
  PD.ROBTable["W2"] = new ReadOnlyBufferDescriptor(W2_, sizeof(float), 2); 
  DFTParams DP4_;   float W4_[4]; 
  initializeDFTParams(4, &DP4_, W4_); 
  PD.ROBTable["DP4"] = new ReadOnlyBufferDescriptor(&DP4_, sizeof(DP4_), 1); 
  PD.ROBTable["W4"] = new ReadOnlyBufferDescriptor(W4_, sizeof(float), 4); 
  DFTParams DP8_;   float W8_[8]; 
  initializeDFTParams(8, &DP8_, W8_); 
  PD.ROBTable["DP8"] = new ReadOnlyBufferDescriptor(&DP8_, sizeof(DP8_), 1); 
  PD.ROBTable["W8"] = new ReadOnlyBufferDescriptor(W8_, sizeof(float), 8); 
  DFTParams DP16_;   float W16_[16]; 
  initializeDFTParams(16, &DP16_, W16_); 
  PD.ROBTable["DP16"] = new ReadOnlyBufferDescriptor(&DP16_, sizeof(DP16_), 1); 
  PD.ROBTable["W16"] = new ReadOnlyBufferDescriptor(W16_, sizeof(float), 16); 
  DFTParams DP32_;   float W32_[32]; 
  initializeDFTParams(32, &DP32_, W32_); 
  PD.ROBTable["DP32"] = new ReadOnlyBufferDescriptor(&DP32_, sizeof(DP32_), 1); 
  PD.ROBTable["W32"] = new ReadOnlyBufferDescriptor(W32_, sizeof(float), 32); 
  DFTParams DP64_;   float W64_[64]; 
  initializeDFTParams(64, &DP64_, W64_); 
  PD.ROBTable["DP64"] = new ReadOnlyBufferDescriptor(&DP64_, sizeof(DP64_), 1); 
  PD.ROBTable["W64"] = new ReadOnlyBufferDescriptor(W64_, sizeof(float), 64); 

  // Queue
  const int QSize = 100 * 2000 * 64 * 2; 
  PD.QTable["TestSourceQ"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                 "01TestSourceK", "02Reorder64K", 
                                                 false, false, true, false);
  PD.QTable["Reorder64Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                 "02Reorder64K", "03Reorder32K", 
                                                false, true, true, false);
  PD.QTable["Reorder32Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                 "03Reorder32K", "04Reorder16K", 
                                                false, true, true, false);
  PD.QTable["Reorder16Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                 "04Reorder16K", "05Reorder8K", 
                                                false, true, true, false);
  PD.QTable["Reorder8Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                 "05Reorder8K", "06Reorder4K", 
                                               false, true, true, false);
  PD.QTable["Reorder4Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                 "06Reorder4K", "07Combine2K", 
                                               false, true, true, false);
  PD.QTable["Combine2Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                 "07Combine2K", "08Combine4K", 
                                               false, true, true, false);
  PD.QTable["Combine4Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                 "08Combine4K", "09Combine8K", 
                                               false, true, true, false);
  PD.QTable["Combine8Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                 "09Combine8K", "10Combine16K", 
                                               false, true, true, false);
  PD.QTable["Combine16Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                 "10Combine16K", "11Combine32K", 
                                                false, true, true, false);
  PD.QTable["Combine32Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                 "11Combine32K", "12Combine64K", 
                                                false, true, true, false);
  PD.QTable["Combine64Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                                 "12Combine64K", "13TestSinkK", 
                                                false, true, true, false);


  // Construct Kernels
  PD.KernelTable["01TestSourceK"] = new KernelDescriptor("TestSource", true, false);
  PD.KernelTable["01TestSourceK"]->OutQ[0] = "TestSourceQ";
  PD.KernelTable["01TestSourceK"]->ROB[0] = "I";
  PD.KernelTable["01TestSourceK"]->ROB[1] = "N64";

  PD.KernelTable["02Reorder64K"] = new KernelDescriptor("FFTReorderSimple", false, true);
  PD.KernelTable["02Reorder64K"]->InQ[0] = "TestSourceQ";
  PD.KernelTable["02Reorder64K"]->OutQ[0] = "Reorder64Q";
  PD.KernelTable["02Reorder64K"]->ROB[0] = "N64";

  PD.KernelTable["03Reorder32K"] = new KernelDescriptor("FFTReorderSimple", false, true);
  PD.KernelTable["03Reorder32K"]->InQ[0] = "Reorder64Q";
  PD.KernelTable["03Reorder32K"]->OutQ[0] = "Reorder32Q";
  PD.KernelTable["03Reorder32K"]->ROB[0] = "N32";

  PD.KernelTable["04Reorder16K"] = new KernelDescriptor("FFTReorderSimple", false, true);
  PD.KernelTable["04Reorder16K"]->InQ[0] = "Reorder32Q";
  PD.KernelTable["04Reorder16K"]->OutQ[0] = "Reorder16Q";
  PD.KernelTable["04Reorder16K"]->ROB[0] = "N16";
  
  PD.KernelTable["05Reorder8K"] = new KernelDescriptor("FFTReorderSimple", false, true);
  PD.KernelTable["05Reorder8K"]->InQ[0] = "Reorder16Q";
  PD.KernelTable["05Reorder8K"]->OutQ[0] = "Reorder8Q";
  PD.KernelTable["05Reorder8K"]->ROB[0] = "N8";

  PD.KernelTable["06Reorder4K"] = new KernelDescriptor("FFTReorderSimple", false, true);
  PD.KernelTable["06Reorder4K"]->InQ[0] = "Reorder8Q";
  PD.KernelTable["06Reorder4K"]->OutQ[0] = "Reorder4Q";
  PD.KernelTable["06Reorder4K"]->ROB[0] = "N4";

  PD.KernelTable["07Combine2K"] = new KernelDescriptor("CombineDFT", false, true);
  PD.KernelTable["07Combine2K"]->InQ[0] = "Reorder4Q";
  PD.KernelTable["07Combine2K"]->OutQ[0] = "Combine2Q";
  PD.KernelTable["07Combine2K"]->ROB[0] = "N2";
  PD.KernelTable["07Combine2K"]->ROB[1] = "DP2";
  PD.KernelTable["07Combine2K"]->ROB[2] = "W2";

  PD.KernelTable["08Combine4K"] = new KernelDescriptor("CombineDFT", false, true);
  PD.KernelTable["08Combine4K"]->InQ[0] = "Combine2Q";
  PD.KernelTable["08Combine4K"]->OutQ[0] = "Combine4Q";
  PD.KernelTable["08Combine4K"]->ROB[0] = "N4";
  PD.KernelTable["08Combine4K"]->ROB[1] = "DP4";
  PD.KernelTable["08Combine4K"]->ROB[2] = "W4";

  PD.KernelTable["09Combine8K"] = new KernelDescriptor("CombineDFT", false, true);
  PD.KernelTable["09Combine8K"]->InQ[0] = "Combine4Q";
  PD.KernelTable["09Combine8K"]->OutQ[0] = "Combine8Q";
  PD.KernelTable["09Combine8K"]->ROB[0] = "N8";
  PD.KernelTable["09Combine8K"]->ROB[1] = "DP8";
  PD.KernelTable["09Combine8K"]->ROB[2] = "W8";

  PD.KernelTable["10Combine16K"] = new KernelDescriptor("CombineDFT", false, true);
  PD.KernelTable["10Combine16K"]->InQ[0] = "Combine8Q";
  PD.KernelTable["10Combine16K"]->OutQ[0] = "Combine16Q";
  PD.KernelTable["10Combine16K"]->ROB[0] = "N16";
  PD.KernelTable["10Combine16K"]->ROB[1] = "DP16";
  PD.KernelTable["10Combine16K"]->ROB[2] = "W16";

  PD.KernelTable["11Combine32K"] = new KernelDescriptor("CombineDFT", false, true);
  PD.KernelTable["11Combine32K"]->InQ[0] = "Combine16Q";
  PD.KernelTable["11Combine32K"]->OutQ[0] = "Combine32Q";
  PD.KernelTable["11Combine32K"]->ROB[0] = "N32";
  PD.KernelTable["11Combine32K"]->ROB[1] = "DP32";
  PD.KernelTable["11Combine32K"]->ROB[2] = "W32";

  PD.KernelTable["12Combine64K"] = new KernelDescriptor("CombineDFT", false, true);
  PD.KernelTable["12Combine64K"]->InQ[0] = "Combine32Q";
  PD.KernelTable["12Combine64K"]->OutQ[0] = "Combine64Q";
  PD.KernelTable["12Combine64K"]->ROB[0] = "N64";
  PD.KernelTable["12Combine64K"]->ROB[1] = "DP64";
  PD.KernelTable["12Combine64K"]->ROB[2] = "W64";

  PD.KernelTable["13TestSinkK"] = new KernelDescriptor("TestSink", false, false);
  PD.KernelTable["13TestSinkK"]->InQ[0] = "Combine64Q";
  PD.KernelTable["13TestSinkK"]->ROB[0] = "I";
  PD.KernelTable["13TestSinkK"]->ROB[1] = "N64";

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
            "DANBI FFT2 benchmark\n");
  ::fprintf(stderr, 
            "  Usage: FFT2Run \n"); 
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
            "# DANBI fast fourier transform benchmark\n");
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
  *iterations = 30000000;
  *cores = 1; 
  *log = const_cast<char *>("FFT2-Log");

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

  // Parse options 
  ret = parseOption(argc, argv, &iterations, &cores, &log);
  if (ret) return 1; 

  // Initialize PMCs
  for (int i = 0; i < PERF_COUNT_HW_MAX; ++i)
    PMCs[i].initialize(i, false, true); 

  // Run benchmark 
  ret = benchmarkFFT2_64(cores, iterations, log);
  if (ret) return 2; 

  // Read PMCs
  for (int i = 0; i < PERF_COUNT_HW_MAX; ++i)
    PMCs[i].readCounter(HWCounters + i); 

  // Print out results
  showResult(cores, iterations); 

  return 0; 
}

