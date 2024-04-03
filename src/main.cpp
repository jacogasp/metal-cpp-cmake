#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION

#include <array>
#include <chrono>
#include <format>
#include <iostream>
#include <numeric>

#include "metal_adder.hpp"

using namespace std::chrono_literals;
using Clock = std::chrono::high_resolution_clock;

template<typename T>
void serial_sum(const T& A, const T& B)
{
  for (int i = 0; i < A.size(); ++i) {
    [[maybe_unused]] const auto x = A[i] + B[i];
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

  std::cout << std::format("Total time: {} ms, iterations: {}, iteration time: "
                           "{} ms, std dev: {:.2f}\n",
                           total_time, durations.size(), mean, stddev);
}

int main()
{
  const auto device = MTL::CreateSystemDefaultDevice();
  const auto adder  = MetalAdder(*device);

  constexpr int iterations = 100;
  std::array<long long, iterations> durations{};

  // Benchmark GPU
  std::cout << "Benchmarking GPU...\n";
  for (auto& duration : durations) {
    const auto start = Clock::now();
    adder.submit_command();
    const auto end = Clock::now();
    duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
            .count();
  }
  adder.verify_results();
  print_stats(durations);

  // Benchmark CPU (serial)
  std::cout << "\nBenchmarking CPU...\n";
  const auto A = adder.buffer_A();
  const auto B = adder.buffer_B();

  for (auto& duration : durations) {
    const auto start = Clock::now();
    serial_sum(A, B);
    const auto end = Clock::now();
    duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
            .count();
  }
  print_stats(durations);

  device->release();
  return 0;
}