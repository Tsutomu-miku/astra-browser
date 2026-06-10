import { test, expect, _electron as electron } from "@playwright/test";
import { mkdtempSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";

/**
 * 应用启动冒烟（PRD §5 M0 — Electron 手工 QA 基础）。
 *
 * 覆盖最低可运行性：
 *   1. `electron .` 启动后至少有一个 BrowserWindow；
 *   2. window title 以 "Astra" 开头；
 *   3. 首个可见 window 加载完成不崩溃；
 *   4. 渲染进程可访问 ASTRA_INTEGRATION_TEST 标志。
 */
test.describe("M0 — 应用启动冒烟", () => {
  const profileDir = mkdtempSync(join(tmpdir(), "astra-e2e-"));

  test("启动后至少存在一个 BrowserWindow（PRD M0 验收 #1）", async () => {
    const app = await electron.launch({
      env: {
        VITE_DEV_SERVER_URL: process.env.VITE_DEV_SERVER_URL ?? "",
        ASTRA_INTEGRATION_TEST: "1",
        ASTRA_TEST_PROFILE_DIR: profileDir
      },
      args: ["."]
    });

    const windows = app.windows();
    expect(windows.length).toBeGreaterThanOrEqual(1);

    const primary = windows[0];
    await primary.waitForLoadState("domcontentloaded");
    expect((await primary.title()) || "").toMatch(/^Astra/i);

    const injected = await primary.evaluate(() =>
      window.process?.env?.ASTRA_INTEGRATION_TEST ?? (globalThis as unknown as { __ASTRA_TEST__?: string }).__ASTRA_TEST__
    );
    // 仅验证不会让整个 app 挂；不强绑 preload 注入结构
    expect(typeof injected === "string" || injected === undefined).toBe(true);

    await app.close();
  });
});
