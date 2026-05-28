import { createElement } from "react";
import { renderToStaticMarkup } from "react-dom/server";
import { readFileSync } from "node:fs";
import { join } from "node:path";
import { describe, expect, it, vi } from "vitest";

import type { SitePermissionRule } from "../src/renderer/domain/browser";
import { PermissionList } from "../src/renderer/surfaces/panels/site/components/PermissionList";
import { SiteSummary } from "../src/renderer/surfaces/panels/site/components/SiteSummary";
import { getSitePermissionSummary } from "../src/renderer/surfaces/panels/model/siteInfoState";

const panelsCss = readFileSync(join(__dirname, "../src/renderer/styles/panels.css"), "utf8");
const rule: SitePermissionRule = {
  decision: "allow",
  origin: "https://example.com",
  permission: "media",
  profileId: "personal",
  updatedAt: 1
};

describe("site information panel", () => {
  it("renders the site summary with a copy origin action", () => {
    const html = renderToStaticMarkup(createElement(SiteSummary, {
      identityLabel: "Secure",
      origin: "https://example.com",
      permissionSummary: getSitePermissionSummary([rule], "personal", "https://example.com"),
      profileName: "Personal",
      security: "secure",
      url: "https://example.com/docs",
      onCopyOrigin: vi.fn(),
      onResetPermissions: vi.fn()
    }));

    expect(html).toContain("Copy origin");
    expect(html).toContain("https://example.com");
    expect(html).toContain("1 allowed");
  });

  it("renders origin-scoped permission controls", () => {
    const html = renderToStaticMarkup(createElement(PermissionList, {
      origin: "https://example.com",
      profileId: "personal",
      rules: [rule],
      onClear: vi.fn(),
      onSet: vi.fn()
    }));

    expect(html).toContain('aria-label="Site permissions"');
    expect(html).toContain("Camera and microphone");
    expect(html).toContain('aria-pressed="true"');
  });

  it("styles the site summary action", () => {
    expect(panelsCss).toContain(".site-summary-actions");
    expect(panelsCss).toContain(".site-summary-action");
  });
});
