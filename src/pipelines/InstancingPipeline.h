#pragma once
#include "Pipeline.h"
#include <ResourceManager.h>
#include "Primitives.h"

class InstancingPipeline : public Pipeline
{
public:
    void init(wgpu::Device &device, wgpu::Queue &queue, wgpu::TextureFormat &swapChainFormat, wgpu::TextureFormat &depthTextureFormat, wgpu::Buffer &cameraUniforms, wgpu::Buffer &lightingUniforms);
    void addCube(ResourceManager::InstancedVertexAttributes cube);
    void addSphere(ResourceManager::InstancedVertexAttributes sphere);
    void addQuad(ResourceManager::InstancedVertexAttributes quad);
    void clearAll() override;
    void commit() override;
    void terminate() override;
    void draw(wgpu::RenderPassEncoder &renderPass) override;
    size_t objectCount() override;

private:
    std::vector<std::vector<ResourceManager::InstancedVertexAttributes>> instances;
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

    void drawInstanced(wgpu::RenderPassEncoder renderPass, wgpu::Buffer &instanceBuffer, wgpu::Buffer &vertexBuffer, wgpu::Buffer &indexBuffer, size_t instances);
};