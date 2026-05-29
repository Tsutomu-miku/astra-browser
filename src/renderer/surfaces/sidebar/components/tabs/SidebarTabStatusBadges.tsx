import { FiColumns, FiMoon, FiVolumeX } from "react-icons/fi";

import type { TabStatusBadge } from "../../model/sidebarItemState";

export function SidebarTabStatusBadges({
  badges,
  variant = "row"
}: {
  badges: TabStatusBadge[];
  variant?: "pinned" | "row";
}) {
  if (badges.length === 0) return null;

  const badgeClassName = variant === "pinned" ? "pinned-tab-status-badge" : "tab-status-badge";
  const containerClassName = variant === "pinned" ? "pinned-tab-status-badges" : "tab-status-badges";

  return (
    <span className={containerClassName} aria-label={badges.map((badge) => badge.label).join(", ")}>
      {badges.map((badge) => (
        <span className={`${badgeClassName} is-${badge.id}`} key={badge.id} aria-hidden="true">
          <TabStatusIcon badge={badge} />
          {variant === "row" && <span>{badge.label}</span>}
        </span>
      ))}
    </span>
  );
}

function TabStatusIcon({ badge }: { badge: TabStatusBadge }) {
  if (badge.id === "split") return <FiColumns />;
  if (badge.id === "muted") return <FiVolumeX />;
  return <FiMoon />;
}
