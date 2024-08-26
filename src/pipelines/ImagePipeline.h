#pragma once
#include <webgpu/webgpu.hpp>
#include "ResourceManager.h"
#include <vector>
#include "Colormap.h"

class ImagePipeline
{
public:
    bool init(wgpu::Device &device, wgpu::TextureFormat &swapChainFormat, wgpu::Queue &queue);
    void draw(wgpu::RenderPassEncoder &renderPass);
    void commit();
    void clearAll();
    void addImage(std::vector<float> &data, glm::vec2 position, glm::vec2 scale, size_t width, size_t height, Colormap colormap);
    void terminate();
    size_t objectCount() { return images.size(); };

private:
    wgpu::ShaderModule shaderModule = nullptr;
    wgpu::RenderPipeline pipeline = nullptr;

    std::vector<ResourceManager::ImageAttributes> images;
    std::vector<ResourceManager::ImageAttributes> prevImages;
    std::vector<float> data;

    wgpu::Queue queue = nullptr;
    wgpu::Device device = nullptr;
    wgpu::Buffer imageBuffer = nullptr;

    std::vector<wgpu::Texture> textures;
    std::vector<wgpu::TextureView> textureViews;

    wgpu::Texture colormapTexture = nullptr;
    wgpu::TextureView colormapTextureView = nullptr;
    wgpu::Sampler colormapSampler = nullptr;

    wgpu::BindGroupLayout bindGroupLayout = nullptr;
    std::vector<wgpu::BindGroup> bindGroups;

    void createLayout();
    void createBindGroups();
    bool isPrevLayout();

    wgpu::Texture createTexture(ResourceManager::ImageAttributes &image);
    void createTextures();
    wgpu::TextureView createTextureView(wgpu::Texture &texture);
    void createTextureViews();

    void initColormap();

    void copyDataToTextures();
    void copyDataToTexture(ResourceManager::ImageAttributes &image, std::vector<float> &data, wgpu::Texture &texture);
    void reallocateBuffer(wgpu::Buffer &buffer, size_t count);
};