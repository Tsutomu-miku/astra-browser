import { describe, expect, it } from "vitest";

import type { DownloadEntry } from "../src/renderer/domain/browser";
import { getDownloadActionsState } from "../src/renderer/surfaces/panels/downloads/model/downloadActions";

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
      progress: 100
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
});
