import { describe, expect, it } from "vitest";

import type { ShortcutEventLike } from "../src/renderer/common/shortcuts/keyboardShortcuts";
import { resolveShortcut } from "../src/renderer/common/shortcuts/keyboardShortcuts";
import type { PiPToggleResult } from "../src/renderer/types/electron";

function event(overrides: Partial<ShortcutEventLike>): ShortcutEventLike {
  return {
    altKey: false,
    ctrlKey: false,
    key: "",
    metaKey: false,
    shiftKey: false,
    ...overrides
  };
}

/**
 * PiP 主进程 JS 中 `findActiveWebviewId` 的纯逻辑测试：
 *   因为 findActiveWebviewId 依赖 Electron BrowserWindow，无法在 jsdom 中测试，
 *   这里提取其等价逻辑做纯逻辑测试。
 */
function simulateFindActiveWebviewId(
  windows: Array<{
    destroyed: boolean;
    webContents: {
      destroyed: boolean;
      getAllWebContents?: () => Array<{
        destroyed: boolean;
        getType: () => string;
        id: number;
      }>;
    };
  }>
): number | null {
  for (const win of windows) {
    if (win.destroyed || !win.webContents || win.webContents.destroyed) continue;
    const children = win.webContents.getAllWebContents?.() ?? [];
    for (const child of children) {
      if (!child.destroyed && child.getType() === "webview") {
        return child.id;
      }
    }
  }
  return null;
}

/**
 * PiP IPC 入口 `togglePictureInPicture` 的纯逻辑等价物：
 *   当 webContentsId 无效时返回 no-active-tab 错误。
 */
function simulateTogglePiPWithId(
  webContentsId: number | undefined,
  lookup: Map<number, { destroyed: boolean; exec: (script: string) => unknown }>
): { success: boolean; reason?: string; entering?: boolean } {
  if (typeof webContentsId !== "number") return { success: false, reason: "no-active-tab" };
  const wc = lookup.get(webContentsId);
  if (!wc || wc.destroyed) return { success: false, reason: "destroyed" };
  return { success: true, entering: true };
}

describe("Media Session & PiP (U-1/U-2)", () => {
  describe("shortcut resolution", () => {
    it("resolves Cmd+Alt+P as togglePictureInPicture", () => {
      expect(resolveShortcut(event({ altKey: true, metaKey: true, key: "p" }))).toEqual({
        type: "togglePictureInPicture"
      });
      expect(resolveShortcut(event({ altKey: true, ctrlKey: true, key: "p" }))).toEqual({
        type: "togglePictureInPicture"
      });
    });

    it("does not resolve Ctrl+P or Meta+P (reserved for print)", () => {
      expect(resolveShortcut(event({ ctrlKey: true, key: "p" }))).toBeNull();
      expect(resolveShortcut(event({ metaKey: true, key: "p" }))).toBeNull();
    });

    it("does not resolve plain Alt+P without command modifier", () => {
      expect(resolveShortcut(event({ altKey: true, key: "p" }))).toBeNull();
    });

    it("still resolves Cmd+Alt+C as toggleCompactMode", () => {
      expect(resolveShortcut(event({ altKey: true, metaKey: true, key: "c" }))).toEqual({
        type: "toggleCompactMode"
      });
    });
  });

  describe("findActiveWebviewId pure logic", () => {
    it("returns the first non-destroyed webview child across windows", () => {
      const result = simulateFindActiveWebviewId([
        { destroyed: true, webContents: { destroyed: false } },
        {
          destroyed: false,
          webContents: {
            destroyed: false,
            getAllWebContents: () => [
              { destroyed: false, getType: () => "backgroundPage", id: 1 },
              { destroyed: false, getType: () => "webview", id: 42 }
            ]
          }
        },
        {
          destroyed: false,
          webContents: {
            destroyed: false,
            getAllWebContents: () => [
              { destroyed: false, getType: () => "webview", id: 99 }
            ]
          }
        }
      ]);
      expect(result).toBe(42);
    });

    it("skips destroyed webviews and returns null when none found", () => {
      const result = simulateFindActiveWebviewId([
        {
          destroyed: false,
          webContents: {
            destroyed: false,
            getAllWebContents: () => [
              { destroyed: true, getType: () => "webview", id: 10 }
            ]
          }
        }
      ]);
      expect(result).toBeNull();
    });

    it("handles windows with no getAllWebContents implementation", () => {
      expect(simulateFindActiveWebviewId([
        { destroyed: false, webContents: { destroyed: false } }
      ])).toBeNull();
    });
  });

  describe("togglePictureInPicture pure logic", () => {
    it("returns no-active-tab for undefined id", () => {
      const r = simulateTogglePiPWithId(undefined, new Map());
      expect(r).toEqual({ success: false, reason: "no-active-tab" });
    });

    it("returns destroyed for unknown id", () => {
      const r = simulateTogglePiPWithId(5, new Map());
      expect(r).toEqual({ success: false, reason: "destroyed" });
    });

    it("returns success for valid id", () => {
      const map = new Map();
      map.set(42, { destroyed: false, exec: () => ({ success: true }) });
      const r = simulateTogglePiPWithId(42, map);
      expect(r.success).toBe(true);
      expect(r.entering).toBe(true);
    });
  });

  describe("types", () => {
    it("PiPToggleResult accepts valid success payloads", () => {
      const okEnter: PiPToggleResult = { success: true, entering: true };
      const okExit: PiPToggleResult = { success: true, entering: false };
      const fail: PiPToggleResult = { success: false, reason: "no-video" };

      expect(okEnter.entering).toBe(true);
      expect(okExit.entering).toBe(false);
      expect(fail.reason).toBe("no-video");
    });
  });
});
