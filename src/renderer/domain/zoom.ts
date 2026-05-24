export const DEFAULT_ZOOM_FACTOR = 1;
export const MAX_ZOOM_FACTOR = 3;
export const MIN_ZOOM_FACTOR = 0.25;
export const ZOOM_STEP = 0.1;

export function normalizeZoomFactor(value: unknown): number {
  const zoom = Number(value);
  if (!Number.isFinite(zoom)) {
    return DEFAULT_ZOOM_FACTOR;
  }

  return clampZoomFactor(zoom);
}

export function stepZoomFactor(value: number, direction: 1 | -1): number {
  return clampZoomFactor(Number((value + ZOOM_STEP * direction).toFixed(2)));
}

export function formatZoomPercent(value: number): string {
  return `${Math.round(normalizeZoomFactor(value) * 100)}%`;
}

function clampZoomFactor(value: number): number {
  return Math.min(MAX_ZOOM_FACTOR, Math.max(MIN_ZOOM_FACTOR, value));
}
