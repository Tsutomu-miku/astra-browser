import { type KeyboardEvent } from "react";
import { FiChevronDown, FiChevronRight } from "react-icons/fi";

import { getDisclosureKeyboardToggleIntent } from "../../../../common/disclosure/disclosureKeyboard";

export function SidebarSectionHeader({
  count,
  dropLabel,
  isCollapsed = false,
  onToggle,
  title
}: {
  count: number;
  dropLabel?: string;
  isCollapsed?: boolean;
  onToggle?: () => void;
  title: string;
}) {
  const handleKeyboardToggle = (event: KeyboardEvent<HTMLButtonElement>) => {
    const intent = getDisclosureKeyboardToggleIntent(event.key, isCollapsed);
    if (!intent) return;

    event.preventDefault();
    event.stopPropagation();
    onToggle?.();
  };

  const content = (
    <>
      <span className="sidebar-section-title">
        {onToggle && (isCollapsed ? <FiChevronRight /> : <FiChevronDown />)}
        <span>{title}</span>
      </span>
      {dropLabel ? (
        <span className="sidebar-section-drop-label">{dropLabel}</span>
      ) : (
        <span className="sidebar-section-count">{count}</span>
      )}
    </>
  );

  return (
    <header className="sidebar-section-header" data-collapsed={isCollapsed}>
      {onToggle ? (
        <button
          className="sidebar-section-header-button"
          type="button"
          data-collapsed={isCollapsed}
          aria-expanded={!isCollapsed}
          aria-label={`${isCollapsed ? "Expand" : "Collapse"} ${title}`}
          onClick={onToggle}
          onKeyDown={handleKeyboardToggle}
        >
          {content}
        </button>
      ) : content}
    </header>
  );
}
