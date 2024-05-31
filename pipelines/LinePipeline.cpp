#include "LinePipeline.h"
#include "ResourceManager.h"

#ifndef RESOURCE_DIR
#define RESOURCE_DIR "this will be defined by cmake depending on the build type. This define is to disable error squiggles"
#endif

using namespace wgpu;
using LineVertexAttributes = ResourceManager::LineVertexAttributes;

void LinePipeline::init(wgpu::Device &device, wgpu::TextureFormat &swapChainFormat, wgpu::TextureFormat &depthTextureFormat, wgpu::BindGroupLayout &bindGroupLayout)
{
    shaderModule = ResourceManager::loadShaderModule(RESOURCE_DIR "/line_shader.wgsl", device);
    RenderPipelineDescriptor pipelineDesc;

    // This is for instanced rendering
    std::vector<VertexAttribute> primitiveVertexAttribs(2);

    // Position attribute
    primitiveVertexAttribs[0].shaderLocation = 0;
    primitiveVertexAttribs[0].format = VertexFormat::Float32x3;
    primitiveVertexAttribs[0].offset = offsetof(LineVertexAttributes, position);

    // Color attribute
    primitiveVertexAttribs[1].shaderLocation = 1;
    primitiveVertexAttribs[1].format = VertexFormat::Float32x3;
    primitiveVertexAttribs[1].offset = offsetof(LineVertexAttributes, color);

    VertexBufferLayout lineVertexBufferLayout;
    lineVertexBufferLayout.attributeCount = (uint32_t)primitiveVertexAttribs.size();
    lineVertexBufferLayout.attributes = primitiveVertexAttribs.data();
    lineVertexBufferLayout.arrayStride = sizeof(LineVertexAttributes);
    lineVertexBufferLayout.stepMode = VertexStepMode::Vertex;

    std::vector<VertexBufferLayout> vertexBufferLayouts = {lineVertexBufferLayout};

    pipelineDesc.vertex.bufferCount = static_cast<uint32_t>(vertexBufferLayouts.size());
    pipelineDesc.vertex.buffers = vertexBufferLayouts.data();

    pipelineDesc.vertex.module = shaderModule;
    pipelineDesc.vertex.entryPoint = "vs_main";
    pipelineDesc.vertex.constantCount = 0;
    pipelineDesc.vertex.constants = nullptr;

    pipelineDesc.primitive.topology = PrimitiveTopology::LineList;
    pipelineDesc.primitive.stripIndexFormat = IndexFormat::Undefined;
    pipelineDesc.primitive.frontFace = FrontFace::CCW;
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

    if (pipeline == nullptr)
        throw std::runtime_error("Failed to create line pipeline!");
}

void LinePipeline::terminate()
{
    if (shaderModule != nullptr)
        shaderModule.release();
    if (pipeline != nullptr)
        pipeline.release();
    if (lineVertexBuffer != nullptr)
        lineVertexBuffer.release();
}

void LinePipeline::updateLines(wgpu::Device &device, wgpu::Queue &queue, std::vector<ResourceManager::LineVertexAttributes> &lines)
{
    lineCount = static_cast<int>(lines.size());
    if (lineVertexBuffer != nullptr)
    {
        lineVertexBuffer.destroy();
        lineVertexBuffer = nullptr;
    }
    BufferDescriptor lineBufferDesc;
    if (lineCount > 0)
    {
        lineBufferDesc.size = sizeof(LineVertexAttributes) * lineCount;
        lineBufferDesc.usage = BufferUsage::Vertex | BufferUsage::CopyDst;
        lineBufferDesc.mappedAtCreation = false;
        lineVertexBuffer = device.createBuffer(lineBufferDesc);
        queue.writeBuffer(lineVertexBuffer, 0, lines.data(), lineBufferDesc.size);
    }
}

void LinePipeline::drawLines(wgpu::RenderPassEncoder renderPass)
{
    if (lineCount > 0)
    {
        renderPass.setPipeline(pipeline);
        renderPass.setVertexBuffer(0, lineVertexBuffer, 0, lineCount * sizeof(LineVertexAttributes));

        renderPass.draw(lineCount, 1, 0, 0);
    }
}
