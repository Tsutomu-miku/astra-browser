import { BrowserItemIcon } from "../../../../common/icons/BrowserItemIcon";
import type { FaviconCache } from "../../../../domain/browser";

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
  return (
    <BrowserItemIcon
      className={className}
      faviconCache={faviconCache}
      faviconUrl={faviconUrl}
      status={status}
      url={url}
    />
  );
}
