import { useMemo, useState } from "react";
import { FiX } from "react-icons/fi";

import type { ClosedTab, HistoryEntry } from "../../domain/browser-core";
import type { BrowserController } from "../../hooks/types";

export function HistoryPanel({ controller }: { controller: BrowserController }) {
  const { actions, activeWorkspace, setPanel, state } = controller;
  const [query, setQuery] = useState("");
  const visibleHistory = useMemo(() => filterHistory(state.history, query).slice(0, 80), [query, state.history]);

  return (
    <aside className="history-panel">
      <header className="panel-header">
        <h2>History</h2>
        <button className="icon-button" title="Close history" type="button" onClick={() => setPanel(null)}><FiX /></button>
      </header>
      <div className="panel-scroll">
        <ClosedTabList closedTabs={activeWorkspace.closedTabs} onRestore={actions.restoreClosedTab} />
        <section className="history-list" aria-label="Browsing history">
          <div className="panel-section-header">
            <h3 className="panel-section-title">Recent history</h3>
            <button className="toolbar-button" type="button" disabled={state.history.length === 0} onClick={actions.clearHistory}>Clear</button>
          </div>
          <input
            className="panel-search"
            autoComplete="off"
            placeholder="Search history"
            spellCheck={false}
            value={query}
            onChange={(event) => setQuery(event.target.value)}
          />
          {visibleHistory.length === 0 ? (
            <p className="empty-state">{query ? "No matching history" : "No history yet"}</p>
          ) : visibleHistory.map((entry) => (
            <HistoryItem
              entry={entry}
              key={entry.id}
              onOpen={actions.openUrlInActiveWorkspace}
              onRemove={actions.removeHistoryEntry}
            />
          ))}
        </section>
      </div>
    </aside>
  );
}

function ClosedTabList({ closedTabs, onRestore }: { closedTabs: ClosedTab[]; onRestore: (closedIndex: number) => void }) {
  if (closedTabs.length === 0) return null;

  return (
    <section className="closed-list" aria-label="Recently closed tabs">
      <h3 className="panel-section-title">Recently closed</h3>
      {closedTabs.slice(0, 8).map((tab, index) => (
        <button className="history-item" key={`${tab.url}-${tab.closedAt}`} type="button" onClick={() => onRestore(index)}>
          <span className="history-title">{tab.title}</span>
          <span className="history-time">{formatTime(tab.closedAt)}</span>
          <span className="history-url">{tab.url}</span>
        </button>
      ))}
    </section>
  );
}

function HistoryItem({
  entry,
  onOpen,
  onRemove
}: {
  entry: HistoryEntry;
  onOpen: (url: string, title?: string) => void;
  onRemove: (historyId: string) => void;
}) {
  return (
    <article className="history-item">
      <button className="history-open" type="button" onClick={() => onOpen(entry.url, entry.title)}>
        <span className="history-title">{entry.title}</span>
        <span className="history-url">{entry.url}</span>
      </button>
      <span className="history-time">{formatTime(entry.visitedAt)}</span>
      <button className="history-remove" type="button" title="Remove history entry" onClick={() => onRemove(entry.id)}><FiX /></button>
    </article>
  );
}

function filterHistory(history: HistoryEntry[], query: string): HistoryEntry[] {
  const normalizedQuery = query.trim().toLowerCase();
  if (!normalizedQuery) return history;

  return history.filter((entry) =>
    entry.title.toLowerCase().includes(normalizedQuery) ||
    entry.url.toLowerCase().includes(normalizedQuery)
  );
}

function formatTime(value: number): string {
  return new Intl.DateTimeFormat(undefined, {
    hour: "2-digit",
    minute: "2-digit"
  }).format(value);
}
