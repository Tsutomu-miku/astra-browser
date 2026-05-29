import type { DragEvent, KeyboardEvent, ReactNode } from "react";
import { FiCornerUpRight, FiFolderPlus } from "react-icons/fi";

import { getTabOrganizationTargetKeyboardIntent } from "../../model/tabOrganizationTargetKeyboard";

export function TabOrganizationDropTargets({
  canCreateGroup,
  canUngroup,
  draggingTabId,
  onCreateGroup,
  onUngroupTab,
  setDraggingTabId
}: {
  canCreateGroup: boolean;
  canUngroup: boolean;
  draggingTabId: string | null;
  onCreateGroup: (tabId: string) => void;
  onUngroupTab: (tabId: string) => void;
  setDraggingTabId: (tabId: string | null) => void;
}) {
  if (!draggingTabId || (!canCreateGroup && !canUngroup)) return null;

  return (
    <>
      {canCreateGroup && (
        <TabDropTarget
          icon={<FiFolderPlus />}
          label="New group"
          ariaLabel="Create new group from dragged tab"
          dropTarget="create-group"
          draggingTabId={draggingTabId}
          onDropTab={onCreateGroup}
          setDraggingTabId={setDraggingTabId}
        />
      )}
      {canUngroup && (
        <TabDropTarget
          icon={<FiCornerUpRight />}
          label="Ungroup tab"
          ariaLabel="Remove dragged tab from group"
          dropTarget="ungroup"
          draggingTabId={draggingTabId}
          onDropTab={onUngroupTab}
          setDraggingTabId={setDraggingTabId}
        />
      )}
    </>
  );
}

function TabDropTarget({
  ariaLabel,
  dropTarget,
  draggingTabId,
  icon,
  label,
  onDropTab,
  setDraggingTabId
}: {
  ariaLabel: string;
  dropTarget: "create-group" | "ungroup";
  draggingTabId: string;
  icon: ReactNode;
  label: string;
  onDropTab: (tabId: string) => void;
  setDraggingTabId: (tabId: string | null) => void;
}) {
  const handleDrop = (event: DragEvent<HTMLElement>) => {
    event.preventDefault();
    event.stopPropagation();
    activateTarget();
  };
  const handleKeyDown = (event: KeyboardEvent<HTMLElement>) => {
    const intent = getTabOrganizationTargetKeyboardIntent(event.key);
    if (!intent) return;

    event.preventDefault();
    event.stopPropagation();
    if (intent === "activate") {
      activateTarget();
    } else {
      setDraggingTabId(null);
    }
  };
  const activateTarget = () => {
    onDropTab(draggingTabId);
    setDraggingTabId(null);
  };

  return (
    <button
      className="tab-organization-drop-target"
      type="button"
      aria-label={ariaLabel}
      data-drop-target="true"
      data-tab-organization-target={dropTarget}
      tabIndex={0}
      onDragOver={(event) => {
        event.preventDefault();
        event.dataTransfer.dropEffect = "move";
      }}
      onDrop={handleDrop}
      onKeyDown={handleKeyDown}
    >
      {icon}
      <span>{label}</span>
    </button>
  );
}
