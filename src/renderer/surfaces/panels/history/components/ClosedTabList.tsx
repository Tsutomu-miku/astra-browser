import type { ClosedTab } from "../../../../domain/browser";
import { formatHistoryTime } from "../model/historyFormat";

export function ClosedTabList({
  closedTabs,
  onRestore
}: {
  closedTabs: ClosedTab[];
  onRestore: (closedIndex: number) => void;
}) {
  if (closedTabs.length === 0) return null;

  return (
    <section className="closed-list" aria-label="Recently closed tabs">
      <h3 className="panel-section-title">Recently closed</h3>
      {closedTabs.slice(0, 8).map((tab, index) => (
        <button className="history-item" key={`${tab.url}-${tab.closedAt}`} type="button" onClick={() => onRestore(index)}>
          <span className="history-title">{tab.title}</span>
          <span className="history-time">{formatHistoryTime(tab.closedAt)}</span>
          <span className="history-url">{tab.url}</span>
        </button>
      ))}
    </section>
  );
}
