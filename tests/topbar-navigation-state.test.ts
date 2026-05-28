import { describe, expect, it } from "vitest";

import { getReloadButtonState } from "../src/renderer/common/navigation/reloadButtonState";

describe("reload button state", () => {
  it("uses reload while the active tab is idle", () => {
    expect(getReloadButtonState(false)).toEqual({
      action: "reload",
      label: "Reload"
    });
  });

  it("uses stop while the active tab is loading", () => {
    expect(getReloadButtonState(true)).toEqual({
      action: "stop",
      label: "Stop loading"
    });
  });
});
