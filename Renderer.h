#pragma once

#include <webgpu/webgpu.hpp>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RIGHT_HANDED
#include <glm/glm.hpp>
#include "ResourceManager.h"
#include "pipelines/InstancingPipeline.h"
#include "pipelines/LinePipeline.h"
#include "pipelines/PostProcessingPipeline.h"
#include "pipelines/ImagePipeline.h"
#include "Camera.h"

// Forward declare
struct GLFWwindow;

class Renderer
{
public:
	Renderer();
	~Renderer();
	uint32_t drawCube(glm::vec3 position, glm::quat rotation, glm::vec3 scale, glm::vec4 color, uint32_t flags = 0);
	uint32_t drawCube(glm::vec3 position, glm::quat rotation, glm::vec3 scale, glm::vec3 color, uint32_t flags = 0);
	uint32_t drawSphere(glm::vec3 position, glm::quat rotation, glm::vec3 scale, glm::vec4 color, uint32_t flags = 0);
	uint32_t drawSphere(glm::vec3 position, float scale, glm::vec3 color, uint32_t flags = 0);
	uint32_t drawQuad(glm::vec3 position, glm::quat rotation, glm::vec3 scale, glm::vec4 color, uint32_t flags = 0);
	uint32_t drawQuad(glm::vec3 position, glm::quat rotation, glm::vec3 scale, glm::vec3 color, uint32_t flags = 0);
	void drawLine(glm::vec3 position1, glm::vec3 position2, glm::vec3 color);
	void drawLine(glm::vec3 position1, glm::vec3 position2, glm::vec3 color1, glm::vec3 color2);
	void drawWireCube(glm::vec3 position, glm::vec3 scale, glm::vec3 color);
	void drawImage(std::vector<float> data, int height, int width, glm::vec2 screenPosition = {0, 0}, glm::vec2 screenSize = {1, 1});

	size_t objectCount()
	{
		return current_id;
	};
	size_t lineCount() { return m_lines.size() / 2; };
	size_t imageCount() { return m_images.size(); };

	// A function called at each frame, guaranteed never to be called before `onInit`.
	void onFrame();

	// A function that tells if the application is still running.
	bool isRunning();

	// A function called when the window is resized.
	void onResize();

	void clearScene();
	void setPresentMode(wgpu::PresentMode mode);
	std::function<void()> defineGUI = nullptr;
	double lastDrawTime = 0;

	enum DrawFlags
	{
		unlit = 1 << 0,
		dontCull = 1 << 1,
	};
	enum UniformFlags
	{
		cullingPlane = 1 << 0,
	};

	static Camera camera;
	struct MyUniforms
	{
		// We add transform matrices
		glm::mat4 projectionMatrix;
		glm::mat4 viewMatrix;
		glm::vec3 cameraWorldPosition;
		float time;
		glm::vec3 cullingNormal;
		float cullingOffset;
		uint32_t flags;
		float _pad[3];
	};
	static_assert(sizeof(MyUniforms) % 16 == 0);

	struct LightingUniforms
	{
		glm::vec3 direction;
		float diffuse_intensity = 1.0f;
		glm::vec3 ambient;
		float ambient_intensity = 0.1f;
		glm::vec3 specular;
		float specular_intensity = 0.5f;
		float alpha = 32.0f;
		float _pad[3];
	};
	static_assert(sizeof(LightingUniforms) % 16 == 0);

	MyUniforms m_uniforms;
	LightingUniforms m_lightingUniforms;
	glm::vec3 backgroundColor = {0.05f, 0.05f, 0.05f};

private:
	void initWindowAndDevice();
	void terminateWindowAndDevice();

	void initSwapChain();
	void terminateSwapChain();

	void initRenderTexture();
	void terminateRenderTexture();

	void initDepthBuffer();
	void terminateDepthBuffer();

	void initUniforms();
	void terminateUniforms();

	void initLightingUniforms();
	void terminateLightingUniforms();
	void updateLightingUniforms();

	void updateProjectionMatrix();
	void updateViewMatrix();

	void initGui();										// called in onInit
	void terminateGui();								// called in onFinish
	void updateGui(wgpu::RenderPassEncoder renderPass); // called in onFrame

private:
	// (Just aliases to make notations lighter)
	using mat4 = glm::mat4;
	using vec4 = glm::vec4;
	using vec3 = glm::vec3;
	using vec2 = glm::vec2;
	uint32_t current_id = 0;
	int width, height;
	wgpu::PresentMode presentMode = wgpu::PresentMode::Fifo;
	bool reinitSwapChain = false;

	// Window and Device
	GLFWwindow *m_window = nullptr;
	wgpu::Instance m_instance = nullptr;
	wgpu::Surface m_surface = nullptr;
	wgpu::Device m_device = nullptr;
	wgpu::Queue m_queue = nullptr;
	wgpu::TextureFormat m_swapChainFormat = wgpu::TextureFormat::Undefined;
	std::unique_ptr<wgpu::ErrorCallback> m_errorCallbackHandle;

	wgpu::SwapChain m_swapChain = nullptr;

	InstancingPipeline m_instancingPipeline;
	LinePipeline m_linePipeline;
	PostProcessingPipeline m_postProcessingPipeline;
	ImagePipeline m_imagePipeline;

	wgpu::TextureFormat m_depthTextureFormat = wgpu::TextureFormat::Depth24Plus;
	wgpu::Texture m_depthTexture = nullptr;
	wgpu::TextureView m_depthTextureView = nullptr;

	std::vector<ResourceManager::InstancedVertexAttributes> m_cubes;
	std::vector<ResourceManager::InstancedVertexAttributes> m_spheres;
	std::vector<ResourceManager::InstancedVertexAttributes> m_quads;
	std::vector<ResourceManager::LineVertexAttributes> m_lines;
	std::vector<ResourceManager::ImageAttributes> m_images;
	std::vector<float> m_imageData;

	wgpu::Buffer m_uniformBuffer = nullptr;
	wgpu::Buffer m_lightingUniformBuffer = nullptr;

	wgpu::Texture m_postTexture = nullptr;
	wgpu::TextureView m_postTextureView = nullptr;
};
