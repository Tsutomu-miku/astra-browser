import { describe, expect, it } from "vitest";

import {
  addWorkspace,
  updateWorkspace
} from "../src/renderer/domain/browser-actions";
import {
  createDefaultState,
  getBrowserPartitions,
  getWorkspacePartition,
  normalizeAddress,
  normalizeState
} from "../src/renderer/domain/browser-core";
import { getActiveWorkspace } from "../src/renderer/domain/selectors";

describe("workspace profiles", () => {
  it("normalizes persisted workspaces with profile identity", () => {
    const state = normalizeState({
      activeWorkspaceId: "space",
      workspaces: [
        {
          id: "space",
          name: "Client Work",
          profileName: "Client Profile",
          profileId: "Client Profile",
          tabs: []
        }
      ]
    });
    const workspace = getActiveWorkspace(state);

    expect(workspace.profileName).toBe("Client Profile");
    expect(workspace.profileId).toBe("client-profile");
    expect(getWorkspacePartition(workspace)).toBe("persist:astra-client-profile");
  });

  it("creates new workspaces with a stable Chromium profile", () => {
    const initial = createDefaultState();
    initial.settings.homepage = "https://team.example/";
    const state = addWorkspace(initial);
    const workspace = getActiveWorkspace(state);

    expect(workspace.profileName).toBe(workspace.name);
    expect(workspace.profileId).toBeTruthy();
    expect(workspace.homepage).toBe("https://team.example/");
    expect(getWorkspacePartition(workspace)).toMatch(/^persist:astra-/);
  });

  it("updates the active workspace homepage without changing global homepage", () => {
    const state = createDefaultState();
    const updated = updateWorkspace(state, { homepage: "research.example" });
    const workspace = getActiveWorkspace(updated);

    expect(workspace.homepage).toBe("https://research.example/");
    expect(updated.settings.homepage).toBe(normalizeAddress(state.settings.homepage));
  });

  it("updates the active workspace profile display name without changing its partition", () => {
    const state = createDefaultState();
    const before = getWorkspacePartition(getActiveWorkspace(state));
    const updated = updateWorkspace(state, { profileName: "Research" });
    const workspace = getActiveWorkspace(updated);

    expect(workspace.profileName).toBe("Research");
    expect(getWorkspacePartition(workspace)).toBe(before);
  });

  it("collects unique Chromium partitions for browser-data clearing", () => {
    const state = createDefaultState();
    state.workspaces[1].profileId = state.workspaces[0].profileId;

    expect(getBrowserPartitions(state)).toEqual(["persist:astra-personal"]);
  });
});
