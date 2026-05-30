import { useState, type FormEvent } from "react";
import { FiClock, FiCommand, FiSearch, FiStar, FiZap } from "react-icons/fi";

import { getOmniboxActionHints } from "../../../common/omnibox/omniboxActions";
import type { OmniboxSuggestion } from "../../../common/omnibox/omniboxSuggestions";
import type { BrowserController } from "../../../app/controller/types";
import { useOmniboxController } from "../../../app/controller/useOmniboxController";

export function StartSearch({
  controller,
  isVisible
}: {
  controller: BrowserController;
  isVisible: boolean;
}) {
  const { actions, state } = controller;
  const [query, setQuery] = useState("");
  const omnibox = useOmniboxController({
    actions,
    addressValue: query,
    setAddressValue: setQuery,
    state
  });
  const suggestionsVisible = omnibox.suggestionsOpen && omnibox.suggestions.length > 0;

  function submit(event: FormEvent) {
    if (!query.trim() && omnibox.suggestions.length === 0) {
      event.preventDefault();
      return;
    }

    omnibox.submitAddress(event);
  }

  return (
    <div className="start-search-area">
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
          onBlur={() => omnibox.setSuggestionsOpen(false)}
          onChange={(event) => omnibox.updateAddressValue(event.target.value)}
          onFocus={() => omnibox.setSuggestionsOpen(true)}
          onKeyDown={omnibox.onAddressKeyDown}
          aria-activedescendant={suggestionsVisible ? `start-search-suggestion-${omnibox.activeIndex}` : undefined}
        />
      </form>
      {suggestionsVisible && (
        <div className="start-search-suggestions" role="listbox" aria-label="Start search suggestions">
          {omnibox.suggestions.map((suggestion, index) => (
            <button
              className="start-search-suggestion"
              id={`start-search-suggestion-${index}`}
              key={suggestion.id}
              type="button"
              role="option"
              aria-selected={index === omnibox.activeIndex}
              onMouseDown={(event) => omnibox.onSuggestionPointerDown(event, suggestion)}
              onMouseEnter={() => omnibox.setActiveSuggestionIndex(index)}
            >
              <span className="start-search-suggestion-icon">
                <StartSuggestionIcon suggestion={suggestion} />
              </span>
              <span className="start-search-suggestion-copy">
                <span>{suggestion.title}</span>
                <small>{suggestion.subtitle}</small>
              </span>
              <span className="start-search-action-hints" aria-label="Alt Split">
                {getOmniboxActionHints(suggestion).map((hint) => (
                  <span className={`start-search-action-hint is-${hint.id}`} key={hint.id}>
                    <kbd>{hint.modifier}</kbd>
                    <span>{hint.label}</span>
                  </span>
                ))}
              </span>
            </button>
          ))}
        </div>
      )}
    </div>
  );
}

function StartSuggestionIcon({ suggestion }: { suggestion: OmniboxSuggestion }) {
  if (suggestion.type === "essential") return <FiZap />;
  if (suggestion.type === "favorite") return <FiStar />;
  if (suggestion.type === "history") return <FiClock />;
  if (suggestion.type === "tab") return <FiCommand />;
  return <FiSearch />;
}
