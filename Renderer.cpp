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

using namespace wgpu;
using VertexAttributes = ResourceManager::VertexAttributes;
using PrimitiveVertexAttributes = ResourceManager::PrimitiveVertexAttributes;
using InstancedVertexAttributes = ResourceManager::InstancedVertexAttributes;

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
	if (!initRenderPipeline())
		return false;
	if (!initInstancingRenderPipeline())
		return false;
	if (!initTextures())
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

	int cubeInstances = static_cast<int>(m_cubes.size());

	if (m_instanceBuffer != nullptr)
		m_instanceBuffer.destroy();
	BufferDescriptor instanceBufferDesc;
	instanceBufferDesc.size = sizeof(InstancedVertexAttributes) * cubeInstances;
	instanceBufferDesc.usage = BufferUsage::Vertex | BufferUsage::CopyDst;
	instanceBufferDesc.mappedAtCreation = false;
	m_instanceBuffer = m_device.createBuffer(instanceBufferDesc);
	m_queue.writeBuffer(m_instanceBuffer, 0, m_cubes.data(), instanceBufferDesc.size);

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

	// renderPass.setPipeline(m_pipeline);

	// renderPass.setVertexBuffer(0, m_vertexBuffer, 0, m_vertexCount * sizeof(VertexAttributes));

	// Set binding group
	renderPass.setBindGroup(0, m_bindGroup, 0, nullptr);

	// renderPass.draw(m_vertexCount, 1, 0, 0);

	renderPass.setPipeline(m_instancingPipeline);

	renderPass.setVertexBuffer(0, m_cubeVertexBuffer, 0, m_cubeVertexCount * sizeof(PrimitiveVertexAttributes));
	renderPass.setVertexBuffer(1, m_instanceBuffer, 0, cubeInstances * sizeof(InstancedVertexAttributes));
	renderPass.setIndexBuffer(m_cubeIndexBuffer, IndexFormat::Uint16, 0, m_cubeIndexCount * sizeof(uint16_t));

	// Set binding group

	renderPass.drawIndexed(m_cubeIndexCount, cubeInstances, 0, 0, 0);

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
	terminateTextures();
	terminateRenderPipeline();
	terminateInstancingRenderPipeline();
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
	m_cameraState.zoom = glm::clamp(m_cameraState.zoom, -2.0f, 2.0f);
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
	m_window = glfwCreateWindow(640, 480, "Game Physics Template", NULL, NULL);
	if (!m_window)
	{
		std::cerr << "Could not open window!" << std::endl;
		return false;
	}

	std::cout << "Requesting adapter..." << std::endl;
	m_surface = glfwGetWGPUSurface(m_instance, m_window);
	RequestAdapterOptions adapterOpts{};
	adapterOpts.compatibleSurface = m_surface;
	Adapter adapter = m_instance.requestAdapter(adapterOpts);
	std::cout << "Got adapter: " << adapter << std::endl;

	SupportedLimits supportedLimits;
	adapter.getLimits(&supportedLimits);

	std::cout << "Requesting device..." << std::endl;
	RequiredLimits requiredLimits = Default;
	requiredLimits.limits.maxVertexAttributes = 6;
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
	std::cout << "Got device: " << m_device << std::endl;

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

	std::cout << "Creating swapchain..." << std::endl;
	SwapChainDescriptor swapChainDesc;
	swapChainDesc.width = static_cast<uint32_t>(width);
	swapChainDesc.height = static_cast<uint32_t>(height);
	swapChainDesc.usage = TextureUsage::RenderAttachment;
	swapChainDesc.format = m_swapChainFormat;
	swapChainDesc.presentMode = PresentMode::Immediate;
	m_swapChain = m_device.createSwapChain(m_surface, swapChainDesc);
	std::cout << "Swapchain: " << m_swapChain << std::endl;
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
	std::cout << "Depth texture: " << m_depthTexture << std::endl;

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
	std::cout << "Depth texture view: " << m_depthTextureView << std::endl;

	return m_depthTextureView != nullptr;
}

void Renderer::terminateDepthBuffer()
{
	m_depthTextureView.release();
	m_depthTexture.destroy();
	m_depthTexture.release();
}
bool Renderer::initInstancingRenderPipeline()
{
	std::cout << "Creating instancing shader module..." << std::endl;
	m_instancingShaderModule = ResourceManager::loadShaderModule(RESOURCE_DIR "/instancing_shader.wgsl", m_device);
	std::cout << "Instancing shader module: " << m_instancingShaderModule << std::endl;

	std::cout << "Creating instancing render pipeline..." << std::endl;
	RenderPipelineDescriptor pipelineDesc;

	// This is for instanced rendering
	std::vector<VertexAttribute> primitiveVertexAttribs(2);

	// Position attribute
	primitiveVertexAttribs[0].shaderLocation = 0;
	primitiveVertexAttribs[0].format = VertexFormat::Float32x3;
	primitiveVertexAttribs[0].offset = offsetof(PrimitiveVertexAttributes, position);

	// Normal attribute
	primitiveVertexAttribs[1].shaderLocation = 1;
	primitiveVertexAttribs[1].format = VertexFormat::Float32x3;
	primitiveVertexAttribs[1].offset = offsetof(PrimitiveVertexAttributes, normal);

	VertexBufferLayout primitiveVertexBufferLayout;
	primitiveVertexBufferLayout.attributeCount = (uint32_t)primitiveVertexAttribs.size();
	primitiveVertexBufferLayout.attributes = primitiveVertexAttribs.data();
	primitiveVertexBufferLayout.arrayStride = sizeof(PrimitiveVertexAttributes);
	primitiveVertexBufferLayout.stepMode = VertexStepMode::Vertex;

	// position, rotation, scale, color
	std::vector<VertexAttribute> instanceAttribs(4);

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
	instanceAttribs[3].format = VertexFormat::Float32x3;
	instanceAttribs[3].offset = offsetof(InstancedVertexAttributes, color);

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
	std::cout << "Render pipeline: " << m_instancingPipeline << std::endl;

	return m_instancingPipeline != nullptr;
}

bool Renderer::initRenderPipeline()
{
	std::cout << "Creating shader module..." << std::endl;
	m_shaderModule = ResourceManager::loadShaderModule(RESOURCE_DIR "/shader.wgsl", m_device);
	std::cout << "Shader module: " << m_shaderModule << std::endl;

	std::cout << "Creating render pipeline..." << std::endl;
	RenderPipelineDescriptor pipelineDesc;

	// Vertex fetch
	std::vector<VertexAttribute> vertexAttribs(6);

	// Position attribute
	vertexAttribs[0].shaderLocation = 0;
	vertexAttribs[0].format = VertexFormat::Float32x3;
	vertexAttribs[0].offset = 0;

	// Normal attribute
	vertexAttribs[1].shaderLocation = 1;
	vertexAttribs[1].format = VertexFormat::Float32x3;
	vertexAttribs[1].offset = offsetof(VertexAttributes, normal);

	// Color attribute
	vertexAttribs[2].shaderLocation = 2;
	vertexAttribs[2].format = VertexFormat::Float32x3;
	vertexAttribs[2].offset = offsetof(VertexAttributes, color);

	// UV attribute
	vertexAttribs[3].shaderLocation = 3;
	vertexAttribs[3].format = VertexFormat::Float32x2;
	vertexAttribs[3].offset = offsetof(VertexAttributes, uv);

	// Tangent attribute
	vertexAttribs[4].shaderLocation = 4;
	vertexAttribs[4].format = VertexFormat::Float32x3;
	vertexAttribs[4].offset = offsetof(VertexAttributes, tangent);

	// Bitangent attribute
	vertexAttribs[5].shaderLocation = 5;
	vertexAttribs[5].format = VertexFormat::Float32x3;
	vertexAttribs[5].offset = offsetof(VertexAttributes, bitangent);

	VertexBufferLayout vertexBufferLayout;
	vertexBufferLayout.attributeCount = (uint32_t)vertexAttribs.size();
	vertexBufferLayout.attributes = vertexAttribs.data();
	vertexBufferLayout.arrayStride = sizeof(VertexAttributes);
	vertexBufferLayout.stepMode = VertexStepMode::Vertex;

	pipelineDesc.vertex.bufferCount = 1;
	pipelineDesc.vertex.buffers = &vertexBufferLayout;

	pipelineDesc.vertex.module = m_shaderModule;
	pipelineDesc.vertex.entryPoint = "vs_main";
	pipelineDesc.vertex.constantCount = 0;
	pipelineDesc.vertex.constants = nullptr;

	pipelineDesc.primitive.topology = PrimitiveTopology::TriangleList;
	pipelineDesc.primitive.stripIndexFormat = IndexFormat::Undefined;
	pipelineDesc.primitive.frontFace = FrontFace::CCW;
	pipelineDesc.primitive.cullMode = CullMode::None;

	FragmentState fragmentState;
	pipelineDesc.fragment = &fragmentState;
	fragmentState.module = m_shaderModule;
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

	m_pipeline = m_device.createRenderPipeline(pipelineDesc);
	std::cout << "Render pipeline: " << m_pipeline << std::endl;

	return m_pipeline != nullptr;
}

void Renderer::terminateRenderPipeline()
{
	m_pipeline.release();
	m_shaderModule.release();
}

void Renderer::terminateInstancingRenderPipeline()
{
	m_instancingPipeline.release();
	m_instancingShaderModule.release();
}
void Renderer::clearScene()
{
	m_cubes.clear();
}

bool Renderer::initTextures()
{
	// Create a sampler
	SamplerDescriptor samplerDesc;
	samplerDesc.addressModeU = AddressMode::Repeat;
	samplerDesc.addressModeV = AddressMode::Repeat;
	samplerDesc.addressModeW = AddressMode::Repeat;
	samplerDesc.magFilter = FilterMode::Linear;
	samplerDesc.minFilter = FilterMode::Linear;
	samplerDesc.mipmapFilter = MipmapFilterMode::Linear;
	samplerDesc.lodMinClamp = 0.0f;
	samplerDesc.lodMaxClamp = 8.0f;
	samplerDesc.compare = CompareFunction::Undefined;
	samplerDesc.maxAnisotropy = 1;
	m_sampler = m_device.createSampler(samplerDesc);

	// Create textures
	m_baseColorTexture = ResourceManager::loadTexture(RESOURCE_DIR "/cobblestone_floor_08_diff_2k.jpg", m_device, &m_baseColorTextureView);
	// m_baseColorTexture = ResourceManager::loadTexture(RESOURCE_DIR "/fourareen2K_albedo.jpg", m_device, &m_baseColorTextureView);
	if (!m_baseColorTexture)
	{
		std::cerr << "Could not load base color texture!" << std::endl;
		return false;
	}

	m_normalTexture = ResourceManager::loadTexture(RESOURCE_DIR "/cobblestone_floor_08_nor_gl_2k.png", m_device, &m_normalTextureView);
	// m_normalTexture = ResourceManager::loadTexture(RESOURCE_DIR "/fourareen2K_normals.png", m_device, &m_normalTextureView);
	if (!m_normalTexture)
	{
		std::cerr << "Could not load normal texture!" << std::endl;
		return false;
	}

	return m_baseColorTextureView != nullptr && m_normalTextureView != nullptr;
}

void Renderer::terminateTextures()
{
	m_baseColorTextureView.release();
	m_baseColorTexture.destroy();
	m_baseColorTexture.release();
	m_normalTextureView.release();
	m_normalTexture.destroy();
	m_normalTexture.release();
	m_sampler.release();
}

bool Renderer::initGeometry()
{
	// Load mesh data from OBJ file
	std::vector<VertexAttributes> vertexData;
	bool success = ResourceManager::loadGeometryFromObj(RESOURCE_DIR "/cylinder.obj", vertexData);

	// bool success = ResourceManager::loadGeometryFromObj(RESOURCE_DIR "/fourareen.obj", vertexData);
	if (!success)
	{
		std::cerr << "Could not load geometry!" << std::endl;
		return false;
	}

	// Create vertex buffer
	BufferDescriptor bufferDesc;
	bufferDesc.size = vertexData.size() * sizeof(VertexAttributes);
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Vertex;
	bufferDesc.mappedAtCreation = false;
	m_vertexBuffer = m_device.createBuffer(bufferDesc);
	m_queue.writeBuffer(m_vertexBuffer, 0, vertexData.data(), bufferDesc.size);

	m_vertexCount = static_cast<int>(vertexData.size());

	std::vector<PrimitiveVertexAttributes> cubeVertexData = {
		// left handed coordinates: z is away, x is right and y is up, position and normals
		// front face
		{{-0.5, -0.5, -0.5}, {0.0, 0.0, -1.0}},
		{{0.5, -0.5, -0.5}, {0.0, 0.0, -1.0}},
		{{0.5, 0.5, -0.5}, {0.0, 0.0, -1.0}},
		{{-0.5, 0.5, -0.5}, {0.0, 0.0, -1.0}},
		// left face
		{{-0.5, -0.5, -0.5}, {-1.0, 0.0, 0.0}},
		{{-0.5, 0.5, -0.5}, {-1.0, 0.0, 0.0}},
		{{-0.5, 0.5, 0.5}, {-1.0, 0.0, 0.0}},
		{{-0.5, -0.5, 0.5}, {-1.0, 0.0, 0.0}},
		// right face
		{{0.5, -0.5, -0.5}, {1.0, 0.0, 0.0}},
		{{0.5, 0.5, -0.5}, {1.0, 0.0, 0.0}},
		{{0.5, 0.5, 0.5}, {1.0, 0.0, 0.0}},
		{{0.5, -0.5, 0.5}, {1.0, 0.0, 0.0}},
		// back face
		{{-0.5, -0.5, 0.5}, {0.0, 0.0, 1.0}},
		{{0.5, -0.5, 0.5}, {0.0, 0.0, 1.0}},
		{{0.5, 0.5, 0.5}, {0.0, 0.0, 1.0}},
		{{-0.5, 0.5, 0.5}, {0.0, 0.0, 1.0}},
		// top face
		{{-0.5, 0.5, -0.5}, {0.0, 1.0, 0.0}},
		{{0.5, 0.5, -0.5}, {0.0, 1.0, 0.0}},
		{{0.5, 0.5, 0.5}, {0.0, 1.0, 0.0}},
		{{-0.5, 0.5, 0.5}, {0.0, 1.0, 0.0}},
		// bottom face
		{{-0.5, -0.5, -0.5}, {0.0, -1.0, 0.0}},
		{{0.5, -0.5, -0.5}, {0.0, -1.0, 0.0}},
		{{0.5, -0.5, 0.5}, {0.0, -1.0, 0.0}},
		{{-0.5, -0.5, 0.5}, {0.0, -1.0, 0.0}},
	};

	m_cubeVertexCount = static_cast<int>(cubeVertexData.size());

	std::vector<uint16_t> cubeIndexData = {
		0, 2, 1, 0, 3, 2,		// front face
		4, 6, 5, 4, 7, 6,		// left face
		8, 9, 10, 8, 10, 11,	// right face
		12, 13, 14, 12, 14, 15, // back face
		16, 18, 17, 16, 19, 18, // top face
		20, 21, 22, 20, 22, 23, // bottom face
	};

	m_cubeIndexCount = static_cast<int>(cubeIndexData.size());

	BufferDescriptor cubeVertexBufferDesc;
	cubeVertexBufferDesc.size = cubeVertexData.size() * sizeof(PrimitiveVertexAttributes);
	cubeVertexBufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Vertex;
	cubeVertexBufferDesc.mappedAtCreation = false;
	m_cubeVertexBuffer = m_device.createBuffer(cubeVertexBufferDesc);
	m_queue.writeBuffer(m_cubeVertexBuffer, 0, cubeVertexData.data(), cubeVertexBufferDesc.size);

	BufferDescriptor cubeIndexBufferDesc;
	cubeIndexBufferDesc.size = cubeIndexData.size() * sizeof(uint16_t);
	cubeIndexBufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Index;
	cubeIndexBufferDesc.mappedAtCreation = false;
	m_cubeIndexBuffer = m_device.createBuffer(cubeIndexBufferDesc);
	m_queue.writeBuffer(m_cubeIndexBuffer, 0, cubeIndexData.data(), cubeIndexBufferDesc.size);

	return m_vertexBuffer != nullptr && m_cubeVertexBuffer != nullptr && m_cubeIndexBuffer != nullptr;
}

void Renderer::terminateGeometry()
{
	m_vertexBuffer.destroy();
	m_vertexBuffer.release();
	m_vertexCount = 0;

	m_cubeVertexBuffer.destroy();
	m_cubeVertexBuffer.release();
	m_cubeIndexBuffer.destroy();
	m_cubeIndexBuffer.release();
	m_instanceBuffer.destroy();
	m_instanceBuffer.release();
	m_cubeVertexCount = 0;
	m_cubeIndexCount = 0;
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
	m_uniforms.color = {0.0f, 1.0f, 0.4f, 1.0f};
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
	std::vector<BindGroupLayoutEntry> bindingLayoutEntries(5, Default);
	//                                                     ^ This was a 4

	// The uniform buffer binding
	BindGroupLayoutEntry &bindingLayout = bindingLayoutEntries[0];
	bindingLayout.binding = 0;
	bindingLayout.visibility = ShaderStage::Vertex | ShaderStage::Fragment;
	bindingLayout.buffer.type = BufferBindingType::Uniform;
	bindingLayout.buffer.minBindingSize = sizeof(MyUniforms);

	// The base color texture binding
	BindGroupLayoutEntry &textureBindingLayout = bindingLayoutEntries[1];
	textureBindingLayout.binding = 1;
	textureBindingLayout.visibility = ShaderStage::Fragment;
	textureBindingLayout.texture.sampleType = TextureSampleType::Float;
	textureBindingLayout.texture.viewDimension = TextureViewDimension::_2D;

	// The normal map binding
	BindGroupLayoutEntry &normalTextureBindingLayout = bindingLayoutEntries[2];
	normalTextureBindingLayout.binding = 2;
	normalTextureBindingLayout.visibility = ShaderStage::Fragment;
	normalTextureBindingLayout.texture.sampleType = TextureSampleType::Float;
	normalTextureBindingLayout.texture.viewDimension = TextureViewDimension::_2D;

	// The texture sampler binding
	BindGroupLayoutEntry &samplerBindingLayout = bindingLayoutEntries[3];
	samplerBindingLayout.binding = 3;
	//                             ^ This was a 2
	samplerBindingLayout.visibility = ShaderStage::Fragment;
	samplerBindingLayout.sampler.type = SamplerBindingType::Filtering;

	// The lighting uniform buffer binding
	BindGroupLayoutEntry &lightingUniformLayout = bindingLayoutEntries[4];
	lightingUniformLayout.binding = 4;
	//                              ^ This was a 3
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
	std::vector<BindGroupEntry> bindings(5);
	//                                   ^ This was a 4

	bindings[0].binding = 0;
	bindings[0].buffer = m_uniformBuffer;
	bindings[0].offset = 0;
	bindings[0].size = sizeof(MyUniforms);

	bindings[1].binding = 1;
	bindings[1].textureView = m_baseColorTextureView;

	bindings[2].binding = 2;
	bindings[2].textureView = m_normalTextureView;

	bindings[3].binding = 3;
	bindings[3].sampler = m_sampler;

	bindings[4].binding = 4;
	bindings[4].buffer = m_lightingUniformBuffer;
	bindings[4].offset = 0;
	bindings[4].size = sizeof(LightingUniforms);

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

void Renderer::drawCube(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale, glm::vec3 color)
{
	m_cubes.push_back({position, rotation, scale, color});
}

void Renderer::updateGui(RenderPassEncoder renderPass)
{
	// Start the Dear ImGui frame
	ImGui_ImplWGPU_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// simulator.onGUI();
	defineGUI();

	// Draw the UI
	ImGui::EndFrame();
	// Convert the UI defined above into low-level drawing commands
	ImGui::Render();
	// Execute the low-level drawing commands on the WebGPU backend
	ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), renderPass);
}
