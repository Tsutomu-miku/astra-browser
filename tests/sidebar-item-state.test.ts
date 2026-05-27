import { describe, expect, it } from "vitest";

import { isSidebarUrlActive } from "../src/renderer/surfaces/sidebar/model/sidebarItemState";

describe("sidebar item state", () => {
  it("matches equivalent quick item urls for active highlighting", () => {
    expect(isSidebarUrlActive("https://example.com/", "https://example.com")).toBe(true);
    expect(isSidebarUrlActive("https://example.com/docs#intro", "https://example.com/docs")).toBe(true);
  });

  it("keeps different pages distinct", () => {
    expect(isSidebarUrlActive("https://example.com/docs", "https://example.com/blog")).toBe(false);
  });
});
