#pragma once

#include "types.h"

#include <vector>

namespace noise_kriging {

/// 列主元高斯消元法求解线性方程组 A x = b（A 为 n×n 展平一维数组）
bool solveLinearSystem(std::vector<double>& A,
                       std::vector<double>& b,
                       std::vector<double>& x,
                       int n);

/// 预分解后的普通克里金求解器（用于高效批量像元估值）
class OrdinaryKrigingSolver {
public:
    OrdinaryKrigingSolver(const std::vector<Sample>& samples,
                          const VariogramParams& params,
                          double regEps = 1e-8);

    /// 估计单点 (x0, y0) 的声级与克里金方差
    KrigingEstimate predict(double x0, double y0) const;

private:
    const std::vector<Sample>& samples_;
    VariogramParams params_;
    int n_ = 0;
    int dim_ = 0;
    double c0Total_ = 0.0;
    std::vector<double> invK_;  // (N+1)x(N+1) 逆矩阵，用于 O(N²) 批量预测
};

/// 零拷贝 LOOCV 留一法交叉验证（逐折跳过 excludeIdx 直接构造 (N)×(N) 方程组求解）
LoocvMetrics evaluateKrigingLoocv(const std::vector<Sample>& samples,
                                  const VariogramParams& params,
                                  double regEps,
                                  std::vector<ResidualRecord>& residualsOut);

}  // namespace noise_kriging
