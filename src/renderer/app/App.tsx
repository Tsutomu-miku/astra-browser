import { useEffect } from "react";

import { getChromeAccent } from "../common/theme/chromeTheme";
import { useBrowserController } from "./controller/useBrowserController";
import { CommandPalette } from "../surfaces/command/CommandPalette";
import { FindBar } from "../surfaces/find/FindBar";
import { GlancePanel } from "../surfaces/glance/GlancePanel";
import { DownloadsPanel, HistoryPanel, SettingsPanel } from "../surfaces/panels/Panels";
import { SiteInfoPanel } from "../surfaces/panels/SiteInfoPanel";
import { PermissionPrompt } from "../surfaces/permissions/PermissionPrompt";
import { Sidebar } from "../surfaces/sidebar/Sidebar";
import { Topbar } from "../surfaces/topbar/Topbar";
import { WebviewGrid } from "../surfaces/webview/WebviewGrid";

export function App() {
  const controller = useBrowserController();
  const chromeAccent = getChromeAccent(controller.state.settings, controller.activeWorkspace);

  useEffect(() => {
    document.documentElement.style.setProperty("--accent", chromeAccent);
    return () => {
      document.documentElement.style.removeProperty("--accent");
    };
  }, [chromeAccent]);

  return (
    <>
      <Sidebar controller={controller} />
      <main className={`browser ${controller.compactMode ? "is-compact-mode" : ""} ${controller.compactToolbarPeeking ? "is-peeking-chrome" : ""} ${controller.floatingToolbarOpen ? "is-floating-toolbar-open" : ""}`}>
        {controller.compactMode && (
          <button
            className="compact-peek-zone"
            type="button"
            aria-label="Show toolbar"
            title="Show toolbar"
            onClick={controller.actions.peekCompactToolbar}
            onBlur={controller.releaseCompactToolbar}
            onFocus={controller.holdCompactToolbar}
            onPointerEnter={controller.holdCompactToolbar}
            onPointerLeave={controller.releaseCompactToolbar}
          />
        )}
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
