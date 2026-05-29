import { FiCompass, FiFileText, FiGlobe, FiLoader, FiMoon } from "react-icons/fi";

import { getCachedFaviconUrl, type FaviconCache } from "../../../../domain/browser";
import { getSidebarItemIconState } from "../../model/sidebarItemIcon";

export function SidebarItemIcon({
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
  const icon = getSidebarItemIconState(url);
  const resolvedFaviconUrl = faviconUrl || getCachedFaviconUrl(faviconCache, url);

  return (
    <span
      className={`${className} sidebar-item-icon is-${icon.kind}`}
      aria-hidden="true"
      data-icon-kind={icon.kind}
      data-icon-status={status}
    >
      <span className="sidebar-item-icon-glyph">
        {resolvedFaviconUrl ? (
          <img className="sidebar-item-icon-image" src={resolvedFaviconUrl} alt="" draggable={false} />
        ) : icon.text ?? <SidebarItemIconGlyph kind={icon.kind} />}
      </span>
      {status && (
        <span className={`sidebar-item-icon-status is-${status}`}>
          {status === "loading" ? <FiLoader /> : <FiMoon />}
        </span>
      )}
    </span>
  );
}

function SidebarItemIconGlyph({ kind }: { kind: ReturnType<typeof getSidebarItemIconState>["kind"] }) {
  if (kind === "internal") return <FiCompass />;
  if (kind === "file") return <FiFileText />;
  return <FiGlobe />;
}
