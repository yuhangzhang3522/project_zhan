#pragma once

#include "types.h"

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace noise_kriging {

using CsvRow = std::map<std::string, std::string>;

std::filesystem::path findProjectRoot(const std::filesystem::path& startHint = {});
std::filesystem::path resolveInputDir(const std::filesystem::path& projectRoot);
std::vector<CsvRow> readCsv(const std::filesystem::path& filePath);
std::map<std::string, std::vector<Sample>> loadPeriodSamples(
    const std::filesystem::path& inputDir,
    const std::string& valueField);

}  // namespace noise_kriging
