import { useCallback, useEffect, useRef, useState } from "react";

const COMPACT_CHROME_PEEK_MS = 1400;

export function useCompactChromePeek(compactMode: boolean) {
  const chromePeekTimeout = useRef<number | null>(null);
  const [compactChromeHeld, setCompactChromeHeld] = useState(false);
  const [compactChromePulse, setCompactChromePulse] = useState(false);

  const clearPeekTimeout = useCallback(() => {
    if (!chromePeekTimeout.current) return;

    window.clearTimeout(chromePeekTimeout.current);
    chromePeekTimeout.current = null;
  }, []);

  const peekCompactChrome = useCallback(() => {
    if (!compactMode) return;

    clearPeekTimeout();

    setCompactChromePulse(true);
    chromePeekTimeout.current = window.setTimeout(() => {
      setCompactChromePulse(false);
      chromePeekTimeout.current = null;
    }, COMPACT_CHROME_PEEK_MS);
  }, [clearPeekTimeout, compactMode]);

  const holdCompactChrome = useCallback(() => {
    if (!compactMode) return;

    setCompactChromeHeld(true);
  }, [compactMode]);

  const releaseCompactChrome = useCallback(() => {
    setCompactChromeHeld(false);
  }, []);

  useEffect(() => {
    if (compactMode) return;

    clearPeekTimeout();
    setCompactChromeHeld(false);
    setCompactChromePulse(false);
  }, [clearPeekTimeout, compactMode]);

  useEffect(() => () => {
    clearPeekTimeout();
  }, [clearPeekTimeout]);

  return {
    compactChromePeeking: compactMode && (compactChromeHeld || compactChromePulse),
    holdCompactChrome,
    peekCompactChrome,
    releaseCompactChrome
  };
}
