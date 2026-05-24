import { describe, expect, it, vi } from "vitest";

import {
  clearSitePermission,
  getOriginFromUrl,
  getPermissionLabel,
  getPermissionRule,
  normalizeSitePermissions,
  upsertSitePermission
} from "../src/renderer/domain/sitePermissions";

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
});
