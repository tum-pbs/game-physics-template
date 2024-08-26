#pragma once
#include <webgpu/webgpu.hpp>
class Pipeline
{
public:
    virtual size_t objectCount() = 0;
    virtual void draw(wgpu::RenderPassEncoder &renderPass) = 0;
    virtual void commit() = 0;
    virtual void terminate() = 0;
    virtual void clearAll() = 0;

protected:
    void reallocateBuffer(wgpu::Buffer &buffer, size_t size);
    wgpu::RenderPipeline pipeline = nullptr;
    wgpu::ShaderModule shaderModule = nullptr;
    wgpu::Device device = nullptr;
    wgpu::Queue queue = nullptr;
    wgpu::TextureFormat swapChainFormat = wgpu::TextureFormat::Undefined;
};