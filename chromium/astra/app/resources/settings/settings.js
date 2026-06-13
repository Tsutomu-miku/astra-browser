// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Astra Settings — JavaScript
 * =========================================================================
 *
 * Client-side JS for the Astra settings page (astra://settings).
 * Communicates with the browser side via WebUIMessageHandler
 * (astra_settings_handler.cc) using chrome.send() / window.domAutomationController.
 *
 * Message protocol (JS -> C++):
 *   - getAllSettings(cbId) -> all settings as nested object
 *   - getGeneralSettings(cbId) -> {startupBehavior, defaultSearchEngine}
 *   - setGeneralSettings(cbId, settingsObj) -> boolean success
 *   - getSidebarSettings(cbId) -> {position, width, autoHide, enabled}
 *   - setSidebarSettings(cbId, settingsObj) -> boolean success
 *   - getWorkspaceSettings(cbId) -> {defaultWorkspaceId, accentColorPresets}
 *   - setWorkspaceSettings(cbId, settingsObj) -> boolean success
 *   - getFocusModeSettings(cbId) -> {defaultDurationMinutes, blocklist, soundEnabled, ...}
 *   - setFocusModeSettings(cbId, settingsObj) -> boolean success
 *   - getAppearanceSettings(cbId) -> {theme, accentColor, useSystemTheme, compactMode}
 *   - setAppearanceSettings(cbId, settingsObj) -> boolean success
 *   - getWorkspaces(cbId) -> array of workspace objects
 *   - createWorkspace(cbId, name, [accentColor]) -> workspace object
 *   - deleteWorkspace(cbId, workspaceId) -> boolean success
 *   - renameWorkspace(cbId, workspaceId, newName) -> boolean success
 *   - setDefaultWorkspace(cbId, workspaceId) -> boolean success
 *
 * Response protocol (C++ -> JS):
 *   - resolveAstraPromise(cbId, success, resultOrError)
 *
 * TODO(astra): Use cr.js / cr.sendWithPromise() pattern from Chromium's
 *   WebUI utilities once we integrate more deeply with chrome/browser/resources.
 *
 * TODO(astra): Add observer-based push updates for preferences.
 *   The C++ side should call JS functions when preferences change, rather
 *   than the JS side only knowing about changes it initiated.
 *
 * Chromium pattern: chrome/browser/resources/settings/settings_page.js
 */

(() => {
  'use strict';

  // =========================================================================
  // Message passing infrastructure
  // =========================================================================

  /** Counter for generating unique callback IDs. */
  let nextCallbackId = 0;

  /** Map of callback ID -> { resolve, reject } for pending promises. */
  const pendingPromises = new Map();

  /**
   * Sends a message to the browser side and returns a promise.
   * @param {string} messageName
   * @param  {...any} args
   * @returns {Promise<any>}
   */
  function sendMessage(messageName, ...args) {
    return new Promise((resolve, reject) => {
      const callbackId = String(nextCallbackId++);
      pendingPromises.set(callbackId, { resolve, reject });

      if (window.chrome && typeof chrome.send === 'function') {
        chrome.send(messageName, [callbackId, ...args]);
      } else {
        console.warn(`chrome.send not available — mock mode for "${messageName}"`);
        setTimeout(() => {
          pendingPromises.delete(callbackId);
          reject(new Error('chrome.send not available'));
        }, 0);
      }
    });
  }

  /**
   * Global function called by the C++ side to resolve or reject a promise.
   * @param {string} callbackId
   * @param {boolean} success
   * @param {*} resultOrError
   */
  window.resolveAstraPromise = function(callbackId, success, resultOrError) {
    const promise = pendingPromises.get(callbackId);
    if (!promise) {
      console.warn(`resolveAstraPromise: unknown callback id "${callbackId}"`);
      return;
    }
    pendingPromises.delete(callbackId);

    if (success) {
      promise.resolve(resultOrError);
    } else {
      promise.reject(new Error(resultOrError || 'Unknown error'));
    }
  };

  // =========================================================================
  // Browser API wrappers
  // =========================================================================

  const api = {
    getAllSettings: () => sendMessage('getAllSettings'),
    getGeneralSettings: () => sendMessage('getGeneralSettings'),
    setGeneralSettings: (settings) => sendMessage('setGeneralSettings', settings),
    getSidebarSettings: () => sendMessage('getSidebarSettings'),
    setSidebarSettings: (settings) => sendMessage('setSidebarSettings', settings),
    getWorkspaceSettings: () => sendMessage('getWorkspaceSettings'),
    setWorkspaceSettings: (settings) => sendMessage('setWorkspaceSettings', settings),
    getFocusModeSettings: () => sendMessage('getFocusModeSettings'),
    setFocusModeSettings: (settings) => sendMessage('setFocusModeSettings', settings),
    getAppearanceSettings: () => sendMessage('getAppearanceSettings'),
    setAppearanceSettings: (settings) => sendMessage('setAppearanceSettings', settings),
    getWorkspaces: () => sendMessage('getWorkspaces'),
    createWorkspace: (name, accentColor) => sendMessage('createWorkspace', name, accentColor || null),
    deleteWorkspace: (id) => sendMessage('deleteWorkspace', id),
    renameWorkspace: (id, name) => sendMessage('renameWorkspace', id, name),
    setDefaultWorkspace: (id) => sendMessage('setDefaultWorkspace', id),
  };

  // =========================================================================
  // Save status indicator
  // =========================================================================

  let saveStatusTimeout = null;

  function showSaveStatus(message, isError) {
    const statusEl = document.getElementById('save-status');
    const textEl = document.getElementById('save-status-text');

    textEl.textContent = message;
    statusEl.classList.remove('hidden');
    statusEl.style.backgroundColor = isError
      ? 'var(--astra-danger)'
      : 'var(--astra-text-primary)';

    if (saveStatusTimeout) {
      clearTimeout(saveStatusTimeout);
    }

    saveStatusTimeout = setTimeout(() => {
      statusEl.classList.add('hidden');
    }, 2000);
  }

  // =========================================================================
  // Navigation
  // =========================================================================

  function setupNavigation() {
    const navItems = document.querySelectorAll('.nav-item');

    navItems.forEach(item => {
      item.addEventListener('click', () => {
        const sectionId = item.dataset.section;

        // Update active nav item
        navItems.forEach(nav => nav.classList.remove('is-active'));
        item.classList.add('is-active');

        // Scroll to section
        const section = document.getElementById('section-' + sectionId);
        if (section) {
          section.scrollIntoView({ behavior: 'smooth', block: 'start' });
        }
      });
    });

    // Set initial active section based on scroll position
    updateActiveNavOnScroll();
    window.addEventListener('scroll', updateActiveNavOnScroll, { passive: true });
  }

  function updateActiveNavOnScroll() {
    const sections = document.querySelectorAll('.settings-section');
    const navItems = document.querySelectorAll('.nav-item');

    let currentSection = null;
    const scrollPos = window.scrollY + 100;

    sections.forEach(section => {
      if (section.offsetTop <= scrollPos) {
        currentSection = section.id.replace('section-', '');
      }
    });

    if (currentSection) {
      navItems.forEach(item => {
        item.classList.toggle('is-active', item.dataset.section === currentSection);
      });
    }
  }

  // =========================================================================
  // General settings
  // =========================================================================

  function setupGeneralSettings() {
    const startupSelect = document.getElementById('startup-behavior');
    const searchSelect = document.getElementById('default-search-engine');

    let debounceTimer = null;

    function saveGeneralSettings() {
      if (debounceTimer) clearTimeout(debounceTimer);
      debounceTimer = setTimeout(async () => {
        const settings = {
          startupBehavior: startupSelect.value,
          defaultSearchEngine: searchSelect.value,
        };
        try {
          await api.setGeneralSettings(settings);
          showSaveStatus('General settings saved');
        } catch (err) {
          console.error('Failed to save general settings:', err);
          showSaveStatus('Failed to save settings', true);
        }
      }, 300);
    }

    startupSelect.addEventListener('change', saveGeneralSettings);
    searchSelect.addEventListener('change', saveGeneralSettings);
  }

  function renderGeneralSettings(settings) {
    if (settings.startupBehavior) {
      document.getElementById('startup-behavior').value = settings.startupBehavior;
    }
    if (settings.defaultSearchEngine) {
      document.getElementById('default-search-engine').value = settings.defaultSearchEngine;
    }
  }

  // =========================================================================
  // Sidebar settings
  // =========================================================================

  function setupSidebarSettings() {
    const enabledToggle = document.getElementById('sidebar-enabled');
    const positionSelect = document.getElementById('sidebar-position');
    const widthSlider = document.getElementById('sidebar-width');
    const widthValue = document.getElementById('sidebar-width-value');
    const autoHideToggle = document.getElementById('sidebar-auto-hide');

    let debounceTimer = null;

    function saveSidebarSettings() {
      if (debounceTimer) clearTimeout(debounceTimer);
      debounceTimer = setTimeout(async () => {
        const settings = {
          enabled: enabledToggle.checked,
          position: positionSelect.value,
          width: parseInt(widthSlider.value, 10),
          autoHide: autoHideToggle.checked,
        };
        try {
          await api.setSidebarSettings(settings);
          showSaveStatus('Sidebar settings saved');
        } catch (err) {
          console.error('Failed to save sidebar settings:', err);
          showSaveStatus('Failed to save settings', true);
        }
      }, 300);
    }

    widthSlider.addEventListener('input', () => {
      widthValue.textContent = widthSlider.value + 'px';
    });

    enabledToggle.addEventListener('change', saveSidebarSettings);
    positionSelect.addEventListener('change', saveSidebarSettings);
    widthSlider.addEventListener('input', saveSidebarSettings);
    autoHideToggle.addEventListener('change', saveSidebarSettings);
  }

  function renderSidebarSettings(settings) {
    if (settings.enabled !== undefined) {
      document.getElementById('sidebar-enabled').checked = settings.enabled;
    }
    if (settings.position) {
      document.getElementById('sidebar-position').value = settings.position;
    }
    if (settings.width !== undefined) {
      document.getElementById('sidebar-width').value = settings.width;
      document.getElementById('sidebar-width-value').textContent = settings.width + 'px';
    }
    if (settings.autoHide !== undefined) {
      document.getElementById('sidebar-auto-hide').checked = settings.autoHide;
    }
  }

  // =========================================================================
  // Workspace settings
  // =========================================================================

  function setupWorkspaceSettings() {
    const defaultSelect = document.getElementById('default-workspace');
    const addBtn = document.getElementById('add-workspace-btn');

    defaultSelect.addEventListener('change', async () => {
      const workspaceId = defaultSelect.value;
      try {
        await api.setDefaultWorkspace(workspaceId);
        showSaveStatus('Default workspace updated');
        // Refresh workspace list to show updated default badge
        loadAndRenderWorkspaces();
      } catch (err) {
        console.error('Failed to set default workspace:', err);
        showSaveStatus('Failed to update default workspace', true);
      }
    });

    addBtn.addEventListener('click', async () => {
      const name = prompt('Enter workspace name:');
      if (!name || name.trim() === '') return;

      try {
        const workspace = await api.createWorkspace(name.trim());
        console.log('Created workspace:', workspace);
        showSaveStatus('Workspace created');
        loadAndRenderWorkspaces();
      } catch (err) {
        console.error('Failed to create workspace:', err);
        showSaveStatus('Failed to create workspace: ' + err.message, true);
      }
    });
  }

  async function loadAndRenderWorkspaces() {
    try {
      const [workspaces, wsSettings] = await Promise.all([
        api.getWorkspaces(),
        api.getWorkspaceSettings(),
      ]);
      renderWorkspacesList(workspaces);
      renderDefaultWorkspaceSelect(workspaces, wsSettings.defaultWorkspaceId);
      renderAccentPresets(wsSettings.accentColorPresets || []);
    } catch (err) {
      console.error('Failed to load workspace data:', err);
    }
  }

  function renderDefaultWorkspaceSelect(workspaces, defaultId) {
    const select = document.getElementById('default-workspace');
    select.innerHTML = '';

    if (!workspaces || workspaces.length === 0) {
      const option = document.createElement('option');
      option.value = '';
      option.textContent = 'No workspaces';
      select.appendChild(option);
      return;
    }

    for (const ws of workspaces) {
      const option = document.createElement('option');
      option.value = ws.id;
      option.textContent = ws.name;
      if (ws.id === defaultId || ws.isDefault) {
        option.selected = true;
      }
      select.appendChild(option);
    }
  }

  function renderWorkspacesList(workspaces) {
    const list = document.getElementById('workspaces-list');
    list.innerHTML = '';

    if (!workspaces || workspaces.length === 0) {
      const empty = document.createElement('div');
      empty.className = 'workspace-item';
      empty.style.justifyContent = 'center';
      empty.style.color = 'var(--astra-text-tertiary)';
      empty.style.fontSize = '13px';
      empty.textContent = 'No workspaces yet';
      list.appendChild(empty);
      return;
    }

    for (const ws of workspaces) {
      const item = document.createElement('div');
      item.className = 'workspace-item';
      item.dataset.workspaceId = ws.id;

      // Accent color dot
      const accent = document.createElement('div');
      accent.className = 'workspace-item-accent';
      accent.style.backgroundColor = ws.accentColor || '#6366f1';
      item.appendChild(accent);

      // Info
      const info = document.createElement('div');
      info.className = 'workspace-item-info';

      const name = document.createElement('div');
      name.className = 'workspace-item-name';
      name.textContent = ws.name;
      info.appendChild(name);

      const meta = document.createElement('div');
      meta.className = 'workspace-item-meta';
      const count = ws.tabCount || 0;
      meta.textContent = `${count} tab${count !== 1 ? 's' : ''}`;
      if (ws.isDefault) {
        meta.textContent += ' — ';
        const badge = document.createElement('span');
        badge.className = 'default-badge';
        badge.textContent = 'Default';
        meta.appendChild(badge);
      }
      info.appendChild(meta);

      item.appendChild(info);

      // Actions
      const actions = document.createElement('div');
      actions.className = 'workspace-item-actions';

      // Rename button
      const renameBtn = document.createElement('button');
      renameBtn.className = 'btn btn-secondary btn-sm';
      renameBtn.textContent = 'Rename';
      renameBtn.addEventListener('click', async () => {
        const newName = prompt('Rename workspace:', ws.name);
        if (!newName || newName.trim() === '' || newName === ws.name) return;
        try {
          await api.renameWorkspace(ws.id, newName.trim());
          showSaveStatus('Workspace renamed');
          loadAndRenderWorkspaces();
        } catch (err) {
          console.error('Failed to rename workspace:', err);
          showSaveStatus('Failed to rename: ' + err.message, true);
        }
      });
      actions.appendChild(renameBtn);

      // Delete button
      if (!ws.isDefault) {
        const deleteBtn = document.createElement('button');
        deleteBtn.className = 'btn btn-danger btn-sm';
        deleteBtn.textContent = 'Delete';
        deleteBtn.addEventListener('click', async () => {
          if (!confirm(`Delete workspace "${ws.name}"?`)) return;
          try {
            await api.deleteWorkspace(ws.id);
            showSaveStatus('Workspace deleted');
            loadAndRenderWorkspaces();
          } catch (err) {
            console.error('Failed to delete workspace:', err);
            showSaveStatus('Failed to delete: ' + err.message, true);
          }
        });
        actions.appendChild(deleteBtn);
      }

      item.appendChild(actions);
      list.appendChild(item);
    }
  }

  function renderAccentPresets(presets) {
    const container = document.getElementById('accent-presets');
    container.innerHTML = '';

    if (!presets || presets.length === 0) {
      return;
    }

    for (const color of presets) {
      const swatch = document.createElement('div');
      swatch.className = 'accent-swatch';
      swatch.style.backgroundColor = color;
      swatch.title = color;
      swatch.dataset.color = color;

      // TODO(astra): Connect swatches to a setting.
      // For now they are just visual presets.
      swatch.addEventListener('click', () => {
        // Highlight selected swatch
        document.querySelectorAll('.accent-swatch').forEach(s => {
          s.classList.remove('is-selected');
        });
        swatch.classList.add('is-selected');

        // Update the accent color picker as well
        const accentInput = document.getElementById('accent-color');
        if (accentInput) {
          accentInput.value = color;
          document.getElementById('accent-color-value').textContent = color;
        }
      });

      container.appendChild(swatch);
    }
  }

  // =========================================================================
  // Focus mode settings
  // =========================================================================

  function setupFocusModeSettings() {
    const durationSlider = document.getElementById('focus-duration');
    const durationValue = document.getElementById('focus-duration-value');
    const soundToggle = document.getElementById('focus-sound-enabled');
    const addBtn = document.getElementById('add-blocked-site-btn');
    const input = document.getElementById('blocklist-input');

    let debounceTimer = null;

    function saveFocusSettings() {
      if (debounceTimer) clearTimeout(debounceTimer);
      debounceTimer = setTimeout(async () => {
        const blocklist = getCurrentBlocklist();
        const settings = {
          defaultDurationMinutes: parseInt(durationSlider.value, 10),
          soundEnabled: soundToggle.checked,
          blocklist: blocklist,
        };
        try {
          await api.setFocusModeSettings(settings);
          showSaveStatus('Focus mode settings saved');
        } catch (err) {
          console.error('Failed to save focus settings:', err);
          showSaveStatus('Failed to save settings', true);
        }
      }, 500);
    }

    durationSlider.addEventListener('input', () => {
      durationValue.textContent = durationSlider.value + ' min';
    });

    durationSlider.addEventListener('input', saveFocusSettings);
    soundToggle.addEventListener('change', saveFocusSettings);

    // Blocklist add
    addBtn.addEventListener('click', () => addBlockedSite());
    input.addEventListener('keydown', (e) => {
      if (e.key === 'Enter') addBlockedSite();
    });

    function addBlockedSite() {
      const site = input.value.trim();
      if (!site) return;

      const blocklist = document.getElementById('blocklist');
      // Check for duplicates
      const existing = blocklist.querySelectorAll('.blocklist-item-site');
      for (const el of existing) {
        if (el.textContent === site) {
          input.value = '';
          return;
        }
      }

      addBlocklistItem(site);
      input.value = '';
      saveFocusSettings();
    }
  }

  function getCurrentBlocklist() {
    const items = document.querySelectorAll('#blocklist .blocklist-item-site');
    return Array.from(items).map(el => el.textContent);
  }

  function addBlocklistItem(site) {
    const blocklist = document.getElementById('blocklist');

    const item = document.createElement('div');
    item.className = 'blocklist-item';

    const siteEl = document.createElement('span');
    siteEl.className = 'blocklist-item-site';
    siteEl.textContent = site;
    item.appendChild(siteEl);

    const removeBtn = document.createElement('button');
    removeBtn.className = 'blocklist-item-remove';
    removeBtn.textContent = '✕';
    removeBtn.setAttribute('aria-label', 'Remove site');
    removeBtn.addEventListener('click', () => {
      item.remove();
      // Trigger save
      const event = new Event('change');
      document.getElementById('focus-duration').dispatchEvent(event);
    });
    item.appendChild(removeBtn);

    blocklist.appendChild(item);
  }

  function renderFocusModeSettings(settings) {
    if (settings.defaultDurationMinutes !== undefined) {
      const slider = document.getElementById('focus-duration');
      const valueEl = document.getElementById('focus-duration-value');
      slider.value = settings.defaultDurationMinutes;
      valueEl.textContent = settings.defaultDurationMinutes + ' min';
    }

    if (settings.soundEnabled !== undefined) {
      document.getElementById('focus-sound-enabled').checked = settings.soundEnabled;
    }

    if (settings.blocklist) {
      const blocklist = document.getElementById('blocklist');
      blocklist.innerHTML = '';
      for (const site of settings.blocklist) {
        addBlocklistItem(site);
      }
    }
  }

  // =========================================================================
  // Appearance settings
  // =========================================================================

  function setupAppearanceSettings() {
    const themeSelect = document.getElementById('appearance-theme');
    const accentInput = document.getElementById('accent-color');
    const accentValue = document.getElementById('accent-color-value');
    const systemToggle = document.getElementById('use-system-theme');
    const compactToggle = document.getElementById('compact-mode');

    let debounceTimer = null;

    function saveAppearanceSettings() {
      if (debounceTimer) clearTimeout(debounceTimer);
      debounceTimer = setTimeout(async () => {
        const settings = {
          theme: themeSelect.value,
          accentColor: accentInput.value,
          useSystemTheme: systemToggle.checked,
          compactMode: compactToggle.checked,
        };
        try {
          await api.setAppearanceSettings(settings);
          showSaveStatus('Appearance settings saved');
        } catch (err) {
          console.error('Failed to save appearance settings:', err);
          showSaveStatus('Failed to save settings', true);
        }
      }, 300);
    }

    accentInput.addEventListener('input', () => {
      accentValue.textContent = accentInput.value;
    });

    themeSelect.addEventListener('change', saveAppearanceSettings);
    accentInput.addEventListener('input', saveAppearanceSettings);
    systemToggle.addEventListener('change', saveAppearanceSettings);
    compactToggle.addEventListener('change', saveAppearanceSettings);
  }

  function renderAppearanceSettings(settings) {
    if (settings.theme) {
      document.getElementById('appearance-theme').value = settings.theme;
    }
    if (settings.accentColor) {
      const input = document.getElementById('accent-color');
      input.value = settings.accentColor;
      document.getElementById('accent-color-value').textContent = settings.accentColor;
    }
    if (settings.useSystemTheme !== undefined) {
      document.getElementById('use-system-theme').checked = settings.useSystemTheme;
    }
    if (settings.compactMode !== undefined) {
      document.getElementById('compact-mode').checked = settings.compactMode;
    }
  }

  // =========================================================================
  // Page initialization
  // =========================================================================

  async function init() {
    // Set up UI event handlers
    setupNavigation();
    setupGeneralSettings();
    setupSidebarSettings();
    setupWorkspaceSettings();
    setupFocusModeSettings();
    setupAppearanceSettings();

    // Load all settings from the browser
    try {
      const allSettings = await api.getAllSettings();
      console.log('Loaded settings:', allSettings);

      if (allSettings.general) {
        renderGeneralSettings(allSettings.general);
      }
      if (allSettings.sidebar) {
        renderSidebarSettings(allSettings.sidebar);
      }
      if (allSettings.focusMode) {
        renderFocusModeSettings(allSettings.focusMode);
      }
      if (allSettings.appearance) {
        renderAppearanceSettings(allSettings.appearance);
      }

      // Workspaces are loaded separately because they involve more data
      // and the list needs independent refreshes.
      loadAndRenderWorkspaces();
    } catch (err) {
      console.error('Failed to load settings:', err);
      // Try loading individual sections as fallback
      try {
        const [general, sidebar, focusMode, appearance] = await Promise.all([
          api.getGeneralSettings().catch(() => ({})),
          api.getSidebarSettings().catch(() => ({})),
          api.getFocusModeSettings().catch(() => ({})),
          api.getAppearanceSettings().catch(() => ({})),
        ]);
        renderGeneralSettings(general);
        renderSidebarSettings(sidebar);
        renderFocusModeSettings(focusMode);
        renderAppearanceSettings(appearance);
        loadAndRenderWorkspaces();
      } catch (err2) {
        console.error('Failed to load any settings:', err2);
      }
    }

    // Set first nav item as active
    const firstNav = document.querySelector('.nav-item');
    if (firstNav) {
      firstNav.classList.add('is-active');
    }
  }

  // Initialize when DOM is ready
  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
  } else {
    init();
  }

})();
