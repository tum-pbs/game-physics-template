#pragma once
#include <webgpu/webgpu.hpp>
#include <glm/glm.hpp>
#include <algorithm>
#include "ResourceManager.h"
#include "pipelines/InstancingPipeline.h"
#include "pipelines/LinePipeline.h"
#include "pipelines/PostProcessingPipeline.h"
#include "pipelines/ImagePipeline.h"
#include "Camera.h"
#include "Colormap.h"
#include <GLFW/glfw3.h>

/// @brief Renderer
///
/// This class is the main interface to the rendering engine. It provides functions to draw cubes, spheres, lines, images, etc.
/// One instance of this class should be created in the main function of the application.
/// The class is responsible for creating the window, initializing the device, and handling the rendering.
/// After each onFrame call, the rendering engine will draw the scene and present it to the screen.
/// renderer.clearScreen() should be called at the beginning of each frame to clear all added draw objects.
/// Otherwise, the objects will be drawn in every frame.
class Renderer
{

public:
	Renderer();
	~Renderer();

	/// @brief Draw a cube in the next frame
	///
	/// Call this function every frame you want to draw a cube
	///
	/// @param position
	///     The center of the cube
	/// @param rotation
	///     The rotation of the cube
	/// @param scale
	///     The scale of the cube. scale (1,1,1) will result in a cube with side length 1
	/// @param color
	///     The color of the cube: (r, g, b, a)
	/// @param flags
	///     Flags to modify the cube: Renderer::DrawFlags. Can be combined with bitwise OR. Possible flags:
	///
	///         - unlit: The cube will be drawn without lighting, meaning full brightness
	///
	///         - dontCull: The cube will be drawn even if it is cut by the culling plane
	/// @return
	///     The id of the cube.
	uint32_t drawCube(glm::vec3 position = vec3(0), glm::quat rotation = glm::quat(vec3(0)), glm::vec3 scale = vec3(1), glm::vec4 color = vec4(1), uint32_t flags = 0);

	/// @brief Draw a sphere in the next frame
	///
	/// Call this function every frame you want to draw a sphere
	///
	/// @param position
	///     The center of the sphere
	/// @param radius
	///     The radius of the sphere
	/// @param color
	///     The color of the sphere: (r, g, b, a)
	/// @param flags
	///     Flags to modify the sphere: Renderer::DrawFlags. Can be combined with bitwise OR. Possible flags:
	///
	///         - unlit: The sphere will be drawn without lighting, meaning full brightness
	///
	///         - dontCull: The sphere will be drawn even if it is cut by the culling plane
	/// @return
	///     The id of the sphere.
	uint32_t drawSphere(glm::vec3 position = vec3(0), float radius = 1, glm::vec4 color = vec4(1), uint32_t flags = 0);

	/// @brief Draw an ellipsoid in the next frame
	///
	/// Call this function every frame you want to draw an ellipsoid
	///
	/// @param position
	///    The center of the ellipsoid
	/// @param rotation
	///    The rotation of the ellipsoid
	/// @param scale
	///    The scale of the ellipsoid. scale (1,1,1) will result in a sphere with radius 1, scale (1,2,1) will result in an ellipsoid with radius 1 in x and z direction and 2 in y direction
	/// @param color
	///    The color of the ellipsoid: (r, g, b, a)
	/// @param flags
	///    Flags to modify the ellipsoid: Renderer::DrawFlags. Can be combined with bitwise OR. Possible flags:
	///
	///        - unlit: The ellipsoid will be drawn without lighting, meaning full brightness
	///
	///        - dontCull: The ellipsoid will be drawn even if it is cut by the culling plane
	/// @return
	///    The id of the ellipsoid.
	uint32_t drawEllipsoid(glm::vec3 position = vec3(0), glm::quat rotation = glm::quat(vec3(0)), glm::vec3 scale = vec3(1), glm::vec4 color = vec4(1), uint32_t flags = 0);

	/// @brief Draw a quad in the next frame
	///
	/// Call this function every frame you want to draw a quad
	///
	/// @param position
	///    The center of the quad
	/// @param rotation
	///    The rotation of the quad
	/// @param scale
	///    The scale of the quad. scale (1,1) will result in a quad with side length 1.
	/// @param color
	///    The color of the quad: (r, g, b, a)
	/// @param flags
	///    Flags to modify the quad: Renderer::DrawFlags. Can be combined with bitwise OR. Possible flags:
	///
	///        - unlit: The quad will be drawn without lighting, meaning full brightness
	///
	///		   - dontCull: The quad will be drawn even if it is cut by the culling plane
	/// @return
	///    The id of the quad.
	uint32_t drawQuad(glm::vec3 position = vec3(0), glm::quat rotation = glm::quat(vec3(0)), glm::vec2 scale = glm::vec2(1), glm::vec4 color = vec4(1), uint32_t flags = 0);

	/// @brief Draw a line in the next frame
	///
	/// Call this function every frame you want to draw a line
	///
	/// @param position1
	///    The start of the line
	/// @param position2
	///    The end of the line
	/// @param color
	///    The color of the line: (r, g, b). lines do not support transparency
	void drawLine(glm::vec3 position1, glm::vec3 position2, glm::vec3 color);

	/// @brief Draw a line with a color gradient in the next frame
	///
	/// Call this function every frame you want to draw a line
	///
	/// @param position1
	///    The start of the line
	/// @param position2
	///    The end of the line
	/// @param color1
	///    The color of the start of the line: (r, g, b).
	/// @param color2
	///    The color of the end of the line: (r, g, b).
	void drawLine(glm::vec3 position1, glm::vec3 position2, glm::vec3 color1, glm::vec3 color2);

	/// @brief Draw a wire cube in the next frame
	///
	/// Call this function every frame you want to draw a wire cube
	///
	/// @param position
	///    The center of the cube
	/// @param scale
	///    The scale of the cube. scale (1,1,1) will result in a cube with side length 1
	/// @param color
	///    The color of the wire cube: (r, g, b)
	void drawWireCube(glm::vec3 position = vec3(0), glm::vec3 scale = vec3(1), glm::vec3 color = vec3(1));

	/// @brief Draw an image in the next frame
	///
	/// Call this function every frame you want to draw an image.
	///
	/// *The colormap will automatically adjust to the data!*
	/// @param data
	///    The image data. The data should be a 1D vectro of floats, where each float represents a pixel. Pixel *rows* should be concatenated.
	/// @param height
	///    Number of pixels in y direction of the input data
	/// @param width
	///   Number of pixels in x direction of the input data
	/// @param colormap
	///    The colormap to use. Default is "hot". See `Colormap` documentation for available colormaps (all from matplotlib).
	/// @param screenPosition
	///   The center of the image on the screen. (0,0) is the middle of the screen, (1,1) is the top right corner.
	/// @param screenSize
	///  The size of the image on the screen. (1,1) will fill the whole screen.
	void drawImage(std::vector<float> data, int height, int width, Colormap colormap = Colormap("hot"), glm::vec2 screenPosition = {0, 0}, glm::vec2 screenSize = {1, 1});

	/// @brief Draw an image in the next frame
	///
	/// Call this function every frame you want to draw an image. Use this if you do *not* want the colormap range to change each frame.
	///
	/// @param data
	///    The image data. The data should be a 1D vectro of floats, where each float represents a pixel. Pixel *rows* should be concatenated.
	/// @param height
	///    Number of pixels in y direction of the input data
	/// @param width
	///   Number of pixels in x direction of the input data
	/// @param vmin
	///    The minimum value of the data. All values below this will be clipped.
	/// @param vmax
	///    The maximum value of the data. All values above this will be clipped.
	/// @param colormap
	///    The colormap to use. Default is "hot". See `Colormap` documentation for available colormaps (all from matplotlib).
	/// @param screenPosition
	///   The center of the image on the screen. (0,0) is the middle of the screen, (1,1) is the top right corner.
	/// @param screenSize
	///  The size of the image on the screen. (1,1) will fill the whole screen.
	void drawImage(std::vector<float> data, int height, int width, float vmin, float vmax, Colormap colormap = Colormap("hot"), glm::vec2 screenPosition = {0, 0}, glm::vec2 screenSize = {1, 1});

	/// @brief Enable culling planes for the next frame.
	///
	/// Don't draw any pixels with positions greater than the values in offsets.
	/// Looks like three planes, where one octant is excluded from rendering
	/// @param offsets
	///		The axis offets for the individual axes
	void drawCullingPlanes(const glm::vec3 &offsets = glm::vec3(0));

	/// @brief The background color of the scene
	glm::vec3 backgroundColor = {0.05f, 0.05f, 0.05f};

	/// @brief Get the number of spheres, ellipsoids, cubes and quads to be drawn next frame
	/// @return
	///    The number of objects to be drawn next frame
	size_t objectCount()
	{
		return current_id;
	};

	/// @brief Get the number of lines to be drawn next frame
	/// @return
	///    The number of lines to be drawn next frame
	size_t lineCount() { return linePipeline.objectCount(); };

	/// @brief Get the number of images to be drawn next frame
	/// @return
	///    The number of images to be drawn next frame
	size_t imageCount() { return imagePipeline.objectCount(); };

	/// @brief Draw all current objects to the screen.
	void onFrame();

	/// @brief Check if the window is still open. If the window is closed, the rendering engine will stop.
	/// @return
	///   True if the window is still open, false if the window is closed
	bool isRunning();

	/// @brief Callback function that is called when the window is resized
	void onResize();

	/// @brief Clear all objects that are currently drawn
	void clearScene();

	/// @brief Enable or disable frame rate synchronization
	void setPresentMode(wgpu::PresentMode mode);

	/// @brief Enables depth sorting for the current frame.
	///
	/// When enabled, all primitives get drawn in order from back to front, respective to their centers and the camera.
	///
	/// Enable this only when drawing transparent objects, it is slow.
	void enableDepthSorting();

	/// @brief This function is called once per frame inside an ImGui context.
	std::function<void()> defineGUI = nullptr;

	/// @brief Tracks the time taken up by the actual rendering last frame
	double lastDrawTime = 0;

	/// @brief Flags to modify the rendering of objects
	enum DrawFlags
	{
		/// The object will be drawn without lighting, meaning full brightness
		unlit = 1 << 0,
		/// The object will be drawn even if it is cut by the culling plane
		dontCull = 1 << 1,
	};

	/// @brief If enabled, the renderer will draw the culling plane
	enum UniformFlags
	{
		cullingPlane = 1 << 0,
	};

	/// @brief The main camera of the scene
	static Camera camera;

	/// @brief Essential uniform buffer for rendering
	struct RenderUniforms
	{
		/// @brief The projection matrix. Updated every frame from the main camera
		glm::mat4 projectionMatrix;
		/// @brief The view matrix. Updated every frame from the main camera
		glm::mat4 viewMatrix;
		/// @brief The position of the camera in world space. Updated every frame from the main camera
		glm::vec3 cameraWorldPosition;
		/// @brief The time since the start of the application in seconds
		float time;
		/// @brief The offsets of the culling plane
		glm::vec3 cullingOffsets;
		/// @brief Renderer::UniformFlags. Can be combined with bitwise OR. Possible flags:
		///
		///         - cullingPlane: The culling plane will be drawn
		uint32_t flags;
	};
	static_assert(sizeof(RenderUniforms) % 16 == 0);

	/// @brief Uniform buffer for lighting
	struct LightingUniforms
	{
		/// @brief The direction of the main light
		glm::vec3 direction;
		/// @brief The intensity of the main light
		float diffuse_intensity = 1.0f;
		/// @brief The color of the ambient light
		glm::vec3 ambient;
		/// @brief The intensity of the ambient light
		float ambient_intensity = 0.1f;
		/// @brief The color of the specular highlights
		glm::vec3 specular;
		/// @brief The intensity of the specular highlights
		float specular_intensity = 0.5f;
		/// @brief The shininess of the specular highlights
		float alpha = 32.0f;
		/// @brief Padding to align the struct to 16 bytes, required by the GPU
		float _pad[3];
	};
	static_assert(sizeof(LightingUniforms) % 16 == 0);

	RenderUniforms renderUniforms;
	LightingUniforms lightingUniforms;

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

	void initGui();
	void terminateGui();
	void updateGui(wgpu::RenderPassEncoder renderPass);

	using mat4 = glm::mat4;
	using vec4 = glm::vec4;
	using vec3 = glm::vec3;
	using vec2 = glm::vec2;
	uint32_t current_id = 0;
	int width, height;
	wgpu::PresentMode presentMode = wgpu::PresentMode::Fifo;
	bool reinitSwapChain = false;

	bool sortDepth = false;

	GLFWwindow *window = nullptr;
	wgpu::Instance instance = nullptr;
	wgpu::Surface surface = nullptr;
	wgpu::Device device = nullptr;
	wgpu::Queue queue = nullptr;
	wgpu::TextureFormat swapChainFormat = wgpu::TextureFormat::Undefined;
	std::unique_ptr<wgpu::ErrorCallback> errorCallbackHandle;
	wgpu::SwapChain swapChain = nullptr;

	InstancingPipeline instancingPipeline;
	LinePipeline linePipeline;
	PostProcessingPipeline postProcessingPipeline;
	ImagePipeline imagePipeline;

	wgpu::TextureFormat depthTextureFormat = wgpu::TextureFormat::Depth24Plus;
	wgpu::Texture depthTexture = nullptr;
	wgpu::TextureView depthTextureView = nullptr;

	wgpu::Buffer uniformBuffer = nullptr;
	wgpu::Buffer lightingUniformBuffer = nullptr;

	wgpu::Texture postTexture = nullptr;
	wgpu::TextureView postTextureView = nullptr;
};
