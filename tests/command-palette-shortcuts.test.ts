import { createElement } from "react";
import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, it, vi } from "vitest";

import type { BrowserController } from "../src/renderer/app/controller/types";
import { CommandPalette } from "../src/renderer/surfaces/command/CommandPalette";

describe("CommandPalette shortcut hints", () => {
  it("renders shortcut badges beside command action hints", () => {
    const html = renderToStaticMarkup(createElement(CommandPalette, {
      controller: {
        actions: {
          openUrlInActiveWorkspace: vi.fn(),
          openUrlInSplit: vi.fn()
        },
        commandQuery: "",
        commands: [{
          title: "Enter compact mode",
          subtitle: "Hide toolbar and float browser chrome on hover",
          shortcut: "Ctrl/Cmd+Alt+C",
          run: vi.fn(),
          runInSplit: vi.fn()
        }],
        setCommandOpen: vi.fn(),
        setCommandQuery: vi.fn()
      } as unknown as BrowserController
    }));

    expect(html).toContain('class="command-shortcut"');
    expect(html).toContain("Ctrl/Cmd+Alt+C");
    expect(html).toContain('class="command-action-hint is-split"');
  });
});
