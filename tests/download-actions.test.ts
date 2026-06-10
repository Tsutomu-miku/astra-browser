import { describe, expect, it } from "vitest";

import type { DownloadEntry } from "../src/renderer/domain/browser";
import { getDownloadActionsState } from "../src/renderer/surfaces/panels/downloads/model/downloadActions";
import { getDownloadMeta } from "../src/renderer/surfaces/panels/downloads/model/downloadMeta";

function download(overrides: Partial<DownloadEntry>): DownloadEntry {
  return {
    filename: "archive.zip",
    id: "download",
    receivedBytes: 25,
    savePath: "",
    startedAt: 1,
    state: "progressing",
    totalBytes: 100,
    ...overrides
  };
}

describe("getDownloadActionsState", () => {
  it("enables file actions only for completed downloads with a save path", () => {
    expect(getDownloadActionsState(download({
      receivedBytes: 100,
      savePath: "/tmp/archive.zip",
      state: "completed"
    }))).toEqual({
      canOpen: true,
      canShowInFolder: true,
      canPause: false,
      canResume: false,
      canCancel: false,
      canRetry: false,
      canRemove: true,
      progress: 100,
      isPaused: false,
      isTerminal: true
    });

    expect(getDownloadActionsState(download({ state: "completed" }))).toMatchObject({
      canOpen: false,
      canShowInFolder: false
    });
    expect(getDownloadActionsState(download({ state: "progressing" }))).toMatchObject({
      canOpen: false,
      canShowInFolder: false,
      progress: 25
    });
  });

  it("treats completed unknown-size downloads as complete progress", () => {
    expect(getDownloadActionsState(download({
      receivedBytes: 0,
      state: "completed",
      totalBytes: 0
    })).progress).toBe(100);
  });

  it("gates pause control on canPause flag for progressing downloads", () => {
    expect(getDownloadActionsState(download({ state: "progressing", canPause: true }))).toMatchObject({
      canPause: true,
      canResume: false,
      canCancel: true,
      canRemove: false,
      isPaused: false,
      isTerminal: false
    });
    expect(getDownloadActionsState(download({ state: "progressing", canPause: false }))).toMatchObject({
      canPause: false
    });
  });

  it("exposes resume + cancel and zero progress for paused downloads", () => {
    const state = getDownloadActionsState(download({ state: "paused", canPause: true, receivedBytes: 50 }));
    expect(state).toMatchObject({
      canPause: false,
      canResume: true,
      canCancel: true,
      canRemove: false,
      isPaused: true,
      isTerminal: false,
      progress: 0
    });
  });

  it("marks cancelled downloads as terminal and removable", () => {
    expect(getDownloadActionsState(download({ state: "cancelled" }))).toMatchObject({
      canRemove: true,
      isTerminal: true
    });
  });

  it("marks interrupted downloads as terminal, removable, and retryable", () => {
    expect(getDownloadActionsState(download({ state: "interrupted", savePath: "/tmp/bad.zip" }))).toMatchObject({
      canRemove: true,
      canRetry: true,
      isTerminal: true
    });
  });

  it("returns zero progress for unknown-size active downloads", () => {
    expect(getDownloadActionsState(download({ state: "progressing", totalBytes: 0, receivedBytes: 0 })).progress).toBe(0);
  });
});

describe("getDownloadMeta", () => {
  it("shows completed label with size", () => {
    const d = download({ state: "completed", totalBytes: 1024, receivedBytes: 1024 });
    expect(getDownloadMeta(d, 100)).toContain("Completed");
    expect(getDownloadMeta(d, 100)).toContain("KB");
  });

  it("shows Paused label with progress", () => {
    const d = download({ state: "paused", totalBytes: 100, receivedBytes: 50 });
    expect(getDownloadMeta(d, 0)).toContain("Paused");
  });

  it("shows Cancelled label", () => {
    const d = download({ state: "cancelled" });
    expect(getDownloadMeta(d, 25)).toContain("Cancelled");
  });

  it("shows Interrupted label", () => {
    const d = download({ state: "interrupted" });
    expect(getDownloadMeta(d, 50)).toContain("Interrupted");
  });

  it("falls back to Unknown size when totalBytes is 0", () => {
    const d = download({ state: "progressing", totalBytes: 0 });
    expect(getDownloadMeta(d, 0)).toContain("Unknown size");
  });
});
