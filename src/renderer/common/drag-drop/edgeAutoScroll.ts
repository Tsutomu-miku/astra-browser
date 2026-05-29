export interface EdgeAutoScrollOptions {
  edgeSize?: number;
  maxDelta?: number;
  minDelta?: number;
}

const DEFAULT_EDGE_SIZE = 44;
const DEFAULT_MAX_DELTA = 18;
const DEFAULT_MIN_DELTA = 3;

export function getEdgeAutoScrollDelta(
  element: HTMLElement,
  clientY: number,
  options: EdgeAutoScrollOptions = {}
): number {
  const rect = element.getBoundingClientRect();
  if (rect.height <= 0) return 0;

  const edgeSize = Math.max(1, Math.min(options.edgeSize ?? DEFAULT_EDGE_SIZE, rect.height / 2));
  const maxDelta = Math.max(1, options.maxDelta ?? DEFAULT_MAX_DELTA);
  const minDelta = Math.max(1, Math.min(options.minDelta ?? DEFAULT_MIN_DELTA, maxDelta));
  const topDistance = clientY - rect.top;
  const bottomDistance = rect.bottom - clientY;

  if (topDistance < edgeSize) {
    return -getScaledEdgeDelta(topDistance, edgeSize, minDelta, maxDelta);
  }

  if (bottomDistance < edgeSize) {
    return getScaledEdgeDelta(bottomDistance, edgeSize, minDelta, maxDelta);
  }

  return 0;
}

export function scrollElementNearEdge(
  element: HTMLElement,
  clientY: number,
  options: EdgeAutoScrollOptions = {}
): boolean {
  const delta = getEdgeAutoScrollDelta(element, clientY, options);
  if (delta === 0) return false;

  const currentScrollTop = element.scrollTop;
  const maxScrollTop = Math.max(0, element.scrollHeight - element.clientHeight);
  element.scrollTop = Math.max(0, Math.min(maxScrollTop, currentScrollTop + delta));
  return element.scrollTop !== currentScrollTop;
}

function getScaledEdgeDelta(
  distanceFromEdge: number,
  edgeSize: number,
  minDelta: number,
  maxDelta: number
): number {
  const clampedDistance = Math.max(0, Math.min(edgeSize, distanceFromEdge));
  const intensity = (edgeSize - clampedDistance) / edgeSize;
  return Math.ceil(minDelta + intensity * (maxDelta - minDelta));
}
