import type { KeyboardEvent } from "react";
import { FiX } from "react-icons/fi";

import type { SidebarSearchTarget } from "../sidebarFiltering";

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
    </div>
  );
}
