export type ShortcutIntent =
  | { type: "closePanels" }
  | { type: "closeTab" }
  | { type: "focusAddress" }
  | { type: "newTab" }
  | { type: "openFind" }
  | { type: "openCommand" }
  | { type: "restoreClosedTab" }
  | { type: "selectAdjacentTab"; direction: 1 | -1 }
  | { type: "selectTabIndex"; index: number }
  | { type: "selectWorkspaceIndex"; index: number }
  | { type: "resetZoom" }
  | { type: "toggleSidebar" }
  | { type: "toggleSplit" }
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
    return { type: "selectTabIndex", index: Number(key) - 1 };
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

  if (key === "tab") {
    return { type: "selectAdjacentTab", direction: event.shiftKey ? -1 : 1 };
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
    k: { type: "openCommand" },
    l: { type: "focusAddress" },
    t: { type: "newTab" },
    w: { type: "closeTab" }
  };

  return shortcuts[key] ?? null;
}

function isDigitKey(key: string): boolean {
  return /^[1-9]$/.test(key);
}
