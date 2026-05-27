import { describe, expect, it } from "vitest";

import { getStartOpenIntent } from "../src/renderer/surfaces/start/startOpenIntent";

describe("getStartOpenIntent", () => {
  it("uses normal, preview, and split modifiers for start page entries", () => {
    expect(getStartOpenIntent("https://docs.example", "Docs", {
      altKey: false,
      shiftKey: false
    })).toEqual({
      title: "Docs",
      type: "open",
      url: "https://docs.example"
    });

    expect(getStartOpenIntent("https://docs.example", "Docs", {
      altKey: true,
      shiftKey: true
    })).toEqual({
      title: "Docs",
      type: "preview",
      url: "https://docs.example"
    });

    expect(getStartOpenIntent("https://docs.example", "Docs", {
      altKey: false,
      shiftKey: true
    })).toEqual({
      title: "Docs",
      type: "split",
      url: "https://docs.example"
    });
  });
});
