#include "idw.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace noise_idw {

double idwEstimate(const std::vector<Sample>& samples,
                   double x0,
                   double y0,
                   double power,
                   const IdwConfig& cfg,
                   int excludeIdx) {
    std::vector<std::pair<double, int>> candidates;
    candidates.reserve(samples.size());

    for (int i = 0; i < static_cast<int>(samples.size()); ++i) {
        if (i == excludeIdx) continue;
        double d = std::hypot(samples[i].x - x0, samples[i].y - y0);
        if (d < 1e-9) {
            return samples[i].z;  // 与已知采样点重合，精确插值直接返回观测值
        }
        candidates.emplace_back(d, i);
    }

    if (candidates.empty()) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    if (cfg.neighbors > 0 && static_cast<int>(candidates.size()) > cfg.neighbors) {
        std::nth_element(candidates.begin(),
                         candidates.begin() + cfg.neighbors,
                         candidates.end());
        candidates.resize(cfg.neighbors);
    }

    double num = 0.0;
    double den = 0.0;
    for (const auto& [d, idx] : candidates) {
        double w = 1.0 / std::pow(d, power);
        num += w * samples[idx].z;
        den += w;
    }
    return num / den;
}

LoocvMetrics evaluateLoocv(const std::vector<Sample>& samples,
                           double power,
                           const IdwConfig& cfg,
                           std::vector<ResidualRecord>& residualsOut) {
    const int n = static_cast<int>(samples.size());
    residualsOut.clear();
    residualsOut.reserve(n);

    LoocvMetrics metrics;
    if (n == 0) return metrics;

    double meanObs = 0.0;
    for (const auto& s : samples) meanObs += s.z;
    meanObs /= static_cast<double>(n);

    double sumAbs = 0.0;
    double sumSq = 0.0;
    double sumErr = 0.0;
    double ssTot = 0.0;
    double maxErr = 0.0;
    int valid = 0;

    for (int i = 0; i < n; ++i) {
        double pred = idwEstimate(samples, samples[i].x, samples[i].y, power, cfg, i);
        double err = std::isnan(pred) ? std::numeric_limits<double>::quiet_NaN()
                                      : (pred - samples[i].z);
        residualsOut.push_back({samples[i].id, samples[i].z, pred, err});
        if (std::isnan(pred)) continue;

        sumAbs += std::fabs(err);
        sumSq += err * err;
        sumErr += err;
        maxErr = std::max(maxErr, std::fabs(err));
        double diffMean = samples[i].z - meanObs;
        ssTot += diffMean * diffMean;
        ++valid;
    }

    metrics.count = valid;
    if (valid > 0) {
        metrics.mae = sumAbs / valid;
        metrics.rmse = std::sqrt(sumSq / valid);
        metrics.me = sumErr / valid;
        metrics.maxErr = maxErr;
        metrics.r2 = (ssTot > 0.0) ? (1.0 - sumSq / ssTot) : 0.0;
    }
    return metrics;
}

}  // namespace noise_idw
