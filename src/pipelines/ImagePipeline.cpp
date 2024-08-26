#include "ImagePipeline.h"

#ifndef RESOURCE_DIR
#define RESOURCE_DIR "this will be defined by cmake depending on the build type. This define is to disable error squiggles"
#endif

using namespace wgpu;

void ImagePipeline::reallocateBuffer(Buffer &buffer, size_t count)
{
    if (buffer != nullptr)
    {
        buffer.destroy();
        buffer = nullptr;
    }
    BufferDescriptor bufferDesc;
    bufferDesc.size = sizeof(ResourceManager::ImageAttributes) * count;
    bufferDesc.usage = BufferUsage::Vertex | BufferUsage::CopyDst;
    bufferDesc.mappedAtCreation = false;
    buffer = device.createBuffer(bufferDesc);
}

bool ImagePipeline::init(Device &device, TextureFormat &swapChainFormat, Queue &queue)
{
    this->device = device;
    this->queue = queue;
    shaderModule = ResourceManager::loadShaderModule(RESOURCE_DIR "/image_shader.wgsl", device);
    RenderPipelineDescriptor pipelineDesc;
    pipelineDesc.label = "Image pipeline";

    std::vector<VertexAttribute> instanceAttribs(8);

    instanceAttribs[0].shaderLocation = 0;
    instanceAttribs[0].format = VertexFormat::Float32;
    instanceAttribs[0].offset = offsetof(ResourceManager::ImageAttributes, x);

    instanceAttribs[1].shaderLocation = 1;
    instanceAttribs[1].format = VertexFormat::Float32;
    instanceAttribs[1].offset = offsetof(ResourceManager::ImageAttributes, y);

    instanceAttribs[2].shaderLocation = 2;
    instanceAttribs[2].format = VertexFormat::Float32;
    instanceAttribs[2].offset = offsetof(ResourceManager::ImageAttributes, sx);

    instanceAttribs[3].shaderLocation = 3;
    instanceAttribs[3].format = VertexFormat::Float32;
    instanceAttribs[3].offset = offsetof(ResourceManager::ImageAttributes, sy);

    instanceAttribs[4].shaderLocation = 4;
    instanceAttribs[4].format = VertexFormat::Sint32;
    instanceAttribs[4].offset = offsetof(ResourceManager::ImageAttributes, offset);

    instanceAttribs[5].shaderLocation = 5;
    instanceAttribs[5].format = VertexFormat::Sint32;
    instanceAttribs[5].offset = offsetof(ResourceManager::ImageAttributes, width);

    instanceAttribs[6].shaderLocation = 6;
    instanceAttribs[6].format = VertexFormat::Sint32;
    instanceAttribs[6].offset = offsetof(ResourceManager::ImageAttributes, height);

    instanceAttribs[7].shaderLocation = 7;
    instanceAttribs[7].format = VertexFormat::Float32;
    instanceAttribs[7].offset = offsetof(ResourceManager::ImageAttributes, _pad);

    VertexBufferLayout instanceBufferLayout;
    instanceBufferLayout.attributeCount = (uint32_t)instanceAttribs.size();
    instanceBufferLayout.attributes = instanceAttribs.data();
    instanceBufferLayout.arrayStride = sizeof(ResourceManager::ImageAttributes);
    instanceBufferLayout.stepMode = VertexStepMode::Instance;

    pipelineDesc.vertex.bufferCount = 1;
    pipelineDesc.vertex.buffers = &instanceBufferLayout;

    pipelineDesc.vertex.module = shaderModule;
    pipelineDesc.vertex.entryPoint = "vs_main";
    pipelineDesc.vertex.constantCount = 0;
    pipelineDesc.vertex.constants = nullptr;

    pipelineDesc.primitive.topology = PrimitiveTopology::TriangleList;
    pipelineDesc.primitive.stripIndexFormat = IndexFormat::Undefined;
    pipelineDesc.primitive.cullMode = CullMode::Back;

    FragmentState fragmentState;
    pipelineDesc.fragment = &fragmentState;
    fragmentState.module = shaderModule;
    fragmentState.entryPoint = "fs_main";
    fragmentState.constantCount = 0;
    fragmentState.constants = nullptr;

    BlendState blendState;
    blendState.color.srcFactor = BlendFactor::SrcAlpha;
    blendState.color.dstFactor = BlendFactor::OneMinusSrcAlpha;
    blendState.color.operation = BlendOperation::Add;
    blendState.alpha.srcFactor = BlendFactor::Zero;
    blendState.alpha.dstFactor = BlendFactor::One;
    blendState.alpha.operation = BlendOperation::Add;

    ColorTargetState colorTarget;
    colorTarget.format = swapChainFormat;
    colorTarget.blend = &blendState;
    colorTarget.writeMask = ColorWriteMask::All;

    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;

    pipelineDesc.depthStencil = nullptr;

    pipelineDesc.multisample.count = 1;
    pipelineDesc.multisample.mask = ~0u;
    pipelineDesc.multisample.alphaToCoverageEnabled = false;

    createLayout();

    PipelineLayoutDescriptor layoutDesc{};
    layoutDesc.bindGroupLayoutCount = 1;
    layoutDesc.bindGroupLayouts = (WGPUBindGroupLayout *)&bindGroupLayout;
    PipelineLayout layout = device.createPipelineLayout(layoutDesc);
    pipelineDesc.layout = layout;

    pipeline = device.createRenderPipeline(pipelineDesc);

    return pipeline != nullptr;
}

void ImagePipeline::draw(RenderPassEncoder &renderPass)
{
    if (images.size() == 0)
        return;
    renderPass.setPipeline(pipeline);
    for (size_t i = 0; i < images.size(); i++)
    {
        renderPass.setBindGroup(0, bindGroups[i], 0, nullptr);
        renderPass.setVertexBuffer(0, imageBuffer, sizeof(ResourceManager::ImageAttributes) * i, sizeof(ResourceManager::ImageAttributes));
        renderPass.draw(6, 1, 0, 0);
    }
}

void ImagePipeline::updateImages(std::vector<ResourceManager::ImageAttributes> &images, std::vector<float> &data)
{
    prevImages = this->images;
    this->images = images;
    this->data = data;
    if (!isPrevLayout())
    {
        createTextures();
        createTextureViews();
        createBindGroups();
        reallocateBuffer(imageBuffer, images.size());
    }

    if (images.size() > 0)
    {
        queue.writeBuffer(imageBuffer, 0, images.data(), sizeof(ResourceManager::ImageAttributes) * images.size());
        copyDataToTextures();
    }
}

void ImagePipeline::copyDataToTexture(ResourceManager::ImageAttributes &image, std::vector<float> &data, Texture &texture)
{
    ImageCopyTexture destination;
    destination.texture = texture;
    destination.aspect = TextureAspect::All;
    destination.mipLevel = 0;
    destination.origin = {0, 0, 0};

    TextureDataLayout source;
    source.offset = 0;
    source.bytesPerRow = image.width * sizeof(float);
    source.rowsPerImage = image.height;

    size_t size = image.width * image.height * sizeof(float);

    queue.writeTexture(destination, data.data() + image.offset, size, source, {static_cast<uint32_t>(image.width), static_cast<uint32_t>(image.height), 1u});
}

void ImagePipeline::terminate()
{
    if (shaderModule != nullptr)
        shaderModule.release();
    if (pipeline != nullptr)
        pipeline.release();
    for (auto &texture : textures)
    {
        if (texture != nullptr)
            texture.release();
    }
    for (auto &textureView : textureViews)
    {
        if (textureView != nullptr)
            textureView.release();
    }
    for (auto &bindGroup : bindGroups)
    {
        if (bindGroup != nullptr)
            bindGroup.release();
    }
    if (sampler != nullptr)
        sampler.release();
    if (bindGroupLayout != nullptr)
        bindGroupLayout.release();
    if (imageBuffer != nullptr)
        imageBuffer.release();
}

void ImagePipeline::createLayout()
{
    BindGroupLayoutEntry layoutEntry0;
    layoutEntry0.binding = 0;
    layoutEntry0.visibility = ShaderStage::Fragment;
    layoutEntry0.texture.sampleType = TextureSampleType::UnfilterableFloat;
    layoutEntry0.texture.viewDimension = TextureViewDimension::_2D;

    BindGroupLayoutDescriptor layoutDescriptor{};
    layoutDescriptor.entryCount = 1;
    layoutDescriptor.entries = &layoutEntry0;
    bindGroupLayout = device.createBindGroupLayout(layoutDescriptor);
}

void ImagePipeline::createBindGroups()
{
    for (auto &bindGroup : bindGroups)
    {
        if (bindGroup != nullptr)
            bindGroup.release();
    }
    bindGroups.clear();
    for (auto &textureView : textureViews)
    {
        BindGroupEntry entry;
        entry.binding = 0;
        entry.textureView = textureView;

        BindGroupDescriptor bindGroupDesc;
        bindGroupDesc.layout = bindGroupLayout;
        bindGroupDesc.entryCount = 1;
        bindGroupDesc.entries = &entry;
        bindGroups.push_back(device.createBindGroup(bindGroupDesc));
    }
}

bool ImagePipeline::isPrevLayout()
{
    if (images.size() != prevImages.size())
        return false;

    for (size_t i = 0; i < images.size(); i++)
    {
        if (images[i].width != prevImages[i].width)
            return false;
        if (images[i].height != prevImages[i].height)
            return false;
    }
    return true;
}

void ImagePipeline::createTextures()
{
    for (auto &texture : textures)
    {
        if (texture != nullptr)
            texture.release();
    }
    textures.clear();

    for (auto &image : images)
    {
        textures.push_back(createTexture(image));
    }
}

Texture ImagePipeline::createTexture(ResourceManager::ImageAttributes &image)
{
    TextureDescriptor textureDesc;
    textureDesc.dimension = TextureDimension::_2D;
    textureDesc.size = {(uint32_t)image.width, (uint32_t)image.height, 1u};
    textureDesc.mipLevelCount = 1;
    textureDesc.sampleCount = 1;
    textureDesc.format = TextureFormat::R32Float;
    textureDesc.usage = TextureUsage::TextureBinding | TextureUsage::CopyDst;
    textureDesc.viewFormatCount = 0;
    textureDesc.viewFormats = nullptr;
    return device.createTexture(textureDesc);
}

TextureView ImagePipeline::createTextureView(Texture &texture)
{
    TextureViewDescriptor textureViewDesc;
    textureViewDesc.aspect = TextureAspect::All;
    textureViewDesc.baseArrayLayer = 0;
    textureViewDesc.arrayLayerCount = 1;
    textureViewDesc.baseMipLevel = 0;
    textureViewDesc.mipLevelCount = 1;
    textureViewDesc.dimension = TextureViewDimension::_2D;
    textureViewDesc.format = TextureFormat::R32Float;
    return texture.createView(textureViewDesc);
}

void ImagePipeline::createTextureViews()
{
    for (auto &textureView : textureViews)
    {
        if (textureView != nullptr)
            textureView.release();
    }
    textureViews.clear();

    for (auto &texture : textures)
    {
        textureViews.push_back(createTextureView(texture));
    }
}

void ImagePipeline::copyDataToTextures()
{
    for (size_t i = 0; i < images.size(); i++)
    {
        copyDataToTexture(images[i], data, textures[i]);
    }
}
