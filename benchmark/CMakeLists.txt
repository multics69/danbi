add_custom_target(Benchmarks)

function(add_danbi_benchmark benchmark_dirname)
  string(REGEX MATCH "([^/]+)$" benchmark_dirname ${benchmark_dirname})
  add_danbi_executable(${benchmark_dirname}Run ${ARGN})
  add_dependencies(Benchmarks ${benchmark_dirname}Run)
endfunction()

add_danbi_benchmark(bConf
  bConf/bConf.cpp
  )

add_danbi_benchmark(mRCQ
  mRCQ/mRCQ.cpp
  )

add_danbi_benchmark(mCuBoQ
  mCuBoQ/mCuBoQ.cpp
  )

add_danbi_benchmark(mCuBoQ2
  mCuBoQ2/mCuBoQ2.cpp
  )

add_danbi_benchmark(mSSQPS
  mSSQPS/mSSQPS.cpp
  )

add_danbi_benchmark(mMSQ
  mMSQ/mMSQ.cpp
  )

add_danbi_benchmark(mLSQ
  mLSQ/mLSQ.cpp
  )

add_danbi_benchmark(RecursiveGaussian
  RecursiveGaussian/RecursiveGaussian.cpp
  )

add_danbi_benchmark(FFT2
  FFT2/FFT2.cpp
  )

add_danbi_benchmark(FFT-fusion
  FFT-fusion/FFT-fusion.cpp
  )

add_danbi_benchmark(MergeSort
  MergeSort/MergeSort.cpp
  )

add_danbi_benchmark(MergeSort2
  MergeSort2/MergeSort2.cpp
  )

add_danbi_benchmark(MergeSort3
  MergeSort3/MergeSort3.cpp
  )

add_danbi_benchmark(TDE
  TDE/TDE.cpp
  )

add_danbi_benchmark(TDE-fusion
  TDE-fusion/TDE-fusion.cpp
  )

add_danbi_benchmark(SRAD2
  SRAD2/SRAD2.cpp
  )

add_danbi_benchmark(FilterBank
  FilterBank/FilterBank.cpp
  )

add_danbi_benchmark(FMRadio
  FMRadio/FMRadio.cpp
  )

add_danbi_benchmark(mrWordCount
  mrWordCount/mrWordCount.cpp
  )
