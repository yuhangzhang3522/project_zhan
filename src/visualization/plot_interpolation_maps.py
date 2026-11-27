"""
石湖校区校园噪声时空插值 300 DPI 专题制图与 GB 3096-2008 达标率统计脚本
位置: src/visualization/plot_interpolation_maps.py
依据标准: GB 3096-2008《声环境质量标准》、GB/T 20257.1-2017《国家基本比例尺地图图式》
坐标系: WGS 84 / UTM Zone 51N (EPSG:32651)
"""

from pathlib import Path
import numpy as np
import pandas as pd
import rasterio
from rasterio.windows import from_bounds
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from matplotlib.lines import Line2D

# 中文字体与全局出版级排版配置
plt.rcParams["font.sans-serif"] = ["Microsoft YaHei", "SimHei", "DejaVu Sans"]
plt.rcParams["axes.unicode_minus"] = False
plt.rcParams["savefig.dpi"] = 300

PROJECT_ROOT = Path(__file__).resolve().parents[2]
INPUT_DIR = (
    PROJECT_ROOT / "02_work" / "02_Input data"
    if (PROJECT_ROOT / "02_work" / "02_Input data").exists()
    else PROJECT_ROOT / "02_work" / "Input data"
)
BASEMAP_TIF = (
    PROJECT_ROOT / "02_work" / "01_data preprocessing" / "01_data_range" / "campus" / "石湖校区0.24.tif"
)
RASTERS_DIR = PROJECT_ROOT / "outputs" / "rasters"
TABLES_DIR = PROJECT_ROOT / "outputs" / "tables"
FIGURES_DIR = PROJECT_ROOT / "outputs" / "figures"
FIGURES_DIR.mkdir(parents=True, exist_ok=True)
TABLES_DIR.mkdir(parents=True, exist_ok=True)

PERIOD_META = {
    "morning": ("早间 (08:00–09:00)", "OK 球状模型", "ok_morning.asc"),
    "noon":    ("午间 (11:30–12:30)", "IDW (p=1.5, k=全部)", "idw_noon_best_p1_5_k0.asc"),
    "night":   ("晚间 (19:00–20:00)", "OK 球状模型", "ok_night.asc"),
}


def read_asc_grid(path: Path):
    """读取 ESRI ASCII Grid (.asc)，返回 (data_2d, extent=[xmin, xmax, ymin, ymax], cellsize)"""
    header = {}
    with path.open("r", encoding="utf-8") as f:
        for _ in range(6):
            k, v = f.readline().strip().split()
            header[k.lower()] = float(v)
        data = np.loadtxt(f)
    ncols = int(header["ncols"])
    nrows = int(header["nrows"])
    xll = header["xllcorner"]
    yll = header["yllcorner"]
    cell = header["cellsize"]
    nodata = header["nodata_value"]
    data = np.where(np.isclose(data, nodata), np.nan, data)
    extent = [xll, xll + ncols * cell, yll, yll + nrows * cell]
    return data, extent, cell


def load_basemap_crop(extent, max_dim=1400):
    """按插值外包框裁切并降采样正射影像底图（EPSG:32651）"""
    xmin, xmax, ymin, ymax = extent
    with rasterio.open(BASEMAP_TIF) as src:
        win = from_bounds(xmin, ymin, xmax, ymax, transform=src.transform)
        scale = max(win.width, win.height) / max_dim
        out_h = max(1, int(round(win.height / max(scale, 1.0))))
        out_w = max(1, int(round(win.width / max(scale, 1.0))))
        rgb = src.read(
            [1, 2, 3],
            window=win,
            out_shape=(3, out_h, out_w),
            boundless=True,
            fill_value=245,
        )
    img = np.transpose(rgb, (1, 2, 0))
    return img


def add_scalebar_and_north(ax, extent, bar_len=200.0):
    """在地图子图左下角绘制 200 m 比例尺，右上角绘制指北针（遵循 GB/T 20257.1-2017）"""
    xmin, xmax, ymin, ymax = extent
    w = xmax - xmin
    h = ymax - ymin

    # 200 m 黑白分段比例尺（左下角）
    x0 = xmin + 0.04 * w
    y0 = ymin + 0.055 * h
    bar_h = 0.016 * h
    bg = mpatches.FancyBboxPatch(
        (x0 - 0.015 * w, y0 - 0.025 * h),
        bar_len + 0.03 * w,
        0.085 * h,
        boxstyle="round,pad=0.005",
        facecolor="white",
        edgecolor="#1A2B4C",
        linewidth=0.7,
        alpha=0.88,
        zorder=8,
    )
    ax.add_patch(bg)
    ax.add_patch(mpatches.Rectangle((x0, y0), bar_len / 2, bar_h, facecolor="#1A2B4C", edgecolor="#1A2B4C", lw=0.6, zorder=9))
    ax.add_patch(mpatches.Rectangle((x0 + bar_len / 2, y0), bar_len / 2, bar_h, facecolor="white", edgecolor="#1A2B4C", lw=0.6, zorder=9))
    ax.text(x0, y0 + bar_h * 1.45, "0", fontsize=7.5, ha="center", va="bottom", color="#1A2B4C", fontweight="bold", zorder=10)
    ax.text(x0 + bar_len / 2, y0 + bar_h * 1.45, f"{int(bar_len/2)}", fontsize=7.5, ha="center", va="bottom", color="#1A2B4C", fontweight="bold", zorder=10)
    ax.text(x0 + bar_len, y0 + bar_h * 1.45, f"{int(bar_len)} m", fontsize=7.5, ha="center", va="bottom", color="#1A2B4C", fontweight="bold", zorder=10)

    # 指北针（右上角）
    nx = xmax - 0.065 * w
    ny = ymax - 0.13 * h
    arrow_len = 0.075 * h
    ax.add_patch(mpatches.Circle((nx, ny + arrow_len * 0.45), radius=0.045 * h, facecolor="white", edgecolor="#1A2B4C", lw=0.7, alpha=0.85, zorder=8))
    ax.annotate(
        "",
        xy=(nx, ny + arrow_len),
        xytext=(nx, ny),
        arrowprops=dict(arrowstyle="-|>,head_width=0.35,head_length=0.55", color="#1A2B4C", lw=1.6),
        zorder=9,
    )
    ax.text(nx, ny + arrow_len + 0.012 * h, "N", fontsize=8.5, ha="center", va="bottom", color="#1A2B4C", fontweight="bold", zorder=10)


def format_utm_axes(ax, extent):
    xmin, xmax, ymin, ymax = extent
    ax.set_xlim(xmin, xmax)
    ax.set_ylim(ymin, ymax)
    ax.set_aspect("equal", adjustable="box")
    ax.grid(True, linestyle=":", linewidth=0.55, color="#FFFFFF", alpha=0.75, zorder=5)
    ax.tick_params(labelsize=7.5, colors="#222222")
    ax.set_xlabel("UTM 51N Easting (m)", fontsize=8, color="#1A2B4C")
    ax.set_ylabel("UTM 51N Northing (m)", fontsize=8, color="#1A2B4C")
    ax.xaxis.set_major_locator(plt.MultipleLocator(400))
    ax.yaxis.set_major_locator(plt.MultipleLocator(300))


def plot_fig1_and_compliance(base_pts, metrics_df):
    """图 1：早/中/晚最优插值声场分布图 + 输出 GB 3096-2008 达标率统计表"""
    rasters = {}
    extent = None
    cell = 5.0
    for period, (_, _, fname) in PERIOD_META.items():
        arr, ext, cell = read_asc_grid(RASTERS_DIR / fname)
        rasters[period] = arr
        extent = ext

    basemap = load_basemap_crop(extent)

    # 计算 GB 3096-2008 达标统计
    stats_rows = []
    cell_area_m2 = cell * cell
    for period, (cn_name, model_label, fname) in PERIOD_META.items():
        arr = rasters[period]
        valid = arr[~np.isnan(arr)]
        n_valid = valid.size
        area_ha = n_valid * cell_area_m2 / 10000.0
        leq_area_energy = 10.0 * np.log10(np.mean(10.0 ** (valid / 10.0)))
        pct_over_55 = float(np.mean(valid > 55.0) * 100.0)
        pct_over_60 = float(np.mean(valid > 60.0) * 100.0)
        area_over_55_ha = float(np.sum(valid > 55.0) * cell_area_m2 / 10000.0)
        area_over_60_ha = float(np.sum(valid > 60.0) * cell_area_m2 / 10000.0)

        p_obs = metrics_df[metrics_df["period"] == period]["leq_energy"].values
        stats_rows.append({
            "period": period,
            "period_cn": cn_name,
            "optimal_model": model_label,
            "raster_file": fname,
            "valid_pixels": n_valid,
            "valid_area_ha": round(area_ha, 2),
            "area_leq_energy_dB": round(float(leq_area_energy), 2),
            "area_mean_dB": round(float(np.mean(valid)), 2),
            "area_min_dB": round(float(np.min(valid)), 2),
            "area_max_dB": round(float(np.max(valid)), 2),
            "area_gt55_ha": round(area_over_55_ha, 2),
            "pct_area_gt55": round(pct_over_55, 2),
            "area_gt60_ha": round(area_over_60_ha, 2),
            "pct_area_gt60": round(pct_over_60, 2),
            "points_gt55": int(np.sum(p_obs > 55.0)),
            "points_gt60": int(np.sum(p_obs > 60.0)),
        })

    stats_df = pd.DataFrame(stats_rows)
    stats_path = TABLES_DIR / "gb3096_compliance_stats.csv"
    stats_df.to_csv(stats_path, index=False, encoding="utf-8-sig")
    print(f"[Saved] {stats_path}")
    print(stats_df[["period", "optimal_model", "area_leq_energy_dB", "pct_area_gt55", "pct_area_gt60"]].to_string(index=False))

    # 绘制三时段分布图
    fig, axes = plt.subplots(1, 3, figsize=(18.5, 6.4), constrained_layout=False)
    fig.subplots_adjust(left=0.045, right=0.915, top=0.84, bottom=0.14, wspace=0.14)

    vmin, vmax = 48.0, 68.0
    grid_pts = base_pts[base_pts["point_type"] == "grid"]
    grad_pts = base_pts[base_pts["point_type"] == "gradient"]

    im_handle = None
    for idx, (period, (cn_name, model_label, _)) in enumerate(PERIOD_META.items()):
        ax = axes[idx]
        arr = rasters[period]
        ax.imshow(basemap, extent=extent, origin="upper", zorder=1)
        im_handle = ax.imshow(
            arr, extent=extent, origin="upper", cmap="YlOrRd",
            vmin=vmin, vmax=vmax, alpha=0.72, zorder=2
        )

        # 等声级线：55 dB(A) 1类区昼间限值（绿虚线），60 dB(A) 2类区参考线（深蓝点划线）
        nrows, ncols = arr.shape
        xs = np.linspace(extent[0] + cell / 2, extent[1] - cell / 2, ncols)
        ys = np.linspace(extent[3] - cell / 2, extent[2] + cell / 2, nrows)
        X, Y = np.meshgrid(xs, ys)

        if np.nanmin(arr) < 55.0 < np.nanmax(arr):
            cs55 = ax.contour(X, Y, arr, levels=[55.0], colors=["#008B45"], linestyles="--", linewidths=1.5, zorder=4)
            ax.clabel(cs55, fmt="55 dB", fontsize=7.5, inline=True)
        if np.nanmin(arr) < 60.0 < np.nanmax(arr):
            cs60 = ax.contour(X, Y, arr, levels=[60.0], colors=["#08306B"], linestyles="-.", linewidths=1.6, zorder=4)
            ax.clabel(cs60, fmt="60 dB", fontsize=7.5, inline=True)

        # 采样点叠加
        ax.scatter(grid_pts["utm_e"], grid_pts["utm_n"], s=24, c="#1A2B4C", edgecolors="white", linewidths=0.7, marker="o", zorder=6)
        ax.scatter(grad_pts["utm_e"], grad_pts["utm_n"], s=28, c="#8A2BE2", edgecolors="white", linewidths=0.7, marker="^", zorder=6)

        row_s = stats_rows[idx]
        ax.set_title(
            f"{cn_name}  |  {model_label}\n"
            f"区域能量均值 {row_s['area_leq_energy_dB']:.2f} dB(A)  |  >55 dB: {row_s['pct_area_gt55']:.1f}%  |  >60 dB: {row_s['pct_area_gt60']:.1f}%",
            fontsize=9.5, fontweight="bold", color="#1A2B4C", pad=8,
        )
        format_utm_axes(ax, extent)
        if idx > 0:
            ax.set_ylabel("")
        add_scalebar_and_north(ax, extent, bar_len=200.0)

    # 共享色带
    cbar_ax = fig.add_axes([0.93, 0.19, 0.015, 0.60])
    cbar = fig.colorbar(im_handle, cax=cbar_ax)
    cbar.set_label("等效连续 A 声级 $L_{Aeq}$ [dB(A)]", fontsize=9.5, color="#1A2B4C", fontweight="bold")
    cbar.ax.tick_params(labelsize=8.5)

    # 统一图例
    legend_handles = [
        Line2D([0], [0], color="#008B45", lw=1.6, ls="--", label="GB 3096-2008 1类区昼间限值 (55 dB(A))"),
        Line2D([0], [0], color="#08306B", lw=1.6, ls="-.", label="GB 3096-2008 2类区参考线 (60 dB(A))"),
        Line2D([0], [0], marker="o", color="w", markerfacecolor="#1A2B4C", markersize=6.5, label="网格采样点 (P01–P23)"),
        Line2D([0], [0], marker="^", color="w", markerfacecolor="#8A2BE2", markersize=7.0, label="声源梯度点 (P24–P32)"),
    ]
    fig.legend(handles=legend_handles, loc="lower center", bbox_to_anchor=(0.48, 0.045), ncol=4, frameon=True, fontsize=8.5, edgecolor="#CCCCCC")

    fig.suptitle("苏州科技大学石湖校区早/中/晚校园声环境时空插值分布图", fontsize=14, fontweight="bold", color="#1A2B4C", y=0.955)
    fig.text(
        0.5, 0.012,
        "坐标系：WGS 84 / UTM Zone 51N (EPSG:32651)  |  底图：石湖校区 0.24 m 正射影像  |  外推截断半径：160 m  |  参照标准：GB 3096-2008、GB/T 20257.1-2017",
        ha="center", fontsize=8, color="#555555",
    )

    out_fig = FIGURES_DIR / "fig1_noise_maps_3periods.png"
    fig.savefig(out_fig, dpi=300)
    plt.close(fig)
    print(f"[Saved] {out_fig}")
    return rasters, extent, basemap, stats_df


def plot_fig2_diff(rasters, extent, basemap, base_pts):
    """图 2：午-早、午-晚声级差值变化图（ΔL_Aeq, RdBu_r）"""
    diff_nm = rasters["noon"] - rasters["morning"]
    diff_nn = rasters["noon"] - rasters["night"]

    fig, axes = plt.subplots(1, 2, figsize=(14.5, 6.2))
    fig.subplots_adjust(left=0.055, right=0.90, top=0.84, bottom=0.14, wspace=0.14)

    grid_pts = base_pts[base_pts["point_type"] == "grid"]
    grad_pts = base_pts[base_pts["point_type"] == "gradient"]
    panels = [
        ("午间 − 早间 声级变化 ($\\Delta L_{Aeq} = L_{\\mathrm{noon}} - L_{\\mathrm{morning}}$)", diff_nm),
        ("午间 − 晚间 声级变化 ($\\Delta L_{Aeq} = L_{\\mathrm{noon}} - L_{\\mathrm{night}}$)", diff_nn),
    ]

    im_handle = None
    for idx, (title, arr) in enumerate(panels):
        ax = axes[idx]
        ax.imshow(basemap, extent=extent, origin="upper", zorder=1)
        im_handle = ax.imshow(arr, extent=extent, origin="upper", cmap="RdBu_r", vmin=-12.0, vmax=12.0, alpha=0.75, zorder=2)

        nrows, ncols = arr.shape
        xs = np.linspace(extent[0] + 2.5, extent[1] - 2.5, ncols)
        ys = np.linspace(extent[3] - 2.5, extent[2] + 2.5, nrows)
        X, Y = np.meshgrid(xs, ys)
        if np.nanmin(arr) < 0.0 < np.nanmax(arr):
            cs0 = ax.contour(X, Y, arr, levels=[0.0], colors=["#222222"], linestyles="--", linewidths=1.2, zorder=4)
            ax.clabel(cs0, fmt="0 dB", fontsize=7.5, inline=True)

        ax.scatter(grid_pts["utm_e"], grid_pts["utm_n"], s=22, c="#1A2B4C", edgecolors="white", lw=0.6, marker="o", zorder=6)
        ax.scatter(grad_pts["utm_e"], grad_pts["utm_n"], s=26, c="#8A2BE2", edgecolors="white", lw=0.6, marker="^", zorder=6)

        mean_d = float(np.nanmean(arr))
        max_d = float(np.nanmax(arr))
        min_d = float(np.nanmin(arr))
        ax.set_title(
            f"{title}\n区域平均 $\\Delta L_{{Aeq}}$ = {mean_d:+.2f} dB(A)  (范围: {min_d:+.1f} ~ {max_d:+.1f} dB)",
            fontsize=9.5, fontweight="bold", color="#1A2B4C", pad=8,
        )
        format_utm_axes(ax, extent)
        if idx > 0:
            ax.set_ylabel("")
        add_scalebar_and_north(ax, extent, bar_len=200.0)

    cbar_ax = fig.add_axes([0.92, 0.18, 0.017, 0.62])
    cbar = fig.colorbar(im_handle, cax=cbar_ax)
    cbar.set_label("时段声级差值 $\\Delta L_{Aeq}$ [dB(A)]", fontsize=9.5, color="#1A2B4C", fontweight="bold")

    fig.suptitle("石湖校区校园噪声时段差值空间分布图（午间高峰对比）", fontsize=13.5, fontweight="bold", color="#1A2B4C", y=0.955)
    fig.text(
        0.5, 0.02,
        "红色表示午间声级高于对比时段，蓝色表示午间偏低  |  坐标系：EPSG:32651  |  外推截断半径：160 m  |  参照标准：GB/T 20257.1-2017",
        ha="center", fontsize=8, color="#555555",
    )

    out_fig = FIGURES_DIR / "fig2_spatiotemporal_diff.png"
    fig.savefig(out_fig, dpi=300)
    plt.close(fig)
    print(f"[Saved] {out_fig}")


# 「月白 × 竹青 × 墨绿」东方留白统计图表配色规范（不含地图热力色带）
THEME_BG = "#F3F0E8"        # 月白：画布与子图背景
THEME_INK = "#1E2E28"       # 深墨：主标题与强调文字
THEME_DARK = "#2F4A41"      # 墨绿：坐标轴、次级文字与晚间系列
THEME_BAMBOO = "#6E8C6B"    # 竹青：早间 / 数据系列 1
THEME_CLAY = "#C47D52"      # 暖陶赭：午间高峰 / 警示参考线
THEME_MIST = "#A8BCA1"      # 浅竹灰：误差带 / 置信区间
THEME_GRID = "#D6D2C7"      # 素灰：虚线网格与弱边框


def style_stat_axes(ax, grid_axis="both"):
    """应用东方留白统计图样式：月白底、去顶/右边框、素灰虚线网格"""
    ax.set_facecolor(THEME_BG)
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)
    ax.spines["left"].set_color(THEME_DARK)
    ax.spines["bottom"].set_color(THEME_DARK)
    ax.spines["left"].set_linewidth(0.9)
    ax.spines["bottom"].set_linewidth(0.9)
    ax.tick_params(colors=THEME_DARK, labelsize=8.5, length=3.5, width=0.8)
    ax.grid(True, axis=grid_axis, linestyle="--", linewidth=0.75, color=THEME_GRID, alpha=0.7, zorder=0)
    ax.set_axisbelow(True)


def plot_fig3_variogram():
    """图 3：早/中/晚实验半变异函数散点 + 球状/指数理论拟合曲线（月白×竹青×墨绿）"""
    emp = pd.read_csv(TABLES_DIR / "ok_variogram_empirical.csv")
    par = pd.read_csv(TABLES_DIR / "ok_variogram_params.csv")

    fig, axes = plt.subplots(1, 3, figsize=(16.8, 5.4), facecolor=THEME_BG)
    fig.subplots_adjust(left=0.055, right=0.975, top=0.80, bottom=0.17, wspace=0.22)

    period_titles = {
        "morning": "早间 (08:00–09:00)",
        "noon":    "午间 (11:30–12:30)",
        "night":   "晚间 (19:00–20:00)",
    }

    for idx, period in enumerate(["morning", "noon", "night"]):
        ax = axes[idx]
        style_stat_axes(ax, grid_axis="both")
        e_sub = emp[emp["period"] == period]
        p_sph = par[(par["period"] == period) & (par["model"] == "spherical")].iloc[0]
        p_exp = par[(par["period"] == period) & (par["model"] == "exponential")].iloc[0]

        h_max = max(float(e_sub["mean_dist_m"].max()) * 1.12, 500.0)
        h_grid = np.linspace(0.01, h_max, 300)

        r_s = h_grid / p_sph["range_m"]
        g_sph = p_sph["nugget"] + p_sph["partial_sill"] * np.where(r_s < 1.0, 1.5 * r_s - 0.5 * r_s**3, 1.0)
        g_exp = p_exp["nugget"] + p_exp["partial_sill"] * (1.0 - np.exp(-3.0 * h_grid / p_exp["range_m"]))

        ax.scatter(
            e_sub["mean_dist_m"], e_sub["semivariance"],
            s=np.clip(e_sub["pair_count"] * 1.45, 32, 125),
            c=THEME_BAMBOO, edgecolors=THEME_INK, lw=0.85, alpha=0.92, zorder=4,
            label="实验半变异值 $\\hat{\\gamma}(h)$（点径∝点对数）",
        )
        for _, r in e_sub.iterrows():
            ax.annotate(f"n={int(r['pair_count'])}", (r["mean_dist_m"], r["semivariance"]),
                        textcoords="offset points", xytext=(0, 6.5), ha="center", fontsize=7.0, color=THEME_DARK)

        ax.plot(h_grid, g_sph, color=THEME_DARK, lw=2.2, ls="-", zorder=3,
                label=f"球状模型 ($c_0$={p_sph['nugget']:.2f}, $c_1$={p_sph['partial_sill']:.2f}, $a$={p_sph['range_m']:.0f}m)")
        ax.plot(h_grid, g_exp, color=THEME_CLAY, lw=2.0, ls="--", zorder=3,
                label=f"指数模型 ($c_0$={p_exp['nugget']:.2f}, $c_1$={p_exp['partial_sill']:.2f}, $a$={p_exp['range_m']:.0f}m)")

        ax.set_title(period_titles[period], fontsize=10.8, fontweight="bold", color=THEME_INK, pad=10)
        ax.set_xlabel("空间滞后距离 $h$ (m)", fontsize=9.2, color=THEME_DARK)
        if idx == 0:
            ax.set_ylabel("半变异值 $\\gamma(h)$ [$\\mathrm{dB}^2$]", fontsize=9.2, color=THEME_DARK)
        ax.set_xlim(0, h_max)
        ax.set_ylim(0, max(float(e_sub["semivariance"].max()), float(p_sph["sill"]), float(p_exp["sill"])) * 1.28)
        leg = ax.legend(loc="lower right", fontsize=7.6, frameon=True, facecolor=THEME_BG, edgecolor=THEME_GRID, framealpha=0.95)
        for txt in leg.get_texts():
            txt.set_color(THEME_INK)

    fig.suptitle("早/中/晚实验半变异函数与球状/指数理论模型加权最小二乘（WLS）拟合诊断图", fontsize=13.5, fontweight="bold", color=THEME_INK, y=0.945)
    fig.text(
        0.5, 0.03,
        "数据来源：C++17 KrigingInterpolation 输出 ok_variogram_empirical.csv & ok_variogram_params.csv  |  坐标系：EPSG:32651",
        ha="center", fontsize=8.2, color=THEME_DARK, alpha=0.85,
    )

    out_fig = FIGURES_DIR / "fig3_variogram_diagnostics.png"
    fig.savefig(out_fig, dpi=300, facecolor=THEME_BG)
    plt.close(fig)
    print(f"[Saved] {out_fig}")


def plot_fig4_loocv():
    """图 4：IDW（多幂指数）与 OK（球状/指数）LOOCV RMSE/MAE 对比及最优模型实测 vs 预测散点图（月白×竹青×墨绿）"""
    idw_m = pd.read_csv(TABLES_DIR / "idw_loocv_metrics.csv")
    ok_m = pd.read_csv(TABLES_DIR / "ok_loocv_metrics.csv")
    idw_r = pd.read_csv(TABLES_DIR / "idw_loocv_residuals.csv")
    ok_r = pd.read_csv(TABLES_DIR / "ok_loocv_residuals.csv")

    idw_k0 = idw_m[idw_m["neighbors"] == 0].copy()
    models = [
        ("IDW p=1.0", "idw", 1.0),
        ("IDW p=1.5", "idw", 1.5),
        ("IDW p=2.0", "idw", 2.0),
        ("IDW p=2.5", "idw", 2.5),
        ("IDW p=3.0", "idw", 3.0),
        ("OK 球状",   "ok",  "spherical"),
        ("OK 指数",   "ok",  "exponential"),
    ]
    periods = ["morning", "noon", "night"]
    period_cn = {"morning": "早间", "noon": "午间", "night": "晚间"}
    bar_colors = {"morning": THEME_BAMBOO, "noon": THEME_CLAY, "night": THEME_DARK}

    fig = plt.figure(figsize=(16.8, 9.8), facecolor=THEME_BG)
    gs = fig.add_gridspec(2, 3, height_ratios=[1.05, 1.0], hspace=0.36, wspace=0.24,
                          left=0.06, right=0.975, top=0.895, bottom=0.085)

    gs_top = gs[0, :].subgridspec(1, 2, wspace=0.18)
    ax_rmse = fig.add_subplot(gs_top[0, 0])
    ax_mae = fig.add_subplot(gs_top[0, 1])

    x = np.arange(len(models))
    width = 0.25

    for metric_name, ax in [("RMSE", ax_rmse), ("MAE", ax_mae)]:
        style_stat_axes(ax, grid_axis="y")
        for p_idx, period in enumerate(periods):
            vals = []
            for _, kind, param in models:
                if kind == "idw":
                    v = idw_k0[(idw_k0["period"] == period) & (np.isclose(idw_k0["power"], param))][metric_name].values[0]
                else:
                    v = ok_m[(ok_m["period"] == period) & (ok_m["model"] == param)][metric_name].values[0]
                vals.append(v)
            bars = ax.bar(
                x + (p_idx - 1) * width, vals, width=width,
                color=bar_colors[period], alpha=0.92, edgecolor=THEME_BG, lw=0.7,
                label=period_cn[period], zorder=3,
            )
            for b, v in zip(bars, vals):
                ax.text(b.get_x() + b.get_width() / 2, v + 0.10, f"{v:.2f}",
                        ha="center", va="bottom", fontsize=6.8, color=THEME_INK, rotation=90)

        ax.set_xticks(x)
        ax.set_xticklabels([m[0] for m in models], fontsize=8.5, rotation=15, color=THEME_DARK)
        ax.set_ylabel(f"LOOCV {metric_name} [dB(A)]", fontsize=9.5, color=THEME_INK, fontweight="bold")
        ax.set_title(f"多插值方法留一法交叉验证 {metric_name} 对比（n=32）", fontsize=10.8, fontweight="bold", color=THEME_INK, pad=10)
        ax.set_ylim(0, max(6.8, ax.get_ylim()[1] * 1.15))
        leg = ax.legend(loc="upper right", fontsize=8.5, ncol=3, frameon=True, facecolor=THEME_BG, edgecolor=THEME_GRID, framealpha=0.95)
        for txt in leg.get_texts():
            txt.set_color(THEME_INK)

    best_specs = [
        ("morning", "OK 球状模型", ok_r[(ok_r["period"] == "morning") & (ok_r["model"] == "spherical")],
         ok_m[(ok_m["period"] == "morning") & (ok_m["model"] == "spherical")].iloc[0]),
        ("noon", "IDW (p=1.5, k=全部)", idw_r[(idw_r["period"] == "noon") & (np.isclose(idw_r["power"], 1.5)) & (idw_r["neighbors"] == 0)],
         idw_m[(idw_m["period"] == "noon") & (np.isclose(idw_m["power"], 1.5)) & (idw_m["neighbors"] == 0)].iloc[0]),
        ("night", "OK 球状模型", ok_r[(ok_r["period"] == "night") & (ok_r["model"] == "spherical")],
         ok_m[(ok_m["period"] == "night") & (ok_m["model"] == "spherical")].iloc[0]),
    ]

    for idx, (period, label, df_sub, m_row) in enumerate(best_specs):
        ax = fig.add_subplot(gs[1, idx])
        style_stat_axes(ax, grid_axis="both")
        obs = df_sub["observed"].values
        pred = df_sub["predicted"].values
        lo = min(obs.min(), pred.min()) - 2.0
        hi = max(obs.max(), pred.max()) + 2.0

        ax.fill_between([lo, hi], [lo - 5, hi - 5], [lo + 5, hi + 5],
                        color=THEME_MIST, alpha=0.30, label="±5 dB 误差带", zorder=1)
        ax.plot([lo, hi], [lo, hi], color=THEME_INK, lw=1.4, ls="-", label="1:1 理想线", zorder=2)
        ax.scatter(obs, pred, s=42, c=bar_colors[period], edgecolors=THEME_BG, lw=0.85, alpha=0.94, zorder=4)

        ax.text(
            0.04, 0.95,
            f"最优模型：{label}\nRMSE = {m_row['RMSE']:.3f} dB(A)\nMAE = {m_row['MAE']:.3f} dB(A)\nMaxErr = {m_row['MaxErr']:.2f} dB(A)",
            transform=ax.transAxes, ha="left", va="top", fontsize=8.0, color=THEME_INK,
            bbox=dict(boxstyle="round,pad=0.4", facecolor="#FFFFFF", edgecolor=THEME_GRID, alpha=0.92),
        )
        ax.set_xlim(lo, hi)
        ax.set_ylim(lo, hi)
        ax.set_aspect("equal", adjustable="box")
        ax.set_xlabel("实测声级 $L_{Aeq}$ [dB(A)]", fontsize=9.2, color=THEME_DARK)
        if idx == 0:
            ax.set_ylabel("LOOCV 预测声级 $\\hat{L}_{Aeq}$ [dB(A)]", fontsize=9.2, color=THEME_DARK)
        ax.set_title(f"{period_cn[period]}最优模型实测 vs. 预测散点图", fontsize=10.2, fontweight="bold", color=THEME_INK, pad=8)
        leg = ax.legend(loc="lower right", fontsize=7.6, frameon=True, facecolor=THEME_BG, edgecolor=THEME_GRID, framealpha=0.95)
        for txt in leg.get_texts():
            txt.set_color(THEME_INK)

    fig.suptitle("IDW 与普通克里金（OK）留一法交叉验证（LOOCV）精度对比与最优模型诊断", fontsize=14, fontweight="bold", color=THEME_INK, y=0.955)
    fig.text(
        0.5, 0.02,
        "数据来源：C++17 IDWInterpolation & KrigingInterpolation 输出表（outputs/tables/）  |  样本量 n = 32（全时段无缺测）",
        ha="center", fontsize=8.2, color=THEME_DARK, alpha=0.85,
    )

    out_fig = FIGURES_DIR / "fig4_loocv_comparison.png"
    fig.savefig(out_fig, dpi=300, facecolor=THEME_BG)
    plt.close(fig)
    print(f"[Saved] {out_fig}")


def main():
    import sys
    charts_only = "--charts-only" in sys.argv
    if not charts_only:
        base_pts = pd.read_csv(INPUT_DIR / "01_spatial_base_points.csv")
        metrics_df = pd.read_csv(INPUT_DIR / "02_spatiotemporal_interp.csv")
        rasters, extent, basemap, _ = plot_fig1_and_compliance(base_pts, metrics_df)
        plot_fig2_diff(rasters, extent, basemap, base_pts)
    plot_fig3_variogram()
    plot_fig4_loocv()
    print("[Done] 专题图与统计图表已生成。")


if __name__ == "__main__":
    main()
