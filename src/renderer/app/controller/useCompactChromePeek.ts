import { useCallback, useEffect, useRef, useState } from "react";

const COMPACT_CHROME_PEEK_MS = 1400;

export function useCompactChromePeek(compactMode: boolean) {
  const chromePeekTimeout = useRef<number | null>(null);
  const [compactChromePeeking, setCompactChromePeeking] = useState(false);

  const peekCompactChrome = useCallback(() => {
    if (!compactMode) return;

    if (chromePeekTimeout.current) {
      window.clearTimeout(chromePeekTimeout.current);
    }

    setCompactChromePeeking(true);
    chromePeekTimeout.current = window.setTimeout(() => {
      setCompactChromePeeking(false);
      chromePeekTimeout.current = null;
    }, COMPACT_CHROME_PEEK_MS);
  }, [compactMode]);

  useEffect(() => () => {
    if (chromePeekTimeout.current) {
      window.clearTimeout(chromePeekTimeout.current);
    }
  }, []);

  return { compactChromePeeking, peekCompactChrome };
}
