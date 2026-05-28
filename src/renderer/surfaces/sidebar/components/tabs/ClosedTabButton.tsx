import { getHostInitial, type ClosedTab } from "../../../../domain/browser";
import { SidebarItemActionHints } from "./SidebarItemActionHints";

export function ClosedTabButton({
  closedIndex,
  onOpenInSplit,
  onPreview,
  tab,
  onRestore
}: {
  closedIndex: number;
  onOpenInSplit: (url: string, title?: string) => void;
  onPreview: (url: string, title?: string) => void;
  tab: ClosedTab;
  onRestore: (closedIndex: number) => void;
}) {
  const title = tab.title || tab.url;

  return (
    <button
      className="closed-tab-button"
      type="button"
      title={`Restore ${title}`}
      onClick={(event) => {
        if (event.altKey) {
          onPreview(tab.url, tab.title);
        } else if (event.shiftKey) {
          onOpenInSplit(tab.url, tab.title);
        } else {
          onRestore(closedIndex);
        }
      }}
    >
      <span className="closed-tab-icon">{getHostInitial(tab.url)}</span>
      <span className="closed-tab-main">
        <span className="closed-tab-title">{title}</span>
        <span className="closed-tab-url">{tab.url}</span>
      </span>
      <span className="closed-tab-action">Restore</span>
      <SidebarItemActionHints />
    </button>
  );
}
