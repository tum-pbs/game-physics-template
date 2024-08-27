#include "Pipeline.h"
using namespace wgpu;
void Pipeline::reallocateBuffer(wgpu::Buffer &buffer, size_t size)
{
    if (buffer != nullptr)
    {
        buffer.destroy();
        buffer = nullptr;
    }
    BufferDescriptor bufferDesc;
    bufferDesc.size = size;
    bufferDesc.usage = BufferUsage::Vertex | BufferUsage::CopyDst;
    bufferDesc.mappedAtCreation = false;
    buffer = device.createBuffer(bufferDesc);
}