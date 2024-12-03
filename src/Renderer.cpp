
#include "Renderer.h"

#include <glfw3webgpu.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <backends/imgui_impl_wgpu.h>
#include <backends/imgui_impl_glfw.h>

#include "Primitives.h"
#include <algorithm>

using namespace wgpu;

#ifndef RESOURCE_DIR
#define RESOURCE_DIR "this will be defined by cmake depending on the build type. This define is to disable error squiggles"
#endif

Renderer::Renderer(bool _verbose): verbose(_verbose){
	initWindowAndDevice();
	if(verbose)
		std::cout << "Initialized Window and Device" << std::endl;
	glfwGetFramebufferSize(window, &width, &height);
	if(verbose)
		std::cout << "Framebuffer size: " << width << "x" << height << std::endl;
	initSwapChain();
	if(verbose)
		std::cout << "Initialized Swapchain" << std::endl;
	initDepthBuffer();
	if(verbose)
		std::cout << "Initialized Depth Buffer" << std::endl;
	initRenderTexture();
	if(verbose)
		std::cout << "Initialized Render Texture" << std::endl;
	initUniforms();
	if(verbose)
		std::cout << "Initialized Uniforms" << std::endl;
	initLightingUniforms();
	if(verbose)
		std::cout << "Initialized Lighting Uniforms" << std::endl;
	instancingPipeline.init(device, queue, swapChainFormat, depthTextureFormat, uniformBuffer, lightingUniformBuffer);
	if(verbose)
		std::cout << "Initialized Instancing Pipeline" << std::endl;
	linePipeline.init(device, queue, swapChainFormat, depthTextureFormat, uniformBuffer, lightingUniformBuffer);
	if(verbose)
		std::cout << "Initialized Line Pipeline" << std::endl;
	postProcessingPipeline.init(device, swapChainFormat, postTextureView);
	if(verbose)
		std::cout << "Initialized Post Processing Pipeline" << std::endl;
	imagePipeline.init(device, swapChainFormat, queue);
	if(verbose)
		std::cout << "Initialized Image Pipeline" << std::endl;
	initGui();
	if(verbose)
		std::cout << "Initialized GUI" << std::endl;

	wgpu::SurfaceCapabilities capabilities;
	surface.getCapabilities(adapter, &capabilities);
	// std::map<int64_t, bool> supportedPresentModes;
	for (uint32_t i = 0; i < capabilities.presentModeCount; i++){
		supportedPresentModes[capabilities.presentModes[i]] = true;
	}

	if(verbose){
		std::cout << "Capabilities: " << std::endl;
		std::cout << "Next in chain: " << capabilities.nextInChain << std::endl;
		std::cout << "Number of formats: " << capabilities.formatCount << std::endl;
		for (uint32_t i = 0; i < capabilities.formatCount; i++)
			std::cout << "Format [" << i << "] : " << capabilities.formats[i] << std::endl;
		std::cout << "Number of present modes: " << capabilities.presentModeCount << std::endl;
		if(supportedPresentModes.find(WGPUPresentMode_Fifo) != supportedPresentModes.end()){
			std::cout << "Fifo is supported" << std::endl;
		}
		if(supportedPresentModes.find(WGPUPresentMode_FifoRelaxed) != supportedPresentModes.end()){
			std::cout << "Fifo Relaxed is supported" << std::endl;
		}
		if(supportedPresentModes.find(WGPUPresentMode_Immediate) != supportedPresentModes.end()){
			std::cout << "Immediate is supported" << std::endl;
		}
		if(supportedPresentModes.find(WGPUPresentMode_Mailbox) != supportedPresentModes.end()){
			std::cout << "Mailbox is supported" << std::endl;
		}
		for (uint32_t i = 0; i < capabilities.presentModeCount; i++)
			std::cout << "Capability << [" << i << "] : " << capabilities.presentModes[i] << std::endl;
		std::cout << "Number of alpha modes: " << capabilities.alphaModeCount << std::endl;
		for (uint32_t i = 0; i < capabilities.alphaModeCount; i++)
			std::cout << "Alpha mode [" << i << "] : " << capabilities.alphaModes[i] << std::endl;
	}
}

Camera Renderer::camera = Camera();

void Renderer::onFrame()
{
	auto startTime = std::chrono::high_resolution_clock::now();
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
	queue.writeBuffer(uniformBuffer, offsetof(RenderUniforms, cullingOffsets), &renderUniforms.cullingOffsets, sizeof(RenderUniforms::cullingOffsets));
	queue.writeBuffer(uniformBuffer, offsetof(RenderUniforms, flags), &renderUniforms.flags, sizeof(RenderUniforms::flags));

	if (sortDepth)
		instancingPipeline.sortDepth();
	// prepare instanced draw calls
	instancingPipeline.commit();

	// prepare line buffers
	linePipeline.commit();

	// prepare image buffers
	imagePipeline.commit();

	// std::cout << "Preparing to draw" << std::endl;
	TextureView nextTextureView = GetNextSurfaceTextureView();

	// TextureView nextTextureView = GetNextSurfaceTextureView();
	// SurfaceTexture surfaceTexture;
	// surface.getCurrentTexture(&surfaceTexture);
	// if (surfaceTexture.status != SurfaceGetCurrentTextureStatus::Success) {
	// 	throw std::runtime_error("Could not get next texture!");
	// }
	// Texture nextTexture = surfaceTexture.texture;

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
	// std::cout << "Creating post process texture view" << std::endl;

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

	// renderPassDesc.timestampWriteCount = 0;
	renderPassDesc.timestampWrites = nullptr;
	RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc);

	linePipeline.draw(renderPass);
	// std::cout << "Drawing lines" << std::endl;

	instancingPipeline.draw(renderPass);
	// std::cout << "Drawing instanced objects" << std::endl;

	renderPass.end();
	renderPass.release();
	// std::cout << "Ending render pass" << std::endl;

	// auto nextTextureView = nextTexture.createView();

	RenderPassColorAttachment renderPassColorAttachment2{};
	renderPassColorAttachment2.view = nextTextureView;
	renderPassColorAttachment2.resolveTarget = nullptr;
	renderPassColorAttachment2.loadOp = LoadOp::Clear;
	renderPassColorAttachment2.storeOp = StoreOp::Store;
	renderPassColorAttachment2.clearValue = correctedBackgroundColor;
	RenderPassDescriptor postProcessRenderPassDesc{};
	postProcessRenderPassDesc.colorAttachmentCount = 1;
	postProcessRenderPassDesc.colorAttachments = &renderPassColorAttachment2;
	// postProcessRenderPassDesc.timestampWriteCount = 0;
	postProcessRenderPassDesc.timestampWrites = nullptr;
	RenderPassEncoder renderPassPost = encoder.beginRenderPass(postProcessRenderPassDesc);

	postProcessingPipeline.draw(renderPassPost);
	// std::cout << "Drawing post processing" << std::endl;

	imagePipeline.draw(renderPassPost);
	// std::cout << "Drawing images" << std::endl;

	updateGui(renderPassPost);
	// std::cout << "Updating GUI" << std::endl;

	renderPassPost.end();
	renderPassPost.release();
	// std::cout << "Ending post process render pass" << std::endl;

	CommandBufferDescriptor cmdBufferDescriptor{};
	cmdBufferDescriptor.label = "Command buffer";
	CommandBuffer command = encoder.finish(cmdBufferDescriptor);
	encoder.release();
	// std::cout << "Finishing command buffer" << std::endl;
	queue.submit(1, &command);
	// std::cout << "Submitting command buffer" << std::endl;
	command.release();
	// std::cout << "Releasing command buffer" << std::endl;

	nextTextureView.release();
	// std::cout << "Releasing next texture" << std::endl;

	// targetView.release();
	// swapChain.present();
	lastDrawTime = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - startTime).count();

	surface.present();

#ifdef WEBGPU_BACKEND_DAWN
	// Check for pending error callbacks
	device.tick();
#endif
	// device.poll(false);

	// glfwPollEvents();
	// if (reinitSwapChain)
	// {
	// 	terminateSwapChain();
	// 	initSwapChain();
	// 	reinitSwapChain = false;
	// }

}

TextureView Renderer::GetNextSurfaceTextureView() {
	// Get the surface texture
	SurfaceTexture surfaceTexture;
	surface.getCurrentTexture(&surfaceTexture);
	if (surfaceTexture.status != SurfaceGetCurrentTextureStatus::Success) {
		return nullptr;
	}
	Texture texture = surfaceTexture.texture;
	// texture.

	// Create a view for this surface texture
	TextureViewDescriptor viewDescriptor;
	viewDescriptor.label = "Surface texture view";
	viewDescriptor.format = texture.getFormat();
	viewDescriptor.dimension = TextureViewDimension::_2D;
	viewDescriptor.baseMipLevel = 0;
	viewDescriptor.mipLevelCount = 1;
	viewDescriptor.baseArrayLayer = 0;
	viewDescriptor.arrayLayerCount = 1;
	viewDescriptor.aspect = TextureAspect::All;
	TextureView targetView = texture.createView(viewDescriptor);
	// if(texture.getWidth() != width || texture.getHeight() != height){
	// 	if(verbose){
	// 		std::cout << "Resizing texture" << std::endl;
	// 		std::cout << "Old width: " << texture.getWidth() << std::endl;
	// 		std::cout << "Old height: " << texture.getHeight() << std::endl;
	// 		std::cout << "New width: " << width << std::endl;
	// 		std::cout << "New height: " << height << std::endl;
	// 	}
	// 	reinitSwapChain = true;
	// }

#ifndef WEBGPU_BACKEND_WGPU
	// We no longer need the texture, only its view
	// (NB: with wgpu-native, surface textures must not be manually released)
	Texture(surfaceTexture.texture).release();
#endif // WEBGPU_BACKEND_WGPU

	return targetView;
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
	if(verbose){
		std::cout << "Resizing window" << std::endl;
		std::cout << "Old width: " << width << std::endl;
		std::cout << "Old height: " << height << std::endl;
	}
	glfwGetFramebufferSize(window, &width, &height);
	if(verbose){
		std::cout << "New width: " << width << std::endl;
		std::cout << "New height: " << height << std::endl;
	}
	if (width == 0 || height == 0)
		return;
	terminateDepthBuffer();
	terminateRenderTexture();
	terminateSwapChain();

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
#ifdef WGPU_GPU_HIGH_PERFORMANCE
    adapterOpts.powerPreference = WGPUPowerPreference_HighPerformance;
#endif
	adapter = instance.requestAdapter(adapterOpts);

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
	deviceDesc.requiredFeatureCount = 0;
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
	// swapChainFormat = TextureFormat::BGRA8Unorm;

	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, [](GLFWwindow *window, int, int)
								   {
		auto that = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
		if (that != nullptr) that->onResize(); });

	// adapter.release();
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
	if (supportedPresentModes.find(mode) == supportedPresentModes.end()){
		std::cerr << "Present mode "<< mode << " not supported!" << std::endl;
		return;
	}
	presentMode = mode;
	reinitSwapChain = true;
}

void Renderer::enableDepthSorting()
{
	sortDepth = true;
}

void Renderer::initSwapChain()
{	
	if(verbose)
	{	
		std::cout << "##########################################################" << std::endl;
		std::cout << "Initializing Swapchain" << std::endl;
	}
	SurfaceConfiguration config = {};
	// SwapChainDescriptor swapChainDesc;
	config.width = static_cast<uint32_t>(width);
	config.height = static_cast<uint32_t>(height);
	config.usage = TextureUsage::RenderAttachment;

	// WGPUTextureFormat surfaceFormat = wgpuSurfaceGetPreferredFormat(surface, adapter);
	// std::cout << "Surface Format: " << surfaceFormat << std::endl;
	auto surfaceFormat = surface.getPreferredFormat(adapter);
	config.format = surfaceFormat;
	config.device = device;
	config.presentMode = presentMode;
	config.alphaMode = CompositeAlphaMode::Auto;
	// swapChain = device.createSwapChain(surface, swapChainDesc);

	// if (!swapChain)
	// 	throw std::runtime_error("Could not create swap chain!");
	if(verbose){
		std::cout << "Configuring surface" << std::endl;
		std::cout << "Width: " << config.width << std::endl;
		std::cout << "Height: " << config.height << std::endl;
		std::cout << "Format: " << config.format << std::endl;
		std::cout << "Usage: " << config.usage << std::endl;
		std::cout << "Present Mode: " << config.presentMode << std::endl;
		std::cout << "Alpha Mode: " << config.alphaMode << std::endl;
		std::cout << "Device: " << config.device << std::endl;
		std::cout << "Surface: " << surface << std::endl;
	}

	surface.configure(config);
	if(verbose)
		std::cout << "Done" << std::endl;


}

void Renderer::terminateSwapChain()
{
	if(verbose)
		std::cout << "Releasing surface" << std::endl;
	// swapChain.release();
	surface.unconfigure();
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
	if(verbose){
		std::cout << "Creating post process texture" << std::endl;
		std::cout << "Dimension: " << postProcessTextureDesc.dimension << std::endl;
		std::cout << "Format: " << postProcessTextureDesc.format << std::endl;
		std::cout << "Size: " << postProcessTextureDesc.size.width << "x" << postProcessTextureDesc.size.height << "x" << postProcessTextureDesc.size.depthOrArrayLayers << std::endl;
		std::cout << "Usage: " << postProcessTextureDesc.usage << std::endl;
		std::cout << "Sample Count: " << postProcessTextureDesc.sampleCount << std::endl;
		std::cout << "Mip Level Count: " << postProcessTextureDesc.mipLevelCount << std::endl;
		std::cout << "View Format Count: " << postProcessTextureDesc.viewFormatCount << std::endl;
		std::cout << "View Formats: " << postProcessTextureDesc.viewFormats << std::endl;
	}
	postTexture = device.createTexture(postProcessTextureDesc);

	if (postTexture == nullptr)
		throw std::runtime_error("Could not create post process texture!");

	TextureViewDescriptor postProcessTextureViewDesc;
	postProcessTextureViewDesc.aspect = TextureAspect::All;
	postProcessTextureViewDesc.baseArrayLayer = 0;
	postProcessTextureViewDesc.arrayLayerCount = 1;
	postProcessTextureViewDesc.baseMipLevel = 0;
	postProcessTextureViewDesc.mipLevelCount = 1;
	postProcessTextureViewDesc.dimension = TextureViewDimension::_2D;
	postProcessTextureViewDesc.format = postTexture.getFormat();

	if(verbose){
		std::cout << "Creating post process texture view" << std::endl;
		std::cout << "Aspect: " << postProcessTextureViewDesc.aspect << std::endl;
		std::cout << "Base Array Layer: " << postProcessTextureViewDesc.baseArrayLayer << std::endl;
		std::cout << "Array Layer Count: " << postProcessTextureViewDesc.arrayLayerCount << std::endl;
		std::cout << "Base Mip Level: " << postProcessTextureViewDesc.baseMipLevel << std::endl;
		std::cout << "Mip Level Count: " << postProcessTextureViewDesc.mipLevelCount << std::endl;
		std::cout << "Dimension: " << postProcessTextureViewDesc.dimension << std::endl;
		std::cout << "Format: " << postProcessTextureViewDesc.format << std::endl;
	}
	postTextureView = postTexture.createView(postProcessTextureViewDesc);
	if (postTextureView == nullptr)
		throw std::runtime_error("Could not create post process texture view!");
}

void Renderer::terminateRenderTexture()
{
	if(verbose)
		std::cout << "Releasing post process texture view and destroying post texture" << std::endl;
	postTexture.destroy();
	postTexture.release();
}
void Renderer::initDepthBuffer()
{
	// glfwGetFramebufferSize(window, &width, &height);

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
	renderUniforms.flags = 0;
	sortDepth = false;
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
	renderUniforms.cullingOffsets = {0.0f, 0.0f, 1.0f};
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
	// glfwGetFramebufferSize(window, &camera.width, &camera.height);
	camera.width = width;
	camera.height = height;
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

void Renderer::drawCullingPlanes(const glm::vec3 &offsets)
{
	renderUniforms.flags |= UniformFlags::cullingPlane;
	renderUniforms.cullingOffsets = offsets;
}
