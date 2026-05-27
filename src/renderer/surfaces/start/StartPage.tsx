import { useMemo, type CSSProperties, type MouseEvent } from "react";
import { FiClock, FiStar, FiZap } from "react-icons/fi";

import { getReadableUrlTitle } from "../../domain/browser-core";
import type { BrowserController } from "../../hooks/types";
import { StartSearch } from "./components/StartSearch";
import { StartTileGrid } from "./components/StartTileGrid";
import { getStartOpenIntent } from "./startOpenIntent";
import { getStartPageContent } from "./startPageContent";

export function StartPage({
  controller,
  isVisible
}: {
  controller: BrowserController;
  isVisible: boolean;
}) {
  const { actions, activeWorkspace, state } = controller;
  const content = useMemo(() => getStartPageContent(state, activeWorkspace), [activeWorkspace, state]);
  const accentStyle = { "--start-accent": activeWorkspace.accent } as CSSProperties;

  function openOrPreview(event: MouseEvent, url: string, title?: string) {
    const intent = getStartOpenIntent(url, title, {
      altKey: event.altKey,
      shiftKey: event.shiftKey
    });

    if (intent.type === "preview") {
      actions.openGlance(intent.url, intent.title);
    } else if (intent.type === "split") {
      actions.openUrlInSplit(intent.url, intent.title);
    } else {
      actions.navigateActiveTab(intent.url);
    }
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

        <StartSearch controller={controller} isVisible={isVisible} />

        <section className="start-section" aria-label="Essentials">
          <div className="start-section-header">
            <FiZap />
            <h3>Essentials</h3>
          </div>
          <StartTileGrid
            emptyText="Essentials will appear across every Space."
            items={content.essentials}
            onOpen={openOrPreview}
          />
        </section>

        <section className="start-section" aria-label="Favorites">
          <div className="start-section-header">
            <FiStar />
            <h3>Favorites</h3>
          </div>
          <StartTileGrid
            emptyText="Favorites in this Space will appear here."
            items={content.favorites}
            onOpen={openOrPreview}
          />
        </section>

        <section className="start-section" aria-label="Recent history">
          <div className="start-section-header">
            <FiClock />
            <h3>Recent</h3>
          </div>
          <div className="start-history-list">
            {content.recentHistory.length === 0 ? (
              <p className="start-empty">Recently visited pages in this Space will appear here.</p>
            ) : content.recentHistory.map((entry) => (
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
