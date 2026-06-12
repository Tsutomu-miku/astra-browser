# ADR-0009: Direct Chromium Architecture

- Status: Accepted
- Date: 2026-06-12
- Deciders: Architecture

## Context

Astra 不能继续基于 Electron；Electron 暴露不出完整 Chrome 浏览器能力。CEF 也不是目标；CEF 适合嵌入网页内容，但不是完整 Chrome framework，扩展、密码、安全浏览、Profile、WebUI、DevTools、Policy 等能力仍会被迫绕路。

## Decision

Astra 迁移为 direct Chromium browser：

- 基于 Chromium checkout 和 GN/Ninja 构建。
- 复用 `chrome/browser`、`components`、`content`、`ui/views`。
- 在 `//astra` 下实现 Astra-only 产品层。
- 对 `//chrome` 只做小 patch point 注册，不 fork 一套平行浏览器框架。

## Consequences

Positive:

- 原生获得 Chrome framework 能力。
- 后续扩展、密码、Safe Browsing、DevTools、WebUI 不需要自研替代品。
- UI 与 browser lifecycle 对齐 Chromium desktop 架构。

Negative:

- Chromium checkout 体积和构建成本高。
- 需要维护 Chromium patch queue。
- API 随 Chromium 版本变动，需要持续 rebase。

## Non-goals

- 不使用 Electron 作为最终 runtime。
- 不使用 CEF 作为 browser shell。
- 不自研 Chromium 已经提供的浏览器基础服务。
