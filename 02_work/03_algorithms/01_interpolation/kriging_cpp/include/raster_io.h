#pragma once

#include "kriging.h"
#include "types.h"

#include <filesystem>
#include <utility>
#include <vector>

namespace noise_kriging {

/// 同步导出普通克里金估值栅格 (.asc + .prj) 与估计方差栅格 (.asc + .prj)
/// 距最近采样点超过 cfg.maxExtrapolation 的像元置为 NODATA=-9999
std::pair<int, int> writeKrigingRasters(const std::filesystem::path& valAscPath,
                                        const std::filesystem::path& varAscPath,
                                        const std::vector<Sample>& samples,
                                        const OrdinaryKrigingSolver& solver,
                                        const KrigingConfig& cfg);

}  // namespace noise_kriging
