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
  FiUnlock,
  FiX,
  FiZap
} from "react-icons/fi";

import { isEssential, isFavorite } from "../../domain/browser";
import { getUrlIdentity } from "../../domain/browser/urlIdentity";
import { formatZoomPercent } from "../../domain/browser/zoom";
import type { BrowserController } from "../../app/controller/types";
import { useOmniboxController } from "../../app/controller/useOmniboxController";
import { getReloadButtonState } from "../../common/navigation/reloadButtonState";
import { getOmniboxActionHints } from "../../common/omnibox/omniboxActions";
import { PageIdentityContextMenu } from "./components/PageIdentityContextMenu";
import { usePageIdentityContextMenu } from "./components/usePageIdentityContextMenu";
import { WorkspacePillContextMenu } from "./components/WorkspacePillContextMenu";
import { useWorkspacePillContextMenu } from "./components/useWorkspacePillContextMenu";

export function Topbar({ controller }: { controller: BrowserController }) {
  const { activeTab, activeWorkspace, actions, addressValue, compactMode, floatingToolbarOpen, setAddressValue, setPanel, state } = controller;
  const identity = getUrlIdentity(activeTab.url);
  const omnibox = useOmniboxController({ actions, addressValue, setAddressValue, state });
  const reloadButton = getReloadButtonState(activeTab.isLoading);
  const pageIdentityMenu = usePageIdentityContextMenu();
  const workspacePillMenu = useWorkspacePillContextMenu();

  return (
    <header className="topbar">
      <nav className="nav-controls" aria-label="Navigation">
        <button className="icon-button" title="Back" type="button" disabled={!activeTab.canGoBack} onClick={() => actions.runWebviewAction("goBack")}><FiArrowLeft /></button>
        <button className="icon-button" title="Forward" type="button" disabled={!activeTab.canGoForward} onClick={() => actions.runWebviewAction("goForward")}><FiArrowRight /></button>
        <button
          className="icon-button"
          title={reloadButton.label}
          type="button"
          aria-label={reloadButton.label}
          onClick={() => actions.runWebviewAction(reloadButton.action)}
        >
          {activeTab.isLoading ? <FiX /> : <FiRefreshCw />}
        </button>
      </nav>
      <div className="address-area">
        <form className="address-form" onSubmit={omnibox.submitAddress}>
          <button
            className={`address-identity is-${identity.security}`}
            title={identity.host || identity.label}
            type="button"
            onClick={() => setPanel("site")}
            onContextMenu={(event) => pageIdentityMenu.openPageIdentityMenu(event, {
              title: activeTab.title,
              url: activeTab.url
            })}
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
            role="combobox"
            aria-autocomplete="list"
            aria-controls="address-suggestions"
            aria-expanded={omnibox.suggestionsOpen && omnibox.suggestions.length > 0}
            value={addressValue}
            onChange={(event) => omnibox.updateAddressValue(event.target.value)}
            onFocus={() => omnibox.setSuggestionsOpen(true)}
            onBlur={() => omnibox.setSuggestionsOpen(false)}
            onKeyDown={omnibox.onAddressKeyDown}
            aria-activedescendant={omnibox.suggestionsOpen && omnibox.suggestions.length > 0 ? `address-suggestion-${omnibox.activeIndex}` : undefined}
          />
        </form>
        {omnibox.suggestionsOpen && omnibox.suggestions.length > 0 && (
          <div className="omnibox-suggestions" id="address-suggestions" role="listbox" aria-label="Address suggestions">
            {omnibox.suggestions.map((suggestion, index) => (
              <button
                className="omnibox-suggestion"
                id={`address-suggestion-${index}`}
                key={suggestion.id}
                type="button"
                role="option"
                aria-selected={index === omnibox.activeIndex}
                title="Alt-click to open in split view"
                onMouseDown={(event) => omnibox.onSuggestionPointerDown(event, suggestion)}
                onMouseEnter={() => omnibox.setActiveSuggestionIndex(index)}
              >
                <span className="suggestion-main">
                  <span className="suggestion-title">{suggestion.title}</span>
                  <span className="omnibox-action-hints" aria-label="Alt Split">
                    {getOmniboxActionHints(suggestion).map((hint) => (
                      <span className={`omnibox-action-hint is-${hint.id}`} key={hint.id}>
                        <kbd>{hint.modifier}</kbd>
                        <span>{hint.label}</span>
                      </span>
                    ))}
                  </span>
                </span>
                <span className="suggestion-subtitle">{suggestion.subtitle}</span>
              </button>
            ))}
          </div>
        )}
        {pageIdentityMenu.menu && (
          <PageIdentityContextMenu
            item={pageIdentityMenu.menu.item}
            left={pageIdentityMenu.menu.left}
            top={pageIdentityMenu.menu.top}
            onClose={pageIdentityMenu.closeMenu}
            onCopyTitle={actions.copyText}
            onCopyUrl={actions.copyText}
            onOpenGlance={actions.openGlance}
            onOpenInSplit={actions.openUrlInSplit}
            onOpenSiteInfo={() => setPanel("site")}
          />
        )}
      </div>
      <button
        className="toolbar-button"
        title="Workspace settings"
        type="button"
        onClick={() => setPanel("settings")}
        onContextMenu={(event) => workspacePillMenu.openWorkspacePillMenu(event, {
          id: activeWorkspace.id,
          name: activeWorkspace.name
        })}
      >
        <span className="topbar-workspace-dot" style={{ background: activeWorkspace.accent }} />
        <span className="topbar-workspace-name">{activeWorkspace.name}</span>
      </button>
      {workspacePillMenu.menu && (
        <WorkspacePillContextMenu
          canDelete={state.workspaces.length > 1}
          left={workspacePillMenu.menu.left}
          top={workspacePillMenu.menu.top}
          workspaceName={workspacePillMenu.menu.item.name}
          onClose={workspacePillMenu.closeMenu}
          onDeleteWorkspace={() => actions.deleteWorkspace(workspacePillMenu.menu?.item.id ?? activeWorkspace.id)}
          onNewWorkspace={actions.addWorkspace}
          onOpenSettings={() => setPanel("settings")}
        />
      )}
      <div className="page-actions" aria-label="Page actions">
        {compactMode && (
          <button
            className="icon-button compact-toolbar-pin"
            title={floatingToolbarOpen ? "Unpin floating toolbar" : "Pin floating toolbar"}
            type="button"
            aria-label={floatingToolbarOpen ? "Unpin floating toolbar" : "Pin floating toolbar"}
            aria-pressed={floatingToolbarOpen}
            onClick={actions.toggleFloatingToolbar}
          >
            {floatingToolbarOpen ? <FiLock /> : <FiUnlock />}
          </button>
        )}
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
