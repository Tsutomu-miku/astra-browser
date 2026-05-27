import { describe, expect, it } from "vitest";

import { getSecurityGlyph, getUrlIdentity } from "../src/renderer/domain/browser/urlIdentity";

describe("urlIdentity", () => {
  it("marks https pages as secure", () => {
    expect(getUrlIdentity("https://www.example.com/path")).toEqual({
      host: "www.example.com",
      label: "example.com",
      security: "secure"
    });
  });

  it("marks http pages as insecure", () => {
    expect(getUrlIdentity("http://example.com").security).toBe("insecure");
  });

  it("marks non-web protocols as internal", () => {
    expect(getUrlIdentity("file:///tmp/index.html")).toEqual({
      host: "file",
      label: "file",
      security: "internal"
    });
  });

  it("falls back for non-url query text", () => {
    expect(getUrlIdentity("not a url").label).toBe("Search");
    expect(getSecurityGlyph("secure")).toBe("✓");
  });
});
