
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
	glfwGetFramebufferSize(window, &width, &height);
	initSwapChain();
	initDepthBuffer();
	initRenderTexture();
	initUniforms();
	initLightingUniforms();
	instancingPipeline.init(device, queue, swapChainFormat, depthTextureFormat, uniformBuffer, lightingUniformBuffer);
	linePipeline.init(device, queue, swapChainFormat, depthTextureFormat, uniformBuffer, lightingUniformBuffer);
	postProcessingPipeline.init(device, swapChainFormat, postTextureView);
	imagePipeline.init(device, swapChainFormat, queue);
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
	renderUniforms.time = static_cast<float>(glfwGetTime());
	updateViewMatrix();
	queue.writeBuffer(uniformBuffer, offsetof(RenderUniforms, time), &renderUniforms.time, sizeof(RenderUniforms::time));
	queue.writeBuffer(uniformBuffer, offsetof(RenderUniforms, cullingNormal), &renderUniforms.cullingNormal, sizeof(RenderUniforms::cullingNormal));
	queue.writeBuffer(uniformBuffer, offsetof(RenderUniforms, cullingOffset), &renderUniforms.cullingOffset, sizeof(RenderUniforms::cullingOffset));
	queue.writeBuffer(uniformBuffer, offsetof(RenderUniforms, flags), &renderUniforms.flags, sizeof(RenderUniforms::flags));

	// prepare instanced draw calls
	instancingPipeline.commit();

	// prepare line buffers
	linePipeline.commit();

	// prepare image buffers
	imagePipeline.commit();

	TextureView nextTexture = swapChain.getCurrentTextureView();
	if (!nextTexture)
		throw std::runtime_error("Could not get next texture!");

	CommandEncoderDescriptor commandEncoderDesc;
	commandEncoderDesc.label = "Command Encoder";
	CommandEncoder encoder = device.createCommandEncoder(commandEncoderDesc);

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
	postProcessTextureViewDesc.format = swapChainFormat;

	RenderPassColorAttachment renderPassColorAttachment{};
	renderPassColorAttachment.view = postTextureView;
	renderPassColorAttachment.resolveTarget = nullptr;
	renderPassColorAttachment.loadOp = LoadOp::Clear;
	renderPassColorAttachment.storeOp = StoreOp::Store;
	renderPassColorAttachment.clearValue = correctedBackgroundColor;
	renderPassDesc.colorAttachmentCount = 1;
	renderPassDesc.colorAttachments = &renderPassColorAttachment;

	RenderPassDepthStencilAttachment depthStencilAttachment;
	depthStencilAttachment.view = depthTextureView;
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

	linePipeline.draw(renderPass);

	instancingPipeline.draw(renderPass);

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

	postProcessingPipeline.draw(renderPassPost);

	imagePipeline.draw(renderPassPost);

	updateGui(renderPassPost);

	renderPassPost.end();
	renderPassPost.release();

	nextTexture.release();

	CommandBufferDescriptor cmdBufferDescriptor{};
	cmdBufferDescriptor.label = "Command buffer";
	CommandBuffer command = encoder.finish(cmdBufferDescriptor);
	encoder.release();
	queue.submit(command);
	command.release();

	swapChain.present();
	lastDrawTime = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - startTime).count();

#ifdef WEBGPU_BACKEND_DAWN
	// Check for pending error callbacks
	device.tick();
#endif
}

Renderer::~Renderer()
{
	terminateGui();
	terminateLightingUniforms();
	terminateUniforms();
	instancingPipeline.terminate();
	linePipeline.terminate();
	postProcessingPipeline.terminate();
	terminateRenderTexture();
	terminateDepthBuffer();
	terminateSwapChain();
	terminateWindowAndDevice();
}

bool Renderer::isRunning()
{
	return !glfwWindowShouldClose(window);
}

void Renderer::onResize()
{
	glfwGetFramebufferSize(window, &width, &height);
	if (width == 0 || height == 0)
		return;
	terminateDepthBuffer();
	terminateSwapChain();
	terminateRenderTexture();

	initSwapChain();
	initDepthBuffer();
	initRenderTexture();
	postProcessingPipeline.updateBindGroup(postTextureView);

	updateProjectionMatrix();
}

void Renderer::initWindowAndDevice()
{
	instance = createInstance(InstanceDescriptor{});
	if (!instance)
		throw std::runtime_error("Could not initialize WebGPU!");

	if (!glfwInit())
		throw std::runtime_error("Could not initialize GLFW!");

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
	window = glfwCreateWindow(640, 480, "Game Physics Template", NULL, NULL);
	if (!window)
		throw std::runtime_error("Could not open window!");

	surface = glfwGetWGPUSurface(instance, window);

	if (!surface)
		throw std::runtime_error("Could not create surface!");

	RequestAdapterOptions adapterOpts{};
	adapterOpts.compatibleSurface = surface;
	Adapter adapter = instance.requestAdapter(adapterOpts);

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
	device = adapter.requestDevice(deviceDesc);

	errorCallbackHandle = device.setUncapturedErrorCallback([](ErrorType type, char const *message)
															{
		std::cout << "Device error: type " << type;
		if (message) std::cout << " (message: " << message << ")";
		std::cout << std::endl; });

	queue = device.getQueue();

#ifdef WEBGPU_BACKEND_WGPU
	swapChainFormat = surface.getPreferredFormat(adapter);
#else
	swapChainFormat = TextureFormat::BGRA8Unorm;
#endif

	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, [](GLFWwindow *window, int, int)
								   {
		auto that = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
		if (that != nullptr) that->onResize(); });

	adapter.release();
	if (device == nullptr)
		throw std::runtime_error("Could not create device!");
}

void Renderer::terminateWindowAndDevice()
{
	queue.release();
	device.release();
	surface.release();
	instance.release();

	glfwDestroyWindow(window);
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
	swapChainDesc.format = swapChainFormat;
	swapChainDesc.presentMode = presentMode;
	swapChain = device.createSwapChain(surface, swapChainDesc);

	if (!swapChain)
		throw std::runtime_error("Could not create swap chain!");
}

void Renderer::terminateSwapChain()
{
	swapChain.release();
}

void Renderer::initRenderTexture()
{
	TextureDescriptor postProcessTextureDesc;
	postProcessTextureDesc.dimension = TextureDimension::_2D;
	postProcessTextureDesc.format = swapChainFormat;
	postProcessTextureDesc.size = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
	postProcessTextureDesc.usage = TextureUsage::CopySrc | TextureUsage::TextureBinding | TextureUsage::RenderAttachment;
	postProcessTextureDesc.sampleCount = 1;
	postProcessTextureDesc.mipLevelCount = 1;
	postProcessTextureDesc.viewFormatCount = 1;
	postProcessTextureDesc.viewFormats = (WGPUTextureFormat *)&swapChainFormat;
	postTexture = device.createTexture(postProcessTextureDesc);

	TextureViewDescriptor postProcessTextureViewDesc;
	postProcessTextureViewDesc.aspect = TextureAspect::All;
	postProcessTextureViewDesc.baseArrayLayer = 0;
	postProcessTextureViewDesc.arrayLayerCount = 1;
	postProcessTextureViewDesc.baseMipLevel = 0;
	postProcessTextureViewDesc.mipLevelCount = 1;
	postProcessTextureViewDesc.dimension = TextureViewDimension::_2D;
	postProcessTextureViewDesc.format = postTexture.getFormat();
	postTextureView = postTexture.createView(postProcessTextureViewDesc);

	if (postTexture == nullptr)
		throw std::runtime_error("Could not create post process texture!");
	if (postTextureView == nullptr)
		throw std::runtime_error("Could not create post process texture view!");
}

void Renderer::terminateRenderTexture()
{
	postTexture.destroy();
	postTexture.release();
}
void Renderer::initDepthBuffer()
{
	glfwGetFramebufferSize(window, &width, &height);

	TextureDescriptor depthTextureDesc;
	depthTextureDesc.dimension = TextureDimension::_2D;
	depthTextureDesc.format = depthTextureFormat;
	depthTextureDesc.mipLevelCount = 1;
	depthTextureDesc.sampleCount = 1;
	depthTextureDesc.size = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
	depthTextureDesc.usage = TextureUsage::RenderAttachment;
	depthTextureDesc.viewFormatCount = 1;
	depthTextureDesc.viewFormats = (WGPUTextureFormat *)&depthTextureFormat;
	depthTexture = device.createTexture(depthTextureDesc);

	TextureViewDescriptor depthTextureViewDesc;
	depthTextureViewDesc.aspect = TextureAspect::DepthOnly;
	depthTextureViewDesc.baseArrayLayer = 0;
	depthTextureViewDesc.arrayLayerCount = 1;
	depthTextureViewDesc.baseMipLevel = 0;
	depthTextureViewDesc.mipLevelCount = 1;
	depthTextureViewDesc.dimension = TextureViewDimension::_2D;
	depthTextureViewDesc.format = depthTextureFormat;
	depthTextureView = depthTexture.createView(depthTextureViewDesc);

	if (depthTexture == nullptr)
		throw std::runtime_error("Could not create depth texture!");

	if (depthTextureView == nullptr)
		throw std::runtime_error("Could not create depth texture view!");
}

void Renderer::terminateDepthBuffer()
{
	depthTextureView.release();
	depthTexture.destroy();
	depthTexture.release();
}

void Renderer::clearScene()
{
	instancingPipeline.clearAll();
	linePipeline.clearAll();
	imagePipeline.clearAll();
	current_id = 0;
}

void Renderer::initUniforms()
{
	BufferDescriptor bufferDesc;
	bufferDesc.size = sizeof(RenderUniforms);
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Uniform;
	bufferDesc.mappedAtCreation = false;
	uniformBuffer = device.createBuffer(bufferDesc);
	renderUniforms.viewMatrix = camera.viewMatrix;
	renderUniforms.projectionMatrix = camera.projectionMatrix();
	renderUniforms.time = 1.0f;
	renderUniforms.cullingNormal = {0.0f, 0.0f, 1.0f};
	renderUniforms.cullingOffset = 0.0f;
	renderUniforms.flags = 0;
	queue.writeBuffer(uniformBuffer, 0, &renderUniforms, sizeof(RenderUniforms));

	updateProjectionMatrix();
	updateViewMatrix();

	if (uniformBuffer == nullptr)
		throw std::runtime_error("Could not create uniform buffer!");
}

void Renderer::terminateUniforms()
{
	uniformBuffer.destroy();
	uniformBuffer.release();
}

void Renderer::initLightingUniforms()
{
	BufferDescriptor bufferDesc;
	bufferDesc.size = sizeof(LightingUniforms);
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Uniform;
	bufferDesc.mappedAtCreation = false;
	lightingUniformBuffer = device.createBuffer(bufferDesc);

	lightingUniforms.direction = vec3(2.0, 0.5, 1);
	lightingUniforms.ambient = vec3(1, 1, 1);
	lightingUniforms.ambient_intensity = 0.1f;
	lightingUniforms.specular = vec3(1, 1, 1);
	lightingUniforms.diffuse_intensity = 1.0f;
	lightingUniforms.specular_intensity = 0.2f;
	lightingUniforms.alpha = 32;

	updateLightingUniforms();

	if (lightingUniformBuffer == nullptr)
		throw std::runtime_error("Could not create lighting uniform buffer!");
}

void Renderer::terminateLightingUniforms()
{
	lightingUniformBuffer.destroy();
	lightingUniformBuffer.release();
}

void Renderer::updateLightingUniforms()
{
	queue.writeBuffer(lightingUniformBuffer, 0, &lightingUniforms, sizeof(LightingUniforms));
}

void Renderer::updateProjectionMatrix()
{
	glfwGetFramebufferSize(window, &camera.width, &camera.height);
	renderUniforms.projectionMatrix = camera.projectionMatrix();
	queue.writeBuffer(
		uniformBuffer,
		offsetof(RenderUniforms, projectionMatrix),
		&renderUniforms.projectionMatrix,
		sizeof(RenderUniforms::projectionMatrix));
}

void Renderer::updateViewMatrix()
{
	renderUniforms.viewMatrix = camera.viewMatrix;
	queue.writeBuffer(
		uniformBuffer,
		offsetof(RenderUniforms, viewMatrix),
		&renderUniforms.viewMatrix,
		sizeof(RenderUniforms::viewMatrix));
	renderUniforms.cameraWorldPosition = camera.position;
	queue.writeBuffer(
		uniformBuffer,
		offsetof(RenderUniforms, cameraWorldPosition),
		&renderUniforms.cameraWorldPosition,
		sizeof(RenderUniforms::cameraWorldPosition));
}

void Renderer::initGui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::GetIO();

	ImGui_ImplGlfw_InitForOther(window, true);
	ImGui_ImplWGPU_Init(device, 3, swapChainFormat);
}

void Renderer::terminateGui()
{
	ImGui_ImplGlfw_Shutdown();
	ImGui_ImplWGPU_Shutdown();
}

uint32_t Renderer::drawCube(glm::vec3 position, glm::quat rotation, glm::vec3 scale, glm::vec4 color, uint32_t flags)
{
	instancingPipeline.addCube({position,
								rotation,
								scale,
								color,
								current_id,
								flags});
	return current_id++;
}

uint32_t Renderer::drawEllipsoid(glm::vec3 position, glm::quat rotation, glm::vec3 scale, glm::vec4 color, uint32_t flags)
{
	instancingPipeline.addSphere({position, rotation, scale, color, current_id, flags});
	return current_id++;
}

uint32_t Renderer::drawSphere(glm::vec3 position, float scale, glm::vec4 color, uint32_t flags)
{
	return drawEllipsoid(position, glm::quat(vec3(0)), vec3(scale), color, flags);
}

uint32_t Renderer::drawQuad(glm::vec3 position, glm::quat rotation, glm::vec2 scale, glm::vec4 color, uint32_t flags)
{
	instancingPipeline.addQuad({position, rotation, vec3(scale.x, scale.y, 1), color, current_id, flags});
	return current_id++;
}

void Renderer::drawLine(glm::vec3 position1, glm::vec3 position2, glm::vec3 color1, glm::vec3 color2)
{
	linePipeline.addLine({{position1, color1}, {position2, color2}});
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

	if (defineGUI != nullptr)
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

void Renderer::drawImage(std::vector<float> data, int height, int width, Colormap colormap, glm::vec2 screenPosition, glm::vec2 screenSize)
{
	auto [min, max] = std::minmax_element(data.begin(), data.end());
	drawImage(data, height, width, *min, *max, colormap, screenPosition, screenSize);
}

void Renderer::drawImage(std::vector<float> data, int height, int width, float vmin, float vmax, Colormap colormap, glm::vec2 screenPosition, glm::vec2 screenSize)
{
	// expects
	// [[0,1,2,3],
	//  [4,5,6,7],
	//  [8,9,10,11]]
	// for width = 4, height = 3
	for (float &value : data)
	{
		value = (value - vmin) / (vmax - vmin);
	}
	imagePipeline.addImage(data, screenPosition, screenSize, width, height, colormap);
}
