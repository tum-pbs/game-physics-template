#include <webgpu/webgpu.hpp>

class PostProcessingPipeline
{
public:
    wgpu::ShaderModule shaderModule = nullptr;
    wgpu::RenderPipeline pipeline = nullptr;
    bool init(wgpu::Device &device_, wgpu::TextureFormat &swapChainFormat_, wgpu::TextureView &textureView);
    void draw(wgpu::RenderPassEncoder &renderPass);
    void terminate();
    void resize(int width, int height);

private:
    wgpu::Device device = nullptr;

    wgpu::Sampler sampler = nullptr;

    wgpu::Buffer cameraUniforms = nullptr;
    wgpu::Buffer lightingUniforms = nullptr;

    wgpu::BindGroupLayout bindGroupLayout = nullptr;
    wgpu::BindGroup bindGroup = nullptr;

    void initBindGroupLayout();
    void PostProcessingPipeline::updateBindGroup(wgpu::TextureView &textureView);
};