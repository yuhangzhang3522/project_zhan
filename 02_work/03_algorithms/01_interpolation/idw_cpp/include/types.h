#pragma once

#include <string>
#include <vector>

namespace noise_idw {

/// 单个采样点在某一时段的观测样本
struct Sample {
    int id = 0;
    double x = 0.0;  // WGS 84 / UTM Zone 51N Easting (m)
    double y = 0.0;  // WGS 84 / UTM Zone 51N Northing (m)
    double z = 0.0;  // 观测声级 dB(A)
};

/// IDW 插值与栅格输出配置
struct IdwConfig {
    std::string field = "leq_energy";
    std::vector<double> powers = {1.0, 1.5, 2.0, 2.5, 3.0};
    std::vector<int> neighborsList = {0};  // 0 = 使用全部点；>0 = 仅用最近 k 个邻点
    double cellSize = 5.0;                 // 栅格像元分辨率 (m)
    double buffer = 100.0;                 // 采样点外包框外扩距离 (m)
    int neighbors = 0;                     // 当前单次估值使用的邻点数
    double searchRadius = 160.0;           // 栅格外推截断半径 (m)，超出置 NODATA；0 = 不限
    bool bestOnly = false;                 // 仅导出每时段最优参数组合的栅格
};

/// 留一法交叉验证（LOOCV）精度指标
struct LoocvMetrics {
    int count = 0;
    double mae = 0.0;     // 平均绝对误差 (dB)
    double rmse = 0.0;    // 均方根误差 (dB)
    double r2 = 0.0;      // 决定系数 R²
    double me = 0.0;      // 平均误差 (dB)
    double maxErr = 0.0;  // 最大绝对误差 (dB)
};

/// 单点留一法预测残差记录
struct ResidualRecord {
    int pointId = 0;
    double observed = 0.0;
    double predicted = 0.0;
    double residual = 0.0;
};

}  // namespace noise_idw
