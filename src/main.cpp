#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION

#include <array>
#include <chrono>
#include <format>
#include <iostream>
#include <numeric>
#include <string_view>

#include "metal_adder.hpp"

using Clock                            = std::chrono::high_resolution_clock;
using time_unit                        = std::chrono::milliseconds;
constexpr std::string_view time_unit_s = "ms";

template<typename T>
void serial_sum(T const& A, T const& B)
{
  for (int i = 0; i < A.size(); ++i) {
    [[maybe_unused]] auto const x = A[i] + B[i];
  }
}

template<typename T>
void print_stats(const T& durations)
{
  // Mean
  auto total_time = static_cast<float>(
      std::accumulate(durations.begin(), durations.end(), 0LL));
  auto mean = total_time / durations.size();

  // Stddev
  auto stddev = 0.0;
  for (auto x : durations) {
    auto diff = mean - x;
    stddev += diff * diff;
  }

  stddev /= durations.size();
  stddev = std::sqrt(stddev);

  std::cout << std::format("Total time: {} {}, iterations: {}, iteration time: "
                           "{} ms, std dev: {:.2f}\n",
                           total_time, time_unit_s, durations.size(), mean,
                           stddev);
}

int main()
{
  auto const device = MTL::CreateSystemDefaultDevice();
  auto const adder  = MetalAdder(*device);

  constexpr int iterations = 100;
  std::array<long long, iterations> durations{};

  // Benchmark GPU
  std::cout << "Benchmarking GPU...\n";
  for (auto& duration : durations) {
    auto const start = Clock::now();
    adder.submit_command();
    auto const end = Clock::now();
    duration       = std::chrono::duration_cast<time_unit>(end - start).count();
  }
  adder.verify_results();
  print_stats(durations);

  // Benchmark CPU (serial)
  std::cout << "\nBenchmarking CPU...\n";
  auto const A = adder.buffer_A();
  auto const B = adder.buffer_B();

  for (auto& duration : durations) {
    auto const start = Clock::now();
    serial_sum(A, B);
    auto const end = Clock::now();
    duration       = std::chrono::duration_cast<time_unit>(end - start).count();
  }
  print_stats(durations);

  device->release();
  return 0;
}