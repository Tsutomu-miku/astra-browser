import { describe, expect, it } from "vitest";

import { createDefaultState, getWorkspacePartition } from "../src/renderer/domain/browser-core";
import { createFallbackEntries, createStorageEntries } from "../src/renderer/app/controller/useProfileStorageUsage";

describe("profile storage usage", () => {
  it("maps Electron partition usage back to workspaces", () => {
    const state = createDefaultState();
    const personal = state.workspaces[0];
    const entries = createStorageEntries(state.workspaces, [{
      partition: getWorkspacePartition(personal),
      cacheBytes: 512,
      storageBytes: 1024,
      storagePath: "/tmp/profile"
    }]);

    expect(entries[0]).toMatchObject({
      workspaceId: personal.id,
      profileId: personal.profileId,
      cacheBytes: 512,
      storageBytes: 1024,
      totalBytes: 1536,
      storagePath: "/tmp/profile"
    });
    expect(entries[1].totalBytes).toBe(0);
  });

  it("creates fallback entries when Electron runtime APIs are unavailable", () => {
    const state = createDefaultState();
    const entries = createFallbackEntries(state.workspaces);

    expect(entries).toHaveLength(state.workspaces.length);
    expect(entries.every((entry) => entry.totalBytes === 0)).toBe(true);
    expect(entries[0].partition).toBe(getWorkspacePartition(state.workspaces[0]));
  });
});
