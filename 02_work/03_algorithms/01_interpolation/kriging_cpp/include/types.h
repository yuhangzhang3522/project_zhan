#pragma once

#include <string>
#include <vector>

namespace noise_kriging {

/// 采样点观测样本
struct Sample {
    int id = 0;
    double x = 0.0;  // UTM 51N Easting (m)
    double y = 0.0;  // UTM 51N Northing (m)
    double z = 0.0;  // 观测声级 dB(A)
};

/// 理论半变异函数模型类型
enum class VariogramModel {
    Spherical,
    Exponential
};

inline std::string modelName(VariogramModel m) {
    return (m == VariogramModel::Spherical) ? "spherical" : "exponential";
}

/// 实验半变异函数单个滞后分组
struct EmpiricalLag {
    int lagIdx = 0;
    double lagCenter = 0.0;    // 滞后区间中心距离 (m)
    double meanDist = 0.0;     // 组内点对平均距离 (m)
    int pairCount = 0;         // 点对数量
    double semivariance = 0.0; // 实验半变异值 γ(h)
};

/// 拟合后的理论半变异函数参数
struct VariogramParams {
    VariogramModel model = VariogramModel::Spherical;
    double nugget = 0.0;       // 块金值 c0
    double partialSill = 0.0;  // 偏基台值 c1
    double sill = 0.0;         // 基台值 c0 + c1
    double range = 100.0;      // 变程 a (m)
    double wlsSse = 0.0;       // 加权残差平方和
    double wlsRmse = 0.0;      // 加权均方根拟合误差
};

/// 普通克里金单点估值结果
struct KrigingEstimate {
    double value = 0.0;     // 克里金估值
    double variance = 0.0;  // 克里金估计方差 σ²_OK
};

/// 普通克里金运行配置
struct KrigingConfig {
    std::string field = "leq_energy";
    double cellSize = 5.0;       // 栅格分辨率 (m)
    double buffer = 100.0;       // 外包框外扩距离 (m)
    double maxExtrapolation = 160.0;  // 超过该距离置 NODATA (m)
    int numLags = 10;            // 半变异函数滞后分组数
    double maxLagDist = 650.0;   // 最大滞后距离上限 (m)
    double nuggetReg = 1e-8;     // 矩阵对角微弱正则项防奇异
};

/// LOOCV 精度指标
struct LoocvMetrics {
    int count = 0;
    double mae = 0.0;
    double rmse = 0.0;
    double r2 = 0.0;
    double me = 0.0;
    double maxErr = 0.0;
};

/// 单点 LOOCV 残差与方差记录
struct ResidualRecord {
    int pointId = 0;
    double observed = 0.0;
    double predicted = 0.0;
    double residual = 0.0;
    double variance = 0.0;
};

}  // namespace noise_kriging
