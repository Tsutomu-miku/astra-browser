import { FiColumns, FiGrid, FiMaximize2, FiMinus, FiMoreHorizontal, FiSidebar } from "react-icons/fi";

import type { BrowserController } from "../../../hooks/types";

export function SplitPaneOverlay({
  controller,
  isPrimary,
  tabId
}: {
  controller: BrowserController;
  isPrimary: boolean;
  tabId: string;
}) {
  const { actions } = controller;

  return (
    <div className="split-pane-overlay" aria-label="Split pane controls">
      <span className="split-pane-handle" title="Split pane"><FiMoreHorizontal /></span>
      {!isPrimary && (
        <button
          type="button"
          title="Make active pane"
          aria-label="Make active pane"
          onClick={() => actions.focusSplitPane(tabId)}
        >
          <FiMaximize2 />
        </button>
      )}
      <button
        type="button"
        title="Horizontal split layout"
        aria-label="Horizontal split layout"
        onClick={() => actions.setSplitLayout("horizontal")}
      >
        <FiColumns />
      </button>
      <button
        type="button"
        title="Vertical split layout"
        aria-label="Vertical split layout"
        onClick={() => actions.setSplitLayout("vertical")}
      >
        <FiSidebar />
      </button>
      <button
        type="button"
        title="Grid split layout"
        aria-label="Grid split layout"
        onClick={() => actions.setSplitLayout("grid")}
      >
        <FiGrid />
      </button>
      <button
        type="button"
        title={isPrimary ? "Close split view" : "Unsplit tab"}
        aria-label={isPrimary ? "Close split view" : "Unsplit tab"}
        onClick={() => {
          isPrimary ? actions.toggleSplitMode() : actions.removeTabFromSplit(tabId);
        }}
      >
        <FiMinus />
      </button>
    </div>
  );
}
