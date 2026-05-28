export interface PageIdentityContextMenuItem {
  title: string;
  url: string;
}

export function PageIdentityContextMenu({
  item,
  left,
  onClose,
  onCopyTitle,
  onCopyUrl,
  onOpenGlance,
  onOpenInSplit,
  onOpenSiteInfo,
  top
}: {
  item: PageIdentityContextMenuItem;
  left: number;
  onClose: () => void;
  onCopyTitle: (title: string) => void;
  onCopyUrl: (url: string) => void;
  onOpenGlance: (url: string, title?: string) => void;
  onOpenInSplit: (url: string, title?: string) => void;
  onOpenSiteInfo: () => void;
  top: number;
}) {
  const title = item.title || item.url;
  const run = (action: () => void) => {
    action();
    onClose();
  };

  return (
    <div
      className="page-identity-context-menu"
      role="menu"
      style={{ left, top }}
      onContextMenu={(event) => event.preventDefault()}
    >
      <button type="button" role="menuitem" onClick={() => run(onOpenSiteInfo)}>
        Site information
      </button>
      <button type="button" role="menuitem" onClick={() => run(() => onCopyUrl(item.url))}>
        Copy current URL
      </button>
      <button type="button" role="menuitem" onClick={() => run(() => onCopyTitle(title))}>
        Copy page title
      </button>
      <span className="page-identity-context-menu-separator" />
      <button type="button" role="menuitem" onClick={() => run(() => onOpenGlance(item.url, title))}>
        Preview in Glance
      </button>
      <button type="button" role="menuitem" onClick={() => run(() => onOpenInSplit(item.url, title))}>
        Open in split view
      </button>
    </div>
  );
}
