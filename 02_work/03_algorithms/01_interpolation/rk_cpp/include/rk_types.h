#pragma once

#include <string>
#include <vector>

namespace noise_rk {

struct RkSample {
    int id = 0;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double distRoad = 0.0;         // 距最近道路距离 (m)
    double buildingDensity = 0.0;  // 建筑密度 (0-1)
    double greenRatio = 0.0;       // 绿地率 (0-1)
};

struct OlsModel {
    double beta0 = 0.0;            // 截距
    double betaRoad = 0.0;         // 距道路距离系数
    double betaBuilding = 0.0;     // 建筑密度系数
    double betaGreen = 0.0;        // 绿地率系数
    double r2 = 0.0;               // 趋势项拟合优度
    double predict(const RkSample& s) const {
        return beta0 + betaRoad * s.distRoad + betaBuilding * s.buildingDensity +
               betaGreen * s.greenRatio;
    }
};

struct LoocvMetrics {
    int count = 0;
    double mae = 0.0;
    double rmse = 0.0;
    double r2 = 0.0;
    double me = 0.0;
    double maxErr = 0.0;
};

}  // namespace noise_rk
