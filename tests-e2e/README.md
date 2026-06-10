# Electron 集成测试（Playwright）

对齐 `docs/PRD.md` §8 立即执行 #4 与 §6 风险 #10。
单元/组件测试继续使用 `vitest run`（`tests/*.test.ts`，jsdom）；**真实 Electron 行为**（drag-and-drop、webview lifecycle、权限 IPC、下载、多窗口、会话恢复）走这里。

## 运行

```bash
# 仅首次安装 playwright（electron browser，~250MB）
pnpm add -D @playwright/test @playwright/test

# 运行所有 e2e
pnpm test:e2e

# 跑某个文件
pnpm test:e2e tests-e2e/tabs.spec.ts
```

## 工程约定

- **不引入** 全局 window 断言；跨进程断言走 Electron `evaluate`；
- 测试必须幂等：自己建临时 Profile 目录 (`globalSetup.ts`)，退出即清理；
- 每个 spec 开头 `test.use({ launchOptions: { args: ["--profile-dir", ...] } });`；
- PRD §4.3 临界点 1–20 每一条至少一个 smoke，文件命名 `tests-e2e/milestone0/*.spec.ts`、`tests-e2e/milestone1/*.spec.ts`、…。
