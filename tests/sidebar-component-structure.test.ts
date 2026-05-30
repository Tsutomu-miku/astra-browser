import { existsSync, readFileSync } from "node:fs";
import { join } from "node:path";
import { describe, expect, it } from "vitest";

const root = join(__dirname, "..");

describe("sidebar component structure", () => {
  it("keeps cross-section sidebar menu chrome in the common component layer", () => {
    const commonMenuItemPath = join(root, "src/renderer/surfaces/sidebar/components/common/SidebarMenuItem.tsx");
    const oldTabsMenuItemPath = join(root, "src/renderer/surfaces/sidebar/components/tabs/SidebarMenuItem.tsx");
    const sidebarMenuCssPath = join(root, "src/renderer/styles/sidebar-menu.css");
    const stylesEntry = readFileSync(join(root, "src/renderer/styles.css"), "utf8");
    const commonMenuItem = readFileSync(commonMenuItemPath, "utf8");
    const workspaceStrip = readFileSync(join(root, "src/renderer/surfaces/sidebar/components/workspaces/WorkspaceStrip.tsx"), "utf8");

    expect(existsSync(commonMenuItemPath)).toBe(true);
    expect(existsSync(oldTabsMenuItemPath)).toBe(false);
    expect(existsSync(sidebarMenuCssPath)).toBe(true);
    expect(stylesEntry).toContain('@import "./styles/sidebar-menu.css";');
    expect(commonMenuItem).toContain("SidebarMenuSeparator");
    expect(commonMenuItem).not.toContain("tab-context-menu-separator");
    expect(workspaceStrip).toContain('from "../common/SidebarMenuItem"');
    expect(workspaceStrip).not.toContain('from "../tabs/SidebarMenuItem"');
    expect(workspaceStrip).not.toContain("tab-context-menu-separator");
  });
});
