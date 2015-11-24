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

  SRAD2.cpp -- SRAD (Speckle Reducing Anisotropic Diffusion) benchmark 
  Implementation is based on the code from Rodinia benchmark. 
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

struct SRADParams {
  int cols, rows; // size of a tile
  float lambda;
}; 

struct SRADQ0sqr {
  float sum, sum2; 
};

struct SRADCmd {
  int QSource; 
  int IsFinal; 
  int PopNum; 
  int Level; // only for debugging
};

struct SRADCoeffJ {
  float dN;
  float dS;
  float dW;
  float dE;
  float c;

  float J; 
};

// Extract
#include START_OF_DEFAULT_OPTIMIZATION
__parallel __kernel void Extract(void) {
  DECLARE_INPUT_QUEUE(0, float);
  DECLARE_OUTPUT_QUEUE(0, float); // image
  DECLARE_OUTPUT_QUEUE(1, float); // Q0SQR
  DECLARE_ROBX(0, SRADParams, 1); 

  SRADParams* SP = ROBX(0, SRADParams, 0); 
  BEGIN_KERNEL_BODY() {
    KSTART();

    int totalData = SP->cols * SP->rows; 
    RESERVE_POP(0, totalData); 
    RESERVE_PUSH_TICKET_INQ(0, 0, totalData); 
    float sum = 0.0f, sum2 = 0.0f; 
    for (int i = 0; i < totalData; ++i) {
      float t = expf( *POP_ADDRESS_AT(0, float, i) );
      *PUSH_ADDRESS_AT(0, float, i) = t; 
      sum += t; 
      sum2 = t*t; 
    }
    COMMIT_PUSH(0); 
    COMMIT_POP(0);

    float meanROI = sum / totalData;
    float varROI  = (sum2 / totalData) - meanROI*meanROI;
    float q0sqr   = varROI / (meanROI*meanROI);
    RESERVE_PUSH_TICKET_INQ(1, 0, 1); 
    *PUSH_ADDRESS_AT(1, float, 0) = q0sqr; 
    COMMIT_PUSH(1); 

    KEND();
  } END_KERNEL_BODY(); 
}
#include END_OF_DEFAULT_OPTIMIZATION

/// SRAD1
#include START_OF_DEFAULT_OPTIMIZATION
__parallel __kernel void SRAD1(void) {
  DECLARE_INPUT_QUEUE(0, float);  // J 
  DECLARE_INPUT_QUEUE(1, float);  // q0sqr 
  DECLARE_OUTPUT_QUEUE(0, SRADCoeffJ); // SRADCoeffJ
  DECLARE_ROBX(0, SRADParams, 1); 
  DECLARE_ROBX(1, int, ROBX(0, SRADParams, 0)->rows); // iN
  DECLARE_ROBX(2, int, ROBX(0, SRADParams, 0)->rows); // iS
  DECLARE_ROBX(3, int, ROBX(0, SRADParams, 0)->cols); // jW
  DECLARE_ROBX(4, int, ROBX(0, SRADParams, 0)->cols); // jE
 
  // Get parameters
  SRADParams* SP = ROBX(0, SRADParams, 0); 
  int* iN = ROBX(1, int, 0); 
  int* iS = ROBX(2, int, 0); 
  int* jW = ROBX(3, int, 0); 
  int* jE = ROBX(4, int, 0); 
  int totalData = SP->cols * SP->rows; 
  BEGIN_KERNEL_BODY() {
    KSTART();

    // Reserve input and output data 
    float q0sqr; 
    RESERVE_POP(1, 1); 
    q0sqr = *POP_ADDRESS_AT(1, float, 0); 
    COMMIT_POP(1); 

    RESERVE_POP_TICKET_INQ(0, 1, totalData); // J 
    RESERVE_PUSH_TICKET_INQ(0, 1, totalData);  // SRADCoeffJ

    // SRAD
    for (int i = 0 ; i < SP->rows ; i++) {
      for (int j = 0; j < SP->cols; j++) { 
        int k = i * SP->cols + j;
        float Jc = *POP_ADDRESS_AT(0, float, k);
 
	// directional derivates
        float dNk, dSk, dWk, dEk; 
        dNk = *POP_ADDRESS_AT(0, float, iN[i] * SP->cols + j) - Jc; 
        dSk = *POP_ADDRESS_AT(0, float, iS[i] * SP->cols + j) - Jc; 
        dWk = *POP_ADDRESS_AT(0, float, i * SP->cols + jW[j]) - Jc; 
        dEk = *POP_ADDRESS_AT(0, float, i * SP->cols + jE[j]) - Jc; 
			
	float G2 = (dNk*dNk + dSk*dSk + dWk*dWk + dEk*dEk) / (Jc*Jc);
	float L = (dNk + dSk + dWk + dEk) / Jc;
	float num  = (0.5*G2) - ((1.0/16.0)*(L*L)) ;
	float den  = 1 + (.25*L);
	float qsqr = num/(den*den);
 
	// diffusion coefficent (equ 33)
	den = (qsqr-q0sqr) / (q0sqr * (1+q0sqr)) ;
	float ck = 1.0 / (1.0+den) ;
                
	// saturate diffusion coefficent
	if (ck < 0) {ck = 0;}
	else if (ck > 1) {ck = 1;}

        // store the calculated values
        SRADCoeffJ* SC = PUSH_ADDRESS_AT(0, SRADCoeffJ, k);
        SC->dN = dNk;        SC->dS = dSk;
        SC->dW = dWk;        SC->dE = dEk;
        SC->c  = ck;         SC->J  = Jc;
      }
    }

    // Commit input and output data 
    COMMIT_PUSH(0); 
    COMMIT_POP(0);

    KEND();
  } END_KERNEL_BODY(); 
}
#include END_OF_DEFAULT_OPTIMIZATION

/// SRAD2
#include START_OF_DEFAULT_OPTIMIZATION
__parallel __kernel void SRAD2(void) {
  DECLARE_INPUT_QUEUE(0, SRADCoeffJ); // SRADCoeffJ
  DECLARE_OUTPUT_QUEUE(0, float);  // J 
  DECLARE_ROBX(0, SRADParams, 1); 
  DECLARE_ROBX(1, int, ROBX(0, SRADParams, 0)->rows); // iN
  DECLARE_ROBX(2, int, ROBX(0, SRADParams, 0)->rows); // iS
  DECLARE_ROBX(3, int, ROBX(0, SRADParams, 0)->cols); // jW
  DECLARE_ROBX(4, int, ROBX(0, SRADParams, 0)->cols); // jE
 
  // Get parameters
  SRADParams* SP = ROBX(0, SRADParams, 0); 
  int* iN = ROBX(1, int, 0); 
  int* iS = ROBX(2, int, 0); 
  int* jW = ROBX(3, int, 0); 
  int* jE = ROBX(4, int, 0); 
  int totalData = SP->cols * SP->rows; 
  BEGIN_KERNEL_BODY() {
    KSTART();

    // Reserve input and output data 
    RESERVE_POP(0, totalData); // SRADCoeffJ
    RESERVE_PUSH_TICKET_INQ(0, 0, totalData); // J

    // SRAD
    for (int i = 0 ; i < SP->rows ; i++) {
      for (int j = 0; j < SP->cols; j++) { 
	// current index
        int k = i * SP->cols + j;
        SRADCoeffJ* SC = POP_ADDRESS_AT(0, SRADCoeffJ, k);
 
	// diffusion coefficent
        float cN, cS, cW, cE; 
        cN = *POP_ADDRESS_AT(0, float, iN[i] * SP->cols + j) - SC->J;
        cS = *POP_ADDRESS_AT(0, float, iS[i] * SP->cols + j) - SC->J;
        cW = *POP_ADDRESS_AT(0, float, i * SP->cols + jW[j]) - SC->J;
        cE = *POP_ADDRESS_AT(0, float, i * SP->cols + jE[j]) - SC->J;

	// divergence (equ 58)
	float D = cN * SC->dN + cS * SC->dS + cW * SC->dW + cE * SC->dE;
                
	// image update (equ 61)
        *PUSH_ADDRESS_AT(0, float, k) = SC->J + 0.25 * SP->lambda * D;
      }
    }

    // Commit input and output data 
    COMMIT_PUSH(0); 
    COMMIT_POP(0);

    KEND();
  } END_KERNEL_BODY(); 
}
#include END_OF_DEFAULT_OPTIMIZATION

// Compress
#include START_OF_DEFAULT_OPTIMIZATION
__parallel __kernel void Compress(void) {
  DECLARE_INPUT_QUEUE(0, float);
  DECLARE_OUTPUT_QUEUE(0, float);
  DECLARE_ROBX(0, SRADParams, 1); 

  SRADParams* SP = ROBX(0, SRADParams, 0); 
  BEGIN_KERNEL_BODY() {
    KSTART();

    int totalData = SP->cols * SP->rows; 
    RESERVE_POP(0, totalData); 
    RESERVE_PUSH_TICKET_INQ(0, 0, totalData); 
    for (int i = 0; i < totalData; ++i)
      *PUSH_ADDRESS_AT(0, float, i) = logf( *POP_ADDRESS_AT(0, float, i) ); 
    COMMIT_PUSH(0); 
    COMMIT_POP(0);

    KEND();
  } END_KERNEL_BODY(); 
}
#include END_OF_DEFAULT_OPTIMIZATION


/// Test source 
#include START_OF_DEFAULT_OPTIMIZATION
__sequential __startable __kernel void TestSource(void) {
  DECLARE_OUTPUT_QUEUE(0, float);
  DECLARE_ROBX(0, int, 1); // Iter
  DECLARE_ROBX(1, SRADParams, 1); 

  int Iter = *ROBX(0, int, 0); 
  SRADParams* SP = ROBX(1, SRADParams, 0); 
  int totalData = SP->cols * SP->rows; 
  BEGIN_KERNEL_BODY() {
    KSTART();

    // Generate an random image with garbage values
    for(int i = 0; i < Iter; ++i) {
      RESERVE_PUSH(0, totalData); 
      COMMIT_PUSH(0); 
    }

    KEND();
  } END_KERNEL_BODY(); 
}
#include END_OF_DEFAULT_OPTIMIZATION

/// Test sink 
#include START_OF_DEFAULT_OPTIMIZATION
__parallel __kernel void TestSink(void) {
  DECLARE_INPUT_QUEUE(0, float);
  DECLARE_ROBX(0, int, 1); // Iter
  DECLARE_ROBX(1, SRADParams, 1); 

  int Iter = *ROBX(0, int, 0); 
  SRADParams* SP = ROBX(1, SRADParams, 0); 
  int totalData = SP->cols * SP->rows; 
  BEGIN_KERNEL_BODY() {
    KSTART();

    // Iterate
    for (int i = 0; i < Iter; ++i) {
      RESERVE_POP(0, totalData); 
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

/// Benchmark SRAD2
int benchmarkSRAD2(int Cores, int Iter, float QFactor, char *Log, int cols, int rows) {
  /// Benchmark pipeline 
  // [TestSource]--
  //     [[Extract]]--[[ROIStat]]--[[SRAD1]]--[[SRAD2]]--[[Compress]]
  // --[TestSink]
  ProgramDescriptor PD("SRAD2"); 

  // Code
  PD.CodeTable["TestSource"] = new CodeDescriptor(TestSource); 
  PD.CodeTable["Extract"] = new CodeDescriptor(Extract); 
  PD.CodeTable["SRAD1"] = new CodeDescriptor(SRAD1); 
  PD.CodeTable["SRAD2"] = new CodeDescriptor(SRAD2); 
  PD.CodeTable["Compress"] = new CodeDescriptor(Compress); 
  PD.CodeTable["TestSink"] = new CodeDescriptor(TestSink); 

  // ROB
  int I[] = {Iter}; 
  PD.ROBTable["I"] = new ReadOnlyBufferDescriptor(I, sizeof(I), 1); 

  SRADParams SP = {cols, rows, 0.5f};
  PD.ROBTable["SP"] = new ReadOnlyBufferDescriptor(&SP, sizeof(SP), 1); 

  int iN[rows], iS[rows], jW[cols], jE[cols]; 
  for (int i=0; i< rows; i++) {
    iN[i] = i-1;
    iS[i] = i+1;
  }    
  for (int j=0; j< cols; j++) {
    jW[j] = j-1;
    jE[j] = j+1;
  }
  iN[0]    = 0;
  iS[rows-1] = rows-1;
  jW[0]    = 0;
  jE[cols-1] = cols-1;
  PD.ROBTable["iN"] = new ReadOnlyBufferDescriptor(iN, sizeof(int), rows); 
  PD.ROBTable["iS"] = new ReadOnlyBufferDescriptor(iS, sizeof(int), rows); 
  PD.ROBTable["jW"] = new ReadOnlyBufferDescriptor(jW, sizeof(int), cols); 
  PD.ROBTable["jE"] = new ReadOnlyBufferDescriptor(jE, sizeof(int), cols); 

  // Queue
  const int StatQSize = float(Iter) * QFactor; 
  const int QSize = float(std::min(128, std::max(Cores, 10)) * cols * rows) * QFactor; 
  const int OrgStatQSize = float(Iter); 
  const int OrgQSize = float(std::min(128, std::max(Cores, 10)) * cols * rows); 
  PD.QTable["TestSourceQ"] = new QueueDescriptor(OrgQSize, sizeof(float), false, 
                                                 "01TestSourceK", "02ExtractK", 
                                                 false, false, true, false); 
  PD.QTable["ExtractQ"] = new QueueDescriptor(OrgQSize, sizeof(float), false, 
                                              "02ExtractK", "03SRAD1K", 
                                              false, true, false, true); 
  PD.QTable["ExtractStatQ"] = new QueueDescriptor(OrgStatQSize, sizeof(float), false, 
                                                  "02ExtractK", "03SRAD1K", 
                                                  false, true, true, false); 
  PD.QTable["SRAD1Q"] = new QueueDescriptor(OrgQSize, sizeof(SRADCoeffJ), false, 
                                            "03SRAD1K", "04SRAD2K", 
                                            false, true, true, false); 
  PD.QTable["SRAD2Q"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                            "04SRAD2K", "05CompressK", 
                                            false, true, true, false); 
  PD.QTable["CompressQ"] = new QueueDescriptor(QSize, sizeof(float), false, 
                                               "05CompressK", "06TestSinkK", 
                                               false, true, false, false); 

  // Kernel 
  PD.KernelTable["01TestSourceK"] = new KernelDescriptor("TestSource", true, false); 
  PD.KernelTable["01TestSourceK"]->OutQ[0] = "TestSourceQ";
  PD.KernelTable["01TestSourceK"]->ROB[0] = "I";
  PD.KernelTable["01TestSourceK"]->ROB[1] = "SP";

  PD.KernelTable["02ExtractK"] = new KernelDescriptor("Extract", false, true); 
  PD.KernelTable["02ExtractK"]->InQ[0] = "TestSourceQ";
  PD.KernelTable["02ExtractK"]->OutQ[0] = "ExtractQ";
  PD.KernelTable["02ExtractK"]->OutQ[1] = "ExtractStatQ";
  PD.KernelTable["02ExtractK"]->ROB[0] = "SP";

  PD.KernelTable["03SRAD1K"] = new KernelDescriptor("SRAD1", false, true); 
  PD.KernelTable["03SRAD1K"]->InQ[0] = "ExtractQ";
  PD.KernelTable["03SRAD1K"]->InQ[1] = "ExtractStatQ";
  PD.KernelTable["03SRAD1K"]->OutQ[0] = "SRAD1Q";
  PD.KernelTable["03SRAD1K"]->ROB[0] = "SP";
  PD.KernelTable["03SRAD1K"]->ROB[1] = "iN";
  PD.KernelTable["03SRAD1K"]->ROB[2] = "iS";
  PD.KernelTable["03SRAD1K"]->ROB[3] = "jW";
  PD.KernelTable["03SRAD1K"]->ROB[4] = "jE";

  PD.KernelTable["04SRAD2K"] = new KernelDescriptor("SRAD2", false, true); 
  PD.KernelTable["04SRAD2K"]->InQ[0] = "SRAD1Q";
  PD.KernelTable["04SRAD2K"]->OutQ[0] = "SRAD2Q";
  PD.KernelTable["04SRAD2K"]->ROB[0] = "SP";
  PD.KernelTable["04SRAD2K"]->ROB[1] = "iN";
  PD.KernelTable["04SRAD2K"]->ROB[2] = "iS";
  PD.KernelTable["04SRAD2K"]->ROB[3] = "jW";
  PD.KernelTable["04SRAD2K"]->ROB[4] = "jE";

  PD.KernelTable["05CompressK"] = new KernelDescriptor("Compress", false, true); 
  PD.KernelTable["05CompressK"]->InQ[0] = "SRAD2Q";
  PD.KernelTable["05CompressK"]->OutQ[0] = "CompressQ";
  PD.KernelTable["05CompressK"]->ROB[0] = "SP";

  PD.KernelTable["06TestSinkK"] = new KernelDescriptor("TestSink", false, false); 
  PD.KernelTable["06TestSinkK"]->InQ[0] = "CompressQ";
  PD.KernelTable["06TestSinkK"]->ROB[0] = "I";
  PD.KernelTable["06TestSinkK"]->ROB[1] = "SP";

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
            "DANBI SRAD2 benchmark\n");
  ::fprintf(stderr, 
            "  Usage: SRAD2Run \n"); 
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
            "# DANBI SRAD2 benchmark\n");
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
  *iterations = 5000; 
  *cores = 1; 
  *log = const_cast<char *>("SRAD2-Log");

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
  char *log; 
  int ret; 
  float qfactor = 3.0f / 3.0f; 

  // Parse options 
  ret = parseOption(argc, argv, &iterations, &cores, &log);
  if (ret) return 1; 

  // Initialize PMCs
  for (int i = 0; i < PERF_COUNT_HW_MAX; ++i)
    ret = PMCs[i].initialize(i, false, true); 

  // Run benchmark 
  ret = benchmarkSRAD2(cores, iterations, qfactor, log, IMAGE_W, IMAGE_H);
  if (ret) return 2; 

  // Read PMCs
  for (int i = 0; i < PERF_COUNT_HW_MAX; ++i)
    PMCs[i].readCounter(HWCounters + i); 

  // Print out results
  showResult(cores, iterations, qfactor); 

  return 0; 
}
