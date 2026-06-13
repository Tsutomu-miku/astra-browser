// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Astra New Tab Page — JavaScript
 * =========================================================================
 *
 * Client-side JS for the Astra new tab page (astra://newtab).
 * Communicates with the browser side via WebUIMessageHandler
 * (astra_new_tab_handler.cc) using chrome.send() / window.domAutomationController.
 *
 * Message protocol (JS -> C++):
 *   - getPageInfo(cbId) -> {greeting, hasProfile, isOffTheRecord, ...}
 *   - getWorkspaces(cbId) -> array of workspace objects
 *   - openWorkspace(cbId, workspaceId) -> boolean success
 *   - createWorkspace(cbId, name, [accentColor]) -> workspace object
 *   - deleteWorkspace(cbId, workspaceId) -> boolean success
 *   - renameWorkspace(cbId, workspaceId, newName) -> boolean success
 *   - getTopSites(cbId, [count]) -> array of shortcut objects
 *   - addShortcut(cbId, url, [title]) -> shortcut object
 *   - removeShortcut(cbId, url) -> boolean success
 *   - getRecentlyVisited(cbId, [count]) -> array of visit objects
 *   - getRecentlyClosed(cbId, [count]) -> array of recently closed items
 *   - reopenRecentlyClosed(cbId, [itemId]) -> boolean success
 *
 * Response protocol (C++ -> JS):
 *   - resolveAstraPromise(cbId, success, resultOrError)
 *
 * Push updates (C++ -> JS, via CallJavascriptFunction):
 *   - TODO(astra): Add observer-based push updates.
 *     The C++ side should call JS functions when data changes.
 *
 * TODO(astra): Use cr.js / cr.sendWithPromise() pattern from Chromium's
 *   WebUI utilities once we integrate more deeply with chrome/browser/resources.
 *   For now we use a simple custom promise-based wrapper around chrome.send().
 *
 * Chromium pattern: chrome/browser/resources/new_tab_page/new_tab_page.js
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
   * @param {string} messageName - The message name (must match C++ RegisterMessageCallback)
   * @param  {...any} args - Arguments to pass to the C++ handler
   * @returns {Promise<any>} A promise that resolves with the result
   */
  function sendMessage(messageName, ...args) {
    return new Promise((resolve, reject) => {
      const callbackId = String(nextCallbackId++);
      pendingPromises.set(callbackId, { resolve, reject });

      // chrome.send is provided by the content layer for WebUI pages.
      // The first argument is always the callback ID.
      if (window.chrome && typeof chrome.send === 'function') {
        chrome.send(messageName, [callbackId, ...args]);
      } else {
        // Fallback for development / testing without a WebUI context.
        console.warn(`chrome.send not available — mock mode for "${messageName}"`);
        // Reject after a short delay to simulate async behavior.
        setTimeout(() => {
          pendingPromises.delete(callbackId);
          reject(new Error('chrome.send not available'));
        }, 0);
      }
    });
  }

  /**
   * Global function called by the C++ side to resolve or reject a promise.
   * This is the counterpart to sendMessage() above.
   *
   * @param {string} callbackId - The callback ID from sendMessage()
   * @param {boolean} success - Whether the operation succeeded
   * @param {*} resultOrError - Result value (if success) or error message (if failure)
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

  const ntpApi = {
    /** Gets general page info (profile, greeting context, etc.). */
    getPageInfo: () => sendMessage('getPageInfo'),

    /** Fetches the list of workspaces from the browser. */
    getWorkspaces: () => sendMessage('getWorkspaces'),

    /** Opens (activates) a workspace by ID. */
    openWorkspace: (workspaceId) => sendMessage('openWorkspace', workspaceId),

    /** Creates a new workspace with the given name and optional accent color. */
    createWorkspace: (name, accentColor) => sendMessage('createWorkspace', name, accentColor || null),

    /** Deletes a workspace by ID. */
    deleteWorkspace: (workspaceId) => sendMessage('deleteWorkspace', workspaceId),

    /** Renames a workspace. */
    renameWorkspace: (workspaceId, newName) => sendMessage('renameWorkspace', workspaceId, newName),

    /** Fetches the top N most-visited site shortcuts. */
    getTopSites: (count) => sendMessage('getTopSites', count || 8),

    /** Adds a custom shortcut. */
    addShortcut: (url, title) => sendMessage('addShortcut', url, title || ''),

    /** Removes a shortcut by URL. */
    removeShortcut: (url) => sendMessage('removeShortcut', url),

    /** Fetches the N most recently visited pages. */
    getRecentlyVisited: (count) => sendMessage('getRecentlyVisited', count || 5),

    /** Fetches the N most recently closed tabs/windows. */
    getRecentlyClosed: (count) => sendMessage('getRecentlyClosed', count || 8),

    /** Reopens a recently closed item (or the most recent if no ID given). */
    reopenRecentlyClosed: (itemId) => sendMessage('reopenRecentlyClosed', itemId || ''),
  };

  // =========================================================================
  // Rendering: Greeting
  // =========================================================================

  /**
   * Renders the greeting text based on the current time of day.
   */
  function renderGreeting() {
    const hour = new Date().getHours();
    let greeting;
    if (hour < 5) {
      greeting = 'Good night';
    } else if (hour < 12) {
      greeting = 'Good morning';
    } else if (hour < 18) {
      greeting = 'Good afternoon';
    } else {
      greeting = 'Good evening';
    }

    document.getElementById('greeting-text').textContent = greeting;
  }

  // =========================================================================
  // Rendering: Workspace grid
  // =========================================================================

  /**
   * Renders workspace cards in the workspace grid.
   * @param {Array} workspaces - Array of workspace objects from the browser
   */
  function renderWorkspaces(workspaces) {
    const grid = document.getElementById('workspace-grid');
    grid.innerHTML = '';

    if (!workspaces || workspaces.length === 0) {
      const empty = document.createElement('div');
      empty.className = 'shortcut-empty';
      empty.textContent = 'No workspaces yet';
      grid.appendChild(empty);
      return;
    }

    for (const ws of workspaces) {
      const card = createWorkspaceCard(ws);
      grid.appendChild(card);
    }

    // Add a "new workspace" tile at the end
    const addCard = createAddWorkspaceCard();
    grid.appendChild(addCard);
  }

  /**
   * Creates a workspace card DOM element.
   * @param {Object} ws - Workspace data
   * @returns {HTMLElement}
   */
  function createWorkspaceCard(ws) {
    const card = document.createElement('div');
    card.className = 'workspace-card';
    if (ws.isActive) {
      card.classList.add('is-active');
    }
    card.title = ws.name;
    card.dataset.workspaceId = ws.id;

    // Action buttons (rename, delete) — visible on hover
    const actions = document.createElement('div');
    actions.className = 'workspace-card-actions';

    const renameBtn = document.createElement('button');
    renameBtn.className = 'workspace-card-btn rename';
    renameBtn.textContent = '✎';
    renameBtn.title = 'Rename workspace';
    renameBtn.addEventListener('click', (e) => {
      e.stopPropagation();
      handleRenameWorkspace(ws.id, ws.name);
    });
    actions.appendChild(renameBtn);

    if (!ws.isDefault) {
      const deleteBtn = document.createElement('button');
      deleteBtn.className = 'workspace-card-btn delete';
      deleteBtn.textContent = '✕';
      deleteBtn.title = 'Delete workspace';
      deleteBtn.addEventListener('click', (e) => {
        e.stopPropagation();
        handleDeleteWorkspace(ws.id, ws.name);
      });
      actions.appendChild(deleteBtn);
    }

    card.appendChild(actions);

    // Accent color dot
    const accent = document.createElement('div');
    accent.className = 'workspace-accent';
    accent.style.backgroundColor = ws.accentColor || '#6366f1';
    card.appendChild(accent);

    // Workspace name
    const name = document.createElement('div');
    name.className = 'workspace-name';
    name.textContent = ws.name;
    card.appendChild(name);

    // Tab count meta
    const meta = document.createElement('div');
    meta.className = 'workspace-meta';
    const count = ws.tabCount || 0;
    meta.textContent = `${count} tab${count !== 1 ? 's' : ''}`;
    if (ws.isDefault) {
      meta.textContent += ' • Default';
    }
    card.appendChild(meta);

    // Click handler — open the workspace
    card.addEventListener('click', () => {
      ntpApi.openWorkspace(ws.id).catch(err => {
        console.error('Failed to open workspace:', err);
      });
    });

    return card;
  }

  /**
   * Creates an "add workspace" card for quick workspace creation.
   * @returns {HTMLElement}
   */
  function createAddWorkspaceCard() {
    const card = document.createElement('div');
    card.className = 'workspace-card';
    card.style.borderStyle = 'dashed';
    card.style.justifyContent = 'center';
    card.style.alignItems = 'center';
    card.style.color = 'var(--astra-text-secondary)';
    card.style.cursor = 'pointer';
    card.textContent = '+ New workspace';
    card.style.fontWeight = '500';
    card.style.fontSize = '14px';

    card.addEventListener('click', handleNewWorkspace);

    return card;
  }

  // =========================================================================
  // Rendering: Shortcut tiles
  // =========================================================================

  /**
   * Renders shortcut tiles in the most-visited grid.
   * @param {Array} shortcuts - Array of shortcut objects from the browser
   */
  function renderShortcuts(shortcuts) {
    const grid = document.getElementById('shortcuts-grid');
    grid.innerHTML = '';

    if (!shortcuts || shortcuts.length === 0) {
      const empty = document.createElement('div');
      empty.className = 'shortcut-empty';
      empty.textContent = 'Your shortcuts will appear here';
      grid.appendChild(empty);
      return;
    }

    for (const shortcut of shortcuts) {
      const tile = createShortcutTile(shortcut);
      grid.appendChild(tile);
    }

    // Add "add shortcut" tile
    const addTile = createAddShortcutTile();
    grid.appendChild(addTile);
  }

  /**
   * Creates a shortcut tile DOM element.
   * @param {Object} shortcut - Shortcut data
   * @returns {HTMLElement}
   */
  function createShortcutTile(shortcut) {
    const tile = document.createElement('a');
    tile.className = 'shortcut-tile';
    tile.href = shortcut.url;
    tile.title = shortcut.title || shortcut.url;

    // Favicon / initial icon
    const icon = document.createElement('div');
    icon.className = 'shortcut-icon';
    if (shortcut.faviconUrl) {
      icon.style.backgroundImage = `url(${shortcut.faviconUrl})`;
      icon.style.backgroundSize = 'cover';
      icon.textContent = '';
    } else {
      // Fallback: first letter of title or domain
      const label = shortcut.title || shortcut.url || '?';
      icon.textContent = label.charAt(0).toUpperCase();
    }
    tile.appendChild(icon);

    // Title
    const title = document.createElement('div');
    title.className = 'shortcut-title';
    title.textContent = shortcut.title || shortcut.url;
    tile.appendChild(title);

    return tile;
  }

  /**
   * Creates an "add shortcut" tile.
   * @returns {HTMLElement}
   */
  function createAddShortcutTile() {
    const tile = document.createElement('div');
    tile.className = 'shortcut-tile add-shortcut';
    tile.setAttribute('role', 'button');
    tile.setAttribute('tabindex', '0');
    tile.title = 'Add shortcut';

    const icon = document.createElement('div');
    icon.className = 'shortcut-icon';
    icon.textContent = '+';
    tile.appendChild(icon);

    const title = document.createElement('div');
    title.className = 'shortcut-title';
    title.textContent = 'Add shortcut';
    tile.appendChild(title);

    tile.addEventListener('click', handleAddShortcut);
    tile.addEventListener('keydown', (e) => {
      if (e.key === 'Enter' || e.key === ' ') {
        e.preventDefault();
        handleAddShortcut();
      }
    });

    return tile;
  }

  // =========================================================================
  // Rendering: Recently visited
  // =========================================================================

  /**
   * Renders the recently visited list.
   * @param {Array} visits - Array of visit objects from the browser
   */
  function renderRecentlyVisited(visits) {
    const list = document.getElementById('recent-list');
    list.innerHTML = '';

    if (!visits || visits.length === 0) {
      const empty = document.createElement('li');
      empty.className = 'recent-empty';
      empty.textContent = 'No recent visits';
      list.appendChild(empty);
      return;
    }

    for (const visit of visits) {
      const item = document.createElement('a');
      item.className = 'recent-item';
      item.href = visit.url;
      item.title = visit.title || visit.url;

      // Favicon placeholder
      const favicon = document.createElement('div');
      favicon.className = 'recent-favicon';
      favicon.style.backgroundColor = hashColor(visit.url);
      item.appendChild(favicon);

      // Title
      const title = document.createElement('span');
      title.className = 'recent-title';
      title.textContent = visit.title || visit.url;
      item.appendChild(title);

      // Relative time
      const time = document.createElement('span');
      time.className = 'recent-time';
      time.textContent = formatRelativeTime(visit.visitTime);
      item.appendChild(time);

      list.appendChild(item);
    }
  }

  // =========================================================================
  // Rendering: Recently closed
  // =========================================================================

  /**
   * Renders the recently closed tabs/windows list.
   * @param {Array} items - Array of recently closed items from the browser
   */
  function renderRecentlyClosed(items) {
    const list = document.getElementById('recently-closed-list');
    list.innerHTML = '';

    if (!items || items.length === 0) {
      const empty = document.createElement('li');
      empty.className = 'recently-closed-empty';
      empty.textContent = 'No recently closed tabs';
      list.appendChild(empty);
      return;
    }

    for (const item of items) {
      const li = document.createElement('li');
      const entry = document.createElement('a');
      entry.className = 'recently-closed-item';
      entry.href = item.url || '#';
      entry.title = item.title || item.url || '';
      entry.dataset.itemId = item.id || '';

      // Icon — different icons for tabs vs windows
      const icon = document.createElement('div');
      icon.className = 'recently-closed-icon';
      icon.textContent = item.type === 'window' ? '🗔' : '📄';
      entry.appendChild(icon);

      // Title
      const title = document.createElement('span');
      title.className = 'recently-closed-title';
      title.textContent = item.title || item.url || 'Untitled';
      entry.appendChild(title);

      // Relative time
      const time = document.createElement('span');
      time.className = 'recently-closed-meta';
      time.textContent = formatRelativeTime(item.closedTime);
      entry.appendChild(time);

      li.appendChild(entry);
      list.appendChild(li);
    }
  }

  // =========================================================================
  // Action handlers
  // =========================================================================

  /**
   * Handles creating a new workspace from the NTP.
   */
  async function handleNewWorkspace() {
    const name = prompt('Enter workspace name:');
    if (!name || name.trim() === '') {
      return;
    }

    try {
      await ntpApi.createWorkspace(name.trim());
      // Refresh the workspace list
      const workspaces = await ntpApi.getWorkspaces();
      renderWorkspaces(workspaces);
    } catch (err) {
      console.error('Failed to create workspace:', err);
      alert('Failed to create workspace: ' + err.message);
    }
  }

  /**
   * Handles renaming a workspace.
   * @param {string} workspaceId
   * @param {string} currentName
   */
  async function handleRenameWorkspace(workspaceId, currentName) {
    const newName = prompt('Rename workspace:', currentName);
    if (!newName || newName.trim() === '' || newName === currentName) {
      return;
    }

    try {
      await ntpApi.renameWorkspace(workspaceId, newName.trim());
      const workspaces = await ntpApi.getWorkspaces();
      renderWorkspaces(workspaces);
    } catch (err) {
      console.error('Failed to rename workspace:', err);
      alert('Failed to rename workspace: ' + err.message);
    }
  }

  /**
   * Handles deleting a workspace.
   * @param {string} workspaceId
   * @param {string} workspaceName
   */
  async function handleDeleteWorkspace(workspaceId, workspaceName) {
    if (!confirm(`Delete workspace "${workspaceName}"?`)) {
      return;
    }

    try {
      await ntpApi.deleteWorkspace(workspaceId);
      const workspaces = await ntpApi.getWorkspaces();
      renderWorkspaces(workspaces);
    } catch (err) {
      console.error('Failed to delete workspace:', err);
      alert('Failed to delete workspace: ' + err.message);
    }
  }

  /**
   * Handles adding a custom shortcut.
   */
  async function handleAddShortcut() {
    const url = prompt('Enter URL:');
    if (!url || url.trim() === '') {
      return;
    }

    const title = prompt('Enter name (optional):') || '';

    try {
      await ntpApi.addShortcut(url.trim(), title.trim());
      const shortcuts = await ntpApi.getTopSites(8);
      renderShortcuts(shortcuts);
    } catch (err) {
      console.error('Failed to add shortcut:', err);
      alert('Failed to add shortcut: ' + err.message);
    }
  }

  /**
   * Handles restoring the most recently closed tab/window.
   */
  async function handleRestoreLast() {
    try {
      const success = await ntpApi.reopenRecentlyClosed('');
      if (success) {
        // Refresh the list
        const items = await ntpApi.getRecentlyClosed(8);
        renderRecentlyClosed(items);
      } else {
        console.warn('Could not restore last closed item');
      }
    } catch (err) {
      console.error('Failed to restore last closed item:', err);
    }
  }

  // =========================================================================
  // Utility functions
  // =========================================================================

  /**
   * Generates a stable color from a string (for placeholder favicons).
   * @param {string} str - Input string
   * @returns {string} CSS hex color
   */
  function hashColor(str) {
    let hash = 0;
    for (let i = 0; i < str.length; i++) {
      hash = str.charCodeAt(i) + ((hash << 5) - hash);
    }
    // Use a palette of pleasant colors instead of random ones
    const colors = [
      '#6366f1', '#8b5cf6', '#a855f7', '#d946ef',
      '#ec4899', '#f43f5e', '#ef4444', '#f97316',
      '#f59e0b', '#eab308', '#84cc16', '#22c55e',
      '#10b981', '#14b8a6', '#06b6d4', '#0ea5e9',
      '#3b82f6', '#6366f1',
    ];
    return colors[Math.abs(hash) % colors.length];
  }

  /**
   * Formats a timestamp as a relative time string (e.g. "2 hours ago").
   * @param {number} timestampMs - Unix timestamp in milliseconds
   * @returns {string} Human-readable relative time
   */
  function formatRelativeTime(timestampMs) {
    if (!timestampMs) {
      return '';
    }

    const now = Date.now();
    const diffMs = now - timestampMs;
    const diffSec = Math.floor(diffMs / 1000);
    const diffMin = Math.floor(diffSec / 60);
    const diffHour = Math.floor(diffMin / 60);
    const diffDay = Math.floor(diffHour / 24);

    if (diffSec < 60) {
      return 'Just now';
    } else if (diffMin < 60) {
      return `${diffMin}m ago`;
    } else if (diffHour < 24) {
      return `${diffHour}h ago`;
    } else if (diffDay < 7) {
      return `${diffDay}d ago`;
    } else {
      // For older items, show the date
      const date = new Date(timestampMs);
      return date.toLocaleDateString();
    }
  }

  // =========================================================================
  // UI setup: New workspace button
  // =========================================================================

  function setupNewWorkspaceButton() {
    const btn = document.getElementById('new-workspace-btn');
    if (btn) {
      btn.addEventListener('click', handleNewWorkspace);
    }
  }

  // =========================================================================
  // UI setup: Add shortcut button
  // =========================================================================

  function setupAddShortcutButton() {
    const btn = document.getElementById('add-shortcut-btn');
    if (btn) {
      btn.addEventListener('click', handleAddShortcut);
    }
  }

  // =========================================================================
  // UI setup: Restore last closed button
  // =========================================================================

  function setupRestoreLastButton() {
    const btn = document.getElementById('restore-last-btn');
    if (btn) {
      btn.addEventListener('click', handleRestoreLast);
    }
  }

  // =========================================================================
  // UI setup: Search input handling
  // =========================================================================

  function setupSearchInput() {
    const input = document.getElementById('search-input');
    if (!input) return;

    // TODO(astra): Integrate with the browser omnibox.
    // For now, pressing Enter navigates to a search URL or the entered address.
    input.addEventListener('keydown', (e) => {
      if (e.key === 'Enter') {
        const query = input.value.trim();
        if (!query) return;

        // If it looks like a URL, navigate directly; otherwise search.
        // TODO(astra): Use Chromium's proper URL classification.
        if (looksLikeUrl(query)) {
          window.location.href = query.startsWith('http')
            ? query
            : 'https://' + query;
        } else {
          // Default to Google search.
          // TODO(astra): Use the user's default search engine.
          // Chromium subsystem: TemplateURLService
          const searchUrl = 'https://www.google.com/search?q=' +
            encodeURIComponent(query);
          window.location.href = searchUrl;
        }
      }
    });
  }

  /**
   * Simple heuristic to check if a string looks like a URL.
   * TODO(astra): Replace with proper Chromium URL parsing.
   * @param {string} str - Input string
   * @returns {boolean}
   */
  function looksLikeUrl(str) {
    if (str.startsWith('http://') || str.startsWith('https://')) {
      return true;
    }
    // Check for domain-like patterns (contains a dot with TLD)
    const parts = str.split('/')[0];
    if (parts.includes('.') && !parts.includes(' ')) {
      const tld = parts.split('.').pop();
      if (tld.length >= 2 && /^[a-zA-Z]+$/.test(tld)) {
        return true;
      }
    }
    // Check for localhost or IP addresses
    if (parts === 'localhost' || /^\d+\.\d+\.\d+\.\d+$/.test(parts)) {
      return true;
    }
    return false;
  }

  // =========================================================================
  // Data loading
  // =========================================================================

  /**
   * Loads all dynamic data from the browser and renders it.
   */
  async function loadAndRenderAll() {
    // Load workspaces
    try {
      const workspaces = await ntpApi.getWorkspaces();
      renderWorkspaces(workspaces);
    } catch (err) {
      console.error('Failed to load workspaces:', err);
      renderWorkspaces([]);
    }

    // Load top sites / shortcuts
    try {
      const shortcuts = await ntpApi.getTopSites(8);
      renderShortcuts(shortcuts);
    } catch (err) {
      console.error('Failed to load top sites:', err);
      renderShortcuts([]);
    }

    // Load recently visited
    try {
      const visits = await ntpApi.getRecentlyVisited(6);
      renderRecentlyVisited(visits);
    } catch (err) {
      console.error('Failed to load recent visits:', err);
      renderRecentlyVisited([]);
    }

    // Load recently closed
    try {
      const closed = await ntpApi.getRecentlyClosed(8);
      renderRecentlyClosed(closed);
    } catch (err) {
      console.error('Failed to load recently closed:', err);
      renderRecentlyClosed([]);
    }
  }

  // =========================================================================
  // Page initialization
  // =========================================================================

  async function init() {
    // Render static UI
    renderGreeting();
    setupNewWorkspaceButton();
    setupAddShortcutButton();
    setupRestoreLastButton();
    setupSearchInput();

    // Load dynamic data from the browser
    loadAndRenderAll();

    // TODO(astra): Set up observer-based push updates.
    // Instead of polling, the C++ side should call JS functions when
    // workspace data, shortcuts, or recently closed items change.
  }

  // Initialize when DOM is ready
  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
  } else {
    init();
  }

  // Update greeting when the page becomes visible again
  // (in case the user leaves the tab open across day boundaries)
  document.addEventListener('visibilitychange', () => {
    if (!document.hidden) {
      renderGreeting();
      // TODO(astra): Refresh data when the page becomes visible again.
      // This ensures the NTP stays in sync with browser state changes
      // that happened while the tab was backgrounded.
    }
  });

  // =========================================================================
  // Public API (for push updates from C++)
  // =========================================================================

  // These functions are called by the C++ side when data changes.
  // TODO(astra): Implement push update handlers.
  // The C++ side would call these via CallJavascriptFunction().

  /**
   * Called by C++ when workspace data changes.
   * @param {Array} workspaces - Updated workspace list
   */
  window.updateWorkspaces = function(workspaces) {
    renderWorkspaces(workspaces);
  };

  /**
   * Called by C++ when shortcut data changes.
   * @param {Array} shortcuts - Updated shortcut list
   */
  window.updateShortcuts = function(shortcuts) {
    renderShortcuts(shortcuts);
  };

  /**
   * Called by C++ when recently closed items change.
   * @param {Array} items - Updated recently closed list
   */
  window.updateRecentlyClosed = function(items) {
    renderRecentlyClosed(items);
  };

})();
