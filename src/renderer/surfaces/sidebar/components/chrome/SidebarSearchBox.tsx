import { useRef, type KeyboardEvent } from "react";
import { FiX } from "react-icons/fi";

import {
  getSidebarSearchActionHints,
  getSidebarSearchTargetElementId,
  type SidebarSearchTarget
} from "../../sidebarFiltering";

export function SidebarSearchBox({
  activeSearchTarget,
  onClear,
  onKeyDown,
  onQueryChange,
  query
}: {
  activeSearchTarget?: SidebarSearchTarget;
  onClear: () => void;
  onKeyDown: (event: KeyboardEvent<HTMLInputElement>) => void;
  onQueryChange: (query: string) => void;
  query: string;
}) {
  const inputRef = useRef<HTMLInputElement | null>(null);
  const actionHints = getSidebarSearchActionHints(activeSearchTarget);
  const clearSearch = () => {
    onClear();
    inputRef.current?.focus();
  };

  return (
    <div className="sidebar-search">
      <input
        ref={inputRef}
        autoComplete="off"
        spellCheck={false}
        aria-label="Search sidebar"
        placeholder="Search sidebar"
        value={query}
        aria-activedescendant={activeSearchTarget ? getSidebarSearchTargetElementId(activeSearchTarget) : undefined}
        onChange={(event) => onQueryChange(event.target.value)}
        onKeyDown={onKeyDown}
      />
      {query && (
        <button
          className="icon-button"
          title="Clear sidebar search"
          type="button"
          aria-label="Clear sidebar search"
          onClick={clearSearch}
        >
          <FiX />
        </button>
      )}
      {query && actionHints.length > 0 && (
        <div className="sidebar-search-action-hints" aria-label={actionHints.map((hint) => `${hint.modifier} ${hint.label}`).join(", ")}>
          {actionHints.map((hint) => (
            <span className={`sidebar-search-action-hint is-${hint.id}`} key={hint.id}>
              <kbd>{hint.modifier}</kbd>
              <span>{hint.label}</span>
            </span>
          ))}
        </div>
      )}
    </div>
  );
}
