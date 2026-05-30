export interface ContextMenuAnchorPoint {
  clientX: number;
  clientY: number;
}

export interface ContextMenuViewport {
  innerHeight: number;
  innerWidth: number;
  scrollX: number;
  scrollY: number;
}

export interface ContextMenuPositionOptions {
  edgeGap?: number;
  height: number;
  viewport?: ContextMenuViewport;
  width: number;
}

export const DEFAULT_CONTEXT_MENU_EDGE_GAP = 12;

export function getAnchoredContextMenuPosition(
  point: ContextMenuAnchorPoint,
  {
    edgeGap = DEFAULT_CONTEXT_MENU_EDGE_GAP,
    height,
    viewport = getWindowContextMenuViewport(),
    width
  }: ContextMenuPositionOptions
) {
  const maxLeft = Math.max(edgeGap, viewport.innerWidth - width - edgeGap);
  const maxTop = Math.max(edgeGap, viewport.innerHeight - height - edgeGap);

  return {
    left: viewport.scrollX + clamp(point.clientX, edgeGap, maxLeft),
    top: viewport.scrollY + clamp(point.clientY, edgeGap, maxTop)
  };
}

function getWindowContextMenuViewport(): ContextMenuViewport {
  return {
    innerHeight: window.innerHeight,
    innerWidth: window.innerWidth,
    scrollX: window.scrollX,
    scrollY: window.scrollY
  };
}

function clamp(value: number, min: number, max: number) {
  return Math.min(Math.max(value, min), max);
}
