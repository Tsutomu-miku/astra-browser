import { describe, expect, it } from "vitest";

import { getTabOrganizationTargetKeyboardIntent } from "../src/renderer/surfaces/sidebar/model/tabOrganizationTargetKeyboard";

describe("tab organization target keyboard intent", () => {
  it("activates drop targets with Enter and Space", () => {
    expect(getTabOrganizationTargetKeyboardIntent("Enter")).toBe("activate");
    expect(getTabOrganizationTargetKeyboardIntent(" ")).toBe("activate");
  });

  it("cancels tab organization with Escape", () => {
    expect(getTabOrganizationTargetKeyboardIntent("Escape")).toBe("cancel");
  });

  it("ignores unrelated keys", () => {
    expect(getTabOrganizationTargetKeyboardIntent("ArrowDown")).toBeNull();
    expect(getTabOrganizationTargetKeyboardIntent("a")).toBeNull();
  });
});
