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

  RecursiveGaussian.cpp -- Recursive Gaussian Filer
  Implementation is based on the code from NVidia's CUDA SDK. 
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

/// Gaussian parameter precomputation
struct GaussParms {
  float nsigma; 
  float alpha;
  float ema; 
  float ema2; 
  float b1; 
  float b2; 
  float a0; 
  float a1; 
  float a2; 
  float a3; 
  float coefp; 
  float coefn; 
};

/// Maxtrix traspose
#include START_OF_DEFAULT_OPTIMIZATION
__kernel void Transpose(void) {
  DECLARE_INPUT_QUEUE(0, unsigned int); 
  DECLARE_OUTPUT_QUEUE(0, unsigned int);
  DECLARE_ROBX(0, int, 2);

  int Width = *ROBX(0, int, 0); 
  int Height = *ROBX(0, int, 1);
  int Num = Width * Height; 
  BEGIN_KERNEL_BODY() {
    KSTART();

    // Reserve input and output data
    RESERVE_POP(0, Num); 
    RESERVE_PUSH_TICKET_INQ(0, 0, Num); 

    // Transpose
    for (int x = 0; x < Width; ++x) {
      for (int y = 0; y < Height; ++y) {
        int Src = (y*Width) + x; 
        int Dst = (x*Height) + y; 
        *PUSH_ADDRESS_AT(0, unsigned int, Dst) = 
          *POP_ADDRESS_AT(0, unsigned int, Src); 
      }
    }

    // Commit input and output data 
    COMMIT_PUSH(0); 
    COMMIT_POP(0);

    KEND();
  } END_KERNEL_BODY(); 
}
#include END_OF_DEFAULT_OPTIMIZATION

/// Convert floating point rgba color to 32-bit integer
__device unsigned int rgbaFloat4ToUint(float4* rgba) {
  // Clamp to zero 
  if (rgba->x < 0.0f)  rgba->x = 0.0f; 
  if (rgba->y < 0.0f)  rgba->y = 0.0f; 
  if (rgba->z < 0.0f)  rgba->z = 0.0f; 
  if (rgba->w < 0.0f)  rgba->w = 0.0f; 

  unsigned int uiPackedPix = 0U;
  uiPackedPix |= 0x000000FF & (unsigned int)rgba->x;
  uiPackedPix |= 0x0000FF00 & ((unsigned int)(rgba->y) << 8);
  uiPackedPix |= 0x00FF0000 & ((unsigned int)(rgba->z) << 16);
  uiPackedPix |= 0xFF000000 & ((unsigned int)(rgba->w) << 24);
  return uiPackedPix;
}

/// Convert from 32-bit int to float4
__device void rgbaUintToFloat4(const unsigned int uiPackedRGBA, float4* rgba) {
  rgba->x = (float)(uiPackedRGBA & 0xff);
  rgba->y = (float)((uiPackedRGBA >> 8) & 0xff);
  rgba->z = (float)((uiPackedRGBA >> 16) & 0xff);
  rgba->w = (float)((uiPackedRGBA >> 24) & 0xff);
}

/// Recursive Gaussian Filter 
/// - Input queue data should be already transposed. 
///   o Data: y * x
///   o Size: w * h 
#include START_OF_DEFAULT_OPTIMIZATION
__kernel void RecursiveGaussian(void) {
  DECLARE_INPUT_QUEUE(0, unsigned int); 
  DECLARE_OUTPUT_QUEUE(0, unsigned int);
  DECLARE_ROBX(0, int, 2);
  DECLARE_ROBX(1, GaussParms, 1);

  // Get parameters
  int w = *ROBX(0, int, 0); 
  int h = *ROBX(0, int, 1);
  GaussParms* GP = ROBX(1, GaussParms, 0); 
  BEGIN_KERNEL_BODY() {
    KSTART(); 

    // Reserve input and output data 
    RESERVE_POP(0, h); 
    RESERVE_PUSH_TICKET_INQ(0, 0, h); 
    unsigned int* id;
    unsigned int* od;

    // Forward pass
    float4 xp(0.0f);  // previous input
    float4 yp(0.0f);  // previous output
    float4 yb(0.0f);  // previous output by 2
    float4 xc, yc; 
    id = POP_ADDRESS_AT(0, unsigned int, 0); 
    rgbaUintToFloat4(*id, &xp); 
    yb = GP->coefp*xp; 
    yp = yb;
    int y =0;  
    for (; y < h; y++) {
      id = POP_ADDRESS_AT(0, unsigned int, y); 
      od = PUSH_ADDRESS_AT(0, unsigned int, y); 

      rgbaUintToFloat4(*id, &xc);
      yc = GP->a0*xc + GP->a1*xp - GP->b1*yp - GP->b2*yb;
      *od = rgbaFloat4ToUint(&yc); 

      xp = xc; 
      yb = yp; 
      yp = yc; 
    }

    // Reset pointers to point to last element in column
    --y; 
    id = POP_ADDRESS_AT(0, unsigned int, y); 
    od = PUSH_ADDRESS_AT(0, unsigned int, y); 

    // Reverse pass
    // Ensures response is symmetrical
    float4 xn(0.0f);
    float4 xa(0.0f);
    float4 yn(0.0f);
    float4 ya(0.0f);
    rgbaUintToFloat4(*id, &xa); 
    rgbaUintToFloat4(*id, &xn); 
    yn = GP->coefn*xn; 
    ya = yn;
    for (int y = h-1; y >= 0; y--) {
      id = POP_ADDRESS_AT(0, unsigned int, y); 
      od = PUSH_ADDRESS_AT(0, unsigned int, y); 

      rgbaUintToFloat4(*id, &xc); 
      yc = GP->a2*xn + GP->a3*xa - GP->b1*yn - GP->b2*ya;
      xa = xn; 
      xn = xc; 
      ya = yn; 
      yn = yc;
      float4 t; 
      rgbaUintToFloat4(*od, &t); 
      t += yc; 
      *od = rgbaFloat4ToUint(&t);
    }

    // Commit input and output data 
    COMMIT_PUSH(0); 
    COMMIT_POP(0);

    KEND();
  } END_KERNEL_BODY(); 
}
#include END_OF_DEFAULT_OPTIMIZATION


/// Test source 
#include START_OF_DEFAULT_OPTIMIZATION
__kernel void TestSource(void) {
  DECLARE_OUTPUT_QUEUE(0, unsigned int);
  DECLARE_ROBX(0, int, 1);
  DECLARE_ROBX(1, int, 2);

  int Iter = *ROBX(0, int, 0); 
  int Width = *ROBX(1, int, 0); 
  int Height = *ROBX(1, int, 1);
  int Num = Width * Height; 
  BEGIN_KERNEL_BODY() {
    KSTART(); 

    // Iterate
    for (int i = 0; i < Iter; ++i) {
      // Generate a random matrix with garbage values
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
  DECLARE_INPUT_QUEUE(0, unsigned int);
  DECLARE_ROBX(0, int, 1);
  DECLARE_ROBX(1, int, 2);

  int Iter = *ROBX(0, int, 0); 
  int Width = *ROBX(1, int, 0); 
  int Height = *ROBX(1, int, 1);
  int Num = Width * Height; 
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

/// Pre-compute filter coefficients
static void PreProcessGaussParms (float fSigma, int iOrder, GaussParms* pGP) {
  pGP->nsigma = fSigma; // note: fSigma is range-checked and clamped >= 0.1f upstream
  pGP->alpha = 1.695f / pGP->nsigma;
  pGP->ema = exp(-pGP->alpha);
  pGP->ema2 = exp(-2.0f * pGP->alpha);
  pGP->b1 = -2.0f * pGP->ema;
  pGP->b2 = pGP->ema2;
  pGP->a0 = 0.0f;
  pGP->a1 = 0.0f;
  pGP->a2 = 0.0f;
  pGP->a3 = 0.0f;
  pGP->coefp = 0.0f;
  pGP->coefn = 0.0f;
  switch (iOrder) {
    case 0: 
    {
      const float k = (1.0f - pGP->ema)*(1.0f - pGP->ema) /
        (1.0f + (2.0f * pGP->alpha * pGP->ema) - pGP->ema2);
      pGP->a0 = k;
      pGP->a1 = k * (pGP->alpha - 1.0f) * pGP->ema;
      pGP->a2 = k * (pGP->alpha + 1.0f) * pGP->ema;
      pGP->a3 = -k * pGP->ema2;
    } 
    break;
    case 1: 
    {
      pGP->a0 = (1.0f - pGP->ema) * (1.0f - pGP->ema);
      pGP->a1 = 0.0f;
      pGP->a2 = -pGP->a0;
      pGP->a3 = 0.0f;
    } 
    break;
    case 2: 
    {
      const float ea = exp(-pGP->alpha);
      const float k = -(pGP->ema2 - 1.0f)/(2.0f * pGP->alpha * pGP->ema);
      float kn = -2.0f * (-1.0f + (3.0f * ea) - (3.0f * ea * ea) + (ea * ea * ea));
      kn /= (((3.0f * ea) + 1.0f + (3.0f * ea * ea) + (ea * ea * ea)));
      pGP->a0 = kn;
      pGP->a1 = -kn * (1.0f + (k * pGP->alpha)) * pGP->ema;
      pGP->a2 = kn * (1.0f - (k * pGP->alpha)) * pGP->ema;
      pGP->a3 = -kn * pGP->ema2;
    } 
    break;
    default:
      // note: iOrder is range-checked and clamped to 0-2 upstream
      return;
  }
  pGP->coefp = (pGP->a0 + pGP->a1)/(1.0f + pGP->b1 + pGP->b2);
  pGP->coefn = (pGP->a2 + pGP->a3)/(1.0f + pGP->b1 + pGP->b2);
}

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
int benchmarkGaussianFilter(int Cores, int Iter, float QFactor, char* Log, int W, int H, float Sigma, int Order) {
  /// Benchmark pipeline 
  // [TestSource]--
  //     [[RecursiveGaussian1]]--[[Transpose1]]--[[RecursiveGaussian2]]--[[Transpose2]]
  // --[TestSink]
  ProgramDescriptor PD("RecursiveGaussianFilter"); 

  // Code
  PD.CodeTable["TestSource"] = new CodeDescriptor(TestSource); 
  PD.CodeTable["Transpose"] = new CodeDescriptor(Transpose); 
  PD.CodeTable["RecursiveGaussian"] = new CodeDescriptor(RecursiveGaussian); 
  PD.CodeTable["TestSink"] = new CodeDescriptor(TestSink); 

  // ROB
  int WH_[2] = {W, H}; 
  int HW_[2] = {H, W}; 
  int I_[1] = {Iter}; 
  GaussParms GP_; 
  PreProcessGaussParms (Sigma, Order, &GP_);
  PD.ROBTable["WH"] = new ReadOnlyBufferDescriptor(WH_, sizeof(int), 2);
  PD.ROBTable["HW"] = new ReadOnlyBufferDescriptor(HW_, sizeof(int), 2); 
  PD.ROBTable["I"] = new ReadOnlyBufferDescriptor(I_, sizeof(int), 1); 
  PD.ROBTable["GP"] = new ReadOnlyBufferDescriptor(&GP_, sizeof(GaussParms), 1);

  // Queue
  const int QSize = float(std::min(128, std::max(Cores, 10)) * W * H) * QFactor; 
  PD.QTable["TestSourceQ"] = new QueueDescriptor(QSize, sizeof(unsigned int), false, 
						 "01TestSourceK", "02RecursiveGaussian1K", 
						 false, false, true, false); 
  PD.QTable["RecursiveGaussian1Q"] = new QueueDescriptor(QSize, sizeof(unsigned int), false, 
							 "02RecursiveGaussian1K", "03Transpose1K", 
							 false, true, true, false);
  PD.QTable["Transpose1Q"] = new QueueDescriptor(QSize, sizeof(unsigned int), false, 
						 "03Transpose1K", "04RecursiveGaussian2K",
						 false, true, true,  false);
  PD.QTable["RecursiveGaussian2Q"] = new QueueDescriptor(QSize, sizeof(unsigned int), false, 
							 "04RecursiveGaussian2K", "05Transpose2K", 
							 false, true, true, false);
  PD.QTable["Transpose2Q"] = new QueueDescriptor(QSize, sizeof(unsigned int), false, 
						 "05Transpose2K", "06TestSinkK", 
						 false, true, false, false);

  // Kernel 
  PD.KernelTable["01TestSourceK"] = new KernelDescriptor("TestSource", true, false); 
  PD.KernelTable["01TestSourceK"]->OutQ[0] = "TestSourceQ";
  PD.KernelTable["01TestSourceK"]->ROB[0] = "I";
  PD.KernelTable["01TestSourceK"]->ROB[1] = "WH";

  PD.KernelTable["02RecursiveGaussian1K"] = new KernelDescriptor("RecursiveGaussian", false, true); 
  PD.KernelTable["02RecursiveGaussian1K"]->InQ[0] = "TestSourceQ";
  PD.KernelTable["02RecursiveGaussian1K"]->OutQ[0] = "RecursiveGaussian1Q";
  PD.KernelTable["02RecursiveGaussian1K"]->ROB[0] = "HW";
  PD.KernelTable["02RecursiveGaussian1K"]->ROB[1] = "GP";

  PD.KernelTable["03Transpose1K"] = new KernelDescriptor("Transpose", false, true); 
  PD.KernelTable["03Transpose1K"]->InQ[0] = "RecursiveGaussian1Q";
  PD.KernelTable["03Transpose1K"]->OutQ[0] = "Transpose1Q";
  PD.KernelTable["03Transpose1K"]->ROB[0] = "WH";
  
  PD.KernelTable["04RecursiveGaussian2K"] = new KernelDescriptor("RecursiveGaussian", false, true); 
  PD.KernelTable["04RecursiveGaussian2K"]->InQ[0] = "Transpose1Q";
  PD.KernelTable["04RecursiveGaussian2K"]->OutQ[0] = "RecursiveGaussian2Q";
  PD.KernelTable["04RecursiveGaussian2K"]->ROB[0] = "WH";
  PD.KernelTable["04RecursiveGaussian2K"]->ROB[1] = "GP";

  PD.KernelTable["05Transpose2K"] = new KernelDescriptor("Transpose", false, true); 
  PD.KernelTable["05Transpose2K"]->InQ[0] = "RecursiveGaussian2Q";
  PD.KernelTable["05Transpose2K"]->OutQ[0] = "Transpose2Q";
  PD.KernelTable["05Transpose2K"]->ROB[0] = "HW";
  
  PD.KernelTable["06TestSinkK"] = new KernelDescriptor("TestSink", false, false); 
  PD.KernelTable["06TestSinkK"]->InQ[0] = "Transpose2Q";
  PD.KernelTable["06TestSinkK"]->ROB[0] = "I";
  PD.KernelTable["06TestSinkK"]->ROB[1] = "WH";
  
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
            "DANBI recursive gaussian filter benchmark\n");
  ::fprintf(stderr, 
            "  Usage: RecursiveGaussianRun \n"); 
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
            "# DANBI recursive gaussian filter benchmark\n");
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
  *iterations = 3000; 
  *cores = 1; 
  *log = const_cast<char *>("RecursiveGaussian-Log");

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

#define IMAGE_W 512
#define IMAGE_H 512
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
    ret = PMCs[i].initialize(i, false, true); 

  // Run benchmark 
  ret = benchmarkGaussianFilter(cores, iterations, qfactor, log, IMAGE_W, IMAGE_H, 10.0f, 0);
  if (ret) return 2; 

  // Read PMCs
  for (int i = 0; i < PERF_COUNT_HW_MAX; ++i)
    PMCs[i].readCounter(HWCounters + i); 

  // Print out results
  showResult(cores, iterations, qfactor); 

  return 0; 
}

