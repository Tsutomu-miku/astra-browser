import { describe, expect, it } from "vitest";

import {
  buildMemoryBreakdown,
  estimateTabMemoryMb,
  formatBytes,
  type TabMemoryEstimate,
  type WorkspaceMemoryEstimate
} from "../src/renderer/common/memory/memoryUsage";
import { createDefaultState } from "../src/renderer/domain/browser";
import type { BrowserTab } from "../src/renderer/domain/browser";

function makeTab(overrides: Partial<BrowserTab> = {}): BrowserTab {
  return {
    canGoBack: true,
    canGoForward: true,
    faviconUrl: undefined,
    groupId: null,
    hasUnread: false,
    id: `tab-${Math.random().toString(36).slice(2, 8)}`,
    isCameraOn: false,
    isFavorite: false,
    isLoading: false,
    isMediaPlaying: false,
    isMicrophoneOn: false,
    isMuted: false,
    isPinned: false,
    isSleeping: false,
    lastActiveAt: Date.now(),
    title: "Example",
    url: "https://example.com",
    zoomFactor: 1,
    ...overrides
  };
}

describe("memory usage helpers", () => {
  it("estimates tab memory by state and activity", () => {
    const sleeping = estimateTabMemoryMb(makeTab({ isSleeping: true }));
    const live = estimateTabMemoryMb(makeTab({ isSleeping: false }));
    const media = estimateTabMemoryMb(makeTab({ isMediaPlaying: true }));
    const camera = estimateTabMemoryMb(makeTab({ isCameraOn: true }));

    expect(sleeping).toBe(2);
    expect(live).toBeGreaterThan(sleeping);
    expect(media).toBeGreaterThan(live);
    expect(camera).toBeGreaterThan(live);
  });

  it("builds a per-workspace memory breakdown from tab counts", () => {
    const state = createDefaultState();
    state.workspaces[0].tabs = [
      makeTab({ id: "t1", isSleeping: false, title: "Active" }),
      makeTab({ id: "t2", isSleeping: true, title: "Sleeping" }),
      makeTab({ id: "t3", isSleeping: false, isMediaPlaying: true, title: "Media" })
    ];
    state.workspaces[1].tabs = [makeTab({ id: "t4", isSleeping: false })];

    const cacheByPartition = new Map<string, number>([
      [`persist:${state.workspaces[0].profileId}`, 1024 * 1024]
    ]);
    const breakdown = buildMemoryBreakdown(state.workspaces, cacheByPartition, null);

    expect(breakdown.perWorkspace).toHaveLength(2);
    expect(breakdown.totalBytes).toBeGreaterThan(0);
    expect(breakdown.perWorkspace[0].tabs).toHaveLength(3);
    expect(breakdown.perWorkspace[0].tabs.every((row: TabMemoryEstimate) => row.estimatedBytes > 0)).toBe(true);
    expect(breakdown.perWorkspace[0].cacheBytes).toBe(1024 * 1024);
    expect(breakdown.perWorkspace[1].cacheBytes).toBe(0);
    expect(breakdown.browserBytes + breakdown.webviewBytes).toBe(breakdown.totalBytes);
  });

  it("sorts workspace tabs by heaviest footprint first when rendered", () => {
    const state = createDefaultState();
    state.workspaces[0].tabs = [
      makeTab({ id: "light", isSleeping: true, title: "Sleeping tab" }),
      makeTab({ id: "heavy", isSleeping: false, isMediaPlaying: true, title: "Heavy tab" }),
      makeTab({ id: "mid", isSleeping: false, title: "Mid tab" })
    ];

    const breakdown = buildMemoryBreakdown(state.workspaces, new Map(), null);
    const sorted = [...breakdown.perWorkspace[0].tabs].sort(
      (a, b) => b.estimatedBytes - a.estimatedBytes
    );

    expect(sorted[0].tab.id).toBe("heavy");
    expect(sorted[sorted.length - 1].tab.id).toBe("light");
  });

  it("includes measured process memory when a snapshot is available", () => {
    const state = createDefaultState();
    const snapshot = {
      appHeapBytes: 42 * 1024 * 1024,
      appRssBytes: 128 * 1024 * 1024,
      sampledAt: Date.now(),
      totalBytes: 512 * 1024 * 1024,
      webviewCount: 8,
      webviewWorkingSetBytes: 384 * 1024 * 1024
    };
    const breakdown = buildMemoryBreakdown(state.workspaces, new Map(), snapshot);

    expect(breakdown.processSnapshot).toBe(snapshot);
    expect(breakdown.totalBytes).toBe(snapshot.totalBytes);
    expect(breakdown.browserBytes).toBe(snapshot.appRssBytes);
    expect(breakdown.webviewBytes).toBe(snapshot.webviewWorkingSetBytes);
  });

  it("formats bytes in human-readable units", () => {
    expect(formatBytes(0)).toBe("0 B");
    expect(formatBytes(512)).toBe("512 B");
    expect(formatBytes(2048)).toBe("2 KB");
    expect(formatBytes(5 * 1024 * 1024)).toBe("5.0 MB");
    expect(formatBytes(2 * 1024 * 1024 * 1024)).toBe("2.00 GB");
  });

  it("survives empty workspace lists without crashing", () => {
    const breakdown = buildMemoryBreakdown([], new Map(), null);
    expect(breakdown.perWorkspace).toEqual([]);
    expect(breakdown.totalBytes).toBe(0);
    expect(breakdown.browserBytes).toBe(0);
    expect(breakdown.webviewBytes).toBe(0);
  });

  it("allocates per-workspace breakdowns independently", () => {
    const state = createDefaultState();
    const breakdown = buildMemoryBreakdown(state.workspaces, new Map(), null);

    const totals = breakdown.perWorkspace.map((row: WorkspaceMemoryEstimate) => row.totalBytes);
    expect(totals).not.toContain(0);
    expect(new Set(totals).size).toBeGreaterThanOrEqual(1);
  });
});
