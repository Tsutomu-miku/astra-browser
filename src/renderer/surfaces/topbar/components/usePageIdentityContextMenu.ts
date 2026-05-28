import type { MouseEvent } from "react";

import { useAnchoredContextMenu } from "../../../common/context-menu/useAnchoredContextMenu";
import type { PageIdentityContextMenuItem } from "./PageIdentityContextMenu";

export function usePageIdentityContextMenu() {
  const { closeMenu, menu, openMenu } = useAnchoredContextMenu<PageIdentityContextMenuItem>({ menuHeight: 208 });

  function openPageIdentityMenu(event: MouseEvent, item: PageIdentityContextMenuItem) {
    openMenu(event, item);
  }

  return { closeMenu, menu, openPageIdentityMenu };
}
