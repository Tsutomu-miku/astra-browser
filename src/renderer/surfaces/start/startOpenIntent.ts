export type StartOpenIntent =
  | { type: "open"; title?: string; url: string }
  | { type: "preview"; title?: string; url: string }
  | { type: "split"; title?: string; url: string };

export interface StartOpenModifiers {
  altKey: boolean;
  shiftKey: boolean;
}

export interface StartOpenActionHint {
  id: "preview" | "split";
  label: string;
  modifier: string;
}

export function getStartOpenIntent(
  url: string,
  title: string | undefined,
  modifiers: StartOpenModifiers
): StartOpenIntent {
  if (modifiers.altKey) {
    return { title, type: "preview", url };
  }

  if (modifiers.shiftKey) {
    return { title, type: "split", url };
  }

  return { title, type: "open", url };
}

export function getStartOpenActionHints(): StartOpenActionHint[] {
  return [
    { id: "preview", modifier: "Alt", label: "Preview" },
    { id: "split", modifier: "Shift", label: "Split" }
  ];
}
