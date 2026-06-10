import { createElement } from "react";
import { renderToString } from "react-dom/server";
import { describe, expect, it, vi } from "vitest";

import { PermissionPrompt } from "../src/renderer/surfaces/permissions/PermissionPrompt";
import type { BrowserController } from "../src/renderer/app/controller/types";
import {
  clearSitePermission,
  clearSitePermissionsForOrigin,
  getOriginFromUrl,
  getPermissionLabel,
  getPermissionRule,
  normalizeSitePermissions,
  upsertSitePermission
} from "../src/renderer/domain/permissions/sitePermissions";

describe("sitePermissions", () => {
  it("extracts http origins only", () => {
    expect(getOriginFromUrl("https://example.com/path?q=1")).toBe("https://example.com");
    expect(getOriginFromUrl("http://example.com:8080/page")).toBe("http://example.com:8080");
    expect(getOriginFromUrl("file:///tmp/page.html")).toBeNull();
    expect(getOriginFromUrl("not a url")).toBeNull();
  });

  it("upserts and clears permission rules", () => {
    vi.useFakeTimers();
    vi.setSystemTime(1_000);
    const allowed = upsertSitePermission([], "personal", "https://example.com", "geolocation", "allow");
    vi.setSystemTime(2_000);
    const blocked = upsertSitePermission(allowed, "personal", "https://example.com", "geolocation", "block");
    const workAllowed = upsertSitePermission(blocked, "work", "https://example.com", "geolocation", "allow");

    expect(blocked).toHaveLength(1);
    expect(workAllowed).toHaveLength(2);
    expect(getPermissionRule(workAllowed, "personal", "https://example.com", "geolocation")?.decision).toBe("block");
    expect(getPermissionRule(workAllowed, "work", "https://example.com", "geolocation")?.decision).toBe("allow");
    expect(clearSitePermission(workAllowed, "personal", "https://example.com", "geolocation")).toHaveLength(1);
    vi.useRealTimers();
  });

  it("clears all rules for one profile-scoped origin", () => {
    const rules = [
      { profileId: "personal", origin: "https://example.com", permission: "media", decision: "allow" as const, updatedAt: 1 },
      { profileId: "personal", origin: "https://example.com", permission: "geolocation", decision: "block" as const, updatedAt: 2 },
      { profileId: "work", origin: "https://example.com", permission: "media", decision: "allow" as const, updatedAt: 3 },
      { profileId: "personal", origin: "https://other.example", permission: "media", decision: "block" as const, updatedAt: 4 }
    ];

    expect(clearSitePermissionsForOrigin(rules, "personal", "https://example.com")).toEqual([
      { profileId: "work", origin: "https://example.com", permission: "media", decision: "allow", updatedAt: 3 },
      { profileId: "personal", origin: "https://other.example", permission: "media", decision: "block", updatedAt: 4 }
    ]);
  });

  it("normalizes and de-duplicates persisted rules", () => {
    const rules = normalizeSitePermissions([
      { profileId: "personal", origin: "https://example.com/path", permission: "media", decision: "allow", updatedAt: 1 },
      { profileId: "personal", origin: "https://example.com", permission: "media", decision: "block", updatedAt: 2 },
      { profileId: "work", origin: "https://example.com", permission: "media", decision: "allow", updatedAt: 3 },
      { origin: "file:///tmp/a", permission: "media", decision: "allow" },
      { origin: "https://example.com", permission: "bad" }
    ]);

    expect(rules).toEqual([
      { profileId: "work", origin: "https://example.com", permission: "media", decision: "allow", updatedAt: 3 },
      { profileId: "personal", origin: "https://example.com", permission: "media", decision: "block", updatedAt: 2 }
    ]);
  });

  it("formats known and unknown permission names", () => {
    expect(getPermissionLabel("media")).toBe("Camera and microphone");
    expect(getPermissionLabel("midiSysex")).toBe("MIDI devices");
    expect(getPermissionLabel("displayCapture")).toBe("Display Capture");
  });

  it("renders the permission prompt with origin + permission and invokes actions", () => {
    const resolve = vi.fn();
    const controller = {
      permissionRequest: {
        id: "req-1",
        origin: "https://example.com",
        partition: "persist:personal",
        permission: "media",
        profileId: "personal"
      },
      actions: { resolvePermissionRequest: resolve }
    } as unknown as BrowserController;
    const html = renderToString(createElement(PermissionPrompt, { controller }));
    expect(html).toContain("permission-prompt");
    expect(html).toContain("https://example.com");
    expect(html.toLowerCase()).toContain("camera and microphone");
    expect(html).toContain("Block");
    expect(html).toContain("Allow");

    /* Confirm prompt with no pending request returns null. */
    const emptyHtml = renderToString(
      createElement(PermissionPrompt, {
        controller: { ...controller, permissionRequest: null } as unknown as BrowserController
      })
    );
    expect(emptyHtml).toBe("");
  });
});
