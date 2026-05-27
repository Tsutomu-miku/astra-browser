import type { KeyboardEvent } from "react";
import { FiX } from "react-icons/fi";

import { getSidebarSearchActionHints, type SidebarSearchTarget } from "../sidebarFiltering";

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
  const actionHints = getSidebarSearchActionHints(activeSearchTarget);

  return (
    <div className="sidebar-search">
      <input
        autoComplete="off"
        spellCheck={false}
        aria-label="Search sidebar"
        placeholder="Search sidebar"
        value={query}
        aria-activedescendant={activeSearchTarget ? `sidebar-search-${activeSearchTarget.type}-${activeSearchTarget.id}` : undefined}
        onChange={(event) => onQueryChange(event.target.value)}
        onKeyDown={onKeyDown}
      />
      {query && (
        <button className="icon-button" title="Clear tab search" type="button" onClick={onClear}><FiX /></button>
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
