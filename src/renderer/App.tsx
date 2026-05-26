import { useBrowserController } from "./hooks/useBrowserController";
import { CommandPalette } from "./surfaces/command/CommandPalette";
import { FindBar } from "./surfaces/find/FindBar";
import { GlancePanel } from "./surfaces/glance/GlancePanel";
import { DownloadsPanel, HistoryPanel, SettingsPanel } from "./surfaces/panels/Panels";
import { SiteInfoPanel } from "./surfaces/panels/SiteInfoPanel";
import { PermissionPrompt } from "./surfaces/permissions/PermissionPrompt";
import { Sidebar } from "./surfaces/sidebar/Sidebar";
import { Topbar } from "./surfaces/topbar/Topbar";
import { WebviewGrid } from "./surfaces/webview/WebviewGrid";

export function App() {
  const controller = useBrowserController();

  return (
    <>
      <Sidebar controller={controller} />
      <main className={`browser ${controller.compactMode ? "is-compact-mode" : ""}`}>
        <Topbar controller={controller} />
        <WebviewGrid controller={controller} />
        {controller.findOpen && <FindBar controller={controller} />}
      </main>
      {controller.panel === "history" && <HistoryPanel controller={controller} />}
      {controller.panel === "downloads" && <DownloadsPanel controller={controller} />}
      {controller.panel === "settings" && <SettingsPanel controller={controller} />}
      {controller.panel === "site" && <SiteInfoPanel controller={controller} />}
      {controller.glance && <GlancePanel controller={controller} />}
      {controller.permissionRequest && <PermissionPrompt controller={controller} />}
      {controller.commandOpen && <CommandPalette controller={controller} />}
    </>
  );
}
