/**
 * 设置页 16 主区块（对齐 PRD §3.14 "Chrome settings 16-section minimum skeleton"）。
 *
 * 优先级规则（与 PRD 同步）：
 *   core   → P0，M0 骨架 + M1/M2 交付真实内容；
 *   polish → P1，M1/M3 期间填内容；
 *   far    → P2，M3/M4 才实现。
 *
 * PRD §5 M0 要求：骨架可导航（即 16 个 entry 能点入并显示交付信息），
 * 不要求内容真实可交互。`implementation` 字段供导航 + 渲染占位/真实面板使用。
 */

export type SettingsSectionImplementation =
  | "you-and-astra"
  | "autofill"
  | "privacy-and-security"
  | "appearance"
  | "search-engine"
  | "default-browser"
  | "startup"
  | "site-settings"
  | "performance"
  | "accessibility"
  | "extensions"
  | "languages"
  | "downloads"
  | "print"
  | "system"
  | "reset-and-cleanup"
  | "about"
  // Legacy 4 panels — 保留以便老链接还能进入新骨架的相应位置
  | "global"
  | "space"
  | "data"
  | "workspaces";

export type SettingsSectionPriority = "core" | "polish" | "far";

export interface SettingsSection {
  id: SettingsSectionImplementation;
  /** 真实导航条目用的标签（英文 + 短，匹配 Chrome settings 风格） */
  label: string;
  /** 交付时间分带（PRD §5 的 M0–M4），仅用于占位面板显示 */
  milestone: "M0" | "M1" | "M2" | "M3" | "M4";
  /** 对应 PRD 的章节号 */
  prdRef: string;
  /** 对应 §3.14 的优先级估算 */
  priority: SettingsSectionPriority;
  /** 1–2 行摘要；M0 占位面板直接展示这个文案 */
  summary: string;
  /** 条目下方显示的详细交付点列表（M0 占位面板展示） */
  scope: string[];
}

/**
 * 16 个 Chrome-level 主区块 + About Astra + 2 个 legacy 别名（global→appearance，
 * space/→workspaces 并入后也保留别名）。
 *
 * 导航顺序严格按 Chrome settings 侧边栏（PRD §3.14）。
 */
export const SETTINGS_SECTIONS: SettingsSection[] = [
  {
    id: "you-and-astra",
    label: "You and Astra",
    milestone: "M1",
    prdRef: "§3.10 (E-11) Profiles + §3.12 (C-1)",
    priority: "core",
    summary: "本地 Profile、登录、跨设备同步入口。",
    scope: ["多人切换 (P0 C-1)", "登录页", "同步开关（默认关闭）"]
  },
  {
    id: "autofill",
    label: "Autofill and passwords",
    milestone: "M1",
    prdRef: "§3.6 (P-1~P-8)",
    priority: "core",
    summary: "密码库、地址、付款方式自动填充。",
    scope: ["密码库 (P0 P-1)", "地址填充 (P0 P-2)", "支付方式 (P1 P-3)", "健康仪表盘 (P1 P-4)"]
  },
  {
    id: "privacy-and-security",
    label: "Privacy and security",
    milestone: "M1",
    prdRef: "§3.7 (K-1~K-12)",
    priority: "core",
    summary: "Safe Browsing、权限总控、浏览数据清理、第三方 Cookie。",
    scope: [
      "强制 HTTPS (P0 K-1)",
      "权限 UI 5 种 (P0 K-2)",
      "站点设置 (P0 K-5)",
      "增强型安全浏览 (P0 K-6)",
      "无痕/访客 (P0 K-12)",
      "历史清理 / 数据导出"
    ]
  },
  {
    id: "appearance",
    label: "Appearance",
    milestone: "M1",
    prdRef: "§3.14 Appearance + §3.11 (U-12~U-14)",
    priority: "polish",
    summary: "主题、色板、字号、缩放、侧栏/书签栏开关。",
    scope: [
      "默认缩放 (U-7)",
      "站点级缩放例外",
      "亮/暗主题跟随系统 (U-12)",
      "Space 独立配色 (U-14)",
      "全局 Chrome 色板 (U-13)"
    ]
  },
  {
    id: "search-engine",
    label: "Search engine",
    milestone: "M1",
    prdRef: "§3.1 (N-1, N-2)",
    priority: "polish",
    summary: "默认搜索引擎、站点 Tab-to-Search、搜索建议开关。",
    scope: ["默认引擎切换", "Tab-to-Search (N-2)", "地址栏建议开关"]
  },
  {
    id: "default-browser",
    label: "Default browser",
    milestone: "M2",
    prdRef: "§3.13 (W-6)",
    priority: "polish",
    summary: "设置/检查 Astra 为系统默认浏览器。",
    scope: ["默认浏览器 API（macOS/Windows/Linux）", "启动时检查"]
  },
  {
    id: "startup",
    label: "On startup",
    milestone: "M0",
    prdRef: "§3.14 Startup",
    priority: "core",
    summary: "恢复会话、打开指定页或空白。",
    scope: ["已存在：restore / homepage 两档", "后续：固定 URL 列表（M2）"]
  },
  {
    id: "site-settings",
    label: "Site settings",
    milestone: "M1",
    prdRef: "§3.7 (K-5)",
    priority: "core",
    summary: "所有 origin 级权限总控（20+ 权限）。",
    scope: [
      "摄像头 / 麦克风 / 定位 / 通知 / 剪贴板 / MIDI / Pointer lock",
      "按 origin 搜索与覆盖",
      "一次性权限（K-3）与自动重置（K-4）"
    ]
  },
  {
    id: "performance",
    label: "Performance",
    milestone: "M2",
    prdRef: "§3.14 Performance + §3.2 (T-7) + §3.13 (W-14)",
    priority: "polish",
    summary: "内存节省 / 节能 / 单 Tab 资源明细。",
    scope: [
      "Memory Saver 策略（已有）",
      "单 Tab RAM/CPU 明细 (T-7)",
      "节能模式 (W-14)"
    ]
  },
  {
    id: "accessibility",
    label: "Accessibility",
    milestone: "M3",
    prdRef: "§3.11 (U-6~U-11)",
    priority: "polish",
    summary: "屏幕阅读器、焦点、实时字幕、大光标、强制高对比。",
    scope: ["系统 NVDA/VoiceOver 走查 (U-6)", "强制页色 (U-8)", "F7 光标浏览 (U-9)", "实时字幕 (U-11)"]
  },
  {
    id: "extensions",
    label: "Extensions",
    milestone: "M2",
    prdRef: "§3.10 (E-1~E-3)",
    priority: "core",
    summary: "Chrome Web Store (MV3) 兼容、权限、卸载。",
    scope: [
      "扩展商店 MVP (P0 E-1)",
      "Top 20 扩展冒烟 (E-2)",
      "护栏与卸载 (E-3)",
      "扩展 Side Panel (V-2, P2)"
    ]
  },
  {
    id: "languages",
    label: "Languages",
    milestone: "M2",
    prdRef: "§3.14 Languages + §3.9 (V-12)",
    priority: "polish",
    summary: "界面语言、翻译策略、拼写检查。",
    scope: ["翻译 MVP (P0 V-12)", "界面多语言", "Chrome 翻译策略覆盖"]
  },
  {
    id: "downloads",
    label: "Downloads",
    milestone: "M1",
    prdRef: "§3.8 (D-1~D-4)",
    priority: "core",
    summary: "下载中心 UI、进度、暂停/取消、位置、危险下载阻断。",
    scope: ["下载中心 (P0 D-1)", "下载位置、提示开关", "危险下载告警 (P1 D-3)"]
  },
  {
    id: "print",
    label: "Print",
    milestone: "M2",
    prdRef: "§3.13 (W-7)",
    priority: "polish",
    summary: "系统对话框 / 另存 PDF / 页眉页脚 / 边距 / 缩放 / 背景图形。",
    scope: ["打印入口、系统对话框、另存 PDF"]
  },
  {
    id: "system",
    label: "System",
    milestone: "M2",
    prdRef: "§3.14 System + §3.13 (W-10~W-11)",
    priority: "polish",
    summary: "硬件加速、代理、后台运行、自动更新、签名、打开文件。",
    scope: [
      "硬件加速 / 代理",
      "自动更新 (W-10)",
      "macOS Notarization / Windows EV 签名 (W-11)"
    ]
  },
  {
    id: "reset-and-cleanup",
    label: "Reset and cleanup",
    milestone: "M2",
    prdRef: "§3.14 Reset",
    priority: "polish",
    summary: "恢复默认设置、清理垃圾扩展、浏览数据批量清理。",
    scope: ["恢复默认", "清理扩展（依赖 extensions 面板）", "一次性数据清理"]
  },
  {
    id: "about",
    label: "About Astra",
    milestone: "M0",
    prdRef: "§3.14 About",
    priority: "polish",
    summary: "版本号、开源声明、协议、更新通道。",
    scope: ["版本号显示", "开源依赖清单", "更新通道"]
  },
  {
    id: "global",
    label: "Global (legacy → Appearance + System)",
    milestone: "M0",
    prdRef: "legacy",
    priority: "polish",
    summary: "ROADMAP 原 Global 面板，已并入新骨架。",
    scope: ["继续展示 GlobalSettingsSection 真实内容"]
  },
  {
    id: "space",
    label: "Space (legacy → Workspaces)",
    milestone: "M0",
    prdRef: "legacy",
    priority: "polish",
    summary: "ROADMAP 原 Space 面板，已并入新骨架的 Spaces。",
    scope: ["继续展示 SpaceSettingsSection 真实内容"]
  },
  {
    id: "data",
    label: "Data (legacy → Privacy + Downloads)",
    milestone: "M0",
    prdRef: "legacy",
    priority: "core",
    summary: "ROADMAP 原 Data 面板，已并入 Privacy/Downloads。",
    scope: ["继续展示 DataSettingsSection 真实内容"]
  },
  {
    id: "workspaces",
    label: "Workspaces",
    milestone: "M0",
    prdRef: "§3.2 (T-8 ~ T-11)",
    priority: "core",
    summary: "Space 的创建、删除、重命名、配色。",
    scope: ["继续展示 WorkspaceManagementSection 真实内容"]
  }
];

export type SettingsSectionId = SettingsSection["id"];

/** Legacy 4 项仍然真实渲染内容；其余 16 项先进入占位面板。 */
export function isLegacyRealSection(id: SettingsSectionId): boolean {
  return id === "global" || id === "space" || id === "data" || id === "workspaces";
}
