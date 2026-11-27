#pragma once

#include "types.h"

#include <vector>

namespace noise_kriging {

/// 按距离滞后分组计算实验半变异函数
std::vector<EmpiricalLag> computeEmpiricalVariogram(const std::vector<Sample>& samples,
                                                    int numLags,
                                                    double maxLagDist);

/// 计算理论半变异函数值 γ(h)
double evalVariogram(const VariogramParams& params, double h);

/// 计算对应的空间协方差函数值 C(h) = sill - γ(h)
double evalCovariance(const VariogramParams& params, double h);

/// 加权最小二乘法（WLS）拟合球状/指数理论半变异函数参数
VariogramParams fitVariogram(const std::vector<EmpiricalLag>& lags,
                             VariogramModel model,
                             const std::vector<Sample>& samples);

}  // namespace noise_kriging
