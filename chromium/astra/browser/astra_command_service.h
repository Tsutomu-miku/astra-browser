// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_BROWSER_ASTRA_COMMAND_SERVICE_H_
#define ASTRA_BROWSER_ASTRA_COMMAND_SERVICE_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"

class PrefRegistrySimple;
class Profile;

namespace content {
class BrowserContext;
}

namespace astra {

// =========================================================================
// AstraCommandInfo — command definition
// =========================================================================
//
// A command is a user-invocable action that appears in the command palette
// (quick actions / command palette).  Each command has a stable string ID,
// a display name, optional description, category, and metadata like icon
// and keybinding.
//
// Truth model:
//   - Command definitions live in this service (profile-scoped).
//   - Command execution is delegated to Chromium's command system
//     (BrowserCommandController) and Astra-specific command handlers.
//   - This service is a registry and dispatcher — it does not own the
//     actual implementation of each command.
//
// Chromium subsystems reused:
//   - PrefService (for recent commands and custom keybindings).
//   - ProfileKeyedServiceFactory pattern.
//   - BrowserCommandController (for actual command execution, via patch).
//
// Chromium patch points:
//   - Command execution: wire into BrowserCommandController or
//     chrome/browser/ui/command_updater_impl.cc.
//   - Profile keyed service registration: to wire up the factory.
//     Patch point: chrome/browser/profiles/profile_keyed_service_factory*.
// =========================================================================

struct AstraCommandInfo {
  // Unique string identifier for the command (e.g., "tab.new", "workspace.switch").
  std::string command_id;

  // User-visible name shown in the command palette.
  std::string display_name;

  // Optional longer description of what the command does.
  std::string description;

  // Category for grouping and filtering (e.g., "workspace", "tab", "navigation").
  std::string category;

  // Optional icon identifier (for UI presentation).
  std::string icon_id;

  // Optional default keybinding string (e.g., "Ctrl+T", "Cmd+Shift+P").
  std::string keybinding;

  // Whether the command is currently available/enabled.
  bool is_enabled = true;

  // Priority rank for default ordering (higher = more important = shown first).
  int rank = 0;
};

// =========================================================================
// AstraCommandServiceObserver
// =========================================================================
//
// Observer interface for UI layers (command palette, keyboard shortcuts,
// etc.) to react to command changes and execution.  UI must never be the
// source of truth — AstraCommandService is.
// =========================================================================

class AstraCommandServiceObserver : public base::CheckedObserver {
 public:
  // Called after a command has been executed.
  virtual void OnCommandExecuted(const std::string& command_id,
                                 const std::string& command_name) {}

  // Called after a new command is registered.
  virtual void OnCommandRegistered(const std::string& command_id) {}

  // Called after a command is unregistered.
  virtual void OnCommandUnregistered(const std::string& command_id) {}

 protected:
  ~AstraCommandServiceObserver() override = default;
};

// =========================================================================
// AstraCommandService
// =========================================================================
//
// Profile-scoped keyed service that provides a searchable registry of
// commands (actions) the user can invoke quickly via the command palette.
//
// Truth source for:
//   - Command registry (all registered commands and their metadata).
//   - Recent command history (last N executed commands).
//   - Custom keybinding overrides.
//
// Not owned here:
//   - Actual command implementations (Chromium BrowserCommandController
//     and Astra command delegates own those).
//   - Tab state, workspace state, etc. (other Chromium/Astra services).
//
// Persistence:
//   Recent command IDs and custom keybindings persist through Chromium's
//   PrefService.  No custom file I/O.
//
// Command execution is a STUB in this service — it records the execution
// and notifies observers.  Real command handling requires hooking into
// Chromium's command system (BrowserCommandController).
// =========================================================================

class AstraCommandService final : public KeyedService {
 public:
  explicit AstraCommandService(Profile* profile);
  AstraCommandService(const AstraCommandService&) = delete;
  AstraCommandService& operator=(const AstraCommandService&) = delete;
  ~AstraCommandService() override;

  // KeyedService:
  void Shutdown() override;

  // -- Observers ---------------------------------------------------------

  void AddObserver(AstraCommandServiceObserver* observer);
  void RemoveObserver(AstraCommandServiceObserver* observer);

  // -- Command query -----------------------------------------------------

  // Returns all registered commands, sorted by rank (descending) then name.
  std::vector<AstraCommandInfo> GetAllCommands() const;

  // Returns the command with the given id, or nullptr if not found.
  const AstraCommandInfo* GetCommand(const std::string& command_id) const;

  // Returns all commands in the given category.
  std::vector<AstraCommandInfo> GetCommandsByCategory(
      const std::string& category) const;

  // Searches commands by name and description using simple substring/word
  // matching (case-insensitive).  An empty query returns all enabled commands.
  std::vector<AstraCommandInfo> SearchCommands(const std::string& query) const;

  // Returns the total number of registered commands.
  size_t command_count() const { return commands_.size(); }

  // -- Command execution -------------------------------------------------

  // Executes the command with the given id.
  // Returns true if the command exists and is enabled.
  // Records the execution in recent commands and notifies observers.
  //
  // TODO(astra): This is a stub that only records execution.  Real
  // implementation needs to dispatch to Chromium's command system.
  // Chromium owner: BrowserCommandController / CommandUpdater.
  // Patch point: chrome/browser/ui/browser_command_controller.h.
  bool ExecuteCommand(const std::string& command_id);

  // -- Recent commands ---------------------------------------------------

  // Returns the most recently executed commands (most recent first),
  // up to the configured maximum (default: 10).
  std::vector<AstraCommandInfo> GetRecentCommands() const;

  // Returns the maximum number of recent commands tracked.
  size_t max_recent_commands() const { return max_recent_commands_; }

  // -- Suggested commands ------------------------------------------------

  // Returns suggested commands based on current context (recent + popular).
  // The suggestion algorithm combines recency and rank to produce a
  // short list of relevant commands.
  std::vector<AstraCommandInfo> GetSuggestedCommands() const;

  // -- Command registration ----------------------------------------------

  // Registers a new command.  Returns true if the command was added
  // (i.e., no command with the same id already existed).
  // Fires OnCommandRegistered.
  bool RegisterCommand(const AstraCommandInfo& command_info);

  // Unregisters a command by id.  Returns true if the command existed
  // and was removed.
  // Fires OnCommandUnregistered.
  bool UnregisterCommand(const std::string& command_id);

  // Enables or disables a command.  Returns true if the command existed
  // and its enabled state changed.
  bool SetCommandEnabled(const std::string& command_id, bool enabled);

  // -- Custom keybindings ------------------------------------------------

  // Returns the custom keybinding for a command, or empty string if none.
  std::string GetCustomKeybinding(const std::string& command_id) const;

  // Sets a custom keybinding for a command.  Overrides the default.
  // Passing an empty string clears the custom binding.
  void SetCustomKeybinding(const std::string& command_id,
                           const std::string& keybinding);

  // -- Incognito compatibility -------------------------------------------

  // Returns true if this service instance is associated with an incognito
  // (off-the-record) profile.
  bool IsIncognito() const;

 private:
  // Non-const lookup helper for internal use.
  AstraCommandInfo* FindCommand(const std::string& command_id);

  // Registers all built-in commands.  Called from the constructor.
  void RegisterBuiltInCommands();

  // Adds a command to the recent list (at the front).
  // Removes duplicates and enforces the size limit.
  void AddToRecentCommands(const std::string& command_id);

  // Loads persisted state from the profile's PrefService.
  // Called from the constructor.
  void LoadFromPrefs();

  // Saves the recent commands list to PrefService.
  void SaveRecentToPrefs() const;

  // Saves custom keybindings to PrefService.
  void SaveKeybindingsToPrefs() const;

  raw_ptr<Profile> profile_;
  std::vector<AstraCommandInfo> commands_;
  base::ObserverList<AstraCommandServiceObserver> observers_;

  // List of recently executed command IDs (most recent first).
  std::vector<std::string> recent_command_ids_;

  // Custom keybindings: command_id -> keybinding string.
  std::unordered_map<std::string, std::string> custom_keybindings_;

  // Maximum number of recent commands to track.
  static constexpr size_t kMaxRecentCommands = 10;
  size_t max_recent_commands_ = kMaxRecentCommands;
};

// =========================================================================
// AstraCommandServiceFactory
// =========================================================================
//
// Factory for AstraCommandService.
//
// Incognito behavior: the factory uses kOwnInstance for regular incognito
// profiles because command execution state (recent commands, active
// keybindings) should be per-browsing-context.  An incognito window has
// its own command palette state that does not leak into the main profile.
// Guest sessions also get their own instance (kOwnInstance).
// System profile gets no instance (kNone).
// =========================================================================

class AstraCommandServiceFactory final : public ProfileKeyedServiceFactory {
 public:
  static AstraCommandService* GetForProfile(Profile* profile);
  static AstraCommandServiceFactory* GetInstance();

  // Registers command-service-related prefs on the profile's PrefRegistry.
  // TODO(astra): Wire this into profile keyed service registration so it
  // runs during profile creation.  Chromium patch point:
  // chrome/browser/profiles/profile_keyed_service_factory registrations.
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 private:
  AstraCommandServiceFactory();
  ~AstraCommandServiceFactory() override;

  // ProfileKeyedServiceFactory:
  KeyedService* BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_COMMAND_SERVICE_H_
