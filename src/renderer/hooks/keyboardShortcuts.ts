export type ShortcutIntent =
  | { type: "closePanels" }
  | { type: "closeTab" }
  | { type: "findMatch"; direction: 1 | -1 }
  | { type: "focusAddress" }
  | { type: "fillSplitGrid" }
  | { type: "newTab" }
  | { type: "openDownloads" }
  | { type: "openFind" }
  | { type: "openCommand" }
  | { type: "openHistory" }
  | { type: "navigateHistory"; direction: 1 | -1 }
  | { type: "reloadPage"; hard: boolean }
  | { type: "restoreClosedTab" }
  | { type: "selectAdjacentWorkspace"; direction: 1 | -1 }
  | { type: "selectAdjacentTab"; direction: 1 | -1 }
  | { type: "selectLastTab" }
  | { type: "selectTabIndex"; index: number }
  | { type: "selectWorkspaceIndex"; index: number }
  | { type: "resetZoom" }
  | { type: "toggleFavorite" }
  | { type: "toggleMute" }
  | { type: "toggleCompactMode" }
  | { type: "toggleFloatingSidebar" }
  | { type: "toggleFloatingToolbar" }
  | { type: "toggleSidebar" }
  | { type: "toggleSplit" }
  | { type: "toggleSplitGrid" }
  | { type: "toggleSplitHorizontal" }
  | { type: "toggleSplitVertical" }
  | { type: "unsplitAll" }
  | { type: "zoomIn" }
  | { type: "zoomOut" };

export interface ShortcutEventLike {
  altKey: boolean;
  ctrlKey: boolean;
  key: string;
  metaKey: boolean;
  shiftKey: boolean;
}

export function resolveShortcut(event: ShortcutEventLike): ShortcutIntent | null {
  const key = event.key.toLowerCase();
  const commandModifier = event.metaKey || event.ctrlKey;

  if (event.key === "Escape") {
    return { type: "closePanels" };
  }

  if (!commandModifier && event.altKey && isDigitKey(key)) {
    if (key === "9") {
      return { type: "selectLastTab" };
    }

    return { type: "selectTabIndex", index: Number(key) - 1 };
  }

  if (!commandModifier && event.altKey && event.key === "ArrowLeft") {
    return { type: "navigateHistory", direction: -1 };
  }

  if (!commandModifier && event.altKey && event.key === "ArrowRight") {
    return { type: "navigateHistory", direction: 1 };
  }

  if (!commandModifier && event.altKey && !event.shiftKey && key === "d") {
    return { type: "focusAddress" };
  }

  if (!commandModifier) {
    return null;
  }

  if (isDigitKey(key)) {
    return { type: "selectWorkspaceIndex", index: Number(key) - 1 };
  }

  if (event.shiftKey && key === "t") {
    return { type: "restoreClosedTab" };
  }

  if (event.shiftKey && key === "y") {
    return { type: "openDownloads" };
  }

  if (key === "tab") {
    return { type: "selectAdjacentTab", direction: event.shiftKey ? -1 : 1 };
  }

  if (key === "r") {
    return { type: "reloadPage", hard: event.shiftKey };
  }

  if (key === "d") {
    return { type: "toggleFavorite" };
  }

  if (!event.shiftKey && key === "m") {
    return { type: "toggleMute" };
  }

  if (key === "[") {
    return { type: "navigateHistory", direction: -1 };
  }

  if (key === "]") {
    return { type: "navigateHistory", direction: 1 };
  }

  if (event.altKey && key === "g") {
    return { type: "toggleSplitGrid" };
  }

  if (event.altKey && key === "h") {
    return { type: "toggleSplitHorizontal" };
  }

  if (event.altKey && key === "q") {
    return { type: "selectAdjacentWorkspace", direction: -1 };
  }

  if (event.altKey && key === "e") {
    return { type: "selectAdjacentWorkspace", direction: 1 };
  }

  if (event.altKey && key === "v") {
    return { type: "toggleSplitVertical" };
  }

  if (event.altKey && key === "u") {
    return { type: "unsplitAll" };
  }

  if (event.altKey && key === "s") {
    return { type: "toggleFloatingSidebar" };
  }

  if (event.altKey && key === "t") {
    return { type: "toggleFloatingToolbar" };
  }

  if (event.altKey && key === "c") {
    return { type: "toggleCompactMode" };
  }

  if (key === "g") {
    return { type: "findMatch", direction: event.shiftKey ? -1 : 1 };
  }

  if (key === "0") {
    return { type: "resetZoom" };
  }

  if (key === "-" || key === "_") {
    return { type: "zoomOut" };
  }

  if (key === "=" || key === "+") {
    return { type: "zoomIn" };
  }

  const shortcuts: Record<string, ShortcutIntent> = {
    "\\": { type: "toggleSplit" },
    b: { type: "toggleSidebar" },
    f: { type: "openFind" },
    h: { type: "openHistory" },
    j: { type: "focusAddress" },
    k: { type: "openCommand" },
    l: { type: "focusAddress" },
    s: { type: "toggleCompactMode" },
    t: { type: "newTab" },
    w: { type: "closeTab" }
  };

  return shortcuts[key] ?? null;
}

function isDigitKey(key: string): boolean {
  return /^[1-9]$/.test(key);
}
