import { describe, expect, it, vi } from "vitest";

import { getCommandPresentation } from "../src/renderer/surfaces/command/model/commandPresentation";

describe("getCommandPresentation", () => {
  it("classifies content and tab command surfaces for quick scanning", () => {
    expect(getCommandPresentation({
      title: "Docs",
      subtitle: "Essential · https://docs.example/",
      run: vi.fn()
    })).toEqual({ icon: "content", label: "Essential" });
    expect(getCommandPresentation({
      title: "Example",
      subtitle: "Sleeping tab · https://example.com/",
      run: vi.fn()
    })).toEqual({ icon: "memory", label: "Sleeping" });
    expect(getCommandPresentation({
      title: "Reopen Example",
      subtitle: "Recently closed · https://example.com/",
      run: vi.fn()
    })).toEqual({ icon: "closed", label: "Closed" });
    expect(getCommandPresentation({
      title: "New tab",
      subtitle: "Open homepage in this workspace",
      run: vi.fn()
    })).toEqual({ icon: "tab", label: "Tab" });
  });

  it("classifies split, Space, Memory Saver, and search commands", () => {
    expect(getCommandPresentation({
      title: "Split layout grid",
      subtitle: "Arrange split tabs in a grid",
      run: vi.fn()
    })).toEqual({ icon: "split", label: "Split" });
    expect(getCommandPresentation({
      title: "Switch to Work",
      subtitle: "Workspace",
      run: vi.fn()
    })).toEqual({ icon: "space", label: "Space" });
    expect(getCommandPresentation({
      title: "Disable Memory Saver",
      subtitle: "Auto-sleep idle tabs after 30 minutes",
      run: vi.fn()
    })).toEqual({ icon: "memory", label: "Memory" });
    expect(getCommandPresentation({
      title: "Search zen browser",
      subtitle: "Search with selected engine",
      run: vi.fn()
    })).toEqual({ icon: "search", label: "Search" });
  });
});
