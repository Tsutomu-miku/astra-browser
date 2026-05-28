import { FiDatabase, FiGlobe, FiGrid, FiLayers } from "react-icons/fi";

import { SETTINGS_SECTIONS, type SettingsSectionId } from "../model/settingsSections";

export function SettingsSectionNav({
  activeSection,
  onSelect
}: {
  activeSection: SettingsSectionId;
  onSelect: (section: SettingsSectionId) => void;
}) {
  return (
    <nav className="settings-section-nav" aria-label="Settings sections">
      {SETTINGS_SECTIONS.map((section) => (
        <button
          className="settings-section-tab"
          key={section.id}
          type="button"
          aria-pressed={section.id === activeSection}
          onClick={() => onSelect(section.id)}
        >
          <span className="settings-section-tab-icon">{getSectionIcon(section.id)}</span>
          <span className="settings-section-tab-copy">
            <span>{section.label}</span>
            <small>{section.summary}</small>
          </span>
        </button>
      ))}
    </nav>
  );
}

function getSectionIcon(section: SettingsSectionId) {
  if (section === "global") return <FiGlobe />;
  if (section === "space") return <FiLayers />;
  if (section === "data") return <FiDatabase />;
  return <FiGrid />;
}
