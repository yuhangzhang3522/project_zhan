#pragma once

#include "rk_types.h"

#include <vector>

namespace noise_rk {

/// 多元线性回归（OLS）：z = β0 + β1*dist_road + β2*building_density + β3*green_ratio
/// excludeIdx >= 0 时跳过该样本（用于严格无泄露 LOOCV）
OlsModel fitOls(const std::vector<RkSample>& samples, int excludeIdx = -1);

/// 回归克里金（OLS 趋势 + 残差球状普通克里金）留一法交叉验证
LoocvMetrics evaluateRkLoocv(const std::vector<RkSample>& samples,
                             std::vector<double>& predOut);

}  // namespace noise_rk
