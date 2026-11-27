#include "rk_solver.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace noise_rk {

namespace {

bool solveLinear(std::vector<double>& A, std::vector<double>& b, std::vector<double>& x, int n) {
    x.assign(n, 0.0);
    for (int col = 0; col < n; ++col) {
        int piv = col;
        double maxAbs = std::fabs(A[col * n + col]);
        for (int row = col + 1; row < n; ++row) {
            double v = std::fabs(A[row * n + col]);
            if (v > maxAbs) {
                maxAbs = v;
                piv = row;
            }
        }
        if (maxAbs < 1e-12) return false;
        if (piv != col) {
            for (int k = col; k < n; ++k) std::swap(A[col * n + k], A[piv * n + k]);
            std::swap(b[col], b[piv]);
        }
        const double diag = A[col * n + col];
        for (int row = col + 1; row < n; ++row) {
            const double f = A[row * n + col] / diag;
            A[row * n + col] = 0.0;
            for (int k = col + 1; k < n; ++k) A[row * n + k] -= f * A[col * n + k];
            b[row] -= f * b[col];
        }
    }
    for (int row = n - 1; row >= 0; --row) {
        double s = b[row];
        for (int k = row + 1; k < n; ++k) s -= A[row * n + k] * x[k];
        x[row] = s / A[row * n + row];
    }
    return true;
}

double sphCov(double c0, double c1, double range, double h) {
    const double sill = c0 + c1;
    if (h <= 1e-12) return sill;
    if (h >= range) return 0.0;
    const double r = h / range;
    const double g = c0 + c1 * (1.5 * r - 0.5 * r * r * r);
    return std::max(0.0, sill - g);
}

}  // namespace

OlsModel fitOls(const std::vector<RkSample>& samples, int excludeIdx) {
    constexpr int p = 4;
    std::vector<double> XtX(p * p, 0.0);
    std::vector<double> Xty(p, 0.0);

    double sumZ = 0.0;
    int cnt = 0;
    for (int i = 0; i < static_cast<int>(samples.size()); ++i) {
        if (i == excludeIdx) continue;
        const auto& s = samples[i];
        const double x[p] = {1.0, s.distRoad, s.buildingDensity, s.greenRatio};
        for (int r = 0; r < p; ++r) {
            Xty[r] += x[r] * s.z;
            for (int c = 0; c < p; ++c) XtX[r * p + c] += x[r] * x[c];
        }
        sumZ += s.z;
        ++cnt;
    }

    // 轻微岭正则项避免共线性奇异
    for (int r = 1; r < p; ++r) XtX[r * p + r] += 1e-6;

    std::vector<double> beta;
    OlsModel m;
    if (cnt >= p && solveLinear(XtX, Xty, beta, p)) {
        m.beta0 = beta[0];
        m.betaRoad = beta[1];
        m.betaBuilding = beta[2];
        m.betaGreen = beta[3];
    } else if (cnt > 0) {
        m.beta0 = sumZ / cnt;
    }

    if (cnt > 1) {
        const double meanZ = sumZ / cnt;
        double ssRes = 0.0, ssTot = 0.0;
        for (int i = 0; i < static_cast<int>(samples.size()); ++i) {
            if (i == excludeIdx) continue;
            const double e = samples[i].z - m.predict(samples[i]);
            const double d = samples[i].z - meanZ;
            ssRes += e * e;
            ssTot += d * d;
        }
        m.r2 = (ssTot > 0.0) ? (1.0 - ssRes / ssTot) : 0.0;
    }
    return m;
}

LoocvMetrics evaluateRkLoocv(const std::vector<RkSample>& samples,
                             std::vector<double>& predOut) {
    const int n = static_cast<int>(samples.size());
    predOut.assign(n, 0.0);
    LoocvMetrics metrics;
    if (n < 5) return metrics;

    double meanObs = 0.0;
    for (const auto& s : samples) meanObs += s.z;
    meanObs /= static_cast<double>(n);

    double sumAbs = 0.0, sumSq = 0.0, sumErr = 0.0, ssTot = 0.0, maxErr = 0.0;
    int valid = 0;

    for (int ex = 0; ex < n; ++ex) {
        const OlsModel ols = fitOls(samples, ex);

        std::vector<int> idx;
        std::vector<double> res;
        idx.reserve(n - 1);
        res.reserve(n - 1);
        double resVar = 0.0;
        for (int i = 0; i < n; ++i) {
            if (i == ex) continue;
            const double r = samples[i].z - ols.predict(samples[i]);
            idx.push_back(i);
            res.push_back(r);
            resVar += r * r;
        }
        resVar = std::max(resVar / std::max(1, n - 2), 1e-4);

        // 残差采用球状协方差模型（默认 c0=0.25*var, c1=0.75*var, range=250m）
        const double c0 = 0.25 * resVar;
        const double c1 = 0.75 * resVar;
        const double range = 250.0;

        const int m = n - 1;
        const int dim = m + 1;
        std::vector<double> A(dim * dim, 0.0);
        std::vector<double> b(dim, 0.0);
        for (int r = 0; r < m; ++r) {
            for (int c = 0; c < m; ++c) {
                const double d = (r == c) ? 0.0
                                          : std::hypot(samples[idx[r]].x - samples[idx[c]].x,
                                                       samples[idx[r]].y - samples[idx[c]].y);
                A[r * dim + c] = sphCov(c0, c1, range, d);
            }
            A[r * dim + r] += 1e-8;
            A[r * dim + m] = 1.0;
            A[m * dim + r] = 1.0;
            const double d0 = std::hypot(samples[idx[r]].x - samples[ex].x,
                                         samples[idx[r]].y - samples[ex].y);
            b[r] = sphCov(c0, c1, range, d0);
        }
        b[m] = 1.0;

        std::vector<double> sol;
        double resHat = 0.0;
        if (solveLinear(A, b, sol, dim)) {
            for (int r = 0; r < m; ++r) resHat += sol[r] * res[r];
        }

        const double pred = ols.predict(samples[ex]) + resHat;
        predOut[ex] = pred;
        const double err = pred - samples[ex].z;
        sumAbs += std::fabs(err);
        sumSq += err * err;
        sumErr += err;
        maxErr = std::max(maxErr, std::fabs(err));
        const double dm = samples[ex].z - meanObs;
        ssTot += dm * dm;
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

}  // namespace noise_rk
