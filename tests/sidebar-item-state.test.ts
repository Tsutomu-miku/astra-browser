import { describe, expect, it } from "vitest";

import {
  getSidebarTabAccessibilityLabel,
  getTabStatusBadges,
  isSidebarFavoriteActive,
  isSidebarUrlActive
} from "../src/renderer/surfaces/sidebar/model/sidebarItemState";

describe("sidebar item state", () => {
  it("matches equivalent quick item urls for active highlighting", () => {
    expect(isSidebarUrlActive("https://example.com/", "https://example.com")).toBe(true);
    expect(isSidebarUrlActive("https://example.com/docs#intro", "https://example.com/docs")).toBe(true);
  });

  it("keeps different pages distinct", () => {
    expect(isSidebarUrlActive("https://example.com/docs", "https://example.com/blog")).toBe(false);
  });

  it("uses tab identity before URL fallback for Favorite active state", () => {
    expect(isSidebarFavoriteActive(
      { id: "active-tab", url: "https://docs.example/" },
      { tabId: "favorite-tab", url: "https://docs.example/" }
    )).toBe(false);
    expect(isSidebarFavoriteActive(
      { id: "favorite-tab", url: "https://other.example/" },
      { tabId: "favorite-tab", url: "https://docs.example/" }
    )).toBe(true);
    expect(isSidebarFavoriteActive(
      { id: "active-tab", url: "https://docs.example/" },
      { url: "https://docs.example" }
    )).toBe(true);
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

  it("builds accessible tab labels with active and status state", () => {
    const badges = getTabStatusBadges({
      id: "tab",
      isMuted: true,
      isSleeping: false
    }, ["tab"]);

    expect(getSidebarTabAccessibilityLabel({
      isActive: true,
      kind: "pinned tab",
      statusBadges: badges,
      tab: { title: "Mail", url: "https://mail.example" }
    })).toBe("Mail, active, pinned tab, Split, Muted");
  });
});
