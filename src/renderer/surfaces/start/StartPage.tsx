import { useMemo, type CSSProperties, type MouseEvent } from "react";
import { FiClock, FiStar, FiZap } from "react-icons/fi";

import { getReadableUrlTitle } from "../../domain/browser";
import type { Favorite } from "../../domain/browser";
import type { StartEntryContextMenuItem } from "./components/useStartEntryContextMenu";
import type { BrowserController } from "../../app/controller/types";
import { StartEntryActionHints } from "./components/StartEntryActionHints";
import { StartEntryContextMenu } from "./components/StartEntryContextMenu";
import { StartSearch } from "./components/StartSearch";
import { StartTileGrid } from "./components/StartTileGrid";
import { useStartEntryContextMenu } from "./components/useStartEntryContextMenu";
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
  const { closeMenu, menu, openMenu } = useStartEntryContextMenu();

  function runQuickEntry(item: Favorite | StartEntryContextMenuItem) {
    const tab = activeWorkspace.tabs.find((candidate) => (
      candidate.id === item.tabId ||
      (!item.tabId && candidate.url === item.url)
    ));
    tab ? actions.selectTab(tab.id) : actions.openUrlInActiveWorkspace(item.url, item.title);
  }

  function openQuickEntry(event: MouseEvent, item: Favorite) {
    const intent = getStartOpenIntent(item.url, item.title, {
      altKey: event.altKey,
      shiftKey: event.shiftKey
    });

    if (intent.type === "preview") {
      actions.openGlance(intent.url, intent.title);
    } else if (intent.type === "split") {
      actions.openUrlInSplit(intent.url, intent.title);
    } else {
      runQuickEntry(item);
    }
  }

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
            kind="essential"
            onContextMenu={openMenu}
            onOpen={openQuickEntry}
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
            kind="favorite"
            onContextMenu={openMenu}
            onOpen={openQuickEntry}
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
                onContextMenu={(event) => openMenu(event, entry, "history")}
                onClick={(event) => openOrPreview(event, entry.url, entry.title)}
              >
                <span>{entry.title}</span>
                <small>{getReadableUrlTitle(entry.url)}</small>
                <StartEntryActionHints />
              </button>
            ))}
          </div>
        </section>
        {menu && (
          <StartEntryContextMenu
            item={menu.item}
            kind={menu.kind}
            left={menu.left}
            top={menu.top}
            onClose={closeMenu}
            onOpen={runQuickEntry}
            onOpenInSplit={actions.openUrlInSplit}
            onPreview={actions.openGlance}
            onRemove={(item, kind) => {
              if (kind === "history") {
                actions.removeHistoryEntry(item.id);
              } else if (kind === "essential") {
                actions.removeEssential(item.url);
              } else {
                actions.removeWorkspaceFavorite(item.id);
              }
            }}
          />
        )}
      </div>
    </section>
  );
}
