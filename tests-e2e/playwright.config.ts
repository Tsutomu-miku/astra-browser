import { defineConfig, devices } from "@playwright/test";

/**
 * Playwright + Electron 集成测试配置。
 *
 * 仅在 CI 或本地 `pnpm test:e2e` 触发；不影响默认的 `vitest run` DOM 测试。
 *
 * 对齐 PRD §6 风险 #10：
 *   "Milestone 1 末必须引入 Playwright/Codecept + Electron 集成测试"。
 *
 * 选择 Playwright 理由：
 *   1. 社区维护 electron 官方支持（@playwright/test 内建 Electron 启动器）；
 *   2. 可跨窗口、跨 webview、跨 BrowserView 做断言；
 *   3. 后续可覆盖拖拽、权限 IPC、下载、自动更新、会话恢复等 vitest 无法触及的行为。
 */
export default defineConfig({
  testDir: ".",
  testMatch: "**/*.spec.ts",
  fullyParallel: true,
  forbidOnly: !!process.env.CI,
  retries: process.env.CI ? 2 : 0,
  workers: process.env.CI ? 2 : undefined,
  reporter: process.env.CI ? "github" : "list",
  timeout: 45_000,
  use: {
    trace: "on-first-retry",
    screenshot: "only-on-failure"
  },
  projects: [
    {
      name: "electron-main",
      use: {
        ...devices["Desktop Electron"],
        electronApp: {
          executablePath: require("electron"),
          // 见 src/main/main.js，dev mode 下从 VITE_DEV_SERVER_URL 启动
          env: {
            VITE_DEV_SERVER_URL: process.env.VITE_DEV_SERVER_URL ?? "",
            ASTRA_INTEGRATION_TEST: "1"
          },
          args: ["."]
        }
      }
    }
  ]
});
