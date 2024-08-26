#include <webgpu/webgpu.hpp>

class PostProcessingPipeline
{
public:
    bool init(wgpu::Device &device, wgpu::TextureFormat &swapChainFormat, wgpu::TextureView &textureView);
    void draw(wgpu::RenderPassEncoder &renderPass);
    void terminate();
    void updateBindGroup(wgpu::TextureView &textureView);

private:
    wgpu::ShaderModule shaderModule = nullptr;
    wgpu::RenderPipeline pipeline = nullptr;
    wgpu::Device device = nullptr;

    wgpu::Sampler sampler = nullptr;

    wgpu::Buffer cameraUniforms = nullptr;
    wgpu::Buffer lightingUniforms = nullptr;

    wgpu::BindGroupLayout bindGroupLayout = nullptr;
    wgpu::BindGroup bindGroup = nullptr;

    void initBindGroupLayout();
};