import { useEffect, useMemo, useState } from "react";
import { FiCompass, FiFileText, FiGlobe, FiLoader, FiMoon } from "react-icons/fi";

import { getCachedFaviconUrl, getFaviconCacheKey, type FaviconCache } from "../../domain/browser";
import { getBrowserItemIconState } from "./browserItemIconState";

// Session-level LRU for cross-session favicon lookups resolved via the main
// process. Keys are origins (or full declared-favicon URLs); values are data:
// URLs. Capped to avoid unbounded growth; sidebar + history can surface
// thousands of distinct sites but favicons are tiny.
const FAVICON_PROXY_CACHE_LIMIT = 512;
const faviconProxyCache = new Map<string, string>();

function rememberProxyHit(key: string, dataUrl: string) {
  if (faviconProxyCache.size >= FAVICON_PROXY_CACHE_LIMIT) {
    const oldest = faviconProxyCache.keys().next().value;
    if (oldest !== undefined) faviconProxyCache.delete(oldest);
  }
  faviconProxyCache.set(key, dataUrl);
}

type FaviconTier = "declared" | "originRoot" | "proxy" | "glyph";

export function BrowserItemIcon({
  className,
  faviconCache,
  faviconUrl,
  status,
  url
}: {
  className: string;
  faviconCache?: FaviconCache;
  faviconUrl?: string;
  status?: "loading" | "sleeping";
  url: string;
}) {
  const icon = getBrowserItemIconState(url);
  const origin = useMemo(() => getFaviconCacheKey(url), [url]);

  const declaredFaviconUrl = faviconUrl || getCachedFaviconUrl(faviconCache, url);
  const originRootFaviconUrl = origin ? `${origin}/favicon.ico` : undefined;

  // Tier progression: declared → origin /favicon.ico → IPC cross-session proxy → glyph.
  const [tier, setTier] = useState<FaviconTier>(declaredFaviconUrl ? "declared" : "originRoot");
  // Resolved data: URL from the proxy IPC, stored in component state and the
  // module-level LRU so re-renders / other tabs with the same origin skip IPC.
  const [proxyDataUrl, setProxyDataUrl] = useState<string | null>(() => {
    if (!declaredFaviconUrl || !origin) return null;
    return faviconProxyCache.get(declaredFaviconUrl) ?? faviconProxyCache.get(origin) ?? null;
  });

  // Reset progression whenever the inputs change (navigated to a new site).
  useEffect(() => {
    setTier(declaredFaviconUrl ? "declared" : "originRoot");
    if (origin) {
      setProxyDataUrl(
        (declaredFaviconUrl && faviconProxyCache.get(declaredFaviconUrl)) ??
          faviconProxyCache.get(origin) ??
          null
      );
    } else {
      setProxyDataUrl(null);
    }
  }, [declaredFaviconUrl, origin]);

  // Kick off the IPC proxy lookup when we reach that tier and don't have a
  // cached answer yet.
  useEffect(() => {
    if (tier !== "proxy") return;
    if (proxyDataUrl) return;
    if (!declaredFaviconUrl && !originRootFaviconUrl) {
      setTier("glyph");
      return;
    }

    let cancelled = false;
    const probeUrl = declaredFaviconUrl || originRootFaviconUrl!;
    void (async () => {
      const result = await window.astraShell?.getFaviconData?.(probeUrl);
      if (cancelled) return;
      if (result) {
        if (origin) rememberProxyHit(origin, result);
        if (declaredFaviconUrl) rememberProxyHit(declaredFaviconUrl, result);
        setProxyDataUrl(result);
        setTier("proxy");
      } else {
        setTier("glyph");
      }
    })();
    return () => {
      cancelled = true;
    };
  }, [tier, declaredFaviconUrl, originRootFaviconUrl, origin, proxyDataUrl]);

  const currentSrc: string | undefined =
    tier === "declared"
      ? declaredFaviconUrl
      : tier === "originRoot"
        ? originRootFaviconUrl
        : tier === "proxy"
          ? proxyDataUrl ?? undefined
          : undefined;

  const showFavicon = Boolean(currentSrc);

  const advanceTier = () => {
    setTier((current) => {
      if (current === "declared") return "originRoot";
      if (current === "originRoot") return "proxy";
      return "glyph";
    });
  };

  return (
    <span
      className={`${className} sidebar-item-icon is-${icon.kind}`}
      aria-hidden="true"
      data-icon-kind={icon.kind}
      data-icon-status={status}
    >
      <span className="sidebar-item-icon-glyph">
        {showFavicon ? (
          <img
            className="sidebar-item-icon-image"
            src={currentSrc}
            alt=""
            draggable={false}
            onError={advanceTier}
          />
        ) : icon.text ?? <BrowserItemIconGlyph kind={icon.kind} />}
      </span>
      {status && (
        <span className={`sidebar-item-icon-status is-${status}`}>
          {status === "loading" ? <FiLoader /> : <FiMoon />}
        </span>
      )}
    </span>
  );
}

function BrowserItemIconGlyph({ kind }: { kind: ReturnType<typeof getBrowserItemIconState>["kind"] }) {
  if (kind === "internal") return <FiCompass />;
  if (kind === "file") return <FiFileText />;
  return <FiGlobe />;
}
