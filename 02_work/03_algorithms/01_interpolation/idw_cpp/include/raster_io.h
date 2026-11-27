#pragma once

#include "types.h"

#include <filesystem>
#include <vector>

namespace noise_idw {

/// 生成 ESRI ASCII Grid (.asc) 栅格与同名 EPSG:32651 (.prj) 投影文件
/// 返回写入的栅格维度 {ncols, nrows}
std::pair<int, int> writeAsciiGrid(const std::filesystem::path& ascPath,
                                   const std::vector<Sample>& samples,
                                   double power,
                                   const IdwConfig& cfg);

}  // namespace noise_idw
