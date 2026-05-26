import { describe, expect, it } from "vitest";

import { getGlanceNavigationState } from "../src/renderer/surfaces/glance/glanceNavigation";
import type { WebviewElement } from "../src/renderer/types/browser-ui";

describe("getGlanceNavigationState", () => {
  it("reads optional webview history capabilities safely", () => {
    expect(getGlanceNavigationState(null)).toEqual({
      canGoBack: false,
      canGoForward: false
    });

    expect(getGlanceNavigationState({
      canGoBack: () => true,
      canGoForward: () => false
    } as WebviewElement)).toEqual({
      canGoBack: true,
      canGoForward: false
    });
  });
});
