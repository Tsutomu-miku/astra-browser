export type SidebarSectionId = "essentials" | "favorites" | "pinned" | "recentlyClosed" | "tabs";

export function hasSidebarSectionDragReveal(
  sectionId: SidebarSectionId,
  dragging: {
    essentialId: string | null;
    favoriteId: string | null;
    tabId: string | null;
  }
) {
  if (sectionId === "essentials") return Boolean(dragging.essentialId);
  if (sectionId === "favorites") return Boolean(dragging.favoriteId);
  return false;
}
