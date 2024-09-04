#include "InstancingPipeline.h"
#include "Renderer.h"
#include <numeric>

#ifndef RESOURCE_DIR
#define RESOURCE_DIR "this will be defined by cmake depending on the build type. This define is to disable error squiggles"
#endif

using namespace wgpu;
using PrimitiveVertexAttributes = ResourceManager::PrimitiveVertexAttributes;
using InstancedVertexAttributes = ResourceManager::InstancedVertexAttributes;

void InstancingPipeline::init(Device &device, Queue &queue, TextureFormat &swapChainFormat, TextureFormat &depthTextureFormat, wgpu::Buffer &cameraUniforms, wgpu::Buffer &lightingUniforms)
{
    this->cameraUniforms = cameraUniforms;
    this->lightingUniforms = lightingUniforms;
    this->device = device;
    this->queue = queue;
    shaderModule = ResourceManager::loadShaderModule(RESOURCE_DIR "/instancing_shader.wgsl", device);
    RenderPipelineDescriptor pipelineDesc;

    std::vector<VertexAttribute> lineVertexAttribs(2);

    lineVertexAttribs[0].shaderLocation = 0;
    lineVertexAttribs[0].format = VertexFormat::Float32x3;
    lineVertexAttribs[0].offset = offsetof(PrimitiveVertexAttributes, position);

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

    initBindGroupLayout();
    initBindGroup();

    PipelineLayoutDescriptor layoutDesc{};
    layoutDesc.bindGroupLayoutCount = 1;
    layoutDesc.bindGroupLayouts = (WGPUBindGroupLayout *)&bindGroupLayout;
    PipelineLayout layout = device.createPipelineLayout(layoutDesc);
    pipelineDesc.layout = layout;

    pipeline = device.createRenderPipeline(pipelineDesc);

    if (pipeline == nullptr)
        throw std::runtime_error("Failed to create instancing pipeline");

    initGeometry();
}

void InstancingPipeline::terminate()
{
    if (shaderModule != nullptr)
        shaderModule.release();
    if (pipeline != nullptr)
        pipeline.release();
    for (auto &buffer : instanceBuffers)
    {
        if (buffer != nullptr)
        {
            buffer.destroy();
            buffer.release();
        }
    }
    if (bindGroupLayout != nullptr)
        bindGroupLayout.release();
    if (bindGroup != nullptr)
        bindGroup.release();
    terminateGeometry();
}

void InstancingPipeline::addCube(ResourceManager::InstancedVertexAttributes cube)
{
    instances[0].push_back(cube);
}

void InstancingPipeline::addSphere(ResourceManager::InstancedVertexAttributes sphere)
{
    instances[1].push_back(sphere);
}

void InstancingPipeline::addQuad(ResourceManager::InstancedVertexAttributes quad)
{
    instances[2].push_back(quad);
}

template <typename T>
std::vector<size_t> sort_indices(const std::vector<T> &v)
{
    using namespace std;
    vector<size_t>
        idx(v.size());
    iota(idx.begin(), idx.end(), 0);

    stable_sort(idx.begin(), idx.end(),
                [&v](size_t i1, size_t i2)
                { return v[i1] < v[i2]; });

    return idx;
}

void InstancingPipeline::sortDepth()
{
    for (auto &instanceList : instances)
    {
        std::vector<float> depths;
        depths.reserve(instanceList.size());
        for (auto &instance : instanceList)
        {
            glm::vec3 delta = Renderer::camera.position - instance.position;
            float depth = -glm::dot(delta, delta);
            depths.push_back(depth);
        }
        auto indices = sort_indices(depths);
        std::vector<ResourceManager::InstancedVertexAttributes> newOrder;
        newOrder.resize(instanceList.size());
        for (auto &index : indices)
        {
            newOrder.push_back(instanceList[index]);
        }
        instanceList = newOrder;
    }
}

void InstancingPipeline::clearAll()
{
    for (auto &instanceList : instances)
    {
        instanceList.clear();
    }
}

void InstancingPipeline::update(std::vector<ResourceManager::InstancedVertexAttributes> &instances, wgpu::Buffer &instanceBuffer)
{
    size_t count = instances.size();
    size_t new_size = count * sizeof(InstancedVertexAttributes);
    if (instanceBuffer == nullptr || new_size != instanceBuffer.getSize())
        reallocateBuffer(instanceBuffer, new_size);
    if (count > 0)
        queue.writeBuffer(instanceBuffer, 0, instances.data(), new_size);
}

void InstancingPipeline::commit()
{
    for (size_t i = 0; i < instanceBuffers.size(); i++)
    {
        update(instances[i], instanceBuffers[i]);
    }
}

void InstancingPipeline::draw(RenderPassEncoder &renderPass)
{
    for (size_t i = 0; i < instanceBuffers.size(); i++)
    {
        drawInstanced(renderPass, instanceBuffers[i], vertexBuffers[i], indexBuffers[i], instanceBuffers[i].getSize() / sizeof(InstancedVertexAttributes));
    }
}

size_t InstancingPipeline::objectCount()
{
    size_t total = 0;
    for (auto &instanceList : instances)
    {
        total += instanceList.size();
    }
    return total;
}

void InstancingPipeline::addPrimitive(VertexNormalList vertexData, TriangleList triangles)
{
    BufferDescriptor vertexBufferDescriptor;
    vertexBufferDescriptor.size = vertexData.size() * sizeof(PrimitiveVertexAttributes);
    vertexBufferDescriptor.usage = BufferUsage::CopyDst | BufferUsage::Vertex;
    vertexBufferDescriptor.mappedAtCreation = false;
    Buffer vertexBuffer = device.createBuffer(vertexBufferDescriptor);
    queue.writeBuffer(vertexBuffer, 0, vertexData.data(), vertexBufferDescriptor.size);
    vertexBuffers.push_back(vertexBuffer);
    if (vertexBuffer == nullptr)
        throw std::runtime_error("Failed to create vertex buffer");

    BufferDescriptor indexBufferDescriptor;
    indexBufferDescriptor.size = triangles.size() * sizeof(Triangle);
    indexBufferDescriptor.usage = BufferUsage::CopyDst | BufferUsage::Index;
    indexBufferDescriptor.mappedAtCreation = false;
    Buffer indexBuffer = device.createBuffer(indexBufferDescriptor);
    queue.writeBuffer(indexBuffer, 0, triangles.data(), indexBufferDescriptor.size);
    indexBuffers.push_back(indexBuffer);

    if (indexBuffer == nullptr)
        throw std::runtime_error("Failed to create index buffer");

    instanceBuffers.push_back(nullptr);
    instances.push_back({});
}

void InstancingPipeline::initGeometry()
{
    // Load cube geometry
    addPrimitive(cube::vertices, cube::triangles);

    // Load sphere geometry
    IndexedMesh sphereMesh = make_icosphere(2);
    VertexNormalList sphereVertices = sphereMesh.first;
    TriangleList sphereTriangles = sphereMesh.second;
    addPrimitive(sphereVertices, sphereTriangles);

    // Load quad geometry
    addPrimitive(quad::vertices, quad::triangles);
}

void InstancingPipeline::terminateGeometry()
{
    for (auto &buffer : vertexBuffers)
    {
        buffer.destroy();
        buffer.release();
    }
    for (auto &buffer : indexBuffers)
    {
        buffer.destroy();
        buffer.release();
    }
}

void InstancingPipeline::initBindGroupLayout()
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

void InstancingPipeline::initBindGroup()
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

void InstancingPipeline::drawInstanced(wgpu::RenderPassEncoder renderPass, wgpu::Buffer &instanceBuffer, wgpu::Buffer &vertexBuffer, wgpu::Buffer &indexBuffer, size_t instances)
{
    if (instances > 0)
    {
        renderPass.setPipeline(pipeline);
        renderPass.setBindGroup(0, bindGroup, 0, nullptr);
        renderPass.setVertexBuffer(0, vertexBuffer, 0, vertexBuffer.getSize());
        renderPass.setVertexBuffer(1, instanceBuffer, 0, instances * sizeof(InstancedVertexAttributes));
        renderPass.setIndexBuffer(indexBuffer, IndexFormat::Uint16, 0, indexBuffer.getSize());
        renderPass.drawIndexed(static_cast<uint32_t>(indexBuffer.getSize() / sizeof(uint16_t)), static_cast<uint32_t>(instances), 0, 0, 0);
    }
}