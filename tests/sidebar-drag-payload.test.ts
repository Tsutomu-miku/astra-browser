import { describe, expect, it, vi } from "vitest";

import {
  readSidebarTabDragData,
  readSidebarTabDragPayload,
  SIDEBAR_TAB_DRAG_TYPE,
  writeSidebarTabDragPayload
} from "../src/renderer/common/drag-drop/sidebarDragPayload";

describe("sidebar drag payload", () => {
  it("writes and reads the explicit sidebar tab payload before falling back to plain text", () => {
    const data = new Map<string, string>();
    const dataTransfer = {
      effectAllowed: "all",
      getData: vi.fn((type: string) => data.get(type) ?? ""),
      setData: vi.fn((type: string, value: string) => data.set(type, value))
    } as unknown as DataTransfer;

    writeSidebarTabDragPayload(dataTransfer, "tab-1");

    expect(data.get(SIDEBAR_TAB_DRAG_TYPE)).toBe("tab-1");
    expect(data.get("text/plain")).toBe("tab-1");
    expect(readSidebarTabDragPayload(dataTransfer)).toBe("tab-1");
  });

  it("keeps compatibility with plain-text tab drags", () => {
    expect(readSidebarTabDragData((type) => type === "text/plain" ? "tab-legacy" : "")).toBe("tab-legacy");
  });
});
