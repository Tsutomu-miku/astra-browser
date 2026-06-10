import { useEffect } from "react";

import { getChromeAccent } from "../common/theme/chromeTheme";
import { getThemeDefinition } from "../common/theme/themePalette";
import { useBrowserController } from "./controller/useBrowserController";
import { CommandPalette } from "../surfaces/command/CommandPalette";
import { FindBar } from "../surfaces/find/FindBar";
import { GlancePanel } from "../surfaces/glance/GlancePanel";
import { DownloadsPanel, HistoryPanel, SettingsPanel } from "../surfaces/panels/Panels";
import { SiteInfoPanel } from "../surfaces/panels/SiteInfoPanel";
import { PermissionPrompt } from "../surfaces/permissions/PermissionPrompt";
import { SafeBrowsingPrompt } from "../surfaces/permissions/SafeBrowsingPrompt";
import { Sidebar } from "../surfaces/sidebar/Sidebar";
import { Topbar } from "../surfaces/topbar/Topbar";
import { WebviewGrid } from "../surfaces/webview/WebviewGrid";

export function App() {
  const controller = useBrowserController();
  const chromeAccent = getChromeAccent(controller.state.settings, controller.activeWorkspace);
  const theme = controller.state.settings.theme;
  const themeDef = getThemeDefinition(theme);

  useEffect(() => {
    document.documentElement.setAttribute("data-theme", theme);
    document.documentElement.style.setProperty("--accent", chromeAccent);
    document.documentElement.style.setProperty("--accent-2", themeDef.accent2);
    return () => {
      document.documentElement.removeAttribute("data-theme");
      document.documentElement.style.removeProperty("--accent");
      document.documentElement.style.removeProperty("--accent-2");
    };
  }, [theme, chromeAccent, themeDef.accent2]);

  // Expose sidebar width on <body> so the shell grid (base.css) can consume it
  // as the first column. Otherwise dragging the resize handle only changes the
  // sidebar's internal width and the main view stays at the default offset.
  useEffect(() => {
    document.body.style.setProperty("--sidebar-width", `${controller.sidebarWidth}px`);
    document.body.dataset.sidebarCollapsed = String(controller.sidebarCollapsed);
    return () => {
      document.body.style.removeProperty("--sidebar-width");
      document.body.removeAttribute("data-sidebar-collapsed");
    };
  }, [controller.sidebarWidth, controller.sidebarCollapsed]);

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
      {controller.safeBrowsingAlert && (
        <SafeBrowsingPrompt
          alert={controller.safeBrowsingAlert}
          onGoBack={() => controller.dismissSafeBrowsingAlert()}
          onProceed={() => controller.dismissSafeBrowsingAlert()}
        />
      )}
      {controller.commandOpen && <CommandPalette controller={controller} />}
    </>
  );
}
