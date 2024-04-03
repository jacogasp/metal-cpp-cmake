#include <metal_stdlib>

using namespace metal;

kernel void add_arrays(
  device const float* A,
  device const float* B,
  device float* result,
  uint index [[thread_position_in_grid]])
{
  result[index] = A[index] + B[index];
}
