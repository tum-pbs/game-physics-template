#pragma once
#include <sstream>
#include <filesystem>
#include <vector>
#include <string>

std::filesystem::path expand(std::filesystem::path in);
std::filesystem::path resolveFile(std::string fileName, std::vector<std::string> search_paths = {}, bool portable = false);

extern std::filesystem::path workingDirectory;
extern std::filesystem::path binaryDirectory;