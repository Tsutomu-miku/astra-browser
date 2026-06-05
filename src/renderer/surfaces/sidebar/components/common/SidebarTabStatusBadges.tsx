import { FiCamera, FiColumns, FiMic, FiRadio, FiVolume2, FiVolumeX } from "react-icons/fi";

import type { TabStatusBadge } from "../../model/sidebarItemState";

export function SidebarTabStatusBadges({
  badges
}: {
  badges: TabStatusBadge[];
}) {
  if (badges.length === 0) return null;

  return (
    <span className="tab-status-badges" data-sidebar-tab-status-badges="true" aria-label={badges.map((badge) => badge.label).join(", ")}>
      {badges.map((badge) => (
        <span className={`tab-status-badge is-${badge.id}`} data-sidebar-tab-status-badge="true" key={badge.id} aria-hidden="true">
          <TabStatusIcon badge={badge} />
        </span>
      ))}
    </span>
  );
}

function TabStatusIcon({ badge }: { badge: TabStatusBadge }) {
  switch (badge.id) {
    case "split":
      return <FiColumns />;
    case "camera":
      return <FiCamera />;
    case "microphone":
      return <FiMic />;
    case "media-playing":
      return <FiVolume2 />;
    case "muted":
      return <FiVolumeX />;
    case "unread":
      return <FiRadio />;
  }
}
