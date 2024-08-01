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

#include <chrono>

#include "Primitives.h"

using namespace wgpu;
using VertexAttributes = ResourceManager::VertexAttributes;

#ifndef RESOURCE_DIR
#define RESOURCE_DIR "this will be defined by cmake depending on the build type. This define is to disable error squiggles"
#endif
///////////////////////////////////////////////////////////////////////////////
// Public methods

Renderer::Renderer()
{
	// TODO: do some more robust initialization
	initWindowAndDevice();
	glfwGetFramebufferSize(m_window, &width, &height);
	initSwapChain();
	initDepthBuffer();
	initRenderTexture();
	initUniforms();
	initLightingUniforms();
	m_instancingPipeline.init(m_device, m_queue, m_swapChainFormat, m_depthTextureFormat, m_uniformBuffer, m_lightingUniformBuffer);
	m_linePipeline.init(m_device, m_queue, m_swapChainFormat, m_depthTextureFormat, m_uniformBuffer, m_lightingUniformBuffer);
	m_postProcessingPipeline.init(m_device, m_swapChainFormat, m_postTextureView);
	initGui();
}

Camera Renderer::camera = Camera();

void Renderer::onFrame()
{
	auto startTime = std::chrono::high_resolution_clock::now();
	glfwPollEvents();
	updateLightingUniforms();
	if (reinitSwapChain)
	{
		terminateSwapChain();
		initSwapChain();
		reinitSwapChain = false;
	}

	// Update uniform buffer
	m_uniforms.time = static_cast<float>(glfwGetTime());
	m_queue.writeBuffer(m_uniformBuffer, offsetof(MyUniforms, time), &m_uniforms.time, sizeof(MyUniforms::time));
	m_queue.writeBuffer(m_uniformBuffer, offsetof(MyUniforms, cullingNormal), &m_uniforms.cullingNormal, sizeof(MyUniforms::cullingNormal));
	m_queue.writeBuffer(m_uniformBuffer, offsetof(MyUniforms, cullingOffset), &m_uniforms.cullingOffset, sizeof(MyUniforms::cullingOffset));
	m_queue.writeBuffer(m_uniformBuffer, offsetof(MyUniforms, flags), &m_uniforms.flags, sizeof(MyUniforms::flags));

	// prepare instanced draw calls
	m_instancingPipeline.updateCubes(m_cubes);
	m_instancingPipeline.updateSpheres(m_spheres);
	m_instancingPipeline.updateQuads(m_quads);

	// prepare line buffers
	m_linePipeline.updateLines(m_lines);

	TextureView nextTexture = m_swapChain.getCurrentTextureView();
	if (!nextTexture)
		throw std::runtime_error("Could not get next texture!");

	CommandEncoderDescriptor commandEncoderDesc;
	commandEncoderDesc.label = "Command Encoder";
	CommandEncoder encoder = m_device.createCommandEncoder(commandEncoderDesc);

	RenderPassDescriptor renderPassDesc{};

	vec3 correctedBackground = glm::pow(backgroundColor, vec3(2.2f));
	Color correctedBackgroundColor = Color{correctedBackground.r, correctedBackground.g, correctedBackground.b, 1.0f};

	// TODO maybe rewrite the texture view descriptor
	TextureViewDescriptor postProcessTextureViewDesc;
	postProcessTextureViewDesc.aspect = TextureAspect::All;
	postProcessTextureViewDesc.baseArrayLayer = 0;
	postProcessTextureViewDesc.arrayLayerCount = 1;
	postProcessTextureViewDesc.baseMipLevel = 0;
	postProcessTextureViewDesc.mipLevelCount = 1;
	postProcessTextureViewDesc.dimension = TextureViewDimension::_2D;
	postProcessTextureViewDesc.format = m_swapChainFormat;

	RenderPassColorAttachment renderPassColorAttachment{};
	renderPassColorAttachment.view = m_postTextureView;
	renderPassColorAttachment.resolveTarget = nullptr;
	renderPassColorAttachment.loadOp = LoadOp::Clear;
	renderPassColorAttachment.storeOp = StoreOp::Store;
	renderPassColorAttachment.clearValue = correctedBackgroundColor;
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

	m_linePipeline.drawLines(renderPass);

	m_instancingPipeline.draw(renderPass);

	renderPass.end();
	renderPass.release();

	RenderPassColorAttachment renderPassColorAttachment2{};
	renderPassColorAttachment2.view = nextTexture;
	renderPassColorAttachment2.resolveTarget = nullptr;
	renderPassColorAttachment2.loadOp = LoadOp::Clear;
	renderPassColorAttachment2.storeOp = StoreOp::Store;
	renderPassColorAttachment2.clearValue = correctedBackgroundColor;
	RenderPassDescriptor postProcessRenderPassDesc{};
	postProcessRenderPassDesc.colorAttachmentCount = 1;
	postProcessRenderPassDesc.colorAttachments = &renderPassColorAttachment2;
	postProcessRenderPassDesc.timestampWriteCount = 0;
	postProcessRenderPassDesc.timestampWrites = nullptr;
	RenderPassEncoder renderPassPost = encoder.beginRenderPass(postProcessRenderPassDesc);

	m_postProcessingPipeline.draw(renderPassPost);

	updateGui(renderPassPost);
	renderPassPost.end();
	renderPassPost.release();

	nextTexture.release();

	CommandBufferDescriptor cmdBufferDescriptor{};
	cmdBufferDescriptor.label = "Command buffer";
	CommandBuffer command = encoder.finish(cmdBufferDescriptor);
	encoder.release();
	m_queue.submit(command);
	command.release();

	m_swapChain.present();
	lastDrawTime = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - startTime).count();

#ifdef WEBGPU_BACKEND_DAWN
	// Check for pending error callbacks
	m_device.tick();
#endif
}

Renderer::~Renderer()
{
	terminateGui();
	terminateLightingUniforms();
	terminateUniforms();
	m_instancingPipeline.terminate();
	m_linePipeline.terminate();
	m_postProcessingPipeline.terminate();
	terminateRenderTexture();
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
	glfwGetFramebufferSize(m_window, &width, &height);
	// Terminate in reverse order
	terminateDepthBuffer();
	terminateSwapChain();
	terminateRenderTexture();

	// Re-init
	initSwapChain();
	initDepthBuffer();
	initRenderTexture();
	m_postProcessingPipeline.updateBindGroup(m_postTextureView);

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
		m_cameraState.angles.y = glm::clamp(m_cameraState.angles.y, -glm::half_pi<float>() + 1e-5f, glm::half_pi<float>() - 1e-5f);
		updateViewMatrix();
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

void Renderer::initWindowAndDevice()
{
	m_instance = createInstance(InstanceDescriptor{});
	if (!m_instance)
		throw std::runtime_error("Could not initialize WebGPU!");

	if (!glfwInit())
		throw std::runtime_error("Could not initialize GLFW!");

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
	m_window = glfwCreateWindow(640, 480, "Game Physics Template", NULL, NULL);
	if (!m_window)
		throw std::runtime_error("Could not open window!");

	m_surface = glfwGetWGPUSurface(m_instance, m_window);

	if (!m_surface)
		throw std::runtime_error("Could not create surface!");

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
	requiredLimits.limits.maxTextureDimension1D = 8192;
	requiredLimits.limits.maxTextureDimension2D = 8192;
	requiredLimits.limits.maxTextureArrayLayers = 1;
	requiredLimits.limits.maxSampledTexturesPerShaderStage = 2;
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
	if (m_device == nullptr)
		throw std::runtime_error("Could not create device!");
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

void Renderer::setPresentMode(PresentMode mode)
{
	if (mode == presentMode)
		return;
	presentMode = mode;
	reinitSwapChain = true;
}

void Renderer::initSwapChain()
{
	SwapChainDescriptor swapChainDesc;
	swapChainDesc.width = static_cast<uint32_t>(width);
	swapChainDesc.height = static_cast<uint32_t>(height);
	swapChainDesc.usage = TextureUsage::RenderAttachment;
	swapChainDesc.format = m_swapChainFormat;
	swapChainDesc.presentMode = presentMode;
	m_swapChain = m_device.createSwapChain(m_surface, swapChainDesc);

	if (!m_swapChain)
		throw std::runtime_error("Could not create swap chain!");
}

void Renderer::terminateSwapChain()
{
	m_swapChain.release();
}

void Renderer::initRenderTexture()
{
	TextureDescriptor postProcessTextureDesc;
	postProcessTextureDesc.dimension = TextureDimension::_2D;
	postProcessTextureDesc.format = m_swapChainFormat;
	postProcessTextureDesc.size = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
	postProcessTextureDesc.usage = TextureUsage::CopySrc | TextureUsage::TextureBinding | TextureUsage::RenderAttachment;
	postProcessTextureDesc.sampleCount = 1;
	postProcessTextureDesc.mipLevelCount = 1;
	postProcessTextureDesc.viewFormatCount = 1;
	postProcessTextureDesc.viewFormats = (WGPUTextureFormat *)&m_swapChainFormat;
	m_postTexture = m_device.createTexture(postProcessTextureDesc);

	TextureViewDescriptor postProcessTextureViewDesc;
	postProcessTextureViewDesc.aspect = TextureAspect::All;
	postProcessTextureViewDesc.baseArrayLayer = 0;
	postProcessTextureViewDesc.arrayLayerCount = 1;
	postProcessTextureViewDesc.baseMipLevel = 0;
	postProcessTextureViewDesc.mipLevelCount = 1;
	postProcessTextureViewDesc.dimension = TextureViewDimension::_2D;
	postProcessTextureViewDesc.format = m_postTexture.getFormat();
	m_postTextureView = m_postTexture.createView(postProcessTextureViewDesc);

	if (m_postTexture == nullptr)
		throw std::runtime_error("Could not create post process texture!");
	if (m_postTextureView == nullptr)
		throw std::runtime_error("Could not create post process texture view!");
}

void Renderer::terminateRenderTexture()
{
	m_postTexture.destroy();
	m_postTexture.release();
}
void Renderer::initDepthBuffer()
{
	// Get the current size of the window's framebuffer:
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

	if (m_depthTexture == nullptr)
		throw std::runtime_error("Could not create depth texture!");

	if (m_depthTextureView == nullptr)
		throw std::runtime_error("Could not create depth texture view!");
}

void Renderer::terminateDepthBuffer()
{
	m_depthTextureView.release();
	m_depthTexture.destroy();
	m_depthTexture.release();
}

void Renderer::clearScene()
{
	m_cubes.clear();
	m_spheres.clear();
	m_quads.clear();
	m_lines.clear();
	m_images.clear();
	m_imageData.clear();
	current_id = 0;
}

void Renderer::initUniforms()
{
	BufferDescriptor bufferDesc;
	bufferDesc.size = sizeof(MyUniforms);
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Uniform;
	bufferDesc.mappedAtCreation = false;
	m_uniformBuffer = m_device.createBuffer(bufferDesc);
	m_uniforms.viewMatrix = camera.viewMatrix;
	m_uniforms.projectionMatrix = camera.projectionMatrix();
	m_uniforms.time = 1.0f;
	m_uniforms.cullingNormal = {0.0f, 0.0f, 1.0f};
	m_uniforms.cullingOffset = 0.0f;
	m_uniforms.flags = 0;
	m_queue.writeBuffer(m_uniformBuffer, 0, &m_uniforms, sizeof(MyUniforms));

	updateProjectionMatrix();
	updateViewMatrix();

	if (m_uniformBuffer == nullptr)
		throw std::runtime_error("Could not create uniform buffer!");
}

void Renderer::terminateUniforms()
{
	m_uniformBuffer.destroy();
	m_uniformBuffer.release();
}

void Renderer::initLightingUniforms()
{
	// Create uniform buffer
	BufferDescriptor bufferDesc;
	bufferDesc.size = sizeof(LightingUniforms);
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Uniform;
	bufferDesc.mappedAtCreation = false;
	m_lightingUniformBuffer = m_device.createBuffer(bufferDesc);

	// Initial values
	m_lightingUniforms.direction = vec3(2.0, 0.5, 1);
	m_lightingUniforms.ambient = vec3(1, 1, 1);
	m_lightingUniforms.ambient_intensity = 0.1f;
	m_lightingUniforms.specular = vec3(1, 1, 1);
	m_lightingUniforms.diffuse_intensity = 1.0f;
	m_lightingUniforms.specular_intensity = 0.2f;
	m_lightingUniforms.alpha = 32;

	updateLightingUniforms();

	if (m_lightingUniformBuffer == nullptr)
		throw std::runtime_error("Could not create lighting uniform buffer!");
}

void Renderer::terminateLightingUniforms()
{
	m_lightingUniformBuffer.destroy();
	m_lightingUniformBuffer.release();
}

void Renderer::updateLightingUniforms()
{
	m_queue.writeBuffer(m_lightingUniformBuffer, 0, &m_lightingUniforms, sizeof(LightingUniforms));
}

void Renderer::updateProjectionMatrix()
{
	glfwGetFramebufferSize(m_window, &camera.width, &camera.height);
	m_uniforms.projectionMatrix = camera.projectionMatrix();
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
	camera.position = vec3(cx * cy, sx * cy, sy) * std::exp(-m_cameraState.zoom);
	camera.lookAt(vec3(0.0f));
	m_uniforms.viewMatrix = camera.viewMatrix;
	m_queue.writeBuffer(
		m_uniformBuffer,
		offsetof(MyUniforms, viewMatrix),
		&m_uniforms.viewMatrix,
		sizeof(MyUniforms::viewMatrix));
	m_uniforms.cameraWorldPosition = camera.position;
	m_queue.writeBuffer(
		m_uniformBuffer,
		offsetof(MyUniforms, cameraWorldPosition),
		&m_uniforms.cameraWorldPosition,
		sizeof(MyUniforms::cameraWorldPosition));
}

void Renderer::initGui()
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::GetIO();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOther(m_window, true);
	ImGui_ImplWGPU_Init(m_device, 3, m_swapChainFormat);
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

uint32_t Renderer::drawCube(glm::vec3 position, glm::quat rotation, glm::vec3 scale, glm::vec3 color, uint32_t flags)
{
	return drawCube(position, rotation, scale, glm::vec4(color, 1.0f), flags);
}

uint32_t Renderer::drawSphere(glm::vec3 position, glm::quat rotation, glm::vec3 scale, glm::vec4 color, uint32_t flags)
{
	m_spheres.push_back({position, rotation, scale, color, current_id, flags});
	return current_id++;
}

uint32_t Renderer::drawSphere(glm::vec3 position, float scale, glm::vec3 color, uint32_t flags)
{
	return drawSphere(position, glm::quat(vec3(0)), vec3(scale), glm::vec4(color, 1.0f), flags);
}

uint32_t Renderer::drawQuad(glm::vec3 position, glm::quat rotation, glm::vec3 scale, glm::vec4 color, uint32_t flags)
{
	m_quads.push_back({position, rotation, scale, color, current_id, flags});
	return current_id++;
}

uint32_t Renderer::drawQuad(glm::vec3 position, glm::quat rotation, glm::vec3 scale, glm::vec3 color, uint32_t flags)
{
	return drawQuad(position, rotation, scale, glm::vec4(color, 1.0f), flags);
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

void Renderer::drawWireCube(vec3 position, vec3 scale, vec3 color)
{
	std::vector<vec3> points =
		{
			{0, 0, 0},
			{0, 0, 1},

			{0, 0, 0},
			{0, 1, 0},

			{0, 0, 0},
			{1, 0, 0},

			{0, 0, 1},
			{1, 0, 1},

			{0, 0, 1},
			{0, 1, 1},

			{0, 1, 0},
			{1, 1, 0},

			{0, 1, 0},
			{0, 1, 1},

			{1, 0, 0},
			{1, 1, 0},

			{1, 0, 0},
			{1, 0, 1},

			{1, 1, 0},
			{1, 1, 1},

			{1, 0, 1},
			{1, 1, 1},

			{0, 1, 1},
			{1, 1, 1}};
	for (vec3 &point : points)
	{
		point -= 0.5;
		point *= scale;
		point += position;
	}
	for (int i = 0; i < points.size() / 2; i++)
	{
		drawLine(points[i * 2], points[i * 2 + 1], color);
	}
}

void Renderer::drawImage(std::vector<float> data, int height, int width, glm::vec2 screenPosition, glm::vec2 screenSize)
{
	// expects
	// [[0,1,2,3],
	//  [4,5,6,7],
	//  [8,9,10,11]]
	// for width = 4, height = 3
	// v.insert(v.end(), std::begin(a), std::end(a));
	int offset = m_imageData.size();
	m_imageData.insert(m_imageData.end(), data.begin(), data.end());
	m_images.push_back({screenPosition, screenSize, offset, width, height});
}
