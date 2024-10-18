#pragma once
#include "ResourceManager.h"
#include <vector>
#include "Colormap.h"
#include "Pipeline.h"

class ImagePipeline : public Pipeline
{
public:
    bool init(wgpu::Device &device, wgpu::TextureFormat &swapChainFormat, wgpu::Queue &queue);
    void addImage(std::vector<float> &data, glm::vec2 position, glm::vec2 scale, size_t width, size_t height, Colormap colormap);
    void draw(wgpu::RenderPassEncoder &renderPass) override;
    void commit() override;
    void clearAll() override;
    void terminate() override;
    size_t objectCount() override;

private:
    std::vector<ResourceManager::ImageAttributes> images;
    std::vector<ResourceManager::ImageAttributes> prevImages;
    std::vector<float> data;

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
};