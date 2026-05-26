import { describe, expect, it } from "vitest";

import { getSidebarSearchOpenIntent } from "../src/renderer/surfaces/sidebar/sidebarOpenIntent";
import type { SidebarSearchTarget } from "../src/renderer/surfaces/sidebar/sidebarFiltering";

describe("getSidebarSearchOpenIntent", () => {
  const tabTarget: SidebarSearchTarget = {
    id: "tab-1",
    title: "Docs",
    type: "tab",
    url: "https://docs.example"
  };
  const favoriteTarget: SidebarSearchTarget = {
    id: "favorite-1",
    title: "Mail",
    type: "favorite",
    url: "https://mail.example"
  };

  it("opens tabs, previews with Alt, and sends tabs to split with Shift", () => {
    expect(getSidebarSearchOpenIntent(tabTarget, { altKey: false, shiftKey: false })).toEqual({
      tabId: "tab-1",
      type: "selectTab"
    });
    expect(getSidebarSearchOpenIntent(tabTarget, { altKey: true, shiftKey: false })).toEqual({
      title: "Docs",
      type: "preview",
      url: "https://docs.example"
    });
    expect(getSidebarSearchOpenIntent(tabTarget, { altKey: false, shiftKey: true })).toEqual({
      tabId: "tab-1",
      type: "splitTab"
    });
  });

  it("opens url-backed targets or sends them to split with Shift", () => {
    expect(getSidebarSearchOpenIntent(favoriteTarget, { altKey: false, shiftKey: false })).toEqual({
      title: "Mail",
      type: "openUrl",
      url: "https://mail.example"
    });
    expect(getSidebarSearchOpenIntent(favoriteTarget, { altKey: false, shiftKey: true })).toEqual({
      title: "Mail",
      type: "splitUrl",
      url: "https://mail.example"
    });
  });
});
