import { describe, expect, it } from "vitest";

import { getTabStatusBadges, isSidebarUrlActive } from "../src/renderer/surfaces/sidebar/model/sidebarItemState";

describe("sidebar item state", () => {
  it("matches equivalent quick item urls for active highlighting", () => {
    expect(isSidebarUrlActive("https://example.com/", "https://example.com")).toBe(true);
    expect(isSidebarUrlActive("https://example.com/docs#intro", "https://example.com/docs")).toBe(true);
  });

  it("keeps different pages distinct", () => {
    expect(isSidebarUrlActive("https://example.com/docs", "https://example.com/blog")).toBe(false);
  });

  it("describes visible tab status badges in sidebar order", () => {
    expect(getTabStatusBadges({
      id: "tab",
      isMuted: true,
      isSleeping: true
    }, ["tab"]).map((badge) => badge.id)).toEqual(["split", "muted", "sleeping"]);
  });

  it("omits tab status badges when no state needs attention", () => {
    expect(getTabStatusBadges({
      id: "tab",
      isMuted: false,
      isSleeping: false
    }, [])).toEqual([]);
  });
});
