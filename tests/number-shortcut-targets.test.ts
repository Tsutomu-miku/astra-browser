import { describe, expect, it } from "vitest";

import { getNumberShortcutTarget } from "../src/renderer/common/shortcuts/numberShortcutTargets";
import { createFavorite, createTab, type Workspace } from "../src/renderer/domain/browser-core";

function workspaceWithTabs(tabs: Workspace["tabs"]): Pick<Workspace, "tabs"> {
  return { tabs };
}

describe("getNumberShortcutTarget", () => {
  it("orders Essentials before pinned tabs and regular tabs", () => {
    const essential = createFavorite("Mail", "https://mail.example");
    const regular = createTab("Docs", "https://docs.example");
    const pinned = { ...createTab("Calendar", "https://calendar.example"), isPinned: true };
    const secondRegular = createTab("News", "https://news.example");

    const workspace = workspaceWithTabs([regular, pinned, secondRegular]);

    expect(getNumberShortcutTarget([essential], workspace, 0)).toEqual({
      type: "essential",
      title: essential.title,
      url: essential.url
    });
    expect(getNumberShortcutTarget([essential], workspace, 1)).toEqual({ type: "tab", tabId: pinned.id });
    expect(getNumberShortcutTarget([essential], workspace, 2)).toEqual({ type: "tab", tabId: regular.id });
    expect(getNumberShortcutTarget([essential], workspace, 3)).toEqual({ type: "tab", tabId: secondRegular.id });
  });

  it("returns null outside the available shortcut targets", () => {
    const workspace = workspaceWithTabs([createTab("Docs", "https://docs.example")]);

    expect(getNumberShortcutTarget([], workspace, -1)).toBeNull();
    expect(getNumberShortcutTarget([], workspace, 1)).toBeNull();
  });
});
