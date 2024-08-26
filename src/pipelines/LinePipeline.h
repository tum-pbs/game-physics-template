#pragma once
#include "Pipeline.h"
#include "ResourceManager.h"

struct Line
{
    ResourceManager::LineVertexAttributes start;
    ResourceManager::LineVertexAttributes end;
};
class LinePipeline : public Pipeline
{
public:
    void init(
        wgpu::Device &device,
        wgpu::Queue &queue,
        wgpu::TextureFormat &swapChainFormat,
        wgpu::TextureFormat &depthTextureFormat,
        wgpu::Buffer &cameraUniforms,
        wgpu::Buffer &lightingUniforms);
    void terminate() override;
    void commit() override;
    void clearAll() override;
    void addLine(Line line);
    void draw(wgpu::RenderPassEncoder &renderPass) override;
    size_t objectCount() override;

private:
    std::vector<ResourceManager::LineVertexAttributes> lines;

    wgpu::Buffer linePointsBuffer = nullptr;
    wgpu::Buffer cameraUniforms = nullptr;
    wgpu::Buffer lightingUniforms = nullptr;

    wgpu::BindGroupLayout bindGroupLayout = nullptr;
    wgpu::BindGroup bindGroup = nullptr;

    void initBindGroupLayout();
    void initBindGroup();
};