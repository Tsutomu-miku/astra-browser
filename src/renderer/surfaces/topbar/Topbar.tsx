import {
  FiAlertTriangle,
  FiArrowLeft,
  FiArrowRight,
  FiBookmark,
  FiInfo,
  FiLock,
  FiMinus,
  FiPlus,
  FiRefreshCw,
  FiStar,
  FiVolume2,
  FiVolumeX,
  FiX,
  FiZap
} from "react-icons/fi";

import { isEssential, isFavorite } from "../../domain/browser-core";
import { getUrlIdentity } from "../../domain/urlIdentity";
import { formatZoomPercent } from "../../domain/zoom";
import type { BrowserController } from "../../hooks/types";
import { useOmniboxController } from "../../hooks/useOmniboxController";

export function Topbar({ controller }: { controller: BrowserController }) {
  const { activeTab, activeWebview, activeWorkspace, actions, addressValue, setAddressValue, setPanel, state } = controller;
  const identity = getUrlIdentity(activeTab.url);
  const omnibox = useOmniboxController({ actions, addressValue, setAddressValue, state });

  return (
    <header className="topbar">
      <nav className="nav-controls" aria-label="Navigation">
        <button className="icon-button" title="Back" type="button" disabled={!activeTab.canGoBack} onClick={() => actions.runWebviewAction("goBack")}><FiArrowLeft /></button>
        <button className="icon-button" title="Forward" type="button" disabled={!activeTab.canGoForward} onClick={() => actions.runWebviewAction("goForward")}><FiArrowRight /></button>
        <button className="icon-button" title="Reload" type="button" onClick={() => actions.runWebviewAction("reload")}><FiRefreshCw /></button>
      </nav>
      <div className="address-area">
        <form className="address-form" onSubmit={omnibox.submitAddress}>
          <button
            className={`address-identity is-${identity.security}`}
            title={identity.host || identity.label}
            type="button"
            onClick={() => setPanel("site")}
          >
            <span className="identity-glyph"><SecurityIcon security={identity.security} /></span>
            <span className="identity-label">{activeTab.isLoading ? "Loading" : identity.label}</span>
          </button>
          <input
            id="addressInput"
            autoComplete="off"
            inputMode="url"
            spellCheck={false}
            aria-label="Address"
            placeholder="Search or enter address"
            value={addressValue}
            onChange={(event) => omnibox.updateAddressValue(event.target.value)}
            onFocus={() => omnibox.setSuggestionsOpen(true)}
            onBlur={() => omnibox.setSuggestionsOpen(false)}
            onKeyDown={omnibox.onAddressKeyDown}
            aria-activedescendant={omnibox.suggestionsOpen && omnibox.suggestions.length > 0 ? `address-suggestion-${omnibox.activeIndex}` : undefined}
          />
        </form>
        {omnibox.suggestionsOpen && omnibox.suggestions.length > 0 && (
          <div className="omnibox-suggestions" role="listbox" aria-label="Address suggestions">
            {omnibox.suggestions.map((suggestion, index) => (
              <button
                className="omnibox-suggestion"
                id={`address-suggestion-${index}`}
                key={suggestion.id}
                type="button"
                aria-selected={index === omnibox.activeIndex}
                onMouseDown={(event) => omnibox.onSuggestionPointerDown(event, suggestion)}
                onMouseEnter={() => omnibox.setActiveSuggestionIndex(index)}
              >
                <span className="suggestion-title">{suggestion.title}</span>
                <span className="suggestion-subtitle">{suggestion.subtitle}</span>
              </button>
            ))}
          </div>
        )}
      </div>
      <button className="toolbar-button" title="Workspace settings" type="button" onClick={() => setPanel("settings")}>
        <span className="topbar-workspace-dot" style={{ background: activeWorkspace.accent }} />
        <span className="topbar-workspace-name">{activeWorkspace.name}</span>
      </button>
      <div className="page-actions" aria-label="Page actions">
        <button
          className="icon-button"
          title={isFavorite(activeWorkspace, activeTab.url) ? "Remove favorite" : "Add favorite"}
          type="button"
          aria-pressed={isFavorite(activeWorkspace, activeTab.url)}
          onClick={actions.toggleActiveTabFavorite}
        >
          <FiStar />
        </button>
        <button
          className="icon-button"
          title={isEssential(state, activeTab.url) ? "Remove essential" : "Add essential"}
          type="button"
          aria-pressed={isEssential(state, activeTab.url)}
          onClick={actions.toggleActiveTabEssential}
        >
          <FiZap />
        </button>
        <button
          className="icon-button"
          title={activeTab.isPinned ? "Unpin tab" : "Pin tab"}
          type="button"
          aria-pressed={activeTab.isPinned}
          onClick={actions.toggleActiveTabPinned}
        >
          <FiBookmark />
        </button>
        <button
          className="icon-button"
          title={activeTab.isMuted ? "Unmute tab" : "Mute tab"}
          type="button"
          aria-pressed={activeTab.isMuted}
          onClick={actions.toggleActiveTabMuted}
        >
          {activeTab.isMuted ? <FiVolumeX /> : <FiVolume2 />}
        </button>
        <div className="zoom-controls" aria-label="Page zoom">
          <button className="icon-button" title="Zoom out" type="button" onClick={actions.zoomOut}><FiMinus /></button>
          <button className="zoom-value" title="Reset zoom" type="button" onClick={actions.resetActiveTabZoom}>
            {formatZoomPercent(activeTab.zoomFactor)}
          </button>
          <button className="icon-button" title="Zoom in" type="button" onClick={actions.zoomIn}><FiPlus /></button>
        </div>
        <button className="icon-button close-tab-button" title="Close tab" type="button" onClick={actions.closeActiveTab}><FiX /></button>
      </div>
    </header>
  );
}

function SecurityIcon({ security }: { security: ReturnType<typeof getUrlIdentity>["security"] }) {
  if (security === "secure") return <FiLock />;
  if (security === "insecure") return <FiAlertTriangle />;
  return <FiInfo />;
}
