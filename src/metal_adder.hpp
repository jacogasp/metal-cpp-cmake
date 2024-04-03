#ifndef METAL_ADDER_HPP
#define METAL_ADDER_HPP

#include <Metal/Metal.hpp>
#include <vector>

constexpr unsigned int ARRAY_LENGTH = 60 * 180 * 10'000;
constexpr unsigned int BUFFER_SIZE  = ARRAY_LENGTH * sizeof(float);

class MetalAdder
{
  MTL::Device& m_device;
  MTL::ComputePipelineState* m_pipeline_state = nullptr;
  MTL::CommandQueue* m_command_queue          = nullptr;

  MTL::Buffer* m_buffer_A      = nullptr;
  MTL::Buffer* m_buffer_B      = nullptr;
  MTL::Buffer* m_buffer_result = nullptr;

 public:
  MetalAdder() = delete;

  explicit MetalAdder(MTL::Device& device);

  ~MetalAdder() = default;

  void prepare_data() const;

  void submit_command() const;

  void verify_results() const;

  [[nodiscard]] std::vector<float> buffer_A() const;

  [[nodiscard]] std::vector<float> buffer_B() const;

 private:
  void encode_add_command(MTL::ComputeCommandEncoder& encoder) const;

  static void generate_random_floats(MTL::Buffer& buffer);
};

#endif // METAL_ADDER_HPP
