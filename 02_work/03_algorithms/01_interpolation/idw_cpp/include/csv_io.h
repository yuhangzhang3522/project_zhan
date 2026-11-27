#pragma once

#include "types.h"

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace noise_idw {

using CsvRow = std::map<std::string, std::string>;

/// 向上查找项目仓库根目录（同时兼容 02_work/02_Input data 与 02_work/Input data）
std::filesystem::path findProjectRoot(const std::filesystem::path& startHint = {});

/// 定位标准输入数据目录
std::filesystem::path resolveInputDir(const std::filesystem::path& projectRoot);

/// 读取 UTF-8 CSV（自动去除 UTF-8 BOM）
std::vector<CsvRow> readCsv(const std::filesystem::path& filePath);

/// 加载 32 点空间基准坐标与早/中/晚声级数据，按 period 分组返回
std::map<std::string, std::vector<Sample>> loadPeriodSamples(
    const std::filesystem::path& inputDir,
    const std::string& valueField);

}  // namespace noise_idw
