import { useCallback, useEffect, useRef, useState } from "react";

const COMPACT_CHROME_PEEK_MS = 1400;

export function useCompactChromePeek(compactMode: boolean) {
  const sidebarPeekTimeout = useRef<number | null>(null);
  const toolbarPeekTimeout = useRef<number | null>(null);
  const [compactSidebarPulse, setCompactSidebarPulse] = useState(false);
  const [compactToolbarHeld, setCompactToolbarHeld] = useState(false);
  const [compactToolbarPulse, setCompactToolbarPulse] = useState(false);

  const clearSidebarPeekTimeout = useCallback(() => {
    clearPeekTimeout(sidebarPeekTimeout);
  }, []);

  const clearToolbarPeekTimeout = useCallback(() => {
    clearPeekTimeout(toolbarPeekTimeout);
  }, []);

  const peekCompactSidebar = useCallback(() => {
    if (!compactMode) return;

    pulsePeek(sidebarPeekTimeout, clearSidebarPeekTimeout, setCompactSidebarPulse);
  }, [clearSidebarPeekTimeout, compactMode]);

  const peekCompactToolbar = useCallback(() => {
    if (!compactMode) return;

    pulsePeek(toolbarPeekTimeout, clearToolbarPeekTimeout, setCompactToolbarPulse);
  }, [clearToolbarPeekTimeout, compactMode]);

  const peekCompactChrome = useCallback(() => {
    peekCompactSidebar();
    peekCompactToolbar();
  }, [peekCompactSidebar, peekCompactToolbar]);

  const holdCompactToolbar = useCallback(() => {
    if (!compactMode) return;

    setCompactToolbarHeld(true);
  }, [compactMode]);

  const releaseCompactToolbar = useCallback(() => {
    setCompactToolbarHeld(false);
  }, []);

  useEffect(() => {
    if (compactMode) return;

    clearSidebarPeekTimeout();
    clearToolbarPeekTimeout();
    setCompactSidebarPulse(false);
    setCompactToolbarHeld(false);
    setCompactToolbarPulse(false);
  }, [clearSidebarPeekTimeout, clearToolbarPeekTimeout, compactMode]);

  useEffect(() => () => {
    clearSidebarPeekTimeout();
    clearToolbarPeekTimeout();
  }, [clearSidebarPeekTimeout, clearToolbarPeekTimeout]);

  const compactSidebarPeeking = compactMode && compactSidebarPulse;
  const compactToolbarPeeking = compactMode && (compactToolbarHeld || compactToolbarPulse);

  return {
    compactChromePeeking: compactSidebarPeeking || compactToolbarPeeking,
    compactSidebarPeeking,
    compactToolbarPeeking,
    holdCompactToolbar,
    peekCompactChrome,
    peekCompactSidebar,
    peekCompactToolbar,
    releaseCompactToolbar
  };
}

function pulsePeek(
  timeoutRef: { current: number | null },
  clearTimeoutRef: () => void,
  setPulse: (peeking: boolean) => void
) {
  clearTimeoutRef();
  setPulse(true);
  timeoutRef.current = window.setTimeout(() => {
    setPulse(false);
    timeoutRef.current = null;
  }, COMPACT_CHROME_PEEK_MS);
}

function clearPeekTimeout(timeoutRef: { current: number | null }) {
  if (!timeoutRef.current) return;

  window.clearTimeout(timeoutRef.current);
  timeoutRef.current = null;
}
