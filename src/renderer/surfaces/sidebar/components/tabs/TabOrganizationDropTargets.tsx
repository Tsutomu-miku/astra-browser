import type { DragEvent, ReactNode } from "react";
import { FiCornerUpRight, FiFolderPlus } from "react-icons/fi";

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
  draggingTabId,
  icon,
  label,
  onDropTab,
  setDraggingTabId
}: {
  ariaLabel: string;
  draggingTabId: string;
  icon: ReactNode;
  label: string;
  onDropTab: (tabId: string) => void;
  setDraggingTabId: (tabId: string | null) => void;
}) {
  const handleDrop = (event: DragEvent<HTMLElement>) => {
    event.preventDefault();
    const tabId = draggingTabId || event.dataTransfer.getData("text/plain");
    if (tabId) onDropTab(tabId);
    setDraggingTabId(null);
  };

  return (
    <div
      className="tab-organization-drop-target"
      role="button"
      aria-label={ariaLabel}
      data-drop-target="true"
      onDragOver={(event) => {
        event.preventDefault();
        event.dataTransfer.dropEffect = "move";
      }}
      onDrop={handleDrop}
    >
      {icon}
      <span>{label}</span>
    </div>
  );
}
