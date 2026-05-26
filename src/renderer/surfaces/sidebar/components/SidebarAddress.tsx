import { FiSearch } from "react-icons/fi";

import type { BrowserController } from "../../../hooks/types";
import { useOmniboxController } from "../../../hooks/useOmniboxController";

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
              onMouseDown={(event) => omnibox.onSuggestionPointerDown(event, suggestion)}
              onMouseEnter={() => omnibox.setActiveSuggestionIndex(index)}
            >
              <span>{suggestion.title}</span>
              <small>{suggestion.subtitle}</small>
            </button>
          ))}
        </div>
      )}
    </div>
  );
}
