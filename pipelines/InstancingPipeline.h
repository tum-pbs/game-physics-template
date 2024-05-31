#include <webgpu/webgpu.hpp>
#include <ResourceManager.h>

class InstancingPipeline
{
public:
    wgpu::ShaderModule shaderModule = nullptr;
    wgpu::RenderPipeline pipeline = nullptr;

    wgpu::Buffer cubeInstanceBuffer = nullptr;
    wgpu::Buffer cubeVertexBuffer = nullptr;
    wgpu::Buffer cubeIndexBuffer = nullptr;

    wgpu::Buffer sphereInstanceBuffer = nullptr;
    wgpu::Buffer sphereVertexBuffer = nullptr;
    wgpu::Buffer sphereIndexBuffer = nullptr;

    wgpu::Buffer quadInstanceBuffer = nullptr;
    wgpu::Buffer quadVertexBuffer = nullptr;
    wgpu::Buffer quadIndexBuffer = nullptr;

    void init(wgpu::Device &device, wgpu::TextureFormat &swapChainFormat, wgpu::TextureFormat &depthTextureFormat, wgpu::BindGroupLayout &bindGroupLayout);
    void terminate();
    void updateCubes(wgpu::Device &device, wgpu::Queue &queue, std::vector<ResourceManager::InstancedVertexAttributes> &cubes);
    void drawCubes(wgpu::RenderPassEncoder renderPass);
    void updateSpheres(wgpu::Device &device, wgpu::Queue &queue, std::vector<ResourceManager::InstancedVertexAttributes> &spheres);
    void drawSpheres(wgpu::RenderPassEncoder renderPass);
    void updateQuads(wgpu::Device &device, wgpu::Queue &queue, std::vector<ResourceManager::InstancedVertexAttributes> &quads);
    void drawQuads(wgpu::RenderPassEncoder renderPass);
    void initGeometry(wgpu::Device &device, wgpu::Queue &queue);
    void terminateGeometry();

private:
    int cubeInstances = 0;
    int sphereInstances = 0;
    int quadInstances = 0;
};