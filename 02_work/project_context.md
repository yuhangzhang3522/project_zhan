# GIS 算法基础期末大作业 — 项目共享上下文（PROJECT_CONTEXT）

> **用途**：本文件用于保持云端 Gemini（项目总领/方案把关）、本地 Antigravity（本地代码执行/影像处理/出图排版）以及小组成员之间的进度同步。每次完成一个阶段后，请更新「三、当前进度」与「六、下一步待办」。
> **最后更新**：2026-11-26

---

## 一、项目基本信息与硬性要求

- **课程与题目**：苏州科技大学《GIS 算法基础》期末大作业 — **校园噪声制图（石湖校区）**
- **小组成员**：张宇航（组长）、刘文昊、朱万里
- **协作仓库**：<https://github.com/yuhangzhang3522/project_zhan>（组长本地路径：`C:\Users\yuhangZhang\Desktop\大作业\project_zhan`；各成员脚本请统一使用基于仓库根目录的相对路径）
- **关键时间节点**：
  - **2026-11-26**：提交大作业分组名单（3–4 人/组，组长 1 人）
  - **2026-12-09**：提交期末实验报告
  - **2026-12-10**：分组汇报（**PPT ≤ 10 页**，汇报时间 ≤ 8 分钟，提问 2–4 分钟）
- **核心命题（来自 `GIS 算法基础大作业.pdf`）**：
  1. **噪声特性**：多源性、噪声传播、噪声衰减……
  2. **时空模拟**：噪声强度和分布随时间（早/中/晚）改变
  3. **空间关联**：是否存在聚集效应，是否与道路、建筑、树木/绿地等有空间关联
  4. **优化建议**：提出合理的校园降噪与传播优化措施
- **评分标准（各占 20%，总计 100%）**：
  1. 数据采集与处理（20%）
  2. 算法设计的合理性（20%）
  3. 精度评价（20%）
  4. 报告规范性（20%）
  5. 汇报表现（20%）

---

## 二、本地目录与核心数据资产

### 1. 原始项目文件与采集数据
- `01_task/GIS 算法基础大作业.pdf` & `GIS算法基础一期末作业封面.docx`：作业命题、时间节点、评分标准与报告封面模板
- `02_work/01_data preprocessing/02_data_collection/采集任务说明.pdf`：2026-11-25（周三）早/中/晚三轮采集方案与 32 点位 WGS84/GCJ02 坐标表
- `02_work/01_data preprocessing/01_data_range/point/points_final.png`（及 `points_final.json`、`Orthophoto_point_location.csv`）：32 个采样点位布设图与坐标（网格点 1–23，间距约 150 m；梯度点 24–32：篮球场向西 10/30/60 m、二号门向西 10/30/60 m、三食堂向北 10/30/60 m）
- `02_work/01_data preprocessing/01_data_range/campus/石湖校区0.24.tif`：石湖校区正射影像底图（`10008 × 6979` RGB，分辨率 `0.24 m`，投影 **WGS 84 / UTM zone 51N (EPSG:32651)**，左上角坐标 `E=267617.0, N=3460864.011`；*注：因体积约 96.4MB 已在 `.gitignore` 中忽略，克隆后需手动放入该目录*）
- `02_work/01_data preprocessing/02_data_collection/{morning,noon,night}/track.geojson` & `meta.properties`：HONOR 手机（AAP-AN00）+ NoiseCapture 1.3.2 全程连续录制数据（早 4,925 条、中 5,248 条、晚 3,468 条，含 100–16000 Hz 23 个 1/3 倍频程频段；`gain_calibration=0.00`；早间带 `tags=wind` 风噪标签）

### 2. 数据整合与规范化输出（位于 `02_work/01_data preprocessing/`）

1. **`03_data_integration/points_integrated.csv`**（96 行 × 47 列）
   - 整合后的观测全字段中间表：保留实测时间区间 `start_time` / `end_time`、`n_sec`、原始经纬度、高程 `alt_z_m`、固定 UTM 51N 坐标、GPS `accuracy_mean_m`、运动状态（`speed_mean_mps`、`bearing_mean_deg`）、`meta_tags`，以及声级统计量（`leq_energy`、`leq_arith`、`leq_std`、`L10/L50/L90`、`leq_min` / `leq_max`）与 23 个 1/3 倍频程频段。
2. **`04_data_clean/` 三张规范化表（后续 GIS 建模主力数据）**
   - **`01_stations_geo.csv`**（32 行 × 11 列）：点位静态属性——`point_type`（grid / gradient）、`functional_zone`（教学科研区 / 宿舍生活区 / 食堂与校门 / 运动与东侧绿地）、`source_group` & `dist_to_source_m`（10/30/60 m）、`desc`、`lon` / `lat` / `alt_z_m`、`utm_e` / `utm_n`（`EPSG:32651` 固定坐标）。
   - **`02_noise_metrics.csv`**（96 行 × 12 列）：以 `(period, point_id)` 为键的声级指标——`leq_energy`、`leq_std`、`L10` / `L50` / `L90`、波动度 `fluctuation_L10_L90`、低/中/高频段聚合能量（`band_low_100_315_dB`、`band_mid_400_2000_dB`、`band_high_2500_16000_dB`）与 `dominant_band`。
   - **`03_noise_spectrum.csv`**（96 行 × 25 列）：同一批 96 条观测的 23 个 1/3 倍频程（100–16000 Hz）声级。
   - `02_work/02_Input data/` 另存三张表的逐字节同内容副本（`01_spatial_base_points.csv`、`02_spatiotemporal_interp.csv`、`03_multisource_spectrum.csv`），供建模脚本直接引用。


### 3. 标准规范与参考资料（`03_reference/`）

- 已入库：`ISO-9613-2-2024.pdf`、`声环境质量标准.pdf`、`声环境功能区划分技术规范.pdf`、`环境噪声监测技术规范 .pdf`、`环境专题空间数据加工处理技术规范.pdf`、`公开地图内容表示规范.pdf`。
- `国家基本比例尺地图图示.pdf`（约 109.5 MB，扫描版 131 页）超过 GitHub 单文件 100 MB 硬限制，已在 `.gitignore` 中排除；请从小组共享文件 / 网盘获取，放置于 `03_reference/` 下。

---

## 三、当前进度

- [x] **阶段一：数据清洗与规范化整合**（已完成）
  - 核对全部早、中、晚 NoiseCapture 截图时间戳，彻底剔除骑行风噪段（原始全程均值 66.5–74.1 dB(A) → 清洗后合理日变化规律）。
  - 产出 `04_data_clean/01_stations_geo.csv`、`02_noise_metrics.csv`、`03_noise_spectrum.csv` 三份规范表。
  - 2026-11-26：目录重构（`Task_requirements/` → `01_task/`，`_work/` → `02_work/`，预处理流水线重编号为 `01_data_range` / `02_data_collection` / `03_data_integration` / `04_data_clean`）；原先的三张宽表重构为「点位属性 / 声级指标 / 频谱」三张规范表，坐标与 `leq_energy` 已与旧表逐点核对一致。
  - **核心统计结论（32 点全集）**：
    - 早（08:00–09:00）：均值 `57.16 dB(A)`（实测 28 点均值 `57.31`）
    - 中（11:30–12:30）：均值 `62.22 dB(A)`（实测 32 点全覆盖）
    - 晚（19:00–20:00）：均值 `55.34 dB(A)`（实测 22 点均值 `55.67`）
- [x] **阶段二 & 三（模块 1 — 张宇航）：时空插值（IDW / OK / RK 预留）、LOOCV 精度评价与 300 DPI 合规专题制图**（已收官）
  - **纯标准 C++17 模块化工程**（位于 `02_work/03_algorithms/01_interpolation/`，零第三方库依赖，含 VS 2019 `.sln`/`.vcxproj` 与 `CMakeLists.txt`，均已 `Release | x64` 零报错编译通过）：
    - `idw_cpp/`（`IDWInterpolation`）：支持多幂指数（`1.0/1.5/2.0/2.5/3.0`）× 多邻域数（`0/8/12`）与 `160 m` 栅格外推截断半径。
    - `kriging_cpp/`（`KrigingInterpolation`）：手写实验半变异函数、WLS 球状/指数理论拟合、列主元高斯消元求解器与零拷贝 LOOCV，同步输出估值与克里金方差栅格。
    - `rk_cpp/`（`RegressionKriging`）：已完成 OLS（距道路距离/建筑密度/绿地率）+ 残差球状克里金 + LOOCV 完整工程编译，待任务卡 #2 提交 `02_work/02_Input data/04_env_covariates.csv` 后可一键运行。
  - **LOOCV 精度对比（全集 $n=32$，RMSE / MAE，单位 `dB(A)`）**：
    - **早间（08:00–09:00）**：**OK 球状 `4.445 / 3.399`** ≈ OK 指数 `4.449 / 3.402` < IDW($p=1.0, k=0$) `4.713 / 3.621` < IDW($p=1.5, k=0$) `4.966 / 3.785` < IDW($p=2.0, k=0$) `5.232 / 3.924`（因早间存在风噪 `tags=wind`，块基比偏高，克里金平滑效果最优）
    - **午间（11:30–12:30）**：**IDW($p=1.5, k=0$) `4.168 / 3.410` ($R^2=+0.005$)** < IDW($p=2.0, k=0$) `4.198 / 3.441` < OK 球状 `4.233 / 3.453` < OK 指数 `4.292 / 3.490`（午间受局部强点/线声源控制，全局邻域 $p=1.5$ IDW 最优）
    - **晚间（19:00–20:00）**：**OK 球状 `3.105 / 2.546` ($R^2=+0.045$, 变程 $a=451\text{ m}$)** < OK 指数 `3.192 / 2.585` < IDW($p=1.0, k=8$) `3.219 / 2.508` < IDW($p=1.5, k=0$) `3.224 / 2.456`（晚间正空间自相关显著，OK 球状模型最优）
  - **GB 3096-2008 声环境达标与超标面积统计**（有效插值域 `103.50 ha`，外推截断半径 `160 m`，详见 `outputs/tables/gb3096_compliance_stats.csv`）：
    - 早间（OK 球状）：区域能量等效声级 `57.15 dB(A)`，>55 dB 占 `100.00%`（`103.50 ha`），>60 dB 占 `0.00%`（`0.00 ha`）；实测超 55 / 60 dB 点位数 `23 / 8`
    - 午间（IDW $p=1.5$）：区域能量等效声级 `62.47 dB(A)`，>55 dB 占 `99.21%`（`102.67 ha`），>60 dB 占 `90.40%`（`93.56 ha`）；实测超 55 / 60 dB 点位数 `31 / 23`
    - 晚间（OK 球状）：区域能量等效声级 `56.23 dB(A)`，>55 dB 占 `69.60%`（`72.03 ha`），>60 dB 占 `1.77%`（`1.83 ha`）；实测超 55 / 60 dB 点位数 `19 / 3`
  - **新增产出文件清单**：
    - 制图脚本：`src/visualization/plot_interpolation_maps.py`
    - 300 DPI 专题图（`outputs/figures/`）：`fig1_noise_maps_3periods.png`、`fig2_spatiotemporal_diff.png`、`fig3_variogram_diagnostics.png`、`fig4_loocv_comparison.png`
    - 精度与统计表（`outputs/tables/`）：`idw_loocv_metrics.csv`、`idw_loocv_residuals.csv`、`ok_variogram_empirical.csv`、`ok_variogram_params.csv`、`ok_loocv_metrics.csv`、`ok_loocv_residuals.csv`、`gb3096_compliance_stats.csv`
    - 栅格产物（`outputs/rasters/`，均配 EPSG:32651 `.prj`）：`idw_{morning_best_p1_k0,noon_best_p1_5_k0,night_best_p1_k8}.asc`、`ok_{morning,noon,night}.asc`、`ok_var_{morning,noon,night}.asc`
  - **空间自相关与多源频谱（模块 2 & 3 待队友深化）**：晚间 Global Moran's I `I = +0.2189`（$Z = +2.11, p = 0.081$），午间三组梯度 10→30 m 衰减明显，60 m 处受邻近校道交通叠加回升。
- [ ] **阶段四：期末实验报告撰写（12 月 9 日前）**
- [ ] **阶段五：期末汇报 PPT（≤ 10 页）与 8 分钟讲稿（12 月 10 日前）**

---

## 四、视觉与制图规范（如涉及美化图表/PPT）

- **地图规范**：统一使用 **UTM Zone 51N (EPSG:32651)**，叠加 `石湖校区0.24.tif` 底图，所有专题地图必须具备 **图名、图例/色带（单位 dB(A)）、指北针、比例尺、坐标格网**。
- **统计图表主题配色（「月白 × 竹青 × 墨绿」东方留白规范，不含地图热力色带）**：
  - 画布与子图背景（月白）：`#F3F0E8`；去顶/右坐标轴边框（despine），`DPI = 300`
  - 标题、坐标轴与主文字（深墨绿）：`#2F4A41` / `#1E2E28`；网格线与弱边框（素灰）：`#D6D2C7`（虚线 `alpha=0.7`）
  - 数据系列：早间 / 系列 1 竹青 `#6E8C6B`、晚间 / 系列 2 墨绿 `#2F4A41`、午间高峰 / 警示参考线暖陶赭 `#C47D52`、误差带 / 浅色辅助浅竹灰 `#A8BCA1`（`alpha=0.25–0.35`）
  - 地图热力色带：仍保持声学/GIS 标准的 `YlOrRd`（分贝绝对值）与 `RdBu_r`（时段差值 $\Delta L_{Aeq}$）。

---

## 五、给 Antigravity 与团队成员的协作说明

1. **直接复用 `02_work/01_data preprocessing/04_data_clean/` 下的三张规范表**（或 `02_work/02_Input data/` 中的同内容副本）作为后续建模与制图输入，不要重新从原始 `track.geojson` 做猜测性提取。
2. **优先利用本地算力与原图**：直接读取 `02_work/01_data preprocessing/01_data_range/campus/石湖校区0.24.tif` 制作高分辨率（300 DPI）专题图；可进一步结合校园道路/建筑/绿地矢量或影像分类结果，深化“与道路、建筑、树木空间关联”的定量分析（如距离道路/绿地相关性回归或缓冲区对比）。
3. **统一代码与成果归档目录**：
   - 算法代码放入 `02_work/03_algorithms/`（子目录 `01_interpolation/`、`02_spatial_analysis/`、`03_spectrum/`），制图脚本放入 `src/visualization/`，必须使用基于仓库根目录的相对路径；
   - 300 DPI 图表输出至 `outputs/figures/`，精度汇总表输出至 `outputs/tables/`，插值栅格输出至 `outputs/rasters/`；
   - 报告与答辩 PPT 分别放入 `docs/report/` 与 `docs/slides/`。
4. **每次完成新模块（如新算法、新图表、报告草稿）后**，请把关键结论、新增文件路径和精度指标更新回本文件的「三、当前进度」，方便团队成员与云端 Gemini 同步把关。

---

## 六、下一步待办（Next Steps）

1. [x] 将三份 CSV（`01_` / `02_` / `03_`）整理放入 `02_work/01_data preprocessing/04_data_clean/`（并在 `02_work/02_Input data/` 提供建模副本），完成 GitHub 多人协作仓库初始化。
2. [x] 完成模块 1 纯 C++17 IDW、普通克里金（OK）与回归克里金（RK 预留工程）编译验证，以及 4 张 300 DPI 合规专题图与 GB 3096-2008 达标面积统计（`outputs/figures/`、`outputs/tables/`、`outputs/rasters/`）。
3. [ ] 待任务卡 #2（刘文昊）提交 `02_work/02_Input data/04_env_covariates.csv` 后，直接运行 `rk_cpp/x64/Release/RegressionKriging.exe` 补充回归克里金精度对比。
4. [ ] 撰写《GIS 算法基础》期末实验报告（Word + PDF，存入 `docs/report/`）。
5. [ ] 制作 ≤10 页汇报 PPT 与 8 分钟答辩讲稿（存入 `docs/slides/`）。
