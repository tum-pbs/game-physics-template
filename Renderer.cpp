/**
 * This file is part of the "Learn WebGPU for C++" book.
 *   https://github.com/eliemichel/LearnWebGPU
 *
 * MIT License
 * Copyright (c) 2022-2023 Elie Michel
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "Renderer.h"
#include "ResourceManager.h"

#include <glfw3webgpu.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/polar_coordinates.hpp>

#include <imgui.h>
#include <backends/imgui_impl_wgpu.h>
#include <backends/imgui_impl_glfw.h>

#include <iostream>
#include <cassert>
#include <filesystem>
#include <sstream>
#include <string>
#include <array>

#include "Primitives.h"

using namespace wgpu;
using VertexAttributes = ResourceManager::VertexAttributes;
using PrimitiveVertexAttributes = ResourceManager::PrimitiveVertexAttributes;
using InstancedVertexAttributes = ResourceManager::InstancedVertexAttributes;
using LineVertexAttributes = ResourceManager::LineVertexAttributes;

constexpr float PI = 3.14159265358979323846f;

///////////////////////////////////////////////////////////////////////////////
// Public methods

bool Renderer::onInit()
{
	if (!initWindowAndDevice())
		return false;
	if (!initSwapChain())
		return false;
	if (!initDepthBuffer())
		return false;
	if (!initBindGroupLayout())
		return false;
	if (!initInstancingRenderPipeline())
		return false;
	if (!initLinePipeline())
		return false;
	if (!initGeometry())
		return false;
	if (!initUniforms())
		return false;
	if (!initLightingUniforms())
		return false;
	if (!initBindGroup())
		return false;
	if (!initGui())
		return false;
	return true;
}

void Renderer::onFrame()
{

	glfwPollEvents();
	updateDragInertia();
	updateLightingUniforms();
	// Update uniform buffer
	m_uniforms.time = static_cast<float>(glfwGetTime());
	m_queue.writeBuffer(m_uniformBuffer, offsetof(MyUniforms, time), &m_uniforms.time, sizeof(MyUniforms::time));
	m_queue.writeBuffer(m_uniformBuffer, offsetof(MyUniforms, cullingNormal), &m_uniforms.cullingNormal, sizeof(MyUniforms::cullingNormal));
	m_queue.writeBuffer(m_uniformBuffer, offsetof(MyUniforms, cullingOffset), &m_uniforms.cullingOffset, sizeof(MyUniforms::cullingOffset));
	m_queue.writeBuffer(m_uniformBuffer, offsetof(MyUniforms, flags), &m_uniforms.flags, sizeof(MyUniforms::flags));

	int cubeInstances = static_cast<int>(m_cubes.size());
	if (m_cubeInstanceBuffer != nullptr)
	{
		m_cubeInstanceBuffer.destroy();
		m_cubeInstanceBuffer = nullptr;
	}
	BufferDescriptor cubeInstanceBufferDesc;
	if (cubeInstances > 0)
	{
		cubeInstanceBufferDesc.size = sizeof(InstancedVertexAttributes) * cubeInstances;
		cubeInstanceBufferDesc.usage = BufferUsage::Vertex | BufferUsage::CopyDst;
		cubeInstanceBufferDesc.mappedAtCreation = false;
		m_cubeInstanceBuffer = m_device.createBuffer(cubeInstanceBufferDesc);
		m_queue.writeBuffer(m_cubeInstanceBuffer, 0, m_cubes.data(), cubeInstanceBufferDesc.size);
	}

	int quadInstances = static_cast<int>(m_quads.size());
	if (m_quadInstanceBuffer != nullptr)
	{
		m_quadInstanceBuffer.destroy();
		m_quadInstanceBuffer = nullptr;
	}
	BufferDescriptor quadInstanceBufferDesc;
	if (quadInstances > 0)
	{
		quadInstanceBufferDesc.size = sizeof(InstancedVertexAttributes) * quadInstances;
		quadInstanceBufferDesc.usage = BufferUsage::Vertex | BufferUsage::CopyDst;
		quadInstanceBufferDesc.mappedAtCreation = false;
		m_quadInstanceBuffer = m_device.createBuffer(quadInstanceBufferDesc);
		m_queue.writeBuffer(m_quadInstanceBuffer, 0, m_quads.data(), quadInstanceBufferDesc.size);
	}

	int lines = static_cast<int>(m_lines.size());
	if (m_lineVertexBuffer != nullptr)
	{
		m_lineVertexBuffer.destroy();
		m_lineVertexBuffer = nullptr;
	}
	BufferDescriptor lineBufferDesc;
	if (lines > 0)
	{
		lineBufferDesc.size = sizeof(LineVertexAttributes) * lines;
		lineBufferDesc.usage = BufferUsage::Vertex | BufferUsage::CopyDst;
		lineBufferDesc.mappedAtCreation = false;
		m_lineVertexBuffer = m_device.createBuffer(lineBufferDesc);
		m_queue.writeBuffer(m_lineVertexBuffer, 0, m_lines.data(), lineBufferDesc.size);
	}

	TextureView nextTexture = m_swapChain.getCurrentTextureView();
	if (!nextTexture)
	{
		std::cerr << "Cannot acquire next swap chain texture" << std::endl;
		return;
	}

	CommandEncoderDescriptor commandEncoderDesc;
	commandEncoderDesc.label = "Command Encoder";
	CommandEncoder encoder = m_device.createCommandEncoder(commandEncoderDesc);

	RenderPassDescriptor renderPassDesc{};

	RenderPassColorAttachment renderPassColorAttachment{};
	renderPassColorAttachment.view = nextTexture;
	renderPassColorAttachment.resolveTarget = nullptr;
	renderPassColorAttachment.loadOp = LoadOp::Clear;
	renderPassColorAttachment.storeOp = StoreOp::Store;
	renderPassColorAttachment.clearValue = Color{0.05, 0.05, 0.05, 1.0};
	renderPassDesc.colorAttachmentCount = 1;
	renderPassDesc.colorAttachments = &renderPassColorAttachment;

	RenderPassDepthStencilAttachment depthStencilAttachment;
	depthStencilAttachment.view = m_depthTextureView;
	depthStencilAttachment.depthClearValue = 1.0f;
	depthStencilAttachment.depthLoadOp = LoadOp::Clear;
	depthStencilAttachment.depthStoreOp = StoreOp::Store;
	depthStencilAttachment.depthReadOnly = false;
	depthStencilAttachment.stencilClearValue = 0;

#ifdef WEBGPU_BACKEND_WGPU
	depthStencilAttachment.stencilLoadOp = LoadOp::Clear;
	depthStencilAttachment.stencilStoreOp = StoreOp::Store;
#else
	depthStencilAttachment.stencilLoadOp = LoadOp::Undefined;
	depthStencilAttachment.stencilStoreOp = StoreOp::Undefined;
#endif
	depthStencilAttachment.stencilReadOnly = true;

	renderPassDesc.depthStencilAttachment = &depthStencilAttachment;

	renderPassDesc.timestampWriteCount = 0;
	renderPassDesc.timestampWrites = nullptr;
	RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc);

	// Set binding group
	renderPass.setBindGroup(0, m_bindGroup, 0, nullptr);

	if (lines > 0)
	{
		renderPass.setPipeline(m_linePipeline);
		renderPass.setVertexBuffer(0, m_lineVertexBuffer, 0, lines * sizeof(LineVertexAttributes));

		renderPass.draw(lines, 1, 0, 0);
	}

	if (cubeInstances > 0)
	{
		renderPass.setPipeline(m_instancingPipeline);
		renderPass.setVertexBuffer(0, m_cubeVertexBuffer, 0, m_cubeVertexBuffer.getSize());
		renderPass.setVertexBuffer(1, m_cubeInstanceBuffer, 0, cubeInstances * sizeof(InstancedVertexAttributes));
		renderPass.setIndexBuffer(m_cubeIndexBuffer, IndexFormat::Uint16, 0, m_cubeIndexBuffer.getSize());
		renderPass.drawIndexed(static_cast<uint32_t>(cube::indices.size()), cubeInstances, 0, 0, 0);
	}

	if (quadInstances > 0)
	{
		renderPass.setPipeline(m_instancingPipeline);
		renderPass.setVertexBuffer(0, m_quadVertexBuffer, 0, m_quadVertexBuffer.getSize());
		renderPass.setVertexBuffer(1, m_quadInstanceBuffer, 0, quadInstances * sizeof(InstancedVertexAttributes));
		renderPass.setIndexBuffer(m_quadIndexBuffer, IndexFormat::Uint16, 0, m_quadIndexBuffer.getSize());
		renderPass.drawIndexed(static_cast<uint32_t>(quad::indices.size()), quadInstances, 0, 0, 0);
	}

	// We add the GUI drawing commands to the render pass
	updateGui(renderPass);

	renderPass.end();
	renderPass.release();

	nextTexture.release();

	CommandBufferDescriptor cmdBufferDescriptor{};
	cmdBufferDescriptor.label = "Command buffer";
	CommandBuffer command = encoder.finish(cmdBufferDescriptor);
	encoder.release();
	m_queue.submit(command);
	command.release();

	m_swapChain.present();

#ifdef WEBGPU_BACKEND_DAWN
	// Check for pending error callbacks
	m_device.tick();
#endif
}

void Renderer::onFinish()
{
	terminateGui();
	terminateBindGroup();
	terminateLightingUniforms();
	terminateUniforms();
	terminateGeometry();
	terminateInstancingRenderPipeline();
	terminateLinePipeline();
	terminateBindGroupLayout();
	terminateDepthBuffer();
	terminateSwapChain();
	terminateWindowAndDevice();
}

bool Renderer::isRunning()
{
	return !glfwWindowShouldClose(m_window);
}

void Renderer::onResize()
{
	// Terminate in reverse order
	terminateDepthBuffer();
	terminateSwapChain();

	// Re-init
	initSwapChain();
	initDepthBuffer();

	updateProjectionMatrix();
}

void Renderer::onMouseMove(double xpos, double ypos)
{
	if (m_drag.active)
	{
		vec2 currentMouse = vec2(-(float)xpos, (float)ypos);
		vec2 delta = (currentMouse - m_drag.startMouse) * m_drag.sensitivity;
		m_cameraState.angles = m_drag.startCameraState.angles + delta;
		// Clamp to avoid going too far when orbitting up/down
		m_cameraState.angles.y = glm::clamp(m_cameraState.angles.y, -PI / 2 + 1e-5f, PI / 2 - 1e-5f);
		updateViewMatrix();

		// Inertia
		m_drag.velocity = delta - m_drag.previousDelta;
		m_drag.previousDelta = delta;
	}
}

void Renderer::onMouseButton(int button, int action, int /* modifiers */)
{
	ImGuiIO &io = ImGui::GetIO();
	if (io.WantCaptureMouse)
	{
		// Don't rotate the camera if the mouse is already captured by an ImGui
		// interaction at this frame.
		return;
	}

	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		switch (action)
		{
		case GLFW_PRESS:
			m_drag.active = true;
			double xpos, ypos;
			glfwGetCursorPos(m_window, &xpos, &ypos);
			m_drag.startMouse = vec2(-(float)xpos, (float)ypos);
			m_drag.startCameraState = m_cameraState;
			break;
		case GLFW_RELEASE:
			m_drag.active = false;
			break;
		}
	}
}

void Renderer::onScroll(double /* xoffset */, double yoffset)
{
	m_cameraState.zoom += m_drag.scrollSensitivity * static_cast<float>(yoffset);
	updateViewMatrix();
}

///////////////////////////////////////////////////////////////////////////////
// Private methods

bool Renderer::initWindowAndDevice()
{
	m_instance = createInstance(InstanceDescriptor{});
	if (!m_instance)
	{
		std::cerr << "Could not initialize WebGPU!" << std::endl;
		return false;
	}

	if (!glfwInit())
	{
		std::cerr << "Could not initialize GLFW!" << std::endl;
		return false;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
	m_window = glfwCreateWindow(640, 480, "Game Physics Template", NULL, NULL);
	if (!m_window)
	{
		std::cerr << "Could not open window!" << std::endl;
		return false;
	}

	m_surface = glfwGetWGPUSurface(m_instance, m_window);
	RequestAdapterOptions adapterOpts{};
	adapterOpts.compatibleSurface = m_surface;
	Adapter adapter = m_instance.requestAdapter(adapterOpts);

	SupportedLimits supportedLimits;
	adapter.getLimits(&supportedLimits);

	RequiredLimits requiredLimits = Default;
	requiredLimits.limits.maxVertexAttributes = 8;
	requiredLimits.limits.maxVertexBuffers = 3;
	requiredLimits.limits.maxBufferSize = 150000 * sizeof(VertexAttributes);
	requiredLimits.limits.maxVertexBufferArrayStride = sizeof(VertexAttributes);
	requiredLimits.limits.minStorageBufferOffsetAlignment = supportedLimits.limits.minStorageBufferOffsetAlignment;
	requiredLimits.limits.minUniformBufferOffsetAlignment = supportedLimits.limits.minUniformBufferOffsetAlignment;
	requiredLimits.limits.maxInterStageShaderComponents = 17;
	requiredLimits.limits.maxBindGroups = 2;
	requiredLimits.limits.maxUniformBuffersPerShaderStage = 2;
	requiredLimits.limits.maxUniformBufferBindingSize = 16 * 4 * sizeof(float);
	// Allow textures up to 2K
	requiredLimits.limits.maxTextureDimension1D = 2048;
	requiredLimits.limits.maxTextureDimension2D = 2048;
	requiredLimits.limits.maxTextureArrayLayers = 1;
	requiredLimits.limits.maxSampledTexturesPerShaderStage = 2;
	//                                                       ^ This was 1
	requiredLimits.limits.maxSamplersPerShaderStage = 1;

	DeviceDescriptor deviceDesc;
	deviceDesc.label = "My Device";
	deviceDesc.requiredFeaturesCount = 0;
	deviceDesc.requiredLimits = &requiredLimits;
	deviceDesc.defaultQueue.label = "The default queue";
	m_device = adapter.requestDevice(deviceDesc);

	// Add an error callback for more debug info
	m_errorCallbackHandle = m_device.setUncapturedErrorCallback([](ErrorType type, char const *message)
																{
		std::cout << "Device error: type " << type;
		if (message) std::cout << " (message: " << message << ")";
		std::cout << std::endl; });

	m_queue = m_device.getQueue();

#ifdef WEBGPU_BACKEND_WGPU
	m_swapChainFormat = m_surface.getPreferredFormat(adapter);
#else
	m_swapChainFormat = TextureFormat::BGRA8Unorm;
#endif

	// Add window callbacks
	// Set the user pointer to be "this"
	glfwSetWindowUserPointer(m_window, this);
	glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow *window, int, int)
								   {
		auto that = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
		if (that != nullptr) that->onResize(); });
	glfwSetCursorPosCallback(m_window, [](GLFWwindow *window, double xpos, double ypos)
							 {
		auto that = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
		if (that != nullptr) that->onMouseMove(xpos, ypos); });
	glfwSetMouseButtonCallback(m_window, [](GLFWwindow *window, int button, int action, int mods)
							   {
		auto that = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
		if (that != nullptr) that->onMouseButton(button, action, mods); });
	glfwSetScrollCallback(m_window, [](GLFWwindow *window, double xoffset, double yoffset)
						  {
		auto that = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
		if (that != nullptr) that->onScroll(xoffset, yoffset); });

	adapter.release();
	return m_device != nullptr;
}

void Renderer::terminateWindowAndDevice()
{
	m_queue.release();
	m_device.release();
	m_surface.release();
	m_instance.release();

	glfwDestroyWindow(m_window);
	glfwTerminate();
}

bool Renderer::initSwapChain()
{
	// Get the current size of the window's framebuffer:
	int width, height;
	glfwGetFramebufferSize(m_window, &width, &height);

	SwapChainDescriptor swapChainDesc;
	swapChainDesc.width = static_cast<uint32_t>(width);
	swapChainDesc.height = static_cast<uint32_t>(height);
	swapChainDesc.usage = TextureUsage::RenderAttachment;
	swapChainDesc.format = m_swapChainFormat;
	swapChainDesc.presentMode = PresentMode::Immediate;
	m_swapChain = m_device.createSwapChain(m_surface, swapChainDesc);
	return m_swapChain != nullptr;
}

void Renderer::terminateSwapChain()
{
	m_swapChain.release();
}

bool Renderer::initDepthBuffer()
{
	// Get the current size of the window's framebuffer:
	int width, height;
	glfwGetFramebufferSize(m_window, &width, &height);

	// Create the depth texture
	TextureDescriptor depthTextureDesc;
	depthTextureDesc.dimension = TextureDimension::_2D;
	depthTextureDesc.format = m_depthTextureFormat;
	depthTextureDesc.mipLevelCount = 1;
	depthTextureDesc.sampleCount = 1;
	depthTextureDesc.size = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
	depthTextureDesc.usage = TextureUsage::RenderAttachment;
	depthTextureDesc.viewFormatCount = 1;
	depthTextureDesc.viewFormats = (WGPUTextureFormat *)&m_depthTextureFormat;
	m_depthTexture = m_device.createTexture(depthTextureDesc);

	// Create the view of the depth texture manipulated by the rasterizer
	TextureViewDescriptor depthTextureViewDesc;
	depthTextureViewDesc.aspect = TextureAspect::DepthOnly;
	depthTextureViewDesc.baseArrayLayer = 0;
	depthTextureViewDesc.arrayLayerCount = 1;
	depthTextureViewDesc.baseMipLevel = 0;
	depthTextureViewDesc.mipLevelCount = 1;
	depthTextureViewDesc.dimension = TextureViewDimension::_2D;
	depthTextureViewDesc.format = m_depthTextureFormat;
	m_depthTextureView = m_depthTexture.createView(depthTextureViewDesc);

	return m_depthTextureView != nullptr;
}

void Renderer::terminateDepthBuffer()
{
	m_depthTextureView.release();
	m_depthTexture.destroy();
	m_depthTexture.release();
}

bool Renderer::initLinePipeline()
{
	m_lineShaderModule = ResourceManager::loadShaderModule(RESOURCE_DIR "/line_shader.wgsl", m_device);
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

	pipelineDesc.vertex.module = m_lineShaderModule;
	pipelineDesc.vertex.entryPoint = "vs_main";
	pipelineDesc.vertex.constantCount = 0;
	pipelineDesc.vertex.constants = nullptr;

	pipelineDesc.primitive.topology = PrimitiveTopology::LineList;
	pipelineDesc.primitive.stripIndexFormat = IndexFormat::Undefined;
	pipelineDesc.primitive.cullMode = CullMode::Back;

	FragmentState fragmentState;
	pipelineDesc.fragment = &fragmentState;
	fragmentState.module = m_lineShaderModule;
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
	colorTarget.format = m_swapChainFormat;
	colorTarget.blend = &blendState;
	colorTarget.writeMask = ColorWriteMask::All;

	fragmentState.targetCount = 1;
	fragmentState.targets = &colorTarget;

	DepthStencilState depthStencilState = Default;
	depthStencilState.depthCompare = CompareFunction::Less;
	depthStencilState.depthWriteEnabled = true;
	depthStencilState.format = m_depthTextureFormat;
	depthStencilState.stencilReadMask = 0;
	depthStencilState.stencilWriteMask = 0;

	pipelineDesc.depthStencil = &depthStencilState;

	pipelineDesc.multisample.count = 1;
	pipelineDesc.multisample.mask = ~0u;
	pipelineDesc.multisample.alphaToCoverageEnabled = false;

	// Create the pipeline layout
	PipelineLayoutDescriptor layoutDesc{};
	layoutDesc.bindGroupLayoutCount = 1;
	layoutDesc.bindGroupLayouts = (WGPUBindGroupLayout *)&m_bindGroupLayout;
	PipelineLayout layout = m_device.createPipelineLayout(layoutDesc);
	pipelineDesc.layout = layout;

	m_linePipeline = m_device.createRenderPipeline(pipelineDesc);

	return m_linePipeline != nullptr;
}

bool Renderer::initInstancingRenderPipeline()
{
	m_instancingShaderModule = ResourceManager::loadShaderModule(RESOURCE_DIR "/instancing_shader.wgsl", m_device);
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

	pipelineDesc.vertex.module = m_instancingShaderModule;
	pipelineDesc.vertex.entryPoint = "vs_main";
	pipelineDesc.vertex.constantCount = 0;
	pipelineDesc.vertex.constants = nullptr;

	pipelineDesc.primitive.topology = PrimitiveTopology::TriangleList;
	pipelineDesc.primitive.stripIndexFormat = IndexFormat::Undefined;
	pipelineDesc.primitive.cullMode = CullMode::Back;

	FragmentState fragmentState;
	pipelineDesc.fragment = &fragmentState;
	fragmentState.module = m_instancingShaderModule;
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
	colorTarget.format = m_swapChainFormat;
	colorTarget.blend = &blendState;
	colorTarget.writeMask = ColorWriteMask::All;

	fragmentState.targetCount = 1;
	fragmentState.targets = &colorTarget;

	DepthStencilState depthStencilState = Default;
	depthStencilState.depthCompare = CompareFunction::Less;
	depthStencilState.depthWriteEnabled = true;
	depthStencilState.format = m_depthTextureFormat;
	depthStencilState.stencilReadMask = 0;
	depthStencilState.stencilWriteMask = 0;

	pipelineDesc.depthStencil = &depthStencilState;

	pipelineDesc.multisample.count = 1;
	pipelineDesc.multisample.mask = ~0u;
	pipelineDesc.multisample.alphaToCoverageEnabled = false;

	// Create the pipeline layout
	PipelineLayoutDescriptor layoutDesc{};
	layoutDesc.bindGroupLayoutCount = 1;
	layoutDesc.bindGroupLayouts = (WGPUBindGroupLayout *)&m_bindGroupLayout;
	PipelineLayout layout = m_device.createPipelineLayout(layoutDesc);
	pipelineDesc.layout = layout;

	m_instancingPipeline = m_device.createRenderPipeline(pipelineDesc);

	return m_instancingPipeline != nullptr;
}

void Renderer::terminateInstancingRenderPipeline()
{
	m_instancingPipeline.release();
	m_instancingShaderModule.release();
}
void Renderer::clearScene()
{
	m_cubes.clear();
	m_quads.clear();
	m_lines.clear();
	current_id = 0;
}

bool Renderer::initGeometry()
{
	// bool success = ResourceManager::loadGeometryFromObj(RESOURCE_DIR "/fourareen.obj", vertexData);

	BufferDescriptor cubeVertexBufferDesc;
	cubeVertexBufferDesc.size = cube::vertices.size() * sizeof(PrimitiveVertexAttributes);
	cubeVertexBufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Vertex;
	cubeVertexBufferDesc.mappedAtCreation = false;
	m_cubeVertexBuffer = m_device.createBuffer(cubeVertexBufferDesc);
	m_queue.writeBuffer(m_cubeVertexBuffer, 0, cube::vertices.data(), cubeVertexBufferDesc.size);

	BufferDescriptor cubeIndexBufferDesc;
	cubeIndexBufferDesc.size = cube::indices.size() * sizeof(uint16_t);
	cubeIndexBufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Index;
	cubeIndexBufferDesc.mappedAtCreation = false;
	m_cubeIndexBuffer = m_device.createBuffer(cubeIndexBufferDesc);
	m_queue.writeBuffer(m_cubeIndexBuffer, 0, cube::indices.data(), cubeIndexBufferDesc.size);

	BufferDescriptor quadVertexBufferDesc;
	quadVertexBufferDesc.size = quad::vertices.size() * sizeof(PrimitiveVertexAttributes);
	quadVertexBufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Vertex;
	quadVertexBufferDesc.mappedAtCreation = false;
	m_quadVertexBuffer = m_device.createBuffer(quadVertexBufferDesc);
	m_queue.writeBuffer(m_quadVertexBuffer, 0, quad::vertices.data(), quadVertexBufferDesc.size);

	BufferDescriptor quadIndexBufferDesc;
	quadIndexBufferDesc.size = quad::indices.size() * sizeof(uint16_t);
	quadIndexBufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Index;
	quadIndexBufferDesc.mappedAtCreation = false;
	m_quadIndexBuffer = m_device.createBuffer(quadIndexBufferDesc);
	m_queue.writeBuffer(m_quadIndexBuffer, 0, quad::indices.data(), quadIndexBufferDesc.size);

	return m_cubeVertexBuffer != nullptr && m_cubeIndexBuffer != nullptr && m_quadVertexBuffer != nullptr && m_quadIndexBuffer != nullptr;
}

void Renderer::terminateLinePipeline()
{
	m_linePipeline.release();
	m_lineShaderModule.release();
}

void Renderer::terminateGeometry()
{

	m_cubeVertexBuffer.destroy();
	m_cubeVertexBuffer.release();
	m_cubeIndexBuffer.destroy();
	m_cubeIndexBuffer.release();
	m_quadVertexBuffer.destroy();
	m_quadVertexBuffer.release();
	m_quadIndexBuffer.destroy();
	m_quadIndexBuffer.release();
	if (m_cubeInstanceBuffer != nullptr)
	{
		m_cubeInstanceBuffer.destroy();
		m_cubeInstanceBuffer.release();
	}
	if (m_quadInstanceBuffer != nullptr)
	{
		m_quadInstanceBuffer.destroy();
		m_quadInstanceBuffer.release();
	}
	if (m_lineVertexBuffer != nullptr)
	{
		m_lineVertexBuffer.destroy();
		m_lineVertexBuffer.release();
	}
}

bool Renderer::initUniforms()
{
	// Create uniform buffer
	BufferDescriptor bufferDesc;
	bufferDesc.size = sizeof(MyUniforms);
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Uniform;
	bufferDesc.mappedAtCreation = false;
	m_uniformBuffer = m_device.createBuffer(bufferDesc);

	// Upload the initial value of the uniforms
	m_uniforms.modelMatrix = mat4x4(1.0);
	m_uniforms.viewMatrix = glm::lookAt(vec3(-2.0f, -3.0f, 2.0f), vec3(0.0f), vec3(0, 0, 1));
	m_uniforms.projectionMatrix = glm::perspective(45 * PI / 180, 640.0f / 480.0f, 0.01f, 100.0f);
	m_uniforms.time = 1.0f;
	m_uniforms.cullingNormal = {0.0f, 0.0f, 1.0f};
	m_uniforms.cullingOffset = 0.0f;
	m_uniforms.flags = 0;
	m_queue.writeBuffer(m_uniformBuffer, 0, &m_uniforms, sizeof(MyUniforms));

	updateProjectionMatrix();
	updateViewMatrix();

	return m_uniformBuffer != nullptr;
}

void Renderer::terminateUniforms()
{
	m_uniformBuffer.destroy();
	m_uniformBuffer.release();
}

bool Renderer::initLightingUniforms()
{
	// Create uniform buffer
	BufferDescriptor bufferDesc;
	bufferDesc.size = sizeof(LightingUniforms);
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Uniform;
	bufferDesc.mappedAtCreation = false;
	m_lightingUniformBuffer = m_device.createBuffer(bufferDesc);

	// Initial values
	m_lightingUniforms.directions[0] = {0.5f, -0.9f, 0.1f, 0.0f};
	m_lightingUniforms.directions[1] = {0.2f, 0.4f, 0.3f, 0.0f};
	m_lightingUniforms.colors[0] = {1.0f, 0.9f, 0.6f, 1.0f};
	m_lightingUniforms.colors[1] = {0.6f, 0.9f, 1.0f, 1.0f};

	updateLightingUniforms();

	return m_lightingUniformBuffer != nullptr;
}

void Renderer::terminateLightingUniforms()
{
	m_lightingUniformBuffer.destroy();
	m_lightingUniformBuffer.release();
}

void Renderer::updateLightingUniforms()
{
	if (m_lightingUniformsChanged)
	{
		m_queue.writeBuffer(m_lightingUniformBuffer, 0, &m_lightingUniforms, sizeof(LightingUniforms));
		m_lightingUniformsChanged = false;
	}
}

bool Renderer::initBindGroupLayout()
{
	std::vector<BindGroupLayoutEntry> bindingLayoutEntries(2, Default);

	// The uniform buffer binding
	BindGroupLayoutEntry &bindingLayout = bindingLayoutEntries[0];
	bindingLayout.binding = 0;
	bindingLayout.visibility = ShaderStage::Vertex | ShaderStage::Fragment;
	bindingLayout.buffer.type = BufferBindingType::Uniform;
	bindingLayout.buffer.minBindingSize = sizeof(MyUniforms);

	// The lighting uniform buffer binding
	BindGroupLayoutEntry &lightingUniformLayout = bindingLayoutEntries[1];
	lightingUniformLayout.binding = 1;
	lightingUniformLayout.visibility = ShaderStage::Fragment; // only Fragment is needed
	lightingUniformLayout.buffer.type = BufferBindingType::Uniform;
	lightingUniformLayout.buffer.minBindingSize = sizeof(LightingUniforms);

	// Create a bind group layout
	BindGroupLayoutDescriptor bindGroupLayoutDesc{};
	bindGroupLayoutDesc.entryCount = (uint32_t)bindingLayoutEntries.size();
	bindGroupLayoutDesc.entries = bindingLayoutEntries.data();
	m_bindGroupLayout = m_device.createBindGroupLayout(bindGroupLayoutDesc);

	return m_bindGroupLayout != nullptr;
}

void Renderer::terminateBindGroupLayout()
{
	m_bindGroupLayout.release();
}

bool Renderer::initBindGroup()
{
	// Create a binding
	std::vector<BindGroupEntry> bindings(2);

	bindings[0].binding = 0;
	bindings[0].buffer = m_uniformBuffer;
	bindings[0].offset = 0;
	bindings[0].size = sizeof(MyUniforms);

	bindings[1].binding = 1;
	bindings[1].buffer = m_lightingUniformBuffer;
	bindings[1].offset = 0;
	bindings[1].size = sizeof(LightingUniforms);

	BindGroupDescriptor bindGroupDesc;
	bindGroupDesc.layout = m_bindGroupLayout;
	bindGroupDesc.entryCount = (uint32_t)bindings.size();
	bindGroupDesc.entries = bindings.data();
	m_bindGroup = m_device.createBindGroup(bindGroupDesc);

	return m_bindGroup != nullptr;
}

void Renderer::terminateBindGroup()
{
	m_bindGroup.release();
}

void Renderer::updateProjectionMatrix()
{
	// Update projection matrix
	int width, height;
	glfwGetFramebufferSize(m_window, &width, &height);
	float ratio = width / (float)height;
	m_uniforms.projectionMatrix = glm::perspective(45 * PI / 180, ratio, 0.01f, 100.0f);
	m_queue.writeBuffer(
		m_uniformBuffer,
		offsetof(MyUniforms, projectionMatrix),
		&m_uniforms.projectionMatrix,
		sizeof(MyUniforms::projectionMatrix));
}

void Renderer::updateViewMatrix()
{
	float cx = cos(m_cameraState.angles.x);
	float sx = sin(m_cameraState.angles.x);
	float cy = cos(m_cameraState.angles.y);
	float sy = sin(m_cameraState.angles.y);
	vec3 position = vec3(cx * cy, sx * cy, sy) * std::exp(-m_cameraState.zoom);
	m_uniforms.viewMatrix = glm::lookAt(position, vec3(0.0f), vec3(0, 0, 1));
	m_queue.writeBuffer(
		m_uniformBuffer,
		offsetof(MyUniforms, viewMatrix),
		&m_uniforms.viewMatrix,
		sizeof(MyUniforms::viewMatrix));
	m_uniforms.cameraWorldPosition = position;
	m_queue.writeBuffer(
		m_uniformBuffer,
		offsetof(MyUniforms, cameraWorldPosition),
		&m_uniforms.cameraWorldPosition,
		sizeof(MyUniforms::cameraWorldPosition));
}

void Renderer::updateDragInertia()
{
	constexpr float eps = 1e-4f;
	// Apply inertia only when the user released the click.
	if (!m_drag.active)
	{
		// Avoid updating the matrix when the velocity is no longer noticeable
		if (std::abs(m_drag.velocity.x) < eps && std::abs(m_drag.velocity.y) < eps)
		{
			return;
		}
		m_cameraState.angles += m_drag.velocity;
		m_cameraState.angles.y = glm::clamp(m_cameraState.angles.y, -PI / 2 + 1e-5f, PI / 2 - 1e-5f);
		// Dampen the velocity so that it decreases exponentially and stops
		// after a few frames.
		m_drag.velocity *= m_drag.intertia;
		updateViewMatrix();
	}
}

bool Renderer::initGui()
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::GetIO();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOther(m_window, true);
	ImGui_ImplWGPU_Init(m_device, 3, m_swapChainFormat, m_depthTextureFormat);
	return true;
}

void Renderer::terminateGui()
{
	ImGui_ImplGlfw_Shutdown();
	ImGui_ImplWGPU_Shutdown();
}

uint32_t Renderer::drawCube(glm::vec3 position, glm::quat rotation, glm::vec3 scale, glm::vec4 color, uint32_t flags)
{
	m_cubes.push_back({position,
					   rotation,
					   scale,
					   color,
					   current_id,
					   flags});
	return current_id++;
}

uint32_t Renderer::drawCube(glm::vec3 position, glm::quat rotation, glm::vec3 scale, glm::vec3 color)
{
	return drawCube(position, rotation, scale, glm::vec4(color, 1.0f));
}

uint32_t Renderer::drawQuad(glm::vec3 position, glm::quat rotation, glm::vec3 scale, glm::vec4 color, uint32_t flags)
{
	m_quads.push_back({position, rotation, scale, color, current_id, flags});
	return current_id++;
}

uint32_t Renderer::drawQuad(glm::vec3 position, glm::quat rotation, glm::vec3 scale, glm::vec3 color)
{
	return drawQuad(position, rotation, scale, glm::vec4(color, 1.0f));
}

void Renderer::drawLine(glm::vec3 position1, glm::vec3 position2, glm::vec3 color1, glm::vec3 color2)
{
	m_lines.push_back({position1, color1});
	m_lines.push_back({position2, color2});
}

void Renderer::drawLine(glm::vec3 position1, glm::vec3 position2, glm::vec3 color)
{
	drawLine(position1, position2, color, color);
}

void Renderer::updateGui(RenderPassEncoder renderPass)
{
	// Start the Dear ImGui frame
	ImGui_ImplWGPU_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	defineGUI();

	// Draw the UI
	ImGui::EndFrame();
	// Convert the UI defined above into low-level drawing commands
	ImGui::Render();
	// Execute the low-level drawing commands on the WebGPU backend
	ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), renderPass);
}
