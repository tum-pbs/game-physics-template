#include "PostProcessingPipeline.h"
#include "ResourceManager.h"

#ifndef RESOURCE_DIR
#define RESOURCE_DIR "this will be defined by cmake depending on the build type. This define is to disable error squiggles"
#endif

using namespace wgpu;

bool PostProcessingPipeline::init(wgpu::Device &device_, wgpu::TextureFormat &swapChainFormat_, TextureView &textureView)
{
    device = device_;
    shaderModule = ResourceManager::loadShaderModule(RESOURCE_DIR "/post_processing.wgsl", device);
    RenderPipelineDescriptor pipelineDesc;
    pipelineDesc.label = "Post process pipeline";

    pipelineDesc.vertex.bufferCount = 0;
    pipelineDesc.vertex.buffers = nullptr;

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
    colorTarget.format = swapChainFormat_;
    colorTarget.blend = &blendState;
    colorTarget.writeMask = ColorWriteMask::All;

    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;

    pipelineDesc.depthStencil = nullptr;

    pipelineDesc.multisample.count = 1;
    pipelineDesc.multisample.mask = ~0u;
    pipelineDesc.multisample.alphaToCoverageEnabled = false;

    initBindGroupLayout();
    updateBindGroup(textureView);

    // Create the pipeline layout
    PipelineLayoutDescriptor layoutDesc{};
    layoutDesc.bindGroupLayoutCount = 1;
    layoutDesc.bindGroupLayouts = (WGPUBindGroupLayout *)&bindGroupLayout;
    PipelineLayout layout = device.createPipelineLayout(layoutDesc);
    pipelineDesc.layout = layout;

    pipeline = device.createRenderPipeline(pipelineDesc);

    return pipeline != nullptr;
}

void PostProcessingPipeline::draw(wgpu::RenderPassEncoder &renderPass)
{
    renderPass.setPipeline(pipeline);
    renderPass.setBindGroup(0, bindGroup, 0, nullptr);
    renderPass.draw(6, 1, 0, 0);
}

void PostProcessingPipeline::terminate()
{
    if (shaderModule != nullptr)
        shaderModule.release();
    if (pipeline != nullptr)
        pipeline.release();
    if (sampler != nullptr)
        sampler.release();
    if (bindGroup != nullptr)
        bindGroup.release();
}

void PostProcessingPipeline::initBindGroupLayout()
{
    std::vector<BindGroupLayoutEntry> bindingLayoutEntriesPost(2, Default);

    BindGroupLayoutEntry &bindingLayoutPost = bindingLayoutEntriesPost[0];
    bindingLayoutPost.binding = 0;
    bindingLayoutPost.visibility = ShaderStage::Fragment;
    bindingLayoutPost.texture.sampleType = TextureSampleType::Float;
    bindingLayoutPost.texture.viewDimension = TextureViewDimension::_2D;

    BindGroupLayoutEntry &bindingLayoutPost2 = bindingLayoutEntriesPost[1];
    bindingLayoutPost2.binding = 1;
    bindingLayoutPost2.visibility = ShaderStage::Fragment;
    bindingLayoutPost2.sampler.type = SamplerBindingType::Filtering;

    BindGroupLayoutDescriptor bindGroupLayoutDescPost{};
    bindGroupLayoutDescPost.entryCount = (uint32_t)bindingLayoutEntriesPost.size();
    bindGroupLayoutDescPost.entries = bindingLayoutEntriesPost.data();
    bindGroupLayout = device.createBindGroupLayout(bindGroupLayoutDescPost);

    if (bindGroupLayout == nullptr)
        throw std::runtime_error("Could not create post bind group layout!");
}

void PostProcessingPipeline::updateBindGroup(wgpu::TextureView &textureView)
{

    if (sampler != nullptr)
        sampler.release();
    if (bindGroup != nullptr)
        bindGroup.release();

    std::vector<BindGroupEntry> bindingsPost(2);

    bindingsPost[0].binding = 0;
    bindingsPost[0].textureView = textureView;

    SamplerDescriptor samplerDesc;
    samplerDesc.addressModeU = AddressMode::ClampToEdge;
    samplerDesc.addressModeV = AddressMode::ClampToEdge;
    samplerDesc.addressModeW = AddressMode::ClampToEdge;
    samplerDesc.magFilter = FilterMode::Linear;
    samplerDesc.minFilter = FilterMode::Linear;
    samplerDesc.mipmapFilter = MipmapFilterMode::Linear;
    samplerDesc.lodMinClamp = 0.0f;
    samplerDesc.lodMaxClamp = 1.0f;
    samplerDesc.compare = CompareFunction::Undefined;
    samplerDesc.maxAnisotropy = 1;
    sampler = device.createSampler(samplerDesc);

    bindingsPost[1].binding = 1;
    bindingsPost[1].sampler = sampler;

    BindGroupDescriptor bindGroupDescPost;
    bindGroupDescPost.layout = bindGroupLayout;
    bindGroupDescPost.entryCount = (uint32_t)bindingsPost.size();
    bindGroupDescPost.entries = bindingsPost.data();
    bindGroup = device.createBindGroup(bindGroupDescPost);

    if (sampler == nullptr)
        throw std::runtime_error("Could not create post process sampler!");
    if (bindGroup == nullptr)
        throw std::runtime_error("Could not create post process bind group!");
}