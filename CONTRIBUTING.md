# 团队协作指南（CONTRIBUTING）

本项目为苏州科技大学《GIS 算法基础》期末大作业（**校园噪声制图——石湖校区**）团队协作仓库。
**小组成员**：张宇航（组长）、刘文昊、朱万里

为防止多人同时修改导致代码冲突、二进制文件损坏或大文件撑爆仓库，请全员遵循以下 **轻量分支 + Pull Request (PR)** 协作流程。

---

## 一、首次克隆与环境准备

### 1. 克隆仓库到本地
收到组长的 GitHub Collaborator 邀请邮件（或打开邀请链接点击 Accept）后，在本地终端执行：

```bash
git clone https://github.com/yuhangzhang3522/project_zhan.git
cd project_zhan
```

建议顺便配置中文文件名正常显示（防止 `git status` 中文变成八进制转义字符）：

```bash
git config core.quotepath false
```

### 2. 放置正射影像底图（重要）
由于底图 `石湖校区0.24.tif` 体积约 `96.4 MB`，逼近 GitHub `100 MB` 单文件硬上限，已通过 `.gitignore` 排除。
请从**小组群文件 / 网盘**下载 `石湖校区0.24.tif`，手动放入以下相对路径：

```text
_work/data preprocessing/data_range/石湖校区0.24.tif
```

放入后运行 `git status`，确认该 `.tif` 文件不会出现在待提交列表中即可。

---

## 二、日常分支协作流程（轻量分支 + PR）

**核心原则**：`main` 分支始终保持最新、可运行、无冲突的稳定状态；**不要直接在 `main` 分支上修改和推送**。

### 第 1 步：开始工作前，同步最新 `main` 并新建个人任务分支
分支命名建议使用：`类型/姓名缩写或模块`，例如 `feat/lwh-kriging`、`plot/zwl-hotspot`、`docs/zyh-report`。

```bash
git switch main
git pull origin main
git switch -c feat/你的分支名
```

### 第 2 步：在对应目录完成开发或文档编写
- **代码与脚本**：放入 `src/` 对应子目录（`interpolation/`、`spatial_analysis/`、`visualization/`）
- **地图与统计图表（300 DPI）**：输出到 `outputs/figures/`，精度统计表输出到 `outputs/tables/`
- **中间栅格（`.tif`）**：输出到 `outputs/rasters/`（自动被 Git 忽略，不上传仓库）
- **实验报告与汇报 PPT**：放入 `docs/report/` 与 `docs/slides/`
- **进度同步**：完成关键阶段后，同步更新 `_work/project_context.md` 中的「三、当前进度」

### 第 3 步：提交并推送分支到 GitHub

```bash
git status
git add .
git commit -m "feat: 完成普通克里金与IDW留一法交叉验证脚本"
git push -u origin feat/你的分支名
```

**Commit 提交信息前缀规范**：
- `feat:` 新增算法、分析脚本或功能模块
- `plot:` 新增或更新专题地图、分析图表
- `data:` 新增或更新清洗后的 CSV / 矢量特征数据
- `docs:` 撰写或修改实验报告、PPT、说明文档
- `fix:` 修复脚本报错、路径或制图问题

### 第 4 步：在 GitHub 发起 Pull Request (PR) 并合并
1. 打开仓库主页 <https://github.com/yuhangzhang3522/project_zhan>
2. 点击黄色提示条 **Compare & pull request**
3. 简要填写改动说明，点击 **Create pull request**
4. 确认无冲突后合并（Merge pull request）进 `main`；下次做新任务前重复「第 1 步」即可。

---

## 三、三大防踩坑铁律（必看）

1. **严禁写死个人电脑绝对路径**
   - 错误示范：`C:\Users\yuhangZhang\Desktop\大作业\project_zhan\_work\...`（别人电脑运行必报错）
   - 正确示范：使用基于仓库根目录的 `pathlib.Path` 相对路径（详见 `src/README.md`）。
2. **严禁多人同时在不同分支修改同一个 `.docx` 或 `.pptx` 文件**
   - Word（`.docx`）和 PPT（`.pptx`）是二进制文件，Git **无法自动合并文本行**。
   - 多人分工撰写报告时，建议先各自在 `docs/report/` 下按章节新建独立文件（如 Markdown 或独立分章 Word），定稿阶段再由一人统稿合并；修改最终版 `.docx`/`.pptx` 前先在群里打招呼并 `git pull`。
3. **注意单文件大小限制**
   - 切勿强行添加 `> 50 MB` 的栅格、视频或压缩包进 Git；所有 `.tif` 栅格统一走群文件/网盘共享。
