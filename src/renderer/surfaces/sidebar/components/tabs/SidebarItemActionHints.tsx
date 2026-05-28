export function SidebarItemActionHints() {
  return (
    <span className="sidebar-item-action-hints" aria-label="Alt Preview, Shift Split">
      <span className="sidebar-item-action-hint is-preview">
        <kbd>Alt</kbd>
        <span>Preview</span>
      </span>
      <span className="sidebar-item-action-hint is-split">
        <kbd>Shift</kbd>
        <span>Split</span>
      </span>
    </span>
  );
}
