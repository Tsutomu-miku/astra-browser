import { describe, expect, it } from "vitest";

import { createDefaultState } from "../src/renderer/domain/browser-core";
import { createBrowserStateBackup, parseBrowserStateBackup } from "../src/renderer/platform/persistence/browserBackup";

describe("browser backup", () => {
  it("exports and imports normalized browser state", () => {
    const state = createDefaultState();
    state.workspaces[0].name = "Research";
    const imported = parseBrowserStateBackup(createBrowserStateBackup(state));

    expect(imported.workspaces[0].name).toBe("Research");
    expect(imported.activeWorkspaceId).toBe(state.activeWorkspaceId);
    expect(imported.workspaces[0].tabs[0].canGoBack).toBe(false);
  });

  it("accepts raw state-shaped JSON for recovery", () => {
    const imported = parseBrowserStateBackup(JSON.stringify({
      activeWorkspaceId: "missing",
      workspaces: [{
        id: "space",
        name: "Recovered",
        tabs: [{ id: "tab", title: "Docs", url: "docs.example" }]
      }]
    }));

    expect(imported.activeWorkspaceId).toBe("space");
    expect(imported.workspaces[0].tabs[0].url).toBe("https://docs.example/");
  });
});
