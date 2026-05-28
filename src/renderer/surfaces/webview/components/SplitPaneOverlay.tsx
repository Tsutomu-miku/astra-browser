import { FiColumns, FiGrid, FiMaximize2, FiMinus, FiMoreHorizontal, FiSidebar } from "react-icons/fi";

import type { BrowserController } from "../../../app/controller/types";

export function SplitPaneOverlay({
  controller,
  isPrimary,
  tabId,
  title
}: {
  controller: BrowserController;
  isPrimary: boolean;
  tabId: string;
  title: string;
}) {
  const { actions, splitLayout } = controller;
  const paneTitle = title.trim() || "Untitled pane";

  return (
    <div className="split-pane-overlay" aria-label={`${paneTitle} split pane controls`}>
      <span className="split-pane-handle" title="Split pane"><FiMoreHorizontal /></span>
      <span className={`split-pane-state ${isPrimary ? "is-active" : ""}`}>
        {isPrimary ? "Active" : "Split"}
      </span>
      {isPrimary ? (
        <span className="split-pane-title" title={paneTitle}>{paneTitle}</span>
      ) : (
        <button
          className="split-pane-title"
          type="button"
          title={`Make ${paneTitle} active`}
          aria-label={`Make ${paneTitle} active pane`}
          onClick={() => actions.focusSplitPane(tabId)}
        >
          {paneTitle}
        </button>
      )}
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
        aria-pressed={splitLayout === "horizontal"}
        onClick={() => actions.setSplitLayout("horizontal")}
      >
        <FiColumns />
      </button>
      <button
        type="button"
        title="Vertical split layout"
        aria-label="Vertical split layout"
        aria-pressed={splitLayout === "vertical"}
        onClick={() => actions.setSplitLayout("vertical")}
      >
        <FiSidebar />
      </button>
      <button
        type="button"
        title="Grid split layout"
        aria-label="Grid split layout"
        aria-pressed={splitLayout === "grid"}
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
