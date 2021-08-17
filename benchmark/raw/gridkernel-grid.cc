#include <benchmark/benchmark.h>
#include <complex>
#include <iostream>

namespace gridbench {

static int N = 1024;
static int support = 512;

std::complex<float> gridkernel_grid_original(std::complex<float> *wtPtr,
                                             std::complex<float> *gridPtr) {
  std::complex<float> cVis(4.0f, 0.0f);
  float rVis = cVis.real();
  float iVis = cVis.imag();
  for (int suppv = -support; suppv <= support; suppv++) {
    float *wtPtrF = reinterpret_cast<float *>(&wtPtr[suppv]);
    float *gridPtrF = reinterpret_cast<float *>(&gridPtr[suppv]);
    for (int suppu = -support; suppu <= support;
         suppu++, wtPtrF += 2, gridPtrF += 2) {
      gridPtrF[0] += rVis * wtPtrF[0] - iVis * wtPtrF[1];
      gridPtrF[1] += rVis * wtPtrF[1] + iVis * wtPtrF[0];
    }
  }

  return cVis;
}

static void GridKernel_Grid_Original(benchmark::State &state) {
  std::complex<float> *wtPtr = new std::complex<float>[N];
  std::complex<float> *gridPtr = new std::complex<float>[N];
  for (int i = 0; i < N; i++) {
    gridPtr[i] = std::complex<float>(i * 1.1f);
    wtPtr[i] = std::complex<float>(i * 1.17f);
  }

  for (auto _ : state) {
    std::complex<float> cVis = gridkernel_grid_original(wtPtr, gridPtr);
    benchmark::DoNotOptimize(cVis);
  }
}

BENCHMARK(GridKernel_Grid_Original);

std::complex<float> gridkernel_grid_approach1(std::complex<float> *wtPtr,
                                              std::complex<float> *gridPtr) {
  std::complex<float> cVis(4.213f, 2.2132f);

  for (int suppv = -support; suppv <= support; suppv++) {
    for (int suppu = -support; suppu <= support; suppu++, wtPtr++, gridPtr++) {
      (*gridPtr) += std::complex<float>(
          cVis.real() * ((*wtPtr).real()) - cVis.imag() * ((*wtPtr).imag()),
          cVis.real() * ((*wtPtr).imag()) + cVis.imag() * ((*wtPtr).real()));
    }
  }

  return cVis;
}

static void GridKernel_Grid_Approach1(benchmark::State &state) {
  std::complex<float> *wtPtr = new std::complex<float>[N];
  std::complex<float> *gridPtr = new std::complex<float>[N];
  for (int i = 0; i < N; i++) {
    gridPtr[i] = std::complex<float>(i * 1.1f);
    wtPtr[i] = std::complex<float>(i * 1.17f);
  }

  for (auto _ : state) {
    gridkernel_grid_approach1(wtPtr, gridPtr);
  }
}

// BENCHMARK(GridKernel_Grid_Approach1);
} // namespace gridbench
