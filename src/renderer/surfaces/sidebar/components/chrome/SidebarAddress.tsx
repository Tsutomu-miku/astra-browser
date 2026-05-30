import { FiColumns, FiEye, FiSearch } from "react-icons/fi";

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
        <span className="sidebar-address-input-shell">
          {omnibox.completionSuffix && (
            <span className="address-autocomplete" aria-hidden="true">
              <span className="address-autocomplete-prefix">{addressValue}</span>
              <span className="address-autocomplete-suffix">{omnibox.completionSuffix}</span>
            </span>
          )}
          <input
            id="sidebarAddressInput"
            autoComplete="off"
            inputMode="url"
            spellCheck={false}
            aria-label="Sidebar address"
            placeholder="Search or enter address"
            role="combobox"
            aria-autocomplete="both"
            aria-controls="sidebar-address-suggestions"
            aria-expanded={omnibox.suggestionsOpen && omnibox.suggestions.length > 0}
            value={addressValue}
            onBlur={() => omnibox.setSuggestionsOpen(false)}
            onChange={(event) => omnibox.updateAddressValue(event.target.value)}
            onFocus={() => omnibox.setSuggestionsOpen(true)}
            onKeyDown={omnibox.onAddressKeyDown}
            aria-activedescendant={omnibox.suggestionsOpen && omnibox.suggestions.length > 0 ? `sidebar-address-suggestion-${omnibox.activeIndex}` : undefined}
          />
        </span>
      </form>
      {omnibox.suggestionsOpen && omnibox.suggestions.length > 0 && (
        <div className="sidebar-omnibox-suggestions" id="sidebar-address-suggestions" role="listbox" aria-label="Sidebar address suggestions">
          {omnibox.suggestions.map((suggestion, index) => {
            const actionHints = getOmniboxActionHints(suggestion);

            return (
              <button
                className="sidebar-omnibox-suggestion"
                id={`sidebar-address-suggestion-${index}`}
                key={suggestion.id}
                type="button"
                role="option"
                aria-selected={index === omnibox.activeIndex}
                onMouseDown={(event) => omnibox.onSuggestionPointerDown(event, suggestion)}
                onMouseEnter={() => omnibox.setActiveSuggestionIndex(index)}
              >
                <span className="sidebar-suggestion-main">
                  <span>{suggestion.title}</span>
                  {actionHints.length > 0 && (
                    <span className="omnibox-action-hints" aria-label={actionHints.map((hint) => `${hint.modifier} ${hint.label}`).join(", ")}>
                      {actionHints.map((hint) => (
                        <span className={`omnibox-action-hint is-${hint.id}`} data-action-hint={hint.id} key={hint.id} aria-hidden="true">
                          {hint.id === "preview" ? <FiEye /> : <FiColumns />}
                        </span>
                      ))}
                    </span>
                  )}
                </span>
                <small>{suggestion.subtitle}</small>
              </button>
            );
          })}
        </div>
      )}
    </div>
  );
}
