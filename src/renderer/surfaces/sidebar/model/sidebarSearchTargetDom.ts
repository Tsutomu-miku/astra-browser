import { getSidebarSearchTargetElementId, type SidebarSearchTarget } from "../sidebarFiltering";

export function scrollSidebarSearchTargetIntoView(
  target: SidebarSearchTarget,
  root: Pick<Document, "getElementById"> = document
): boolean {
  const element = root.getElementById(getSidebarSearchTargetElementId(target));
  if (!element) return false;

  element.scrollIntoView?.({ block: "nearest" });
  return true;
}
