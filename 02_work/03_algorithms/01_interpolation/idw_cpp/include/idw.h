#pragma once

#include "types.h"

#include <vector>

namespace noise_idw {

/// 计算待估点 (x0, y0) 的反距离加权（IDW）估计值
/// 当 excludeIdx >= 0 时跳过该样本（用于 LOOCV）
double idwEstimate(const std::vector<Sample>& samples,
                   double x0,
                   double y0,
                   double power,
                   const IdwConfig& cfg,
                   int excludeIdx = -1);

/// 对给定样本集执行留一法交叉验证（LOOCV），返回精度指标并输出逐点残差
LoocvMetrics evaluateLoocv(const std::vector<Sample>& samples,
                           double power,
                           const IdwConfig& cfg,
                           std::vector<ResidualRecord>& residualsOut);

}  // namespace noise_idw
