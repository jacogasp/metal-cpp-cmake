#include "metal_adder.hpp"

#include <cassert>
#include <format>
#include <iostream>

MetalAdder::MetalAdder(MTL::Device& device)
    : m_device(device)
{
  NS::Error* error              = nullptr;
  MTL::Library* default_library = m_device.newDefaultLibrary();

  if (default_library == nullptr) {
    std::cerr << "Failed to find the default library\n";
    return;
  }

  auto const function_name =
      NS::String::string("add_arrays", NS::ASCIIStringEncoding);
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

  m_buffer_A = m_device.newBuffer(BUFFER_SIZE, MTL::ResourceStorageModeShared);
  m_buffer_B = m_device.newBuffer(BUFFER_SIZE, MTL::ResourceStorageModeShared);
  m_buffer_result =
      m_device.newBuffer(BUFFER_SIZE, MTL::ResourceStorageModeShared);

  prepare_data();
}

void MetalAdder::prepare_data() const
{
  generate_random_floats(*m_buffer_A);
  generate_random_floats(*m_buffer_B);
}

void MetalAdder::submit_command() const
{
  auto const command_buffer = m_command_queue->commandBuffer();
  assert(command_buffer != nullptr);

  auto const compute_encoder = command_buffer->computeCommandEncoder();
  assert(compute_encoder != nullptr);

  encode_add_command(*compute_encoder);
  compute_encoder->endEncoding();
  command_buffer->commit();
  command_buffer->waitUntilCompleted();
}

void MetalAdder::verify_results() const
{
  auto const a      = static_cast<float*>(m_buffer_A->contents());
  auto const b      = static_cast<float*>(m_buffer_B->contents());
  auto const result = static_cast<float*>(m_buffer_result->contents());
  for (unsigned int i = 0; i < ARRAY_LENGTH; ++i) {
    if (auto const expected = a[i] + b[i]; result[i] != expected) {
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
  return {contents, contents + ARRAY_LENGTH};
}

std::vector<float> MetalAdder::buffer_B() const
{
  auto contents = static_cast<float*>(m_buffer_B->contents());
  return {contents, contents + ARRAY_LENGTH};
}

void MetalAdder::encode_add_command(MTL::ComputeCommandEncoder& encoder) const
{
  encoder.setComputePipelineState(m_pipeline_state);
  encoder.setBuffer(m_buffer_A, 0, 0);
  encoder.setBuffer(m_buffer_B, 0, 1);
  encoder.setBuffer(m_buffer_result, 0, 2);

  auto const grid_size = MTL::Size::Make(ARRAY_LENGTH, 1, 1);

  NS::UInteger thread_groups =
      m_pipeline_state->maxTotalThreadsPerThreadgroup();
  if (thread_groups > ARRAY_LENGTH) {
    thread_groups = ARRAY_LENGTH;
  }

  auto const thread_group_size = MTL::Size::Make(thread_groups, 1, 1);

  encoder.dispatchThreads(grid_size, thread_group_size);
}

void MetalAdder::generate_random_floats(MTL::Buffer& buffer)
{
  auto const data_ptr = static_cast<float*>(buffer.contents());
  for (unsigned long i = 0; i < ARRAY_LENGTH; ++i) {
    data_ptr[i] = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
  }
}