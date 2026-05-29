export type DropAxis = "horizontal" | "vertical";
export type DropPlacement = "after" | "before";

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

export function updateDropPlacement(
  target: DropPlacementTarget,
  pointer: DropPointer,
  axis: DropAxis = "vertical"
): DropPlacement {
  const placement = getPointerDropPlacement(target, pointer, axis);
  target.dataset.dropPlacement = placement;
  return placement;
}

export function clearDropPlacement(target: Pick<DropPlacementTarget, "dataset">) {
  delete target.dataset.dropPlacement;
}
