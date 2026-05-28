import { useMemo, useState } from "react";
import { FiX } from "react-icons/fi";

import type { BrowserController } from "../../app/controller/types";
import { ClosedTabList } from "./history/components/ClosedTabList";
import { HistoryEntryContextMenu } from "./history/components/HistoryEntryContextMenu";
import { HistoryItem } from "./history/components/HistoryItem";
import { useHistoryEntryContextMenu } from "./history/components/useHistoryEntryContextMenu";
import { filterHistory } from "./history/model/historyFilters";

export function HistoryPanel({ controller }: { controller: BrowserController }) {
  const { actions, activeWorkspace, setPanel, state } = controller;
  const { closeMenu, menu, openHistoryMenu } = useHistoryEntryContextMenu();
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
              onContextMenu={openHistoryMenu}
              onOpen={actions.openUrlInActiveWorkspace}
              onOpenInSplit={actions.openUrlInSplit}
              onPreview={actions.openGlance}
              onRemove={actions.removeHistoryEntry}
            />
          ))}
        </section>
        {menu && (
          <HistoryEntryContextMenu
            entry={menu.item}
            left={menu.left}
            top={menu.top}
            onClose={closeMenu}
            onOpen={actions.openUrlInActiveWorkspace}
            onOpenInSplit={actions.openUrlInSplit}
            onPreview={actions.openGlance}
            onRemove={actions.removeHistoryEntry}
          />
        )}
      </div>
    </aside>
  );
}
