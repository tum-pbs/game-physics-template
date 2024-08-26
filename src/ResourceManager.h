#pragma once

#include <webgpu/webgpu.hpp>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <filesystem>

class ResourceManager
{
public:
	using vec4 = glm::vec4;
	using vec3 = glm::vec3;
	using vec2 = glm::vec2;
	using quat = glm::quat;
	using mat3x3 = glm::mat3x3;

	struct Image
	{

		std::vector<glm::vec<4, uint8_t>> data;
		int height;
		int width;
	};

	struct PrimitiveVertexAttributes
	{
		vec3 position;
		vec3 normal;
	};

	struct LineVertexAttributes
	{
		vec3 position;
		vec3 color;
	};

	struct ImageAttributes
	{
		float x;
		float y;
		float sx;
		float sy;
		int offset;
		int width;
		int height;
		float cmapOffset;
	};

	struct InstancedVertexAttributes
	{
		vec3 position;
		quat rotation;
		vec3 scale;
		vec4 color;
		uint32_t id;
		uint32_t flags;
	};

	static wgpu::ShaderModule loadShaderModule(const std::filesystem::path &path, wgpu::Device device);
	static Image loadImage(std::filesystem::path &path);
};
