import { FiColumns, FiEye } from "react-icons/fi";

export function SidebarItemActionHints() {
  return (
    <span className="sidebar-item-action-hints" aria-hidden="true">
      <span className="sidebar-item-action-hint is-preview" data-action-hint="preview">
        <FiEye />
      </span>
      <span className="sidebar-item-action-hint is-split" data-action-hint="split">
        <FiColumns />
      </span>
    </span>
  );
}
