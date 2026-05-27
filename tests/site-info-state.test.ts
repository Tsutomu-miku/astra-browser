import { describe, expect, it } from "vitest";

import {
  getSecurityDescription,
  getSitePermissionSummary
} from "../src/renderer/surfaces/panels/model/siteInfoState";

describe("siteInfoState", () => {
  it("summarizes profile-scoped permission decisions for a site", () => {
    const summary = getSitePermissionSummary([
      { profileId: "personal", origin: "https://example.com", permission: "media", decision: "allow", updatedAt: 1 },
      { profileId: "personal", origin: "https://example.com", permission: "geolocation", decision: "block", updatedAt: 2 },
      { profileId: "work", origin: "https://example.com", permission: "notifications", decision: "allow", updatedAt: 3 },
      { profileId: "personal", origin: "https://other.example", permission: "media", decision: "block", updatedAt: 4 }
    ], "personal", "https://example.com");

    expect(summary).toEqual({
      allowedCount: 1,
      blockedCount: 1,
      label: "1 allowed, 1 blocked",
      totalCount: 2
    });
  });

  it("reports the empty permission summary", () => {
    expect(getSitePermissionSummary([], "personal", "https://example.com").label).toBe("No custom permissions");
    expect(getSitePermissionSummary([], "personal", null).totalCount).toBe(0);
  });

  it("describes security states for panel summary rows", () => {
    expect(getSecurityDescription("secure")).toBe("Connection uses HTTPS.");
    expect(getSecurityDescription("insecure")).toBe("Connection is not encrypted.");
    expect(getSecurityDescription("internal")).toBe("Astra internal page.");
    expect(getSecurityDescription("unknown")).toBe("Security is unavailable for this entry.");
  });
});
