import { useEffect, useMemo, useState, type FormEvent, type KeyboardEvent, type MouseEvent } from "react";
import { FiSearch } from "react-icons/fi";

import { isListNavigationKey } from "../../../common/navigation/listNavigation";
import { buildOmniboxSuggestions, type OmniboxSuggestion } from "../../../hooks/omniboxSuggestions";
import {
  clampOmniboxIndex,
  getNextOmniboxIndex
} from "../../../hooks/omniboxSelection";
import type { BrowserController } from "../../../hooks/types";

export function SidebarAddress({ controller }: { controller: BrowserController }) {
  const { actions, addressValue, compactMode, setAddressValue, state } = controller;
  const [suggestionsOpen, setSuggestionsOpen] = useState(false);
  const [activeSuggestionIndex, setActiveSuggestionIndex] = useState(0);
  const suggestions = useMemo(() => buildOmniboxSuggestions(state, addressValue), [addressValue, state]);
  const activeIndex = clampOmniboxIndex(activeSuggestionIndex, suggestions.length);

  useEffect(() => {
    setActiveSuggestionIndex((index) => clampOmniboxIndex(index, suggestions.length));
  }, [suggestions.length]);

  function submitAddress(event: FormEvent) {
    event.preventDefault();
    runSuggestion(suggestionsOpen ? suggestions[activeIndex] : undefined);
  }

  function onAddressKeyDown(event: KeyboardEvent<HTMLInputElement>) {
    if (!suggestionsOpen && isListNavigationKey(event.key)) {
      setSuggestionsOpen(true);
    }

    if (isListNavigationKey(event.key)) {
      event.preventDefault();
      const key = event.key;
      setActiveSuggestionIndex((index) => getNextOmniboxIndex(index, suggestions.length, key));
    } else if (event.key === "Enter") {
      event.preventDefault();
      runSuggestion(suggestionsOpen ? suggestions[activeIndex] : undefined, event.altKey);
    } else if (event.key === "Escape") {
      setSuggestionsOpen(false);
    }
  }

  function onSuggestionPointerDown(event: MouseEvent, suggestion: OmniboxSuggestion) {
    event.preventDefault();
    runSuggestion(suggestion, event.altKey);
  }

  function runSuggestion(suggestion: OmniboxSuggestion | undefined, openInSplit = false) {
    switch (suggestion?.type) {
      case "tab":
        openInSplit ? actions.openTabInSplit(suggestion.tabId) : actions.selectTab(suggestion.tabId);
        break;
      case "essential":
      case "favorite":
      case "history":
        openInSplit ? actions.openUrlInSplit(suggestion.url, suggestion.title) : actions.navigateActiveTab(suggestion.url);
        break;
      case "navigate":
        openInSplit ? actions.openUrlInSplit(suggestion.value, suggestion.title) : actions.navigateActiveTab(suggestion.value);
        break;
      default:
        openInSplit ? actions.openUrlInSplit(addressValue) : actions.navigateActiveTab(addressValue);
    }
    setSuggestionsOpen(false);
  }

  return (
    <div className="sidebar-address" data-compact={compactMode}>
      <form className="sidebar-address-form" onSubmit={submitAddress}>
        <FiSearch />
        <input
          id="sidebarAddressInput"
          autoComplete="off"
          inputMode="url"
          spellCheck={false}
          aria-label="Sidebar address"
          placeholder="Search or enter address"
          value={addressValue}
          onBlur={() => setSuggestionsOpen(false)}
          onChange={(event) => {
            setAddressValue(event.target.value);
            setSuggestionsOpen(true);
            setActiveSuggestionIndex(0);
          }}
          onFocus={() => setSuggestionsOpen(true)}
          onKeyDown={onAddressKeyDown}
          aria-activedescendant={suggestionsOpen && suggestions.length > 0 ? `sidebar-address-suggestion-${activeIndex}` : undefined}
        />
      </form>
      {suggestionsOpen && suggestions.length > 0 && (
        <div className="sidebar-omnibox-suggestions" role="listbox" aria-label="Sidebar address suggestions">
          {suggestions.map((suggestion, index) => (
            <button
              className="sidebar-omnibox-suggestion"
              id={`sidebar-address-suggestion-${index}`}
              key={suggestion.id}
              type="button"
              aria-selected={index === activeIndex}
              onMouseDown={(event) => onSuggestionPointerDown(event, suggestion)}
              onMouseEnter={() => setActiveSuggestionIndex(index)}
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
