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

    void init(wgpu::Device &device_, wgpu::Queue &queue_, wgpu::TextureFormat &swapChainFormat, wgpu::TextureFormat &depthTextureFormat, wgpu::BindGroupLayout &bindGroupLayout);
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
    int cubeInstances = 0;
    int sphereInstances = 0;
    int quadInstances = 0;

    wgpu::Device device = nullptr;
    wgpu::Queue queue = nullptr;
};