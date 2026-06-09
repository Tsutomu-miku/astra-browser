const { app, BrowserWindow, Menu, shell } = require("electron");

/**
 * Standard native application menu. This is NOT optional for a usable
 * browser: on macOS the menu is the only way the OS forwards native
 * shortcuts (Cmd+Q, Cmd+H, Cmd+C/V/X/Z, Cmd+A, Cmd+P, …) into the webview,
 * and on Windows/Linux it provides the only standard menu bar.
 */
function buildApplicationMenu() {
  const isMac = process.platform === "darwin";

  const sendToFocusedRenderer = (action, ...args) => {
    const win = BrowserWindow.getFocusedWindow();
    if (win && !win.isDestroyed()) {
      win.webContents.send("main-action", action, ...args);
    }
  };

  const withFocusedWindow = (action) => {
    const win = BrowserWindow.getFocusedWindow();
    if (win && !win.isDestroyed()) action(win);
  };

  const withFocusedWebContents = (action) => {
    withFocusedWindow((win) => {
      // The active page lives inside a <webview> in the renderer. The main
      // process cannot reliably walk from the BrowserWindow webContents to
      // the focused <webview> (their hostWebContents relationships don't
      // expose the active one). Instead we dispatch a "print-page" action to
      // the renderer, which knows the active tab's <webview> element and can
      // invoke print on the correct webContents via the `print-webview` IPC.
      // We pass through a default fallback for non-print callers.
      const webContents = win.webContents;
      if (!webContents || webContents.isDestroyed()) return;
      action(webContents);
    });
  };

  const template = [
    // App (macOS) / File (Windows/Linux)
    ...(isMac
      ? [{
        label: app.name,
        submenu: [
          { role: "about" },
          { type: "separator" },
          {
            label: "Settings",
            accelerator: "CmdOrCtrl+,",
            click: () => sendToFocusedRenderer("open-settings-panel")
          },
          { type: "separator" },
          { role: "services" },
          { type: "separator" },
          { role: "hide" },
          { role: "hideOthers" },
          { role: "unhide" },
          { type: "separator" },
          { role: "quit" }
        ]
      }]
      : []),
    {
      label: "File",
      submenu: [
        {
          label: "New Tab",
          accelerator: "CmdOrCtrl+T",
          click: () => sendToFocusedRenderer("new-tab")
        },
        {
          label: "New Window",
          accelerator: isMac ? "Cmd+N" : "Ctrl+N",
          click: () => app.emit("new-window-requested")
        },
        {
          label: "Close Tab",
          accelerator: "CmdOrCtrl+W",
          click: () => sendToFocusedRenderer("close-active-tab")
        },
        { type: "separator" },
        {
          label: "Print…",
          accelerator: "CmdOrCtrl+P",
          click: () => sendToFocusedRenderer("print-page")
        },
        ...(!isMac ? [
          { type: "separator" },
          {
            label: "Settings",
            accelerator: "Ctrl+,",
            click: () => sendToFocusedRenderer("open-settings-panel")
          },
          { type: "separator" },
          { role: "quit" }
        ] : [])
      ]
    },
    {
      label: "Edit",
      submenu: [
        { role: "undo" },
        { role: "redo" },
        { type: "separator" },
        { role: "cut" },
        { role: "copy" },
        { role: "paste" },
        { role: "pasteAndMatchStyle" },
        { role: "selectAll" },
        { type: "separator" },
        {
          label: "Find in Page",
          accelerator: "CmdOrCtrl+F",
          click: () => sendToFocusedRenderer("open-find")
        },
        {
          label: "Find Next",
          accelerator: "CmdOrCtrl+G",
          click: () => sendToFocusedRenderer("find-match", 1)
        },
        {
          label: "Find Previous",
          accelerator: "CmdOrCtrl+Shift+G",
          click: () => sendToFocusedRenderer("find-match", -1)
        }
      ]
    },
    {
      label: "View",
      submenu: [
        {
          label: "Reload",
          accelerator: "CmdOrCtrl+R",
          click: () => sendToFocusedRenderer("reload-page", false)
        },
        {
          label: "Hard Reload",
          accelerator: "CmdOrCtrl+Shift+R",
          click: () => sendToFocusedRenderer("reload-page", true)
        },
        { type: "separator" },
        {
          label: "Actual Size",
          accelerator: "CmdOrCtrl+0",
          click: () => sendToFocusedRenderer("reset-zoom")
        },
        {
          label: "Zoom In",
          accelerator: "CmdOrCtrl+=",
          click: () => sendToFocusedRenderer("zoom-in")
        },
        {
          label: "Zoom Out",
          accelerator: "CmdOrCtrl+-",
          click: () => sendToFocusedRenderer("zoom-out")
        },
        { type: "separator" },
        { role: "togglefullscreen" },
        {
          label: "Toggle Sidebar",
          accelerator: "CmdOrCtrl+B",
          click: () => sendToFocusedRenderer("toggle-sidebar")
        },
        {
          label: "Command Palette",
          accelerator: "CmdOrCtrl+K",
          click: () => sendToFocusedRenderer("open-command")
        },
        {
          label: "Focus Address Bar",
          accelerator: "CmdOrCtrl+L",
          click: () => sendToFocusedRenderer("focus-address")
        },
        { type: "separator" },
        {
          label: "Developer Tools",
          accelerator: isMac ? "Cmd+Alt+I" : "Ctrl+Shift+I",
          click: (menuItem, browserWindow) => browserWindow?.webContents.toggleDevTools()
        }
      ]
    },
    {
      label: "History",
      submenu: [
        {
          label: "Back",
          accelerator: "CmdOrCtrl+[",
          click: () => sendToFocusedRenderer("navigate-history", -1)
        },
        {
          label: "Forward",
          accelerator: "CmdOrCtrl+]",
          click: () => sendToFocusedRenderer("navigate-history", 1)
        },
        { type: "separator" },
        {
          label: "Reopen Closed Tab",
          accelerator: "CmdOrCtrl+Shift+T",
          click: () => sendToFocusedRenderer("restore-closed-tab")
        },
        { type: "separator" },
        {
          label: "Show History",
          accelerator: "CmdOrCtrl+H",
          click: () => sendToFocusedRenderer("open-history")
        }
      ]
    },
    {
      label: "Tab",
      submenu: [
        {
          label: "Select Next Tab",
          accelerator: "Ctrl+Tab",
          click: () => sendToFocusedRenderer("select-adjacent-tab", 1)
        },
        {
          label: "Select Previous Tab",
          accelerator: "Ctrl+Shift+Tab",
          click: () => sendToFocusedRenderer("select-adjacent-tab", -1)
        },
        { type: "separator" },
        {
          label: "Mute/Unmute Tab",
          accelerator: "CmdOrCtrl+M",
          click: () => sendToFocusedRenderer("toggle-active-tab-muted")
        },
        {
          label: "Add to Favorites",
          accelerator: "CmdOrCtrl+D",
          click: () => sendToFocusedRenderer("toggle-active-tab-favorite")
        }
      ]
    },
    {
      label: "Window",
      submenu: [
        { role: "minimize" },
        { role: "zoom" },
        { type: "separator" },
        {
          label: "Downloads",
          accelerator: "CmdOrCtrl+Shift+Y",
          click: () => sendToFocusedRenderer("open-downloads")
        },
        { type: "separator" },
        ...(isMac
          ? [{ role: "front" }, { role: "close" }]
          : [{ role: "close" }])
      ]
    },
    {
      label: "Help",
      submenu: [
        {
          label: "Learn More",
          click: () => shell.openExternal("https://github.com/")
        }
      ]
    }
  ];

  return Menu.buildFromTemplate(template);
}

function installApplicationMenu() {
  Menu.setApplicationMenu(buildApplicationMenu());
}

module.exports = { buildApplicationMenu, installApplicationMenu };
