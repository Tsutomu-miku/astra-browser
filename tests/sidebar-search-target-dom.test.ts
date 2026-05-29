import { describe, expect, it, vi } from "vitest";

import { scrollSidebarSearchTargetIntoView } from "../src/renderer/surfaces/sidebar/model/sidebarSearchTargetDom";

describe("sidebar search target DOM helpers", () => {
  it("scrolls the active search target into view", () => {
    const scrollIntoView = vi.fn();
    const element = document.createElement("button");
    element.scrollIntoView = scrollIntoView;
    const root = {
      getElementById: vi.fn(() => element)
    };

    expect(scrollSidebarSearchTargetIntoView({
      type: "tab",
      id: "docs",
      title: "Docs",
      url: "https://docs.example"
    }, root)).toBe(true);

    expect(root.getElementById).toHaveBeenCalledWith("sidebar-search-tab-docs");
    expect(scrollIntoView).toHaveBeenCalledWith({ block: "nearest" });
  });

  it("does nothing when a search target is not rendered", () => {
    const root = {
      getElementById: vi.fn(() => null)
    };

    expect(scrollSidebarSearchTargetIntoView({
      type: "favorite",
      id: "missing",
      title: "Missing",
      url: "https://missing.example"
    }, root)).toBe(false);
  });
});
