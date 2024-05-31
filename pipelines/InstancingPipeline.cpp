#include "InstancingPipeline.h"
#include "Primitives.h"

#ifndef RESOURCE_DIR
#define RESOURCE_DIR "this will be defined by cmake depending on the build type. This define is to disable error squiggles"
#endif

using namespace wgpu;
using PrimitiveVertexAttributes = ResourceManager::PrimitiveVertexAttributes;
using InstancedVertexAttributes = ResourceManager::InstancedVertexAttributes;

void InstancingPipeline::init(Device &device, TextureFormat &swapChainFormat, TextureFormat &depthTextureFormat, BindGroupLayout &bindGroupLayout)
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

    if (pipeline == nullptr)
        throw std::runtime_error("Failed to create instancing pipeline");
}

void InstancingPipeline::terminate()
{
    if (shaderModule != nullptr)
        shaderModule.release();
    if (pipeline != nullptr)
        pipeline.release();
    if (cubeInstanceBuffer != nullptr)
    {
        cubeInstanceBuffer.destroy();
        cubeInstanceBuffer.release();
    }
    if (sphereInstanceBuffer != nullptr)
    {
        sphereInstanceBuffer.destroy();
        sphereInstanceBuffer.release();
    }
    if (quadInstanceBuffer != nullptr)
    {
        quadInstanceBuffer.destroy();
        quadInstanceBuffer.release();
    }
    terminateGeometry();
}

void InstancingPipeline::updateCubes(wgpu::Device &device, wgpu::Queue &queue, std::vector<ResourceManager::InstancedVertexAttributes> &cubes)
{
    cubeInstances = static_cast<int>(cubes.size());
    if (cubeInstanceBuffer != nullptr)
    {
        cubeInstanceBuffer.destroy();
        cubeInstanceBuffer = nullptr;
    }
    BufferDescriptor cubeInstanceBufferDesc;
    if (cubeInstances > 0)
    {
        cubeInstanceBufferDesc.size = sizeof(InstancedVertexAttributes) * cubeInstances;
        cubeInstanceBufferDesc.usage = BufferUsage::Vertex | BufferUsage::CopyDst;
        cubeInstanceBufferDesc.mappedAtCreation = false;
        cubeInstanceBuffer = device.createBuffer(cubeInstanceBufferDesc);
        queue.writeBuffer(cubeInstanceBuffer, 0, cubes.data(), cubeInstanceBufferDesc.size);
    }
}

void InstancingPipeline::drawCubes(wgpu::RenderPassEncoder renderPass)
{
    // TODO: set bindgroup
    if (cubeInstances > 0)
    {
        renderPass.setPipeline(pipeline);
        renderPass.setVertexBuffer(0, cubeVertexBuffer, 0, cubeVertexBuffer.getSize());
        renderPass.setVertexBuffer(1, cubeInstanceBuffer, 0, cubeInstances * sizeof(InstancedVertexAttributes));
        renderPass.setIndexBuffer(cubeIndexBuffer, IndexFormat::Uint16, 0, cubeIndexBuffer.getSize());
        renderPass.drawIndexed(static_cast<uint32_t>(cube::triangles.size() * 3), cubeInstances, 0, 0, 0);
    }
}

void InstancingPipeline::updateSpheres(wgpu::Device &device, wgpu::Queue &queue, std::vector<ResourceManager::InstancedVertexAttributes> &spheres)
{
    sphereInstances = static_cast<int>(spheres.size());
    if (sphereInstanceBuffer != nullptr)
    {
        sphereInstanceBuffer.destroy();
        sphereInstanceBuffer = nullptr;
    }
    BufferDescriptor sphereInstanceBufferDesc;
    if (sphereInstances > 0)
    {
        sphereInstanceBufferDesc.size = sizeof(InstancedVertexAttributes) * sphereInstances;
        sphereInstanceBufferDesc.usage = BufferUsage::Vertex | BufferUsage::CopyDst;
        sphereInstanceBufferDesc.mappedAtCreation = false;
        sphereInstanceBuffer = device.createBuffer(sphereInstanceBufferDesc);
        queue.writeBuffer(sphereInstanceBuffer, 0, spheres.data(), sphereInstanceBufferDesc.size);
    }
}

void InstancingPipeline::drawSpheres(wgpu::RenderPassEncoder renderPass)
{
    if (sphereInstances > 0)
    {
        renderPass.setPipeline(pipeline);
        renderPass.setVertexBuffer(0, sphereVertexBuffer, 0, sphereVertexBuffer.getSize());
        renderPass.setVertexBuffer(1, sphereInstanceBuffer, 0, sphereInstances * sizeof(InstancedVertexAttributes));
        renderPass.setIndexBuffer(sphereIndexBuffer, IndexFormat::Uint16, 0, sphereIndexBuffer.getSize());
        renderPass.drawIndexed(static_cast<uint32_t>(sphereIndexBuffer.getSize() / 2), sphereInstances, 0, 0, 0);
    }
}

void InstancingPipeline::updateQuads(wgpu::Device &device, wgpu::Queue &queue, std::vector<ResourceManager::InstancedVertexAttributes> &quads)
{
    quadInstances = static_cast<int>(quads.size());
    if (quadInstanceBuffer != nullptr)
    {
        quadInstanceBuffer.destroy();
        quadInstanceBuffer = nullptr;
    }
    BufferDescriptor quadInstanceBufferDesc;
    if (quadInstances > 0)
    {
        quadInstanceBufferDesc.size = sizeof(InstancedVertexAttributes) * quadInstances;
        quadInstanceBufferDesc.usage = BufferUsage::Vertex | BufferUsage::CopyDst;
        quadInstanceBufferDesc.mappedAtCreation = false;
        quadInstanceBuffer = device.createBuffer(quadInstanceBufferDesc);
        queue.writeBuffer(quadInstanceBuffer, 0, quads.data(), quadInstanceBufferDesc.size);
    }
}

void InstancingPipeline::drawQuads(wgpu::RenderPassEncoder renderPass)
{
    if (quadInstances > 0)
    {
        renderPass.setPipeline(pipeline);
        renderPass.setVertexBuffer(0, quadVertexBuffer, 0, quadVertexBuffer.getSize());
        renderPass.setVertexBuffer(1, quadInstanceBuffer, 0, quadInstances * sizeof(InstancedVertexAttributes));
        renderPass.setIndexBuffer(quadIndexBuffer, IndexFormat::Uint16, 0, quadIndexBuffer.getSize());
        renderPass.drawIndexed(static_cast<uint32_t>(quad::triangles.size() * 3), quadInstances, 0, 0, 0);
    }
}

void InstancingPipeline::initGeometry(wgpu::Device &device, wgpu::Queue &queue)
{
    // Load cube geometry

    BufferDescriptor cubeVertexBufferDesc;
    cubeVertexBufferDesc.size = cube::vertices.size() * sizeof(PrimitiveVertexAttributes);
    cubeVertexBufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Vertex;
    cubeVertexBufferDesc.mappedAtCreation = false;
    cubeVertexBuffer = device.createBuffer(cubeVertexBufferDesc);
    queue.writeBuffer(cubeVertexBuffer, 0, cube::vertices.data(), cubeVertexBufferDesc.size);

    BufferDescriptor cubeIndexBufferDesc;
    cubeIndexBufferDesc.size = cube::triangles.size() * sizeof(Triangle);
    cubeIndexBufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Index;
    cubeIndexBufferDesc.mappedAtCreation = false;
    cubeIndexBuffer = device.createBuffer(cubeIndexBufferDesc);
    queue.writeBuffer(cubeIndexBuffer, 0, cube::triangles.data(), cubeIndexBufferDesc.size);

    // Load sphere geometry

    IndexedMesh sphereMesh = make_icosphere(2);
    VertexNormalList sphereVertices = sphereMesh.first;
    TriangleList sphereTriangles = sphereMesh.second;

    BufferDescriptor sphereVertexBufferSDesc;
    sphereVertexBufferSDesc.size = sphereVertices.size() * sizeof(PrimitiveVertexAttributes);
    sphereVertexBufferSDesc.usage = BufferUsage::CopyDst | BufferUsage::Vertex;
    sphereVertexBufferSDesc.mappedAtCreation = false;
    sphereVertexBuffer = device.createBuffer(sphereVertexBufferSDesc);
    queue.writeBuffer(sphereVertexBuffer, 0, sphereVertices.data(), sphereVertexBufferSDesc.size);

    BufferDescriptor sphereIndexBufferDesc;
    sphereIndexBufferDesc.size = sphereTriangles.size() * sizeof(Triangle);
    sphereIndexBufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Index;
    sphereIndexBufferDesc.mappedAtCreation = false;
    sphereIndexBuffer = device.createBuffer(sphereIndexBufferDesc);
    queue.writeBuffer(sphereIndexBuffer, 0, sphereTriangles.data(), sphereIndexBufferDesc.size);

    // Load quad geometry

    BufferDescriptor quadVertexBufferDesc;
    quadVertexBufferDesc.size = quad::vertices.size() * sizeof(PrimitiveVertexAttributes);
    quadVertexBufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Vertex;
    quadVertexBufferDesc.mappedAtCreation = false;
    quadVertexBuffer = device.createBuffer(quadVertexBufferDesc);
    queue.writeBuffer(quadVertexBuffer, 0, quad::vertices.data(), quadVertexBufferDesc.size);

    BufferDescriptor quadIndexBufferDesc;
    quadIndexBufferDesc.size = quad::triangles.size() * sizeof(Triangle);
    quadIndexBufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Index;
    quadIndexBufferDesc.mappedAtCreation = false;
    quadIndexBuffer = device.createBuffer(quadIndexBufferDesc);
    queue.writeBuffer(quadIndexBuffer, 0, quad::triangles.data(), quadIndexBufferDesc.size);

    if (cubeVertexBuffer == nullptr)
        throw std::runtime_error("Failed to create cube vertex buffer");
    if (cubeIndexBuffer == nullptr)
        throw std::runtime_error("Failed to create cube index buffer");
    if (sphereVertexBuffer == nullptr)
        throw std::runtime_error("Failed to create sphere vertex buffer");
    if (sphereIndexBuffer == nullptr)
        throw std::runtime_error("Failed to create sphere index buffer");
    if (quadVertexBuffer == nullptr)
        throw std::runtime_error("Failed to create quad vertex buffer");
    if (quadIndexBuffer == nullptr)
        throw std::runtime_error("Failed to create quad index buffer");
}

void InstancingPipeline::terminateGeometry()
{
    if (cubeVertexBuffer != nullptr)
    {
        cubeVertexBuffer.destroy();
        cubeVertexBuffer.release();
    }
    if (cubeIndexBuffer != nullptr)
    {
        cubeIndexBuffer.destroy();
        cubeIndexBuffer.release();
    }
    if (sphereVertexBuffer != nullptr)
    {
        sphereVertexBuffer.destroy();
        sphereVertexBuffer.release();
    }
    if (sphereIndexBuffer != nullptr)
    {
        sphereIndexBuffer.destroy();
        sphereIndexBuffer.release();
    }
    if (quadVertexBuffer != nullptr)
    {
        quadVertexBuffer.destroy();
        quadVertexBuffer.release();
    }
    if (quadIndexBuffer != nullptr)
    {
        quadIndexBuffer.destroy();
        quadIndexBuffer.release();
    }
}
