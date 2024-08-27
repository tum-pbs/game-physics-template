#include "ResourceManager.h"
#include "stb_image.h"
#include <fstream>

using namespace wgpu;

ShaderModule ResourceManager::loadShaderModule(const std::filesystem::path &path, Device device)
{
	std::ifstream file(path);
	if (!file.is_open())
	{
		return nullptr;
	}
	file.seekg(0, std::ios::end);
	size_t size = file.tellg();
	std::string shaderSource(size, ' ');
	file.seekg(0);
	file.read(shaderSource.data(), size);

	ShaderModuleWGSLDescriptor shaderCodeDesc;
	shaderCodeDesc.chain.next = nullptr;
	shaderCodeDesc.chain.sType = SType::ShaderModuleWGSLDescriptor;
	shaderCodeDesc.code = shaderSource.c_str();
	ShaderModuleDescriptor shaderDesc;
	shaderDesc.nextInChain = &shaderCodeDesc.chain;
#ifdef WEBGPU_BACKEND_WGPU
	shaderDesc.hintCount = 0;
	shaderDesc.hints = nullptr;
#endif

	return device.createShaderModule(shaderDesc);
}

ResourceManager::Image ResourceManager::loadImage(std::filesystem::path &path)
{
	int width, height, channels;
	unsigned char *data = stbi_load(path.string().c_str(), &width, &height, &channels, 4);
	Image image;
	image.width = width;
	image.height = height;
	image.data.resize(width * height);
	for (int i = 0; i < width * height; i++)
	{
		image.data[i] = {data[i * 4], data[i * 4 + 1], data[i * 4 + 2], data[i * 4 + 3]};
	}
	stbi_image_free(data);
	return image;
}
