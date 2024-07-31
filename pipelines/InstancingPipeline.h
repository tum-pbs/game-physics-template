#pragma once
#include <webgpu/webgpu.hpp>
#include <ResourceManager.h>

class InstancingPipeline
{
public:
    void init(wgpu::Device &device_, wgpu::Queue &queue_, wgpu::TextureFormat &swapChainFormat, wgpu::TextureFormat &depthTextureFormat, wgpu::Buffer &cameraUniforms_, wgpu::Buffer &lightingUniforms_);
    void terminate();
    void updateCubes(std::vector<ResourceManager::InstancedVertexAttributes> &cubes);
    void drawCubes(wgpu::RenderPassEncoder renderPass);
    void updateSpheres(std::vector<ResourceManager::InstancedVertexAttributes> &spheres);
    void drawSpheres(wgpu::RenderPassEncoder renderPass);
    void updateQuads(std::vector<ResourceManager::InstancedVertexAttributes> &quads);
    void drawQuads(wgpu::RenderPassEncoder renderPass);
    void initGeometry();
    void terminateGeometry();

private:
    size_t cubeInstances = 0;
    size_t sphereInstances = 0;
    size_t quadInstances = 0;
    size_t prevCubeInstances = 0;
    size_t prevSphereInstances = 0;
    size_t prevQuadInstances = 0;

    wgpu::Device device = nullptr;
    wgpu::Queue queue = nullptr;

    wgpu::ShaderModule shaderModule = nullptr;
    wgpu::RenderPipeline pipeline = nullptr;

    wgpu::Buffer cameraUniforms = nullptr;
    wgpu::Buffer lightingUniforms = nullptr;

    wgpu::Buffer instanceBuffer = nullptr;
    wgpu::Buffer dataBuffer = nullptr;
    wgpu::Buffer cubeIndexBuffer = nullptr;

    wgpu::Buffer sphereInstanceBuffer = nullptr;
    wgpu::Buffer sphereVertexBuffer = nullptr;
    wgpu::Buffer sphereIndexBuffer = nullptr;

    wgpu::Buffer quadInstanceBuffer = nullptr;
    wgpu::Buffer quadVertexBuffer = nullptr;
    wgpu::Buffer quadIndexBuffer = nullptr;

    wgpu::BindGroupLayout bindGroupLayout = nullptr;
    wgpu::BindGroup bindGroup = nullptr;

    void initBindGroupLayout();
    void initBindGroup();

    void reallocateBuffer(wgpu::Buffer &buffer, size_t count);
    void draw(wgpu::RenderPassEncoder renderPass, wgpu::Buffer &instanceBuffer, wgpu::Buffer &vertexBuffer, wgpu::Buffer &indexBuffer, size_t instances);
};