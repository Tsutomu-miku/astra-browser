import { FiSearch } from "react-icons/fi";

import type { BrowserController } from "../../../../app/controller/types";
import { useOmniboxController } from "../../../../app/controller/useOmniboxController";
import { getOmniboxActionHints } from "../../../../common/omnibox/omniboxActions";

export function SidebarAddress({ controller }: { controller: BrowserController }) {
  const { actions, addressValue, compactMode, setAddressValue, state } = controller;
  const omnibox = useOmniboxController({ actions, addressValue, setAddressValue, state });

  return (
    <div className="sidebar-address" data-compact={compactMode}>
      <form className="sidebar-address-form" onSubmit={omnibox.submitAddress}>
        <FiSearch />
        <input
          id="sidebarAddressInput"
          autoComplete="off"
          inputMode="url"
          spellCheck={false}
          aria-label="Sidebar address"
          placeholder="Search or enter address"
          value={addressValue}
          onBlur={() => omnibox.setSuggestionsOpen(false)}
          onChange={(event) => omnibox.updateAddressValue(event.target.value)}
          onFocus={() => omnibox.setSuggestionsOpen(true)}
          onKeyDown={omnibox.onAddressKeyDown}
          aria-activedescendant={omnibox.suggestionsOpen && omnibox.suggestions.length > 0 ? `sidebar-address-suggestion-${omnibox.activeIndex}` : undefined}
        />
      </form>
      {omnibox.suggestionsOpen && omnibox.suggestions.length > 0 && (
        <div className="sidebar-omnibox-suggestions" role="listbox" aria-label="Sidebar address suggestions">
          {omnibox.suggestions.map((suggestion, index) => (
            <button
              className="sidebar-omnibox-suggestion"
              id={`sidebar-address-suggestion-${index}`}
              key={suggestion.id}
              type="button"
              aria-selected={index === omnibox.activeIndex}
              title="Alt-click to open in split view"
              onMouseDown={(event) => omnibox.onSuggestionPointerDown(event, suggestion)}
              onMouseEnter={() => omnibox.setActiveSuggestionIndex(index)}
            >
              <span className="sidebar-suggestion-main">
                <span>{suggestion.title}</span>
                <span className="omnibox-action-hints" aria-label="Alt Split">
                  {getOmniboxActionHints(suggestion).map((hint) => (
                    <span className={`omnibox-action-hint is-${hint.id}`} key={hint.id}>
                      <kbd>{hint.modifier}</kbd>
                      <span>{hint.label}</span>
                    </span>
                  ))}
                </span>
              </span>
              <small>{suggestion.subtitle}</small>
            </button>
          ))}
        </div>
      )}
    </div>
  );
}
