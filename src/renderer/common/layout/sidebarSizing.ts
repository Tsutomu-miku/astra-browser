export const SIDEBAR_DEFAULT_WIDTH = 292;
export const SIDEBAR_MIN_WIDTH = 240;
export const SIDEBAR_MAX_WIDTH = 420;
export const SIDEBAR_RESIZE_KEYBOARD_STEP = 24;

export function clampSidebarWidth(width: number): number {
  if (!Number.isFinite(width)) return SIDEBAR_DEFAULT_WIDTH;
  return Math.min(SIDEBAR_MAX_WIDTH, Math.max(SIDEBAR_MIN_WIDTH, Math.round(width)));
}

export function getSidebarPointerResizeWidth({
  currentClientX,
  startClientX,
  startWidth
}: {
  currentClientX: number;
  startClientX: number;
  startWidth: number;
}): number {
  return clampSidebarWidth(startWidth + currentClientX - startClientX);
}

export function getSidebarKeyboardResizeWidth(currentWidth: number, key: string): number | null {
  if (key === "ArrowLeft") return clampSidebarWidth(currentWidth - SIDEBAR_RESIZE_KEYBOARD_STEP);
  if (key === "ArrowRight") return clampSidebarWidth(currentWidth + SIDEBAR_RESIZE_KEYBOARD_STEP);
  if (key === "Home") return SIDEBAR_MIN_WIDTH;
  if (key === "End") return SIDEBAR_MAX_WIDTH;
  return null;
}
