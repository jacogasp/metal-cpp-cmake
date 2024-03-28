#include "metal_adder.hpp"

#include <iostream>
#include <cassert>
#include <format>

MetalAdder::MetalAdder(MTL::Device& device)
  : m_device(device)
{
  NS::Error* error              = nullptr;
  MTL::Library* default_library = m_device.newDefaultLibrary();

  if (default_library == nullptr) {
    std::cerr << "Failed to find the default library\n";
    return;
  }

  auto const function_name = NS::String::string(
      "add_arrays", NS::ASCIIStringEncoding);
  MTL::Function* add_function = default_library->newFunction(function_name);

  if (add_function == nullptr) {
    std::cerr << "Failed to find 'add_arrays' function\n";
    return;
  }

  m_pipeline_state = m_device.newComputePipelineState(add_function, &error);
  add_function->release();

  if (m_pipeline_state == nullptr) {
    std::cerr << "Failed to create pipeline state, error: " << error << ".\n";
    return;
  }

  m_command_queue = m_device.newCommandQueue();
  if (m_command_queue == nullptr) {
    std::cerr << "Failed to find the command queue\n";
    return;
  }

  m_buffer_A = m_device.newBuffer(bufferSize, MTL::ResourceStorageModeShared);
  m_buffer_B = m_device.newBuffer(bufferSize, MTL::ResourceStorageModeShared);
  m_buffer_result = m_device.newBuffer(bufferSize,
                                       MTL::ResourceStorageModeShared);

  prepare_data();
}

void MetalAdder::prepare_data() const
{
  generate_random_floats(*m_buffer_A);
  generate_random_floats(*m_buffer_B);
}

void MetalAdder::submit_command() const
{
  const auto command_buffer = m_command_queue->commandBuffer();
  assert(command_buffer != nullptr);

  const auto compute_encoder = command_buffer->computeCommandEncoder();
  assert(compute_encoder != nullptr);

  enconde_add_command(*compute_encoder);
  compute_encoder->endEncoding();
  command_buffer->commit();
  command_buffer->waitUntilCompleted();
}

void MetalAdder::verify_results() const
{
  const auto a      = static_cast<float*>(m_buffer_A->contents());
  const auto b      = static_cast<float*>(m_buffer_B->contents());
  const auto result = static_cast<float*>(m_buffer_result->contents());
  for (unsigned int i       = 0; i < arrayLength; ++i) {
    if (const auto expected = a[i] + b[i]; result[i] != expected) {
      std::cout << std::format(
          "Compute ERROR: index={}, result={}, vs {}=a+b\n", i, result[i],
          expected);
      return;
    }
  }
}

std::vector<float> MetalAdder::buffer_A() const
{
  auto contents = static_cast<float*>(m_buffer_A->contents());
  return {contents, contents + arrayLength};
}

std::vector<float> MetalAdder::buffer_B() const
{
  auto contents = static_cast<float*>(m_buffer_B->contents());
  return {contents, contents + arrayLength};
}

void MetalAdder::enconde_add_command(MTL::ComputeCommandEncoder& encoder) const
{
  encoder.setComputePipelineState(m_pipeline_state);
  encoder.setBuffer(m_buffer_A, 0, 0);
  encoder.setBuffer(m_buffer_B, 0, 1);
  encoder.setBuffer(m_buffer_result, 0, 2);

  const auto grid_size = MTL::Size::Make(arrayLength, 1, 1);

  NS::UInteger threadgroups = m_pipeline_state->maxTotalThreadsPerThreadgroup();
  if (threadgroups > arrayLength) {
    threadgroups = arrayLength;
  }

  const auto threadgroup_size = MTL::Size::Make(threadgroups, 1, 1);

  encoder.dispatchThreads(grid_size, threadgroup_size);
}

void MetalAdder::generate_random_floats(MTL::Buffer& buffer)
{
  auto* data_ptr = static_cast<float*>(buffer.contents());
  for (unsigned long i = 0; i < arrayLength; ++i) {
    data_ptr[i] = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
  }
}