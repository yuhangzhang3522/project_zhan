#include "variogram.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace noise_kriging {

namespace {

double basisFunc(VariogramModel model, double h, double range) {
    if (h <= 0.0) return 0.0;
    if (model == VariogramModel::Spherical) {
        if (h >= range) return 1.0;
        const double r = h / range;
        return 1.5 * r - 0.5 * r * r * r;
    }
    // Exponential (effective range where γ(a) ≈ c0 + 0.95 c1)
    return 1.0 - std::exp(-3.0 * h / range);
}

double sampleVariance(const std::vector<Sample>& samples) {
    if (samples.size() < 2) return 1.0;
    double mean = 0.0;
    for (const auto& s : samples) mean += s.z;
    mean /= static_cast<double>(samples.size());
    double ss = 0.0;
    for (const auto& s : samples) {
        double d = s.z - mean;
        ss += d * d;
    }
    return ss / static_cast<double>(samples.size() - 1);
}

}  // namespace

std::vector<EmpiricalLag> computeEmpiricalVariogram(const std::vector<Sample>& samples,
                                                    int numLags,
                                                    double maxLagDist) {
    const int n = static_cast<int>(samples.size());
    double maxPairDist = 0.0;
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            maxPairDist = std::max(maxPairDist, std::hypot(samples[i].x - samples[j].x,
                                                           samples[i].y - samples[j].y));
        }
    }

    const double cutoff = std::min(maxLagDist, 0.55 * maxPairDist);
    const double binWidth = cutoff / static_cast<double>(std::max(1, numLags));

    std::vector<double> sumDist(numLags, 0.0);
    std::vector<double> sumSqDiff(numLags, 0.0);
    std::vector<int> counts(numLags, 0);

    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            const double d = std::hypot(samples[i].x - samples[j].x,
                                        samples[i].y - samples[j].y);
            if (d <= 0.0 || d > cutoff) continue;
            int idx = static_cast<int>(d / binWidth);
            if (idx >= numLags) idx = numLags - 1;
            const double dz = samples[i].z - samples[j].z;
            sumDist[idx] += d;
            sumSqDiff[idx] += dz * dz;
            counts[idx] += 1;
        }
    }

    std::vector<EmpiricalLag> lags;
    for (int b = 0; b < numLags; ++b) {
        if (counts[b] < 3) continue;  // 剔除点对过少的统计不稳定分组
        EmpiricalLag lag;
        lag.lagIdx = b + 1;
        lag.lagCenter = (b + 0.5) * binWidth;
        lag.meanDist = sumDist[b] / static_cast<double>(counts[b]);
        lag.pairCount = counts[b];
        lag.semivariance = sumSqDiff[b] / (2.0 * static_cast<double>(counts[b]));
        lags.push_back(lag);
    }
    return lags;
}

double evalVariogram(const VariogramParams& params, double h) {
    if (h <= 1e-12) return 0.0;
    return params.nugget + params.partialSill * basisFunc(params.model, h, params.range);
}

double evalCovariance(const VariogramParams& params, double h) {
    if (h <= 1e-12) return params.sill;
    return std::max(0.0, params.sill - evalVariogram(params, h));
}

VariogramParams fitVariogram(const std::vector<EmpiricalLag>& lags,
                             VariogramModel model,
                             const std::vector<Sample>& samples) {
    const double varZ = std::max(sampleVariance(samples), 1e-4);
    VariogramParams best;
    best.model = model;
    best.nugget = 0.2 * varZ;
    best.partialSill = 0.8 * varZ;
    best.sill = varZ;
    best.range = 300.0;
    best.wlsSse = std::numeric_limits<double>::max();

    if (lags.empty()) return best;

    // Cressie 风格权重：点对多、距离近的滞后组权重更高
    std::vector<double> weights(lags.size(), 1.0);
    double sumW = 0.0;
    for (size_t i = 0; i < lags.size(); ++i) {
        weights[i] = static_cast<double>(lags[i].pairCount) / std::max(lags[i].meanDist, 10.0);
        sumW += weights[i];
    }

    auto evaluateCandidate = [&](double rangeVal) {
        // 给定变程 a，模型 γ(h) = c0 + c1 * φ(h; a) 关于 (c0, c1) 为线性问题
        double s00 = 0.0, s01 = 0.0, s11 = 0.0;
        double t0 = 0.0, t1 = 0.0;
        for (size_t i = 0; i < lags.size(); ++i) {
            const double w = weights[i];
            const double phi = basisFunc(model, lags[i].meanDist, rangeVal);
            const double g = lags[i].semivariance;
            s00 += w;
            s01 += w * phi;
            s11 += w * phi * phi;
            t0 += w * g;
            t1 += w * phi * g;
        }

        // 枚举无约束解及非负边界解 (c0 >= 0, c1 >= 0)
        std::vector<std::pair<double, double>> cands;
        const double det = s00 * s11 - s01 * s01;
        if (std::fabs(det) > 1e-12) {
            double c0 = (s11 * t0 - s01 * t1) / det;
            double c1 = (-s01 * t0 + s00 * t1) / det;
            if (c0 >= 0.0 && c1 >= 0.0) cands.emplace_back(c0, c1);
        }
        if (s11 > 1e-12) cands.emplace_back(0.0, std::max(0.0, t1 / s11));
        if (s00 > 1e-12) cands.emplace_back(std::max(0.0, t0 / s00), 0.0);

        for (auto [c0, c1] : cands) {
            // 保证至少有微小空间结构，避免纯块金导致退化
            if (c0 + c1 < 1e-6) continue;
            if (c1 < 0.05 * (c0 + c1)) {
                double tot = c0 + c1;
                c1 = 0.05 * tot;
                c0 = 0.95 * tot;
            }
            double sse = 0.0;
            for (size_t i = 0; i < lags.size(); ++i) {
                const double pred = c0 + c1 * basisFunc(model, lags[i].meanDist, rangeVal);
                const double diff = pred - lags[i].semivariance;
                sse += weights[i] * diff * diff;
            }
            if (sse < best.wlsSse) {
                best.model = model;
                best.nugget = c0;
                best.partialSill = c1;
                best.sill = c0 + c1;
                best.range = rangeVal;
                best.wlsSse = sse;
                best.wlsRmse = std::sqrt(sse / std::max(sumW, 1e-12));
            }
        }
    };

    // 粗搜索 [80m, 800m] 步长 5m，再在最优值附近精搜索步长 0.5m
    for (double a = 80.0; a <= 800.0; a += 5.0) {
        evaluateCandidate(a);
    }
    const double coarseBest = best.range;
    for (double a = std::max(50.0, coarseBest - 10.0); a <= coarseBest + 10.0; a += 0.5) {
        evaluateCandidate(a);
    }

    return best;
}

}  // namespace noise_kriging
