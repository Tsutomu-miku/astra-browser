#ifndef ASTRA_BROWSER_ASTRA_WORKSPACE_IMPORT_EXPORT_H_
#define ASTRA_BROWSER_ASTRA_WORKSPACE_IMPORT_EXPORT_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/values.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class PrefService;
class Profile;

namespace astra {

struct AstraWorkspace;

// =========================================================================
// Import mode — controls how imported workspaces interact with existing ones
// =========================================================================
enum class AstraWorkspaceImportMode {
  // Merge imported workspaces with existing ones.
  kMerge,
  // Replace existing non-default workspaces with imported ones.
  kReplace,
};

// =========================================================================
// Conflict resolution strategy — what to do when a workspace name conflicts
// =========================================================================
enum class AstraWorkspaceConflictResolution {
  // Rename the imported workspace (e.g., "Workspace (1)", "Workspace (2)").
  kRename,
  // Skip the conflicting workspace entirely.
  kSkip,
};

// =========================================================================
// Export options — controls what gets included in an export
// =========================================================================
struct AstraWorkspaceExportOptions {
  // Whether to include tab data in the export.
  bool include_tabs = true;
  // Whether to include workspace settings (color, icon, description).
  bool include_settings = true;
  // Whether to include favorite state on tabs.
  bool include_favorites = true;
  // Whether to include metadata like creation time and last used time.
  bool include_metadata = false;
};

// =========================================================================
// Import options — controls how import behaves
// =========================================================================
struct AstraWorkspaceImportOptions {
  // Import mode: merge or replace.
  AstraWorkspaceImportMode mode = AstraWorkspaceImportMode::kMerge;
  // Conflict resolution strategy for duplicate workspace names.
  AstraWorkspaceConflictResolution conflict_resolution =
      AstraWorkspaceConflictResolution::kRename;
  // Whether to open tabs for imported workspaces.
  bool open_tabs = true;
  // Whether to apply favorite state from imported tabs.
  bool apply_favorites = true;
};

// =========================================================================
// Import result — statistics and status from an import operation
// =========================================================================
struct AstraWorkspaceImportResult {
  // Whether the import succeeded (at least one workspace imported).
  bool success = false;
  // Number of workspaces successfully imported.
  size_t imported_count = 0;
  // Number of workspaces skipped due to conflicts.
  size_t skipped_count = 0;
  // Number of workspaces that failed to import.
  size_t failed_count = 0;
  // Total number of tabs opened across all imported workspaces.
  size_t tabs_imported = 0;
  // Error message, if the import failed at the top level.
  std::string error_message;

  // Returns the total number of workspaces processed.
  size_t total() const {
    return imported_count + skipped_count + failed_count;
  }
};

// =========================================================================
// Export result — statistics and status from an export operation
// =========================================================================
struct AstraWorkspaceExportResult {
  // Whether the export succeeded.
  bool success = false;
  // Number of workspaces exported.
  size_t exported_count = 0;
  // Total number of tabs exported across all workspaces.
  size_t tabs_exported = 0;
  // The exported JSON string (empty on failure).
  std::string json;
  // Error message, if the export failed.
  std::string error_message;
};

// =========================================================================
// Validation result — detailed validation status
// =========================================================================
struct AstraWorkspaceValidationResult {
  // Whether the JSON is valid.
  bool is_valid = false;
  // Error message describing why validation failed (empty if valid).
  std::string error_message;
  // Number of workspaces found in the JSON.
  size_t workspace_count = 0;
  // Total number of tabs found across all workspaces.
  size_t total_tab_count = 0;
};

// =========================================================================
// AstraWorkspaceImportExportObserver
// =========================================================================
//
// Observer interface for workspace import/export operations.
//
// All methods have default empty implementations so observers can override
// only the events they care about.
//
// Chromium pattern: base::CheckedObserver with default no-op implementations.
// =========================================================================
class AstraWorkspaceImportExportObserver : public base::CheckedObserver {
 public:
  // Called when an import operation starts.
  // |total_workspaces| is the number of workspaces to import.
  virtual void OnImportStarted(size_t total_workspaces) {}

  // Called when import progress updates.
  // |current| is the number of workspaces processed so far.
  // |total| is the total number of workspaces to import.
  virtual void OnImportProgress(size_t current, size_t total) {}

  // Called when an import operation completes successfully.
  // |result| contains statistics about the import.
  virtual void OnImportCompleted(const AstraWorkspaceImportResult& result) {}

  // Called when an import operation fails.
  // |error_message| describes the failure reason.
  virtual void OnImportFailed(const std::string& error_message) {}

  // Called when an export operation starts.
  virtual void OnExportStarted() {}

  // Called when an export operation completes.
  // |result| contains the exported JSON and statistics.
  virtual void OnExportCompleted(const AstraWorkspaceExportResult& result) {}

 protected:
  ~AstraWorkspaceImportExportObserver() override = default;
};

// =========================================================================
// AstraWorkspaceImportExport
// =========================================================================
//
// Service class for importing and exporting workspace data as JSON.
//
// Export collects workspace metadata (from AstraWorkspaceService) and tab
// URLs (from all Browsers' TabStripModels for the profile) and serializes
// them as a JSON string.  Export is read-only — it never modifies state.
//
// Import parses a JSON string, validates its schema, and creates new
// workspaces with their tabs.  Imported workspaces get fresh IDs to avoid
// conflicts with existing workspaces.  Import opens actual tabs in the
// browser, using Chromium's TabStripModel / Browser infrastructure.
//
// Schema (version 1):
//   {
//     "version": 1,
//     "exported_at": "ISO 8601 timestamp string",
//     "workspaces": [
//       {
//         "id": "workspace-id",
//         "name": "Work",
//         "accent_color": "#5B8FF9",
//         "icon": "star",
//         "description": "Work projects",
//         "order_index": 0,
//         "tabs": [
//           { "title": "GitHub", "url": "https://github.com",
//             "pinned": false, "favorite": true },
//           ...
//         ]
//       },
//       ...
//     ]
//   }
//
// Chromium subsystems reused:
//   - Profile (for service lookup)
//   - BrowserList / TabStripModel (for tab iteration during export)
//   - Browser / NavigateParams (for opening tabs during import)
//   - base::JSONReader / base::JSONWriter (JSON parsing and serialization)
//   - GURL (URL validation)
//   - PrefService (import/export settings persistence)
//   - AstraWorkspaceService (workspace metadata CRUD)
//   - AstraTabFeatures (per-tab workspace + favorite metadata)
//
// Security considerations:
//   - JSON schema is validated before any state changes occur.
//   - URLs are validated with GURL; invalid URLs are skipped.
//   - Potentially dangerous schemes (javascript:, data:, file:, chrome:)
//     are rejected during import to prevent abuse.
//   - The number of workspaces and tabs per workspace is capped.
//   - Only known JSON fields are read; unknown fields are ignored.
// =========================================================================
class AstraWorkspaceImportExport {
 public:
  explicit AstraWorkspaceImportExport(Profile* profile);
  ~AstraWorkspaceImportExport();

  AstraWorkspaceImportExport(const AstraWorkspaceImportExport&) = delete;
  AstraWorkspaceImportExport& operator=(const AstraWorkspaceImportExport&) =
      delete;

  // -- Observers -----------------------------------------------------------

  void AddObserver(AstraWorkspaceImportExportObserver* observer);
  void RemoveObserver(AstraWorkspaceImportExportObserver* observer);

  // -- Export --------------------------------------------------------------

  // Exports all workspaces using the given options.
  // Notifies observers on start and completion.
  AstraWorkspaceExportResult ExportWorkspaces(
      const AstraWorkspaceExportOptions& options);

  // Exports specific workspaces by ID.
  // Workspaces that don't exist are skipped.
  AstraWorkspaceExportResult ExportWorkspacesByIds(
      const std::vector<std::string>& workspace_ids,
      const AstraWorkspaceExportOptions& options);

  // Exports a single workspace by ID.
  // Returns empty result if the workspace doesn't exist.
  AstraWorkspaceExportResult ExportWorkspace(
      const std::string& workspace_id,
      const AstraWorkspaceExportOptions& options);

  // -- Import --------------------------------------------------------------

  // Imports workspaces from a JSON string.
  // Validates the JSON first, then applies import according to options.
  // Notifies observers on start, progress, completion, or failure.
  AstraWorkspaceImportResult ImportWorkspaces(
      const std::string& json,
      const AstraWorkspaceImportOptions& options);

  // -- Static validation utilities -----------------------------------------

  // Validates that |json| is a well-formed workspace export JSON string
  // matching the version 1 schema.
  //
  // Returns a detailed validation result with error messages.
  static AstraWorkspaceValidationResult ValidateWorkspaceJsonWithError(
      const std::string& json);

  // Simple validation: returns true if the JSON is valid.
  static bool ValidateWorkspaceJson(const std::string& json);

  // -- Limits (exposed for tests) ------------------------------------------

  // Maximum number of workspaces that can be imported at once.
  static constexpr size_t kMaxWorkspaces = 50;

  // Maximum number of tabs per workspace in an import.
  static constexpr size_t kMaxTabsPerWorkspace = 200;

  // -- Settings ------------------------------------------------------------
  //
  // Import/export default settings persisted via PrefService.
  // These control the default behavior when no explicit options are given.

  // Gets the default export options from prefs.
  AstraWorkspaceExportOptions GetDefaultExportOptions() const;

  // Sets the default export options and persists them to prefs.
  void SetDefaultExportOptions(const AstraWorkspaceExportOptions& options);

  // Gets the default import options from prefs.
  AstraWorkspaceImportOptions GetDefaultImportOptions() const;

  // Sets the default import options and persists them to prefs.
  void SetDefaultImportOptions(const AstraWorkspaceImportOptions& options);

  // -- File path helpers ---------------------------------------------------
  //
  // Helpers for building default export file names and paths.
  // Actual file I/O is handled by Chromium's file picker / file utilities.
  // These helpers just generate suggested names.

  // Generates a default export file name with timestamp.
  static std::string GenerateExportFileName();

  // Returns the recommended file extension for workspace exports.
  static std::string GetExportFileExtension();

 private:
  // -- Export helpers ------------------------------------------------------

  // Builds the JSON value for a single workspace with the given options.
  base::Value::Dict BuildWorkspaceExportValue(
      const AstraWorkspace& workspace,
      const AstraWorkspaceExportOptions& options) const;

  // Collects tab data for a workspace and returns a JSON list.
  base::Value::List CollectTabsForWorkspace(
      const std::string& workspace_id,
      const AstraWorkspaceExportOptions& options) const;

  // Builds the full export value tree from a list of workspace values.
  base::Value BuildExportValue(
      base::Value::List workspace_list,
      const AstraWorkspaceExportOptions& options) const;

  // -- Import helpers ------------------------------------------------------

  // Imports a single workspace from a validated JSON dict.
  // Returns the new workspace ID on success, empty string on failure.
  std::string ImportWorkspaceFromDict(
      const base::Value::Dict& ws_dict,
      const AstraWorkspaceImportOptions& options,
      size_t order_index);

  // Resolves a name conflict according to the conflict resolution strategy.
  // Returns the resolved name, or empty string if the workspace should be
  // skipped.
  std::string ResolveNameConflict(
      const std::string& original_name,
      AstraWorkspaceConflictResolution resolution) const;

  // Checks if a workspace with the given name already exists.
  bool WorkspaceNameExists(const std::string& name) const;

  // Generates a unique name by appending a counter (e.g., "Name (2)").
  std::string GenerateUniqueName(const std::string& base_name) const;

  // -- Validation helpers --------------------------------------------------

  // Parses JSON string into a base::Value.
  // Returns absl::nullopt on parse failure.
  static absl::optional<base::Value> ParseJson(const std::string& json);

  // Validates a parsed JSON value against the schema.
  static AstraWorkspaceValidationResult ValidateParsedJson(
      const base::Value& value);

  // Validates a single workspace dict from the JSON.
  // Appends to error_message on failure.
  static bool ValidateWorkspaceDict(const base::Value::Dict& ws_dict,
                                     size_t index,
                                     std::string& error_message);

  // Validates a single tab dict from the JSON.
  // Appends to error_message on failure.
  static bool ValidateTabDict(const base::Value::Dict& tab_dict,
                               size_t ws_index,
                               size_t tab_index,
                               std::string& error_message);

  // Checks whether a URL scheme is safe to import.
  static bool IsSafeUrlScheme(const GURL& url);

  // -- Pref helpers --------------------------------------------------------

  // Returns the PrefService for the profile.
  PrefService* GetPrefs() const;

  // -- Observer notifications ----------------------------------------------

  void NotifyImportStarted(size_t total_workspaces);
  void NotifyImportProgress(size_t current, size_t total);
  void NotifyImportCompleted(const AstraWorkspaceImportResult& result);
  void NotifyImportFailed(const std::string& error_message);
  void NotifyExportStarted();
  void NotifyExportCompleted(const AstraWorkspaceExportResult& result);

  raw_ptr<Profile> profile_;
  base::ObserverList<AstraWorkspaceImportExportObserver> observers_;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_WORKSPACE_IMPORT_EXPORT_H_
