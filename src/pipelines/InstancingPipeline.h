#pragma once
#include <webgpu/webgpu.hpp>
#include <ResourceManager.h>
#include "Primitives.h"

class InstancingPipeline
{
public:
    void init(wgpu::Device &device, wgpu::Queue &queue, wgpu::TextureFormat &swapChainFormat, wgpu::TextureFormat &depthTextureFormat, wgpu::Buffer &cameraUniforms, wgpu::Buffer &lightingUniforms);
    void addCube(ResourceManager::InstancedVertexAttributes cube);
    void addSphere(ResourceManager::InstancedVertexAttributes sphere);
    void addQuad(ResourceManager::InstancedVertexAttributes quad);
    void clearAll();
    void commit();
    void terminate();
    void draw(wgpu::RenderPassEncoder &renderPass);

private:
    std::vector<std::vector<ResourceManager::InstancedVertexAttributes>> instances;

    wgpu::Device device = nullptr;
    wgpu::Queue queue = nullptr;

    wgpu::ShaderModule shaderModule = nullptr;
    wgpu::RenderPipeline pipeline = nullptr;

    wgpu::Buffer cameraUniforms = nullptr;
    wgpu::Buffer lightingUniforms = nullptr;

    std::vector<wgpu::Buffer> instanceBuffers;
    std::vector<wgpu::Buffer> vertexBuffers;
    std::vector<wgpu::Buffer> indexBuffers;

    wgpu::BindGroupLayout bindGroupLayout = nullptr;
    wgpu::BindGroup bindGroup = nullptr;

    void addPrimitive(VertexNormalList vertexData, TriangleList triangles);
    void terminateGeometry();
    void initGeometry();

    void update(std::vector<ResourceManager::InstancedVertexAttributes> &instances, wgpu::Buffer &instanceBuffer);

    void initBindGroupLayout();
    void initBindGroup();

    void reallocateBuffer(wgpu::Buffer &buffer, size_t size);
    void drawInstanced(wgpu::RenderPassEncoder renderPass, wgpu::Buffer &instanceBuffer, wgpu::Buffer &vertexBuffer, wgpu::Buffer &indexBuffer, size_t instances);
};