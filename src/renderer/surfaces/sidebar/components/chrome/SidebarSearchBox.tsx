import { useRef, type KeyboardEvent } from "react";
import { FiX } from "react-icons/fi";

import { SidebarModifierActionHints } from "../common/SidebarModifierActionHints";
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
  query,
  resultCount
}: {
  activeSearchTarget?: SidebarSearchTarget;
  onClear: () => void;
  onKeyDown: (event: KeyboardEvent<HTMLInputElement>) => void;
  onQueryChange: (query: string) => void;
  query: string;
  resultCount?: number;
}) {
  const inputRef = useRef<HTMLInputElement | null>(null);
  const actionHints = getSidebarSearchActionHints(activeSearchTarget);
  const showSearchMeta = query && typeof resultCount === "number";
  const resultLabel = typeof resultCount === "number"
    ? resultCount === 0
      ? "No matches"
      : `${resultCount} ${resultCount === 1 ? "result" : "results"}`
    : "";
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
          type="button"
          aria-label="Clear sidebar search"
          onClick={clearSearch}
        >
          <FiX />
        </button>
      )}
      {showSearchMeta && (
        <div className="sidebar-search-meta">
          <span className="sidebar-search-status" role="status" aria-live="polite">
            {resultLabel}
          </span>
          {actionHints.length > 0 && (
            <SidebarModifierActionHints
              className="sidebar-search-action-hints"
              hintClassName="sidebar-search-action-hint"
              hints={actionHints}
            />
          )}
        </div>
      )}
    </div>
  );
}
