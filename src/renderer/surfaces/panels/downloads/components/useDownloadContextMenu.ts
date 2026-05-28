import type { MouseEvent } from "react";

import { useAnchoredContextMenu } from "../../../../common/context-menu/useAnchoredContextMenu";
import type { DownloadEntry } from "../../../../domain/browser";

export function useDownloadContextMenu() {
  const { closeMenu, menu, openMenu } = useAnchoredContextMenu<DownloadEntry>({ menuHeight: 136 });

  function openDownloadMenu(event: MouseEvent, download: DownloadEntry) {
    openMenu(event, download);
  }

  return { closeMenu, menu, openDownloadMenu };
}
