#include "kriging.h"
#include "variogram.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace noise_kriging {

bool solveLinearSystem(std::vector<double>& A,
                       std::vector<double>& b,
                       std::vector<double>& x,
                       int n) {
    x.assign(n, 0.0);
    for (int col = 0; col < n; ++col) {
        // 列主元选取
        int pivotRow = col;
        double maxAbs = std::fabs(A[col * n + col]);
        for (int row = col + 1; row < n; ++row) {
            double v = std::fabs(A[row * n + col]);
            if (v > maxAbs) {
                maxAbs = v;
                pivotRow = row;
            }
        }
        if (maxAbs < 1e-14) {
            return false;
        }
        if (pivotRow != col) {
            for (int k = col; k < n; ++k) {
                std::swap(A[col * n + k], A[pivotRow * n + k]);
            }
            std::swap(b[col], b[pivotRow]);
        }

        const double diag = A[col * n + col];
        for (int row = col + 1; row < n; ++row) {
            const double factor = A[row * n + col] / diag;
            A[row * n + col] = 0.0;
            for (int k = col + 1; k < n; ++k) {
                A[row * n + k] -= factor * A[col * n + k];
            }
            b[row] -= factor * b[col];
        }
    }

    // 回代求解
    for (int row = n - 1; row >= 0; --row) {
        double sum = b[row];
        for (int k = row + 1; k < n; ++k) {
            sum -= A[row * n + k] * x[k];
        }
        x[row] = sum / A[row * n + row];
    }
    return true;
}

namespace {

// 利用列主元高斯-约当消元求 (N+1)x(N+1) 克里金矩阵的逆，加速批量栅格预测
bool invertMatrix(const std::vector<double>& M, std::vector<double>& invM, int n) {
    std::vector<double> aug(n * 2 * n, 0.0);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            aug[i * 2 * n + j] = M[i * n + j];
        }
        aug[i * 2 * n + (n + i)] = 1.0;
    }

    for (int col = 0; col < n; ++col) {
        int pivotRow = col;
        double maxAbs = std::fabs(aug[col * 2 * n + col]);
        for (int row = col + 1; row < n; ++row) {
            double v = std::fabs(aug[row * 2 * n + col]);
            if (v > maxAbs) {
                maxAbs = v;
                pivotRow = row;
            }
        }
        if (maxAbs < 1e-14) return false;

        if (pivotRow != col) {
            for (int k = 0; k < 2 * n; ++k) {
                std::swap(aug[col * 2 * n + k], aug[pivotRow * 2 * n + k]);
            }
        }

        const double diag = aug[col * 2 * n + col];
        for (int k = 0; k < 2 * n; ++k) {
            aug[col * 2 * n + k] /= diag;
        }
        for (int row = 0; row < n; ++row) {
            if (row == col) continue;
            const double factor = aug[row * 2 * n + col];
            if (factor == 0.0) continue;
            for (int k = 0; k < 2 * n; ++k) {
                aug[row * 2 * n + k] -= factor * aug[col * 2 * n + k];
            }
        }
    }

    invM.assign(n * n, 0.0);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            invM[i * n + j] = aug[i * 2 * n + (n + j)];
        }
    }
    return true;
}

}  // namespace

OrdinaryKrigingSolver::OrdinaryKrigingSolver(const std::vector<Sample>& samples,
                                             const VariogramParams& params,
                                             double regEps)
    : samples_(samples),
      params_(params),
      n_(static_cast<int>(samples.size())),
      dim_(n_ + 1),
      c0Total_(params.sill) {
    std::vector<double> K(dim_ * dim_, 0.0);
    for (int i = 0; i < n_; ++i) {
        for (int j = 0; j < n_; ++j) {
            const double d = (i == j) ? 0.0
                                      : std::hypot(samples_[i].x - samples_[j].x,
                                                   samples_[i].y - samples_[j].y);
            K[i * dim_ + j] = evalCovariance(params_, d);
        }
        K[i * dim_ + i] += regEps;
        K[i * dim_ + n_] = 1.0;
        K[n_ * dim_ + i] = 1.0;
    }
    K[n_ * dim_ + n_] = 0.0;

    if (!invertMatrix(K, invK_, dim_)) {
        throw std::runtime_error("普通克里金协方差矩阵奇异，无法求逆。");
    }
}

KrigingEstimate OrdinaryKrigingSolver::predict(double x0, double y0) const {
    for (int i = 0; i < n_; ++i) {
        if (std::hypot(samples_[i].x - x0, samples_[i].y - y0) < 1e-9) {
            return {samples_[i].z, 0.0};
        }
    }

    std::vector<double> rhs(dim_, 1.0);
    for (int i = 0; i < n_; ++i) {
        const double d = std::hypot(samples_[i].x - x0, samples_[i].y - y0);
        rhs[i] = evalCovariance(params_, d);
    }
    rhs[n_] = 1.0;

    double zHat = 0.0;
    double covWeightSum = 0.0;
    for (int i = 0; i < n_; ++i) {
        double lambda = 0.0;
        const double* rowPtr = &invK_[i * dim_];
        for (int j = 0; j < dim_; ++j) {
            lambda += rowPtr[j] * rhs[j];
        }
        zHat += lambda * samples_[i].z;
        covWeightSum += lambda * rhs[i];
    }

    double mu = 0.0;
    const double* muRow = &invK_[n_ * dim_];
    for (int j = 0; j < dim_; ++j) {
        mu += muRow[j] * rhs[j];
    }

    const double varOk = std::max(0.0, c0Total_ - covWeightSum - mu);
    return {zHat, varOk};
}

LoocvMetrics evaluateKrigingLoocv(const std::vector<Sample>& samples,
                                  const VariogramParams& params,
                                  double regEps,
                                  std::vector<ResidualRecord>& residualsOut) {
    const int n = static_cast<int>(samples.size());
    residualsOut.clear();
    residualsOut.reserve(n);

    LoocvMetrics metrics;
    if (n < 3) return metrics;

    double meanObs = 0.0;
    for (const auto& s : samples) meanObs += s.z;
    meanObs /= static_cast<double>(n);

    // 预计算全样本协方差矩阵，LOOCV 每折零拷贝索引跳过 excludeIdx
    std::vector<double> fullCov(n * n, 0.0);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            const double d = (i == j) ? 0.0
                                      : std::hypot(samples[i].x - samples[j].x,
                                                   samples[i].y - samples[j].y);
            fullCov[i * n + j] = evalCovariance(params, d);
        }
        fullCov[i * n + i] += regEps;
    }

    const int m = n - 1;
    const int dim = m + 1;  // == n
    std::vector<double> A(dim * dim, 0.0);
    std::vector<double> b(dim, 0.0);
    std::vector<double> sol(dim, 0.0);
    std::vector<int> activeIdx(m, 0);

    double sumAbs = 0.0, sumSq = 0.0, sumErr = 0.0, ssTot = 0.0, maxErr = 0.0;
    int valid = 0;

    for (int ex = 0; ex < n; ++ex) {
        int pos = 0;
        for (int i = 0; i < n; ++i) {
            if (i != ex) activeIdx[pos++] = i;
        }

        for (int r = 0; r < m; ++r) {
            const int ri = activeIdx[r];
            for (int c = 0; c < m; ++c) {
                A[r * dim + c] = fullCov[ri * n + activeIdx[c]];
            }
            A[r * dim + m] = 1.0;
            A[m * dim + r] = 1.0;
            b[r] = fullCov[ri * n + ex];
        }
        A[m * dim + m] = 0.0;
        b[m] = 1.0;

        // 保留 rhs 副本用于计算克里金估计方差
        const std::vector<double> rhsCopy = b;
        if (!solveLinearSystem(A, b, sol, dim)) {
            residualsOut.push_back({samples[ex].id, samples[ex].z,
                                    std::numeric_limits<double>::quiet_NaN(),
                                    std::numeric_limits<double>::quiet_NaN(),
                                    std::numeric_limits<double>::quiet_NaN()});
            continue;
        }

        double pred = 0.0;
        double covSum = 0.0;
        for (int r = 0; r < m; ++r) {
            pred += sol[r] * samples[activeIdx[r]].z;
            covSum += sol[r] * rhsCopy[r];
        }
        const double mu = sol[m];
        const double varOk = std::max(0.0, params.sill - covSum - mu);
        const double err = pred - samples[ex].z;

        residualsOut.push_back({samples[ex].id, samples[ex].z, pred, err, varOk});

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

}  // namespace noise_kriging
