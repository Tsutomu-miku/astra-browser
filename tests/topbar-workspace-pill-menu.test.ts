import { createElement } from "react";
import { renderToStaticMarkup } from "react-dom/server";
import { readFileSync } from "node:fs";
import { join } from "node:path";
import { describe, expect, it, vi } from "vitest";

import { WorkspacePillContextMenu } from "../src/renderer/surfaces/topbar/components/WorkspacePillContextMenu";

const topbarCss = readFileSync(join(__dirname, "../src/renderer/styles/topbar.css"), "utf8");

describe("workspace pill context menu", () => {
  it("renders Space actions for the topbar workspace pill", () => {
    const html = renderToStaticMarkup(createElement(WorkspacePillContextMenu, {
      canDelete: true,
      left: 10,
      top: 20,
      workspaceName: "Personal",
      onClose: vi.fn(),
      onDeleteWorkspace: vi.fn(),
      onNewWorkspace: vi.fn(),
      onOpenSettings: vi.fn()
    }));

    expect(html).toContain('role="menu"');
    expect(html).toContain("Space settings");
    expect(html).toContain("New Space");
    expect(html).toContain("Delete Personal");
  });

  it("disables delete when the current Space is the only Space", () => {
    const html = renderToStaticMarkup(createElement(WorkspacePillContextMenu, {
      canDelete: false,
      left: 10,
      top: 20,
      workspaceName: "Personal",
      onClose: vi.fn(),
      onDeleteWorkspace: vi.fn(),
      onNewWorkspace: vi.fn(),
      onOpenSettings: vi.fn()
    }));

    expect(html).toContain("disabled");
    expect(html).toContain("Delete Personal");
  });

  it("styles the workspace pill context menu", () => {
    expect(topbarCss).toContain(".workspace-pill-context-menu");
    expect(topbarCss).toContain(".workspace-pill-context-menu button.danger");
    expect(topbarCss).toContain(".workspace-pill-context-menu button:disabled");
  });
});
