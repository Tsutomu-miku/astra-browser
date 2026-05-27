import { useCallback } from "react";

import type { BrowserStore } from "../../stores/browserStore";

export function useAddressBarFocus(store: BrowserStore) {
  return useCallback(() => {
    store.setCommandOpen(false);
    const sidebarInput = document.getElementById("sidebarAddressInput") as HTMLInputElement | null;
    const topbarInput = document.getElementById("addressInput") as HTMLInputElement | null;
    const input = store.compactMode && sidebarInput?.offsetParent ? sidebarInput : topbarInput;
    input?.focus();
    input?.select();
  }, [store]);
}
