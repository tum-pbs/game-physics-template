#include "LinePipeline.h"
#include "Renderer.h"
#include "PathFinder.h"

using namespace wgpu;
using LineVertexAttributes = ResourceManager::LineVertexAttributes;

void LinePipeline::init(Device &device, Queue &queue, TextureFormat &swapChainFormat, TextureFormat &depthTextureFormat, Buffer &cameraUniforms, Buffer &lightingUniforms)
{
    this->cameraUniforms = cameraUniforms;
    this->lightingUniforms = lightingUniforms;
    this->device = device;
    this->queue = queue;
    shaderModule = ResourceManager::loadShaderModule(resolveFile("resources/line_shader.wgsl"), device);
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

    initBindGroupLayout();
    initBindGroup();

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
    if (linePointsBuffer != nullptr)
        linePointsBuffer.release();
    if (bindGroupLayout != nullptr)
        bindGroupLayout.release();
    if (bindGroup != nullptr)
        bindGroup.release();
}

void LinePipeline::commit()
{
    size_t count = lines.size();
    size_t new_size = count * sizeof(LineVertexAttributes);
    if (linePointsBuffer == nullptr || new_size != linePointsBuffer.getSize())
        reallocateBuffer(linePointsBuffer, new_size);
    if (count > 0)
    {
        queue.writeBuffer(linePointsBuffer, 0, lines.data(), new_size);
    }
}

void LinePipeline::clearAll()
{
    lines.clear();
}

void LinePipeline::addLine(Line line)
{
    lines.push_back(line.start);
    lines.push_back(line.end);
}

void LinePipeline::draw(RenderPassEncoder &renderPass)
{
    size_t linePointCount = linePointsBuffer.getSize() / sizeof(LineVertexAttributes);
    if (linePointCount > 0)
    {
        renderPass.setBindGroup(0, bindGroup, 0, nullptr);
        renderPass.setPipeline(pipeline);
        renderPass.setVertexBuffer(0, linePointsBuffer, 0, linePointsBuffer.getSize());

        renderPass.draw(linePointCount, 1, 0, 0);
    }
}

size_t LinePipeline::objectCount()
{
    return lines.size() / 2;
}

void LinePipeline::initBindGroupLayout()
{

    std::vector<BindGroupLayoutEntry> bindingLayoutEntries(2, Default);

    // The uniform buffer binding
    BindGroupLayoutEntry &bindingLayout = bindingLayoutEntries[0];
    bindingLayout.binding = 0;
    bindingLayout.visibility = ShaderStage::Vertex | ShaderStage::Fragment;
    bindingLayout.buffer.type = BufferBindingType::Uniform;
    bindingLayout.buffer.minBindingSize = sizeof(Renderer::RenderUniforms);

    // The lighting uniform buffer binding
    BindGroupLayoutEntry &lightingUniformLayout = bindingLayoutEntries[1];
    lightingUniformLayout.binding = 1;
    lightingUniformLayout.visibility = ShaderStage::Fragment;
    lightingUniformLayout.buffer.type = BufferBindingType::Uniform;
    lightingUniformLayout.buffer.minBindingSize = sizeof(Renderer::LightingUniforms);

    // Create a bind group layout
    BindGroupLayoutDescriptor bindGroupLayoutDesc{};
    bindGroupLayoutDesc.entryCount = (uint32_t)bindingLayoutEntries.size();
    bindGroupLayoutDesc.entries = bindingLayoutEntries.data();
    bindGroupLayout = device.createBindGroupLayout(bindGroupLayoutDesc);

    if (bindGroupLayout == nullptr)
        throw std::runtime_error("Could not create bind group layout!");
}

void LinePipeline::initBindGroup()
{
    std::vector<BindGroupEntry> bindings(2);

    bindings[0].binding = 0;
    bindings[0].buffer = cameraUniforms;
    bindings[0].offset = 0;
    bindings[0].size = sizeof(Renderer::RenderUniforms);

    bindings[1].binding = 1;
    bindings[1].buffer = lightingUniforms;
    bindings[1].offset = 0;
    bindings[1].size = sizeof(Renderer::LightingUniforms);

    BindGroupDescriptor bindGroupDesc;
    bindGroupDesc.layout = bindGroupLayout;
    bindGroupDesc.entryCount = (uint32_t)bindings.size();
    bindGroupDesc.entries = bindings.data();
    bindGroup = device.createBindGroup(bindGroupDesc);

    if (bindGroup == nullptr)
        throw std::runtime_error("Could not create bind group!");
}
