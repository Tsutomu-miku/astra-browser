/* SidebarItems used to hold both TabRow and FavoriteButton. They grew large
 * enough to cross the 300-line architectural threshold, so each has been
 * split into its own file. This barrel preserves the old import paths. */

export { TabRow } from "./SidebarTabRow";
export type { TabDropInfo } from "./SidebarTabRow";
export { FavoriteButton } from "./SidebarFavoriteButton";
