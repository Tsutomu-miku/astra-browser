import { describe, expect, it } from "vitest";

import { createTab } from "../src/renderer/domain/browser";
import {
  getSidebarTabDropIntent,
  getSidebarTabsAreaDropIntent
} from "../src/renderer/surfaces/sidebar/model/sidebarTabDropIntent";

describe("sidebar tab drop intent", () => {
  it("unpins pinned tabs when they are dropped into the regular tab list", () => {
    const pinned = { ...createTab("Mail", "https://mail.example"), isPinned: true };
    const regular = createTab("Docs", "https://docs.example");

    expect(getSidebarTabDropIntent(pinned, regular)).toEqual({ type: "unpinToRegularPosition" });
  });

  it("keeps ordinary tab drops as reorders", () => {
    const regular = createTab("Docs", "https://docs.example");
    const target = createTab("MDN", "https://developer.mozilla.org");
    const pinned = { ...createTab("Mail", "https://mail.example"), isPinned: true };

    expect(getSidebarTabDropIntent(regular, target)).toEqual({ type: "reorder" });
    expect(getSidebarTabDropIntent(pinned, pinned)).toEqual({ type: "reorder" });
    expect(getSidebarTabDropIntent(undefined, target)).toEqual({ type: "reorder" });
  });

  it("unpins pinned tabs dropped onto the tabs area", () => {
    const pinned = { ...createTab("Mail", "https://mail.example"), isPinned: true };
    const regular = createTab("Docs", "https://docs.example");

    expect(getSidebarTabsAreaDropIntent(pinned)).toEqual({ type: "unpinToRegularEnd" });
    expect(getSidebarTabsAreaDropIntent(regular)).toEqual({ type: "reorder" });
    expect(getSidebarTabsAreaDropIntent(undefined)).toEqual({ type: "reorder" });
  });
});
