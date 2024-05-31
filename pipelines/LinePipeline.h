#include <webgpu/webgpu.hpp>
#include "ResourceManager.h"

class LinePipeline
{
public:
    wgpu::ShaderModule shaderModule = nullptr;
    wgpu::RenderPipeline pipeline = nullptr;

    wgpu::Buffer lineVertexBuffer = nullptr;

    bool init(wgpu::Device &device, wgpu::TextureFormat &swapChainFormat, wgpu::TextureFormat &depthTextureFormat, wgpu::BindGroupLayout &bindGroupLayout);
    void terminate();
    void updateLines(wgpu::Device &device, wgpu::Queue &queue, std::vector<ResourceManager::LineVertexAttributes> &lines);
    void drawLines(wgpu::RenderPassEncoder renderPass);

private:
    int lineCount = 0;
};