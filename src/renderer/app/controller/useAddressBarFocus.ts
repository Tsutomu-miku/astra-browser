import { useCallback } from "react";

import type { BrowserStore } from "../../stores/browserStoreTypes";

export function useAddressBarFocus(store: BrowserStore, revealCompactSidebar?: () => void) {
  return useCallback(() => {
    store.setCommandOpen(false);
    const sidebarInput = document.getElementById("sidebarAddressInput") as HTMLInputElement | null;
    const topbarInput = document.getElementById("addressInput") as HTMLInputElement | null;
    if (store.compactMode && sidebarInput) {
      revealCompactSidebar?.();
      focusAddressInputAfterReveal(sidebarInput);
      return;
    }

    focusAddressInput(topbarInput ?? sidebarInput);
  }, [revealCompactSidebar, store]);
}

function focusAddressInputAfterReveal(input: HTMLInputElement) {
  window.requestAnimationFrame(() => focusAddressInput(input));
}

function focusAddressInput(input: HTMLInputElement | null) {
  input?.focus();
  input?.select();
}
