import {
  FiActivity,
  FiAward,
  FiBookOpen,
  FiCheckCircle,
  FiCopy,
  FiDatabase,
  FiDownloadCloud,
  FiEye,
  FiFeather,
  FiGlobe,
  FiGrid,
  FiHome,
  FiImage,
  FiLayers,
  FiPrinter,
  FiRefreshCw,
  FiSettings,
  FiShield,
  FiSliders,
  FiTerminal,
  FiUsers
} from "react-icons/fi";

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
  switch (section) {
    case "you-and-astra":
      return <FiUsers />;
    case "autofill":
      return <FiCopy />;
    case "privacy-and-security":
      return <FiShield />;
    case "appearance":
    case "global":
      return <FiEye />;
    case "search-engine":
      return <FiBookOpen />;
    case "default-browser":
      return <FiCheckCircle />;
    case "startup":
      return <FiHome />;
    case "site-settings":
      return <FiSliders />;
    case "performance":
      return <FiActivity />;
    case "accessibility":
      return <FiFeather />;
    case "extensions":
      return <FiGrid />;
    case "languages":
      return <FiGlobe />;
    case "downloads":
      return <FiDownloadCloud />;
    case "print":
      return <FiPrinter />;
    case "system":
      return <FiTerminal />;
    case "reset-and-cleanup":
      return <FiRefreshCw />;
    case "about":
      return <FiAward />;
    case "space":
    case "workspaces":
      return <FiLayers />;
    case "data":
      return <FiDatabase />;
    default:
      return <FiSettings />;
  }
}
