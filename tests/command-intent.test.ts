import { describe, expect, it, vi } from "vitest";

import { getCommandActionHints, getCommandRunner } from "../src/renderer/surfaces/command/model/commandIntent";
import type { Command } from "../src/renderer/surfaces/command/model/commandTypes";

describe("getCommandRunner", () => {
  it("prefers preview, split, then normal command actions", () => {
    const run = vi.fn();
    const runInSplit = vi.fn();
    const runPreview = vi.fn();
    const command: Command = {
      run,
      runInSplit,
      runPreview,
      subtitle: "Content",
      title: "Docs"
    };

    getCommandRunner(command, { altKey: true, shiftKey: true })();
    getCommandRunner(command, { altKey: false, shiftKey: true })();
    getCommandRunner(command, { altKey: false, shiftKey: false })();

    expect(runPreview).toHaveBeenCalledTimes(1);
    expect(runInSplit).toHaveBeenCalledTimes(1);
    expect(run).toHaveBeenCalledTimes(1);
  });

  it("falls back to the normal action when an alternate action is unavailable", () => {
    const run = vi.fn();
    const command: Command = {
      run,
      subtitle: "Application",
      title: "Settings"
    };

    getCommandRunner(command, { altKey: true, shiftKey: false })();
    getCommandRunner(command, { altKey: false, shiftKey: true })();

    expect(run).toHaveBeenCalledTimes(2);
  });

  it("describes available command alternate actions", () => {
    expect(getCommandActionHints({
      run: vi.fn(),
      runInSplit: vi.fn(),
      runPreview: vi.fn(),
      subtitle: "Content",
      title: "Docs"
    })).toEqual([
      { id: "preview", label: "Preview", modifier: "Alt" },
      { id: "split", label: "Split", modifier: "Shift" }
    ]);
    expect(getCommandActionHints({
      run: vi.fn(),
      subtitle: "Application",
      title: "Settings"
    })).toEqual([]);
  });
});
