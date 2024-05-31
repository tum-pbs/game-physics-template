#include <webgpu/webgpu.hpp>

class InstancingPipeline
{
public:
    wgpu::ShaderModule shaderModule = nullptr;
    wgpu::RenderPipeline pipeline = nullptr;
    bool init(wgpu::Device &device, wgpu::TextureFormat &swapChainFormat, wgpu::TextureFormat &depthTextureFormat, wgpu::BindGroupLayout &bindGroupLayout);
    void terminate();
};