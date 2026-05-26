import { FormEvent, useMemo, useState, type CSSProperties, type MouseEvent } from "react";
import { FiClock, FiSearch, FiStar } from "react-icons/fi";

import { getReadableUrlTitle, type BrowserState, type Workspace } from "../../domain/browser-core";
import type { BrowserController } from "../../hooks/types";

export function StartPage({
  controller,
  isVisible
}: {
  controller: BrowserController;
  isVisible: boolean;
}) {
  const { actions, activeWorkspace, state } = controller;
  const [query, setQuery] = useState("");
  const recentHistory = useMemo(() => getRecentWorkspaceHistory(state, activeWorkspace.id), [activeWorkspace.id, state]);
  const accentStyle = { "--start-accent": activeWorkspace.accent } as CSSProperties;

  function submit(event: FormEvent) {
    event.preventDefault();
    if (query.trim()) {
      actions.navigateActiveTab(query);
    }
  }

  function openOrPreview(event: MouseEvent, url: string, title?: string) {
    event.altKey ? actions.openGlance(url, title) : actions.navigateActiveTab(url);
  }

  return (
    <section
      className={`start-page-shell ${isVisible ? "is-visible" : "is-hidden"}`}
      style={accentStyle}
      aria-label="New tab"
    >
      <div className="start-page">
        <header className="start-hero">
          <span className="start-space-dot" />
          <p className="start-kicker">{activeWorkspace.name} Space</p>
          <h2>New Tab</h2>
        </header>

        <form className="start-search" onSubmit={submit}>
          <FiSearch />
          <input
            autoFocus={isVisible}
            autoComplete="off"
            inputMode="url"
            spellCheck={false}
            aria-label="Search or enter address"
            placeholder="Search or enter address"
            value={query}
            onChange={(event) => setQuery(event.target.value)}
          />
        </form>

        <section className="start-section" aria-label="Favorites">
          <div className="start-section-header">
            <FiStar />
            <h3>Favorites</h3>
          </div>
          <div className="start-tile-grid">
            {activeWorkspace.favorites.length === 0 ? (
              <p className="start-empty">Favorites in this Space will appear here.</p>
            ) : activeWorkspace.favorites.slice(0, 8).map((favorite) => (
              <button
                className="start-tile"
                key={favorite.id}
                type="button"
                title={favorite.url}
                onClick={(event) => openOrPreview(event, favorite.url, favorite.title)}
              >
                <span className="start-tile-icon">{getReadableUrlTitle(favorite.url).slice(0, 1).toUpperCase()}</span>
                <span className="start-tile-title">{favorite.title}</span>
              </button>
            ))}
          </div>
        </section>

        <section className="start-section" aria-label="Recent history">
          <div className="start-section-header">
            <FiClock />
            <h3>Recent</h3>
          </div>
          <div className="start-history-list">
            {recentHistory.length === 0 ? (
              <p className="start-empty">Recently visited pages in this Space will appear here.</p>
            ) : recentHistory.map((entry) => (
              <button
                className="start-history-item"
                key={entry.id}
                type="button"
                title={entry.url}
                onClick={(event) => openOrPreview(event, entry.url, entry.title)}
              >
                <span>{entry.title}</span>
                <small>{getReadableUrlTitle(entry.url)}</small>
              </button>
            ))}
          </div>
        </section>
      </div>
    </section>
  );
}

function getRecentWorkspaceHistory(state: BrowserState, workspaceId: Workspace["id"]) {
  return state.history
    .filter((entry) => entry.workspaceId === workspaceId)
    .slice(0, 5);
}
