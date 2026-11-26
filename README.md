# 校园噪声时空制图与多源空间关联分析（石湖校区）

> 苏州科技大学《GIS 算法基础》期末大作业团队协作仓库  
> **小组成员**：张宇航（组长）、刘文昊、朱万里

---

## 一、项目背景与研究目标

校园环境噪声具有显著的**多源性**（道路交通、食堂与校门人流、运动场活动等）、**空间传播衰减特征**与**早/中/晚动态演变规律**。本项目基于苏州科技大学石湖校区 **32 个布设点位**（23 个均匀网格点 + 9 个典型声源梯度点）早、中、晚三轮实地声级与 1/3 倍频程频谱采集数据，叠加 `0.24 m` 高分辨率正射影像，开展：

1. **多源特性与距离衰减分析**：解析交通、人声、风噪的频谱指纹及 `10 m → 30 m → 60 m` 传播衰减规律；
2. **时空插值模拟与精度评价**：对比 IDW、普通克里金（OK）、回归克里金（RK）、RBF 等算法并通过留一法交叉验证（LOOCV）评估各时段最优制图模型；
3. **空间自相关与环境关联**：基于 Global Moran's I、局部 Getis-Ord $G_i^*$ 热点分析及道路/建筑/绿地空间关联建模，识别校园噪声冷热点格局；
4. **校园降噪优化建议**：结合功能区声环境标准提出针对性的空间降噪与传播阻隔方案。

### 关键时间节点

| 日期 | 里程碑任务 | 备注 |
| :--- | :--- | :--- |
| **2026-11-26** | 提交大作业分组名单与协作仓库搭建 | 3–4 人/组，组长 1 人 |
| **2026-12-09** | 提交期末实验报告 | Word + PDF，归档于 `docs/report/` |
| **2026-12-10** | 期末分组汇报答辩 | PPT ≤ 10 页，汇报 ≤ 8 分钟 + 提问 2–4 分钟 |

---

## 二、仓库目录结构

```text
project_zhan/
├── 01_task/                            # 课程大作业要求与模板
│   ├── GIS 算法基础大作业.pdf           # 命题说明、时间节点与评分标准
│   └── GIS算法基础一期末作业封面.docx     # 实验报告统一封面
├── 02_work/                            # 数据采集、预处理与项目共享上下文
│   ├── project_context.md              # 【核心】项目进度、统计结论与统一规范同步文档
│   ├── Input data/                     # 建模标准输入（三张规范表副本）
│   └── data preprocessing/             # 采集与清洗整合流水线
│       ├── 01_data_range/              # 采样点位与正射影像底图
│       │   ├── campus/                 # 石湖校区0.24.tif（.gitignore 忽略）
│       │   └── point/                  # 点位坐标表、布设图与 JSON
│       ├── 02_data_collection/         # 早(morning)/中(noon)/晚(night) 原始轨迹与采集说明
│       ├── 03_data_integration/        # 整合全字段中间表 points_integrated.csv
│       └── 04_data_clean/              # 清洗后的三张规范表（01 / 02 / 03 CSV）
├── 03_reference/                       # 标准规范与参考资料（PDF；超限大文件走群文件/网盘）
├── src/                                # GIS 算法建模与制图源代码（统一使用相对路径）
│   ├── interpolation/                  # 空间插值（IDW / OK / RK / RBF）与 LOOCV 精度评价
│   ├── spatial_analysis/               # 莫兰指数、Gi* 冷热点、距离衰减与空间关联分析
│   └── visualization/                  # 300 DPI 规范专题地图与统计图表绘制脚本
├── outputs/                            # 模型运行结果与出图归档
│   ├── figures/                        # 300 DPI 专题地图与统计图（.png / .pdf）
│   ├── tables/                         # 精度验证与统计分析汇总表（.csv）
│   └── rasters/                        # 插值栅格成果（*.tif 默认被 Git 忽略，防爆仓）
├── docs/                               # 报告与答辩文稿协作区
│   ├── report/                         # 期末实验报告章节草稿与定稿
│   └── slides/                         # 期末汇报 PPT（≤10页）与 8 分钟讲稿
├── CONTRIBUTING.md                     # 【必读】团队 Git 分支与 PR 协作操作指南
└── README.md                           # 项目主页说明
```

---

## 三、核心数据速览

项目已完成阶段一数据清洗与时空插补，后续建模与出图请**直接调用** `02_work/data preprocessing/04_data_clean/` 下的三张规范表（另在 `02_work/Input data/` 提供内容一致的建模副本，详见 `02_work/project_context.md`）：

- **`01_stations_geo.csv`**（32 行 × 11 列）：32 个采样点的静态属性表——`point_type`（grid / gradient）、`functional_zone`、`source_group`、`dist_to_source_m`（10/30/60 m）、`desc`、`lon` / `lat` / `alt_z_m` 及固定 UTM 51N 坐标（`utm_e` / `utm_n`，`EPSG:32651`）。
- **`02_noise_metrics.csv`**（96 行 × 12 列）：32 点 × 早/中/晚三时段（96 条观测）的声级指标表——`leq_energy`、`leq_std`、`L10/L50/L90`、波动度 `fluctuation_L10_L90`、低/中/高频段聚合能量与 `dominant_band`。
- **`03_noise_spectrum.csv`**（96 行 × 25 列）：同上 96 条观测的 23 个 1/3 倍频程（100–16000 Hz）声级，供频谱指纹与频带能量分析。

`02_work/Input data/` 中的建模副本对应关系：`01_spatial_base_points.csv` ↔ `01_stations_geo.csv`，`02_spatiotemporal_interp.csv` ↔ `02_noise_metrics.csv`，`03_multisource_spectrum.csv` ↔ `03_noise_spectrum.csv`（三组逐字节一致）。

> **底图获取提示**：正射影像底图 `石湖校区0.24.tif`（约 `96.4 MB`，`EPSG:32651`）因逼近 GitHub 单文件 `100 MB` 限制已写入 `.gitignore`。首次克隆仓库后，请从**小组共享文件/网盘**下载该文件并放入 `02_work/data preprocessing/01_data_range/campus/石湖校区0.24.tif`。

---

## 四、团队成员快速上手

1. **克隆仓库并配置中文路径显示**
   ```bash
   git clone https://github.com/yuhangzhang3522/project_zhan.git
   cd project_zhan
   git config core.quotepath false
   ```
2. **放入底图文件**：将 `石湖校区0.24.tif` 放到 `02_work/data preprocessing/01_data_range/campus/` 目录下。
3. **按分支开发并提交 PR**：
   ```bash
   git switch main && git pull origin main
   git switch -c feat/你的姓名缩写-模块名
   # 完成开发或文档后提交
   git add .
   git commit -m "feat: 简要说明完成的模块"
   git push -u origin feat/你的姓名缩写-模块名
   ```
   随后在 GitHub 页面点击 **Compare & pull request** 合并回 `main`。详细防冲突规范请查阅 [`CONTRIBUTING.md`](./CONTRIBUTING.md)。
