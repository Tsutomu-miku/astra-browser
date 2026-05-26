import { describe, expect, it, vi } from "vitest";

import { getVisibleCommands } from "../src/renderer/hooks/commandSearch";
import type { Command } from "../src/renderer/hooks/useCommands";

const commands: Command[] = [
  {
    title: "New tab",
    subtitle: "Open homepage",
    run: vi.fn()
  },
  {
    title: "History Example",
    subtitle: "History · https://history.example/",
    run: vi.fn()
  }
];

describe("getVisibleCommands", () => {
  it("returns normal commands for an empty query", () => {
    expect(getVisibleCommands(commands, "", vi.fn())).toEqual(commands);
  });

  it("adds a search command ahead of filtered commands", () => {
    const visible = getVisibleCommands(commands, "zen browser", vi.fn());

    expect(visible[0].title).toBe("Search zen browser");
    expect(visible[0].subtitle).toBe("Search with selected engine");
    expect(visible).toHaveLength(1);
  });

  it("adds an open-address command for URL-shaped input", () => {
    const openQuery = vi.fn();
    const visible = getVisibleCommands(commands, "example.com", openQuery);
    visible[0].run();

    expect(visible[0].title).toBe("Open example.com");
    expect(visible[0].subtitle).toBe("Open address");
    expect(openQuery).toHaveBeenCalledWith("example.com");
  });

  it("keeps fuzzy app command matches after the query command", () => {
    const visible = getVisibleCommands(commands, "history", vi.fn());

    expect(visible[0].title).toBe("Search history");
    expect(visible[1].title).toBe("History Example");
  });

  it("prioritizes exact app command matches over text search", () => {
    const visible = getVisibleCommands(commands, "History Example", vi.fn());

    expect(visible[0].title).toBe("History Example");
    expect(visible[1].title).toBe("Search History Example");
  });
});
