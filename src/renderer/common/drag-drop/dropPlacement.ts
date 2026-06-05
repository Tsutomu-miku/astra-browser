export type DropAxis = "horizontal" | "vertical";
export type DropPlacement = "after" | "before";
export type DropZonePlacement = DropPlacement | "onto";

interface DropPointer {
  clientX: number;
  clientY: number;
}

interface DropPlacementTarget {
  dataset: DOMStringMap;
  getBoundingClientRect: () => DOMRect;
}

export function getPointerDropPlacement(
  target: Pick<DropPlacementTarget, "getBoundingClientRect">,
  pointer: DropPointer,
  axis: DropAxis = "vertical"
): DropPlacement {
  const rect = target.getBoundingClientRect();
  const midpoint = axis === "horizontal"
    ? rect.left + rect.width / 2
    : rect.top + rect.height / 2;
  const pointerPosition = axis === "horizontal" ? pointer.clientX : pointer.clientY;

  return pointerPosition > midpoint ? "after" : "before";
}

/**
 * Tri-state drop zone used by interactions like Arc's "Tidy Tabs":
 *  - "before": near the top/left edge
 *  - "onto":   the middle "dead zone" (creates a folder/group)
 *  - "after":  near the bottom/right edge
 *
 * The middle dead zone is controlled by `ontoRatio` (0–0.5).
 * For example ontoRatio=0.33 means the middle 33% of the target
 * resolves to "onto", with the outer 33.5% on each side resolving
 * to "before" or "after".
 */
export function getPointerDropZone(
  target: Pick<DropPlacementTarget, "getBoundingClientRect">,
  pointer: DropPointer,
  axis: DropAxis = "vertical",
  ontoRatio = 0.33
): DropZonePlacement {
  const rect = target.getBoundingClientRect();
  const size = axis === "horizontal" ? rect.width : rect.height;
  const start = axis === "horizontal" ? rect.left : rect.top;
  const position = axis === "horizontal" ? pointer.clientX : pointer.clientY;
  const relative = (position - start) / size;

  const ontoStart = 0.5 - ontoRatio / 2;
  const ontoEnd = 0.5 + ontoRatio / 2;
  if (relative < ontoStart) return "before";
  if (relative > ontoEnd) return "after";
  return "onto";
}

export function updateDropPlacement(
  target: DropPlacementTarget,
  pointer: DropPointer,
  axis: DropAxis = "vertical"
): DropPlacement {
  const placement = getPointerDropPlacement(target, pointer, axis);
  target.dataset.dropPlacement = placement;
  return placement;
}

export function updateDropZone(
  target: DropPlacementTarget,
  pointer: DropPointer,
  axis: DropAxis = "vertical",
  ontoRatio = 0.33
): DropZonePlacement {
  const placement = getPointerDropZone(target, pointer, axis, ontoRatio);
  target.dataset.dropPlacement = placement;
  return placement;
}

export function clearDropPlacement(target: Pick<DropPlacementTarget, "dataset">) {
  delete target.dataset.dropPlacement;
}
