import type { MouseEvent } from "react";

import { useAnchoredContextMenu } from "../../../../common/context-menu/useAnchoredContextMenu";
import type { HistoryEntry } from "../../../../domain/browser";

export function useHistoryEntryContextMenu() {
  const { closeMenu, menu, openMenu } = useAnchoredContextMenu<HistoryEntry>();

  function openHistoryMenu(event: MouseEvent, entry: HistoryEntry) {
    openMenu(event, entry);
  }

  return { closeMenu, menu, openHistoryMenu };
}
