#pragma once
#include <webgpu/webgpu.hpp>
#include "ResourceManager.h"

class LinePipeline
{
public:
    void init(
        wgpu::Device &device_,
        wgpu::Queue &queue_,
        wgpu::TextureFormat &swapChainFormat,
        wgpu::TextureFormat &depthTextureFormat,
        wgpu::Buffer &cameraUniforms,
        wgpu::Buffer &lightingUniforms);
    void terminate();
    void updateLines(std::vector<ResourceManager::LineVertexAttributes> &lines);
    void drawLines(wgpu::RenderPassEncoder renderPass);

private:
    int lineCount = 0;
    wgpu::ShaderModule shaderModule = nullptr;
    wgpu::RenderPipeline pipeline = nullptr;

    wgpu::Buffer lineVertexBuffer = nullptr;
    wgpu::Buffer cameraUniforms = nullptr;
    wgpu::Buffer lightingUniforms = nullptr;

    wgpu::Device device = nullptr;
    wgpu::Queue queue = nullptr;

    wgpu::BindGroupLayout bindGroupLayout = nullptr;
    wgpu::BindGroup bindGroup = nullptr;

    void initBindGroupLayout();
    void initBindGroup();
};