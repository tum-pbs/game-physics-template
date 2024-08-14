
#include "Renderer.h"

#include <glfw3webgpu.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <backends/imgui_impl_wgpu.h>
#include <backends/imgui_impl_glfw.h>

#include "Primitives.h"

using namespace wgpu;

#ifndef RESOURCE_DIR
#define RESOURCE_DIR "this will be defined by cmake depending on the build type. This define is to disable error squiggles"
#endif

Renderer::Renderer()
{
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
	m_imagePipeline.init(m_device, m_swapChainFormat, m_queue);
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
	updateViewMatrix();
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

	// prepare image buffers
	m_imagePipeline.updateImages(m_images, m_imageData);

	TextureView nextTexture = m_swapChain.getCurrentTextureView();
	if (!nextTexture)
		throw std::runtime_error("Could not get next texture!");

	CommandEncoderDescriptor commandEncoderDesc;
	commandEncoderDesc.label = "Command Encoder";
	CommandEncoder encoder = m_device.createCommandEncoder(commandEncoderDesc);

	RenderPassDescriptor renderPassDesc{};

	vec3 correctedBackground = glm::pow(backgroundColor, vec3(2.2f));
	Color correctedBackgroundColor = Color{correctedBackground.r, correctedBackground.g, correctedBackground.b, 1.0f};

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

	m_imagePipeline.draw(renderPassPost);

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
	terminateDepthBuffer();
	terminateSwapChain();
	terminateRenderTexture();

	initSwapChain();
	initDepthBuffer();
	initRenderTexture();
	m_postProcessingPipeline.updateBindGroup(m_postTextureView);

	updateProjectionMatrix();
}

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

	int largestVertexBuffer = sizeof(ResourceManager::InstancedVertexAttributes);

	RequiredLimits requiredLimits = Default;
	requiredLimits.limits.maxVertexAttributes = 8;
	requiredLimits.limits.maxVertexBuffers = 3;
	requiredLimits.limits.maxBufferSize = 150000 * largestVertexBuffer;
	requiredLimits.limits.maxVertexBufferArrayStride = largestVertexBuffer;
	requiredLimits.limits.minStorageBufferOffsetAlignment = supportedLimits.limits.minStorageBufferOffsetAlignment;
	requiredLimits.limits.minUniformBufferOffsetAlignment = supportedLimits.limits.minUniformBufferOffsetAlignment;
	requiredLimits.limits.maxInterStageShaderComponents = 17;
	requiredLimits.limits.maxBindGroups = 2;
	requiredLimits.limits.maxUniformBuffersPerShaderStage = 2;
	requiredLimits.limits.maxUniformBufferBindingSize = 16 * 4 * sizeof(float);
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

	glfwSetWindowUserPointer(m_window, this);
	glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow *window, int, int)
								   {
		auto that = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
		if (that != nullptr) that->onResize(); });

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
	glfwGetFramebufferSize(m_window, &width, &height);

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
	BufferDescriptor bufferDesc;
	bufferDesc.size = sizeof(LightingUniforms);
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Uniform;
	bufferDesc.mappedAtCreation = false;
	m_lightingUniformBuffer = m_device.createBuffer(bufferDesc);

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
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::GetIO();

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
	ImGui_ImplWGPU_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	defineGUI();

	ImGui::EndFrame();

	ImGui::Render();
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
	m_images.push_back({screenPosition.x, screenPosition.y, screenSize.x, screenSize.y, offset, width, height, 0.0f});
}
