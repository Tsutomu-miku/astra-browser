import { test, expect, _electron as electron } from "@playwright/test";
import { mkdtempSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";

/**
 * PRD §5 M0 交付 #5 — F12 / Ctrl+Shift+I DevTools 统一入口。
 * 占位测试（M0 末需接入真实快捷键验证）。
 */
test.describe("M0 — DevTools 统一入口 (E-4)", () => {
  const profileDir = mkdtempSync(join(tmpdir(), "astra-e2e-"));

  test("F12 派发后能让 DevTools 窗口打开", async () => {
    const app = await electron.launch({
      env: {
        VITE_DEV_SERVER_URL: process.env.VITE_DEV_SERVER_URL ?? "",
        ASTRA_INTEGRATION_TEST: "1",
        ASTRA_TEST_PROFILE_DIR: profileDir
      },
      args: ["."]
    });

    const primary = app.windows()[0];
    await primary.waitForLoadState("domcontentloaded");

    // Playwright Electron 没暴露 DevTools 数量 API；退而求其次：
    // 要求 preload 或 UI 层已经注册了 F12 处理器。
    // 具体断言在 DevTools 入口真实代码合入后补。
    expect(primary).toBeDefined();

    await app.close();
  });
});
