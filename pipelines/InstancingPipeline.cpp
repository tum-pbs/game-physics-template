#include "InstancingPipeline.h"
#include "ResourceManager.h"

#ifndef RESOURCE_DIR
#define RESOURCE_DIR "this will be defined by cmake depending on the build type. This define is to disable error squiggles"
#endif

using namespace wgpu;
using PrimitiveVertexAttributes = ResourceManager::PrimitiveVertexAttributes;
using InstancedVertexAttributes = ResourceManager::InstancedVertexAttributes;

bool InstancingPipeline::init(Device &device, TextureFormat &swapChainFormat, TextureFormat &depthTextureFormat, BindGroupLayout &bindGroupLayout)
{
    shaderModule = ResourceManager::loadShaderModule(RESOURCE_DIR "/instancing_shader.wgsl", device);
    RenderPipelineDescriptor pipelineDesc;

    // This is for instanced rendering
    std::vector<VertexAttribute> lineVertexAttribs(2);

    // Position attribute
    lineVertexAttribs[0].shaderLocation = 0;
    lineVertexAttribs[0].format = VertexFormat::Float32x3;
    lineVertexAttribs[0].offset = offsetof(PrimitiveVertexAttributes, position);

    // Normal attribute
    lineVertexAttribs[1].shaderLocation = 1;
    lineVertexAttribs[1].format = VertexFormat::Float32x3;
    lineVertexAttribs[1].offset = offsetof(PrimitiveVertexAttributes, normal);

    VertexBufferLayout primitiveVertexBufferLayout;
    primitiveVertexBufferLayout.attributeCount = (uint32_t)lineVertexAttribs.size();
    primitiveVertexBufferLayout.attributes = lineVertexAttribs.data();
    primitiveVertexBufferLayout.arrayStride = sizeof(PrimitiveVertexAttributes);
    primitiveVertexBufferLayout.stepMode = VertexStepMode::Vertex;

    // position, rotation, scale, color, id, flags
    std::vector<VertexAttribute> instanceAttribs(6);

    // Position attribute
    instanceAttribs[0].shaderLocation = 2;
    instanceAttribs[0].format = VertexFormat::Float32x3;
    instanceAttribs[0].offset = offsetof(InstancedVertexAttributes, position);

    // Rotation attribute
    instanceAttribs[1].shaderLocation = 3;
    instanceAttribs[1].format = VertexFormat::Float32x4;
    instanceAttribs[1].offset = offsetof(InstancedVertexAttributes, rotation);

    // Scale attribute
    instanceAttribs[2].shaderLocation = 4;
    instanceAttribs[2].format = VertexFormat::Float32x3;
    instanceAttribs[2].offset = offsetof(InstancedVertexAttributes, scale);

    // Color attribute
    instanceAttribs[3].shaderLocation = 5;
    instanceAttribs[3].format = VertexFormat::Float32x4;
    instanceAttribs[3].offset = offsetof(InstancedVertexAttributes, color);

    // ID attribute
    instanceAttribs[4].shaderLocation = 6;
    instanceAttribs[4].format = VertexFormat::Uint32;
    instanceAttribs[4].offset = offsetof(InstancedVertexAttributes, id);

    // Flags attribute
    instanceAttribs[5].shaderLocation = 7;
    instanceAttribs[5].format = VertexFormat::Uint32;
    instanceAttribs[5].offset = offsetof(InstancedVertexAttributes, flags);

    VertexBufferLayout instanceBufferLayout;
    instanceBufferLayout.attributeCount = (uint32_t)instanceAttribs.size();
    instanceBufferLayout.attributes = instanceAttribs.data();
    instanceBufferLayout.arrayStride = sizeof(InstancedVertexAttributes);
    instanceBufferLayout.stepMode = VertexStepMode::Instance;

    std::vector<VertexBufferLayout> vertexBufferLayouts = {primitiveVertexBufferLayout, instanceBufferLayout};

    pipelineDesc.vertex.bufferCount = static_cast<uint32_t>(vertexBufferLayouts.size());
    pipelineDesc.vertex.buffers = vertexBufferLayouts.data();

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

    DepthStencilState depthStencilState = Default;
    depthStencilState.depthCompare = CompareFunction::Less;
    depthStencilState.depthWriteEnabled = true;
    depthStencilState.format = depthTextureFormat;
    depthStencilState.stencilReadMask = 0;
    depthStencilState.stencilWriteMask = 0;

    pipelineDesc.depthStencil = &depthStencilState;

    pipelineDesc.multisample.count = 1;
    pipelineDesc.multisample.mask = ~0u;
    pipelineDesc.multisample.alphaToCoverageEnabled = false;

    // Create the pipeline layout
    PipelineLayoutDescriptor layoutDesc{};
    layoutDesc.bindGroupLayoutCount = 1;
    layoutDesc.bindGroupLayouts = (WGPUBindGroupLayout *)&bindGroupLayout;
    PipelineLayout layout = device.createPipelineLayout(layoutDesc);
    pipelineDesc.layout = layout;

    pipeline = device.createRenderPipeline(pipelineDesc);

    return pipeline != nullptr;
}

void InstancingPipeline::terminate()
{
    if (shaderModule != nullptr)
        shaderModule.release();
    if (pipeline != nullptr)
        pipeline.release();
}
