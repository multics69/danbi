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

  mrWordCount.cpp -- word count by map-reduce
 */
#include <string.h>
#include <ctype.h>
#include "MapReduce/MapReduce.h"

#define APP_DEBUG 1
#if APP_DEBUG
#define DBG_PRINT(...) printf(__VA_ARGS__)
#else
#define DBG_PRINT(...)
#endif

struct Chunk {
  char* Start; 
  char* End; 
};

class WordCount : public MapReduce<WordCount, Chunk, char*, long> {
public:

private:
  inline bool isWhiteSpace(char c) {
    return (c == ' ')  || (c == '\t') || (c == '\r') || (c == '\n');
  }

  inline void emitWord(Tuple* Tuples, long TuplesSize, long& TIdx, char* Word) {
    // Push to the tuples buffer
    Tuples[TIdx].Key = Word; 
    Tuples[TIdx].Value = 1; 
    ++TIdx; 

    // Check if flushing is needed or not 
    if (TIdx >= TuplesSize)
      flushWord(Tuples, TIdx); 
  }

  inline void flushWord(Tuple* Tuples, long& TIdx) {
    // Do nothing if there is no buffered tuple
    if (TIdx <= 0) return; 

    // Ok, we have something to flush. 
    Handle H; 
    reserveTuples(H, TIdx);
    copyMemToTuple(H, 0, TIdx, Tuples);
    commitTuples(H);
    TIdx = 0; 
  }
public:
  WordCount(const char* ProgramName_, long Cores_, void* Data_, long DataLen_)
    : MapReduce<WordCount, Chunk, char*, long>(
      ProgramName_, Cores_, Data_, DataLen_
#if APP_DEBUG
      , 100 /* SplitSize */
      , 100 /* ReduceBatchSize */
      , 100 /* ProcessBatchSize */
#endif
      ) {
    // Do nothing
  }

  ~WordCount() {
    // Do nothing
  }
  
  inline int compareKey(Tuple* T1, Tuple* T2) {
    return ::strcmp(T1->Key, T2->Key);
  }

  inline void split(void* Data, long DataLen, long Start, long End) {
    char* Corpus = static_cast<char *>(Data); 

    // If it is not the first chunk, skip the first word. 
    if (Start && !isWhiteSpace(Corpus[Start-1])) {
      while(Start < DataLen && !isWhiteSpace(Corpus[Start]))
        ++Start; 
   }

    // Move end point to the next word break
    if (!isWhiteSpace(Corpus[End-1])) {
      while (End < DataLen && !isWhiteSpace(Corpus[End])) 
        ++End;
    }

    // Generate chunk boundary 
    Handle H;
    Chunk* c; 
    reserveDataElements(H, 1); 
    c = getDataElementAt(H, 0); 
    c->Start = &Corpus[Start];
    c->End = &Corpus[End];
    commitDataElements(H);
#if 0
    DBG_PRINT("Split: [%ld:%ld]\n", Start, End);
#endif
  }

  inline long map(Chunk* c) {
    enum {MapBatchSize = 8192}; 
    Tuple Tuples[MapBatchSize];
    long TIdx = 0;
    long NumTuples = 0;

    DBG_PRINT("Map: ");
    for(char* i = c->Start; i < c->End; ++i) {
      // Toupper
      *i = ::toupper(*i); 

      // Skip white space 
      if (*i < 'A' || *i > 'Z') 
        continue; 

      // Find the end of a workd
      char* Start = i; 
      while((*i >= 'A' & *i <= 'Z') || (*i == '\'')) {
        ++i; 
        if (i < c->End) 
          *i = ::toupper(*i); 
        else
          break;
      }

      // If there is a word, emit it. 
      if (i != Start) {
        *i = '\0';
        emitWord(Tuples, MapBatchSize, TIdx, Start);
        DBG_PRINT("%s ", Start);
        ++NumTuples;
      }
    }
    flushWord(Tuples, TIdx);
    DBG_PRINT(" ==> %ld in total\n", NumTuples);
    return NumTuples;
  }

  inline void combine(Tuple* T1, Tuple* T2, Tuple* Out) {
    Out->Key = T1->Key;
    Out->Value = T1->Value + T2->Value; 
    DBG_PRINT("Combine: %s -> %ld\n", Out->Key, Out->Value);
  }

#if APP_DEBUG
  inline void reduce(Tuple* In, Tuple* Out) {
    *Out = *In;
    DBG_PRINT("Reduce: [%s:%ld]\n", Out->Key, Out->Value);
  }

  inline void process(Tuple* Result) {
    DBG_PRINT("Process: [%s:%ld]\n", Result->Key, Result->Value);
  }
#endif
};

#define BENCHMARK_CLASS WordCount
#define BENCHMARK_CLASS_STR "WordCount"
#include "Benchmark/MapReduceMainMockup.h"
