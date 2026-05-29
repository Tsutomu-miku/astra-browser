import { useEffect, useState, type KeyboardEvent, type PointerEvent } from "react";

import {
  SIDEBAR_MAX_WIDTH,
  SIDEBAR_MIN_WIDTH,
  SIDEBAR_DEFAULT_WIDTH,
  clampSidebarWidth,
  getSidebarKeyboardResizeWidth,
  getSidebarPointerResizeWidth
} from "../../../../common/layout/sidebarSizing";

interface SidebarResizeHandleProps {
  isCollapsed: boolean;
  onResize: (width: number) => void;
  width: number;
}

interface DragState {
  pointerId: number;
  startClientX: number;
  startWidth: number;
}

export function SidebarResizeHandle({ isCollapsed, onResize, width }: SidebarResizeHandleProps) {
  const [dragState, setDragState] = useState<DragState | null>(null);
  const clampedWidth = clampSidebarWidth(width);

  useEffect(() => {
    if (!dragState) return;

    const handlePointerMove = (event: globalThis.PointerEvent) => {
      if (event.pointerId !== dragState.pointerId) return;
      onResize(getSidebarPointerResizeWidth({
        currentClientX: event.clientX,
        startClientX: dragState.startClientX,
        startWidth: dragState.startWidth
      }));
    };
    const handlePointerUp = (event: globalThis.PointerEvent) => {
      if (event.pointerId === dragState.pointerId) setDragState(null);
    };

    window.addEventListener("pointermove", handlePointerMove);
    window.addEventListener("pointerup", handlePointerUp);
    window.addEventListener("pointercancel", handlePointerUp);
    return () => {
      window.removeEventListener("pointermove", handlePointerMove);
      window.removeEventListener("pointerup", handlePointerUp);
      window.removeEventListener("pointercancel", handlePointerUp);
    };
  }, [dragState, onResize]);

  if (isCollapsed) return null;

  function onPointerDown(event: PointerEvent<HTMLDivElement>) {
    if (event.button !== 0) return;
    event.preventDefault();
    event.currentTarget.setPointerCapture?.(event.pointerId);
    setDragState({
      pointerId: event.pointerId,
      startClientX: event.clientX,
      startWidth: clampedWidth
    });
  }

  function onKeyDown(event: KeyboardEvent<HTMLDivElement>) {
    const nextWidth = getSidebarKeyboardResizeWidth(clampedWidth, event.key);
    if (nextWidth === null) return;

    event.preventDefault();
    onResize(nextWidth);
  }

  return (
    <div
      className="sidebar-resize-handle"
      role="separator"
      aria-label="Resize sidebar"
      aria-orientation="vertical"
      aria-valuemin={SIDEBAR_MIN_WIDTH}
      aria-valuemax={SIDEBAR_MAX_WIDTH}
      aria-valuenow={clampedWidth}
      tabIndex={0}
      data-dragging={Boolean(dragState)}
      onDoubleClick={() => onResize(SIDEBAR_DEFAULT_WIDTH)}
      onKeyDown={onKeyDown}
      onPointerDown={onPointerDown}
    />
  );
}
