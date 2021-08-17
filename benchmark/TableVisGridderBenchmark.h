#include <benchmark/benchmark.h>
#include <tests/gridding/TableVisGridderTest.h>

static void BM_Spherical_Forward(benchmark::State &state) {
  askap::synthesis::TableVisGridderTest gridderTest;
  gridderTest.setUp();
  for (auto _ : state) {
    gridderTest.testForwardSph();
  }
}
BENCHMARK(BM_Spherical_Forward);

static void BM_Spherical_Reverse(benchmark::State &state) {
  askap::synthesis::TableVisGridderTest gridderTest;
  gridderTest.setUp();
  for (auto _ : state) {
    gridderTest.testReverseSph();
  }
}
BENCHMARK(BM_Spherical_Reverse);

static void BM_WProject_Forward(benchmark::State &state) {
  askap::synthesis::TableVisGridderTest gridderTest;
  gridderTest.setUp();
  for (auto _ : state) {
    gridderTest.testForwardWProject();
  }
}
BENCHMARK(BM_WProject_Forward);

static void BM_WProject_Reverse(benchmark::State &state) {
  askap::synthesis::TableVisGridderTest gridderTest;
  gridderTest.setUp();
  for (auto _ : state) {
    gridderTest.testReverseWProject();
  }
}
BENCHMARK(BM_WProject_Reverse);

