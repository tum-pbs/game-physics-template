#pragma once
#include <webgpu/webgpu.hpp>
#include "ResourceManager.h"

struct Line
{
    ResourceManager::LineVertexAttributes start;
    ResourceManager::LineVertexAttributes end;
};
class LinePipeline
{
public:
    void init(
        wgpu::Device &device,
        wgpu::Queue &queue,
        wgpu::TextureFormat &swapChainFormat,
        wgpu::TextureFormat &depthTextureFormat,
        wgpu::Buffer &cameraUniforms,
        wgpu::Buffer &lightingUniforms);
    void terminate();
    void commit();
    void clearAll();
    void addLine(Line line);
    void draw(wgpu::RenderPassEncoder renderPass);
    size_t objectCount() { return lines.size() / 2; };

private:
    std::vector<ResourceManager::LineVertexAttributes> lines;

    wgpu::ShaderModule shaderModule = nullptr;
    wgpu::RenderPipeline pipeline = nullptr;

    wgpu::Buffer linePointsBuffer = nullptr;
    wgpu::Buffer cameraUniforms = nullptr;
    wgpu::Buffer lightingUniforms = nullptr;

    wgpu::Device device = nullptr;
    wgpu::Queue queue = nullptr;

    wgpu::BindGroupLayout bindGroupLayout = nullptr;
    wgpu::BindGroup bindGroup = nullptr;

    void initBindGroupLayout();
    void initBindGroup();

    void reallocateBuffer(wgpu::Buffer &buffer, size_t size);
};