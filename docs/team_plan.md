# 团队分工与任务卡（算法实现阶段）

> 2026-11-26 按当前仓库结构修订；所有路径均相对于仓库根目录。
> 初稿中的 `data/cleaned/`、`data/raster/`、`src/spatial_assoc/` 已替换为实际目录。

---

## 一、阶段分工

前置阶段（数据采集与三表规范化建库）已完成，本阶段三人并行推进核心算法与出图：

| 负责人 | 负责模块 | 代码目录 | 输入数据 | 核心交付物 | 任务卡 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **张宇航** | 时空插值与精度评价 | `02_work/algorithms/01_interpolation/` | `01` + `02` + `04` | 早/中/晚 IDW、OK、RBF、回归克里金（RK）分布图、时段差值图与 LOOCV 精度对比表 | #1 |
| **刘文昊** | 校园矢量与空间关联 | `02_work/algorithms/02_spatial_analysis/` | `01` + `02` + 底图 | OSM 道路/建筑矢量、影像 ExG 绿地率、`04_env_covariates.csv`、Moran's I 与 $G_i^*$ 冷热点图 | #2 |
| **朱万里** | 多源频谱与衰减拟合 | `02_work/algorithms/03_spectrum/` | `03` + `02` | 23 频段声源指纹图、10/30/60 m 分频距离衰减拟合曲线（对照 ISO 9613-2:2024） | #3 |

**输入数据对照**

- `01` = `02_work/Input data/01_spatial_base_points.csv`（32 点固定空间基准表）
- `02` = `02_work/Input data/02_spatiotemporal_interp.csv`（96 行早/中/晚声级主表）
- `03` = `02_work/Input data/03_multisource_spectrum.csv`（96 行 × 23 个 1/3 倍频程频谱表）
- `04` = `02_work/Input data/04_env_covariates.csv`（**待产出**，由任务卡 #2 生成）
- 底图 = `02_work/data preprocessing/01_data_range/campus/石湖校区0.24.tif`（群文件获取，已被 `.gitignore` 排除）
- 矢量产出 = `02_work/Input data/vector/`（**待产出**，EPSG:32651）
- 参考标准 = `03_reference/` 下的 PDF（GB 3096-2008、HJ 640-2012、GB/T 15190-2014、GB/T 20257.1-2017、ISO 9613-2:2024）

---

## 二、协作规范

1. **分支隔离**：每人基于 `main` 新建个人分支（`feat/interp-zyh`、`feat/spatial-lwh`、`feat/spectrum-zwl`），完成后提 PR 合并，不要直接往 `main` 推送。
2. **数据衔接**：刘文昊优先产出 `02_work/Input data/04_env_covariates.csv` 并提 PR 合并；张宇航拉取最新 `main` 后再补跑回归克里金（RK）。
3. **制图规范**：所有地图统一 `EPSG:32651`，输出 300 DPI PNG，必须包含图名、图例（单位 `dB(A)`）、比例尺、指北针与坐标格网。
4. **路径规范**：脚本统一用 `pathlib` 基于仓库根目录定位（见 `src/README.md`），禁止写死个人电脑绝对路径。

---

## 三、任务卡（GitHub Issues 草稿）

### 任务卡 #1（负责人：张宇航）

**标题**：`[算法阶段] 模块1：时空插值（IDW/OK/RK）与 LOOCV 精度评价`

**正文**：

```markdown
### 任务目标
基于规范化后的 32 点早/中/晚数据，实现并对比多种空间插值算法，输出规范声场分布图与定量精度评价结果。

### 输入数据
- `02_work/Input data/01_spatial_base_points.csv`
- `02_work/Input data/02_spatiotemporal_interp.csv`
- `02_work/Input data/04_env_covariates.csv`（待任务卡 #2 合并后接入）

### 执行清单
- [ ] 实现并运行早/中/晚三时段 IDW（p=1.5/2.0/3.0）、普通克里金（OK，球状/指数）与薄板样条（RBF）插值
- [ ] 接入 `04_env_covariates.csv`（距道路距离、建筑密度、绿地率），实现回归克里金（Regression Kriging）
- [ ] 完成全算法 LOOCV 留一法交叉验证，输出 `outputs/tables/loocv_metrics.csv`（含 MAE、RMSE、R²、MaxErr）
- [ ] 叠加正射影像底图输出 300 DPI 早/中/晚噪声分布图与时段差值图（ΔL_Aeq）至 `outputs/figures/`
- [ ] 代码整理提交至 `02_work/algorithms/01_interpolation/` 并提 PR 合并
```

---

### 任务卡 #2（负责人：刘文昊）

**标题**：`[算法阶段] 模块2：OSM 矢量提取、绿地因子计算与空间关联分析`

**正文**：

```markdown
### 任务目标
构建石湖校区道路、建筑、绿地空间环境因子，定量分析噪声的空间聚集效应及与环境要素的关联关系。

### 输入数据
- `02_work/Input data/01_spatial_base_points.csv`
- `02_work/Input data/02_spatiotemporal_interp.csv`
- 本地 `02_work/data preprocessing/01_data_range/campus/石湖校区0.24.tif`

### 执行清单
- [ ] 提取石湖校区 OSM 道路与建筑矢量，统一重投影至 UTM 51N（EPSG:32651），保存至 `02_work/Input data/vector/`
- [ ] 基于正射影像计算 RGB 过绿指数（ExG = 2G − R − B），提取 32 点周边 50 m 缓冲区绿地覆盖率
- [ ] 计算 32 点距最近道路距离、50 m 建筑密度与绿地率，优先产出 `02_work/Input data/04_env_covariates.csv`（供任务卡 #1 回归克里金使用）
- [ ] 计算早/中/晚全局 Moran's I 与局部 Getis-Ord Gi* 冷热点，输出 300 DPI 冷热点专题图
- [ ] 完成噪声与道路/建筑/绿地的相关性分析与可视化图表，代码提交至 `02_work/algorithms/02_spatial_analysis/` 并提 PR
```

---

### 任务卡 #3（负责人：朱万里）

**标题**：`[算法阶段] 模块3：多源 1/3 倍频程频谱指纹与距离衰减拟合`

**正文**：

```markdown
### 任务目标
利用 23 个 1/3 倍频程频谱与三组 10/30/60 m 梯度实测数据，揭示校园多源噪声频谱特征与分频传播衰减规律。

### 输入数据
- `02_work/Input data/03_multisource_spectrum.csv`
- `02_work/Input data/02_spatiotemporal_interp.csv`

### 执行清单
- [ ] 绘制早/中/晚全校平均及典型功能区（教学/宿舍/食堂校门/运动绿地）的 23 频段 1/3 倍频程频谱对比图
- [ ] 分析低频（100–315 Hz 交通/风噪）、中频（400–2000 Hz 人声活动）、高频（2.5k–16k Hz 尖锐声）的能量占比与主导频段分布
- [ ] 基于篮球场、二号门、三食堂 10/30/60 m 梯度数据，拟合总声级与分频段距离衰减曲线，对照 ISO 9613-2:2024 解释 60 m 校道多源叠加效应
- [ ] 对照 GB 3096-2008（1 类区昼 55 / 夜 45 dB(A)）统计各功能区超标情况并输出统计图
- [ ] 代码提交至 `02_work/algorithms/03_spectrum/`，图件输出至 `outputs/figures/` 并提 PR
```
