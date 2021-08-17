#include <benchmark/benchmark.h>
#include <complex>

namespace degridbench {

  static int N = 1024;
  static int support = 512;

  std::complex<float> gridkernel_degrid_original(std::complex<float> *wtPtr,
                                                std::complex<float> *gridPtr) {
    std::complex<float> cVis(0.0f, 0.0f);
    for (int suppu = -support; suppu <= support; suppu++, wtPtr++, gridPtr++) {
      cVis += std::complex<float>((*wtPtr).real() * (*gridPtr).real() +
                                      (*wtPtr).imag() * (*gridPtr).imag(),
                                  -(*wtPtr).real() * (*gridPtr).imag() +
                                      (*wtPtr).imag() * (*gridPtr).real());
    }
    return cVis;
  }

  static void GridKernel_Degrid_Original(benchmark::State &state) {
    std::complex<float> *wtPtr = new std::complex<float>[N];
    std::complex<float> *gridPtr = new std::complex<float>[N];
    for (int i = 0; i < N; i++) {
      gridPtr[i] = std::complex<float>(i * 1.1f);
      wtPtr[i] = std::complex<float>(i * 1.17f);
    }

    for (auto _ : state) {
      std::complex<float> cVis = gridkernel_degrid_original(wtPtr, gridPtr);
      benchmark::DoNotOptimize(cVis);
    }
  }

  BENCHMARK(GridKernel_Degrid_Original);

  std::complex<float> gridkernel_degrid_approach1(std::complex<float> *wtPtr,
                                                  std::complex<float> *gridPtr) {
    float re = 0.0f;
    float im = 0.0f;
    for (int suppu = -support; suppu <= support; suppu++, wtPtr++, gridPtr++) {
      re += (*wtPtr).real() * (*gridPtr).real() +
            (*wtPtr).imag() * (*gridPtr).imag();
      im += -(*wtPtr).real() * (*gridPtr).imag() +
            (*wtPtr).imag() * (*gridPtr).real();
    }

    return std::complex<float>(re, im);
  }

  static void GridKernel_Degrid_Approach1(benchmark::State &state) {
    std::complex<float> *wtPtr = new std::complex<float>[N];
    std::complex<float> *gridPtr = new std::complex<float>[N];
    for (int i = 0; i < N; i++) {
      gridPtr[i] = std::complex<float>(i * 1.1f);
      wtPtr[i] = std::complex<float>(i * 1.17f);
    }

    for (auto _ : state) {
      std::complex<float> cVis = gridkernel_degrid_approach1(wtPtr, gridPtr);
      benchmark::DoNotOptimize(cVis);
    }
  }

  BENCHMARK(GridKernel_Degrid_Approach1);

  std::complex<float> gridkernel_degrid_approach2(std::complex<float> *wtPtr,
                                                  std::complex<float> *gridPtr) {
    float re = 0.0f;
    float im = 0.0f;
    for (int suppu = -support; suppu <= support; suppu++, wtPtr++, gridPtr++) {
      float *wtPtrF = reinterpret_cast<float *>(&wtPtr);
      float *gridPtrF = reinterpret_cast<float *>(&gridPtr);
      re += wtPtrF[0] * gridPtrF[0] + wtPtrF[1] * gridPtrF[1];
      im += -wtPtrF[0] * gridPtrF[1] + wtPtrF[1] * gridPtrF[0];
    }

    return std::complex<float>(re, im);
  }

  static void GridKernel_Degrid_Approach2(benchmark::State &state) {
    std::complex<float> *wtPtr = new std::complex<float>[N];
    std::complex<float> *gridPtr = new std::complex<float>[N];
    for (int i = 0; i < N; i++) {
      gridPtr[i] = std::complex<float>(i * 1.1f);
      wtPtr[i] = std::complex<float>(i * 1.17f);
    }

    for (auto _ : state) {
      std::complex<float> cVis = gridkernel_degrid_approach2(wtPtr, gridPtr);
      benchmark::DoNotOptimize(cVis);
    }
  }

  BENCHMARK(GridKernel_Degrid_Approach2);

}
