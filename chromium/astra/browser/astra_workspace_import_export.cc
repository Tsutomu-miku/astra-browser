#include "astra/browser/astra_workspace_import_export.h"

#include <string>
#include <vector>

#include "astra/browser/astra_prefs.h"
#include "astra/browser/astra_tab_features.h"
#include "astra/browser/astra_workspace_service.h"
#include "base/guid.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/values.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

namespace astra {

namespace {

// JSON schema keys.
constexpr char kVersionKey[] = "version";
constexpr char kExportedAtKey[] = "exported_at";
constexpr char kWorkspacesKey[] = "workspaces";

// Workspace keys.
constexpr char kWorkspaceIdKey[] = "id";
constexpr char kWorkspaceNameKey[] = "name";
constexpr char kWorkspaceAccentColorKey[] = "accent_color";
constexpr char kWorkspaceIconKey[] = "icon";
constexpr char kWorkspaceDescriptionKey[] = "description";
constexpr char kWorkspaceOrderIndexKey[] = "order_index";
constexpr char kWorkspaceTabsKey[] = "tabs";
constexpr char kWorkspaceCreatedTimeKey[] = "created_time";
constexpr char kWorkspaceLastUsedTimeKey[] = "last_used_time";
constexpr char kWorkspaceIsHibernatedKey[] = "is_hibernated";

// Tab keys.
constexpr char kTabTitleKey[] = "title";
constexpr char kTabUrlKey[] = "url";
constexpr char kTabPinnedKey[] = "pinned";
constexpr char kTabFavoriteKey[] = "favorite";

// Current export format version.
constexpr int kCurrentVersion = 1;

// Default accent color for imported workspaces that don't specify one.
constexpr char kDefaultImportAccentColor[] = "#5B8FF9";

// Default export file name prefix.
constexpr char kExportFileNamePrefix[] = "astra-workspaces-";
constexpr char kExportFileExtension[] = ".json";

// Finds a Browser for |profile| to open tabs in.  Returns the first
// non-incognito browser for the profile, or nullptr if none exists.
//
// TODO(astra): Use a more targeted API if one exists.  BrowserList gives
// all browsers across all profiles; we filter by profile here.
// Chromium owner: BrowserList (chrome/browser/ui/browser_list.h)
Browser* FindBrowserForProfile(Profile* profile) {
  if (!profile) {
    return nullptr;
  }

  for (auto* browser : *BrowserList::GetInstance()) {
    if (browser->profile() == profile && !browser->profile()->IsOffTheRecord()) {
      return browser;
    }
  }
  return nullptr;
}

// Helper to format an error message with a workspace index.
std::string WsError(size_t index, const std::string& message) {
  return base::StringPrintf("Workspace %zu: %s", index, message.c_str());
}

// Helper to format an error message with workspace and tab indices.
std::string TabError(size_t ws_index, size_t tab_index,
                     const std::string& message) {
  return base::StringPrintf("Workspace %zu, tab %zu: %s",
                            ws_index, tab_index, message.c_str());
}

}  // namespace

// =========================================================================
// Constructor / destructor
// =========================================================================

AstraWorkspaceImportExport::AstraWorkspaceImportExport(Profile* profile)
    : profile_(profile) {}

AstraWorkspaceImportExport::~AstraWorkspaceImportExport() = default;

// =========================================================================
// Observers
// =========================================================================

void AstraWorkspaceImportExport::AddObserver(
    AstraWorkspaceImportExportObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraWorkspaceImportExport::RemoveObserver(
    AstraWorkspaceImportExportObserver* observer) {
  observers_.RemoveObserver(observer);
}

// =========================================================================
// Export
// =========================================================================

AstraWorkspaceExportResult AstraWorkspaceImportExport::ExportWorkspaces(
    const AstraWorkspaceExportOptions& options) {
  NotifyExportStarted();

  AstraWorkspaceExportResult result;

  if (!profile_) {
    result.error_message = "No profile available";
    NotifyExportCompleted(result);
    return result;
  }

  AstraWorkspaceService* service =
      AstraWorkspaceServiceFactory::GetForProfile(profile_);
  if (!service) {
    result.error_message = "Workspace service not available";
    NotifyExportCompleted(result);
    return result;
  }

  const auto& workspaces = service->workspaces();

  base::Value::List ws_list;
  ws_list.reserve(workspaces.size());

  for (const auto& ws : workspaces) {
    ws_list.Append(BuildWorkspaceExportValue(ws, options));
    result.exported_count++;

    // Count tabs.
    if (options.include_tabs) {
      size_t tab_count = 0;
      for (auto* browser : *BrowserList::GetInstance()) {
        if (browser->profile() != profile_) continue;
        TabStripModel* tab_strip = browser->tab_strip_model();
        if (!tab_strip) continue;
        for (int i = 0; i < tab_strip->count(); ++i) {
          content::WebContents* web_contents = tab_strip->GetWebContentsAt(i);
          if (!web_contents) continue;
          AstraTabFeatures* features =
              AstraTabFeatures::FromWebContents(web_contents);
          std::string ws_id = features ? features->workspace_id() : "default";
          if (ws_id == ws.id) {
            tab_count++;
          }
        }
      }
      result.tabs_exported += tab_count;
    }
  }

  base::Value root = BuildExportValue(std::move(ws_list), options);

  std::string json;
  if (!base::JSONWriter::WriteWithOptions(
          root, base::JSONWriter::OPTIONS_PRETTY_PRINT, &json)) {
    result.error_message = "Failed to serialize JSON";
    NotifyExportCompleted(result);
    return result;
  }

  result.json = std::move(json);
  result.success = true;

  NotifyExportCompleted(result);
  return result;
}

AstraWorkspaceExportResult AstraWorkspaceImportExport::ExportWorkspacesByIds(
    const std::vector<std::string>& workspace_ids,
    const AstraWorkspaceExportOptions& options) {
  NotifyExportStarted();

  AstraWorkspaceExportResult result;

  if (!profile_) {
    result.error_message = "No profile available";
    NotifyExportCompleted(result);
    return result;
  }

  AstraWorkspaceService* service =
      AstraWorkspaceServiceFactory::GetForProfile(profile_);
  if (!service) {
    result.error_message = "Workspace service not available";
    NotifyExportCompleted(result);
    return result;
  }

  base::Value::List ws_list;

  for (const auto& id : workspace_ids) {
    const AstraWorkspace* ws = service->GetWorkspace(id);
    if (!ws) {
      continue;  // Skip non-existent workspaces.
    }

    ws_list.Append(BuildWorkspaceExportValue(*ws, options));
    result.exported_count++;

    if (options.include_tabs) {
      size_t tab_count = 0;
      for (auto* browser : *BrowserList::GetInstance()) {
        if (browser->profile() != profile_) continue;
        TabStripModel* tab_strip = browser->tab_strip_model();
        if (!tab_strip) continue;
        for (int i = 0; i < tab_strip->count(); ++i) {
          content::WebContents* web_contents = tab_strip->GetWebContentsAt(i);
          if (!web_contents) continue;
          AstraTabFeatures* features =
              AstraTabFeatures::FromWebContents(web_contents);
          std::string ws_id = features ? features->workspace_id() : "default";
          if (ws_id == ws->id) {
            tab_count++;
          }
        }
      }
      result.tabs_exported += tab_count;
    }
  }

  if (ws_list.empty()) {
    result.error_message = "No matching workspaces found";
    NotifyExportCompleted(result);
    return result;
  }

  base::Value root = BuildExportValue(std::move(ws_list), options);

  std::string json;
  if (!base::JSONWriter::WriteWithOptions(
          root, base::JSONWriter::OPTIONS_PRETTY_PRINT, &json)) {
    result.error_message = "Failed to serialize JSON";
    NotifyExportCompleted(result);
    return result;
  }

  result.json = std::move(json);
  result.success = true;

  NotifyExportCompleted(result);
  return result;
}

AstraWorkspaceExportResult AstraWorkspaceImportExport::ExportWorkspace(
    const std::string& workspace_id,
    const AstraWorkspaceExportOptions& options) {
  return ExportWorkspacesByIds({workspace_id}, options);
}

base::Value::Dict AstraWorkspaceImportExport::BuildWorkspaceExportValue(
    const AstraWorkspace& workspace,
    const AstraWorkspaceExportOptions& options) const {
  base::Value::Dict ws_dict;

  ws_dict.Set(kWorkspaceIdKey, workspace.id);
  ws_dict.Set(kWorkspaceNameKey, workspace.name);

  if (options.include_settings) {
    ws_dict.Set(kWorkspaceAccentColorKey, workspace.accent_color);
    ws_dict.Set(kWorkspaceOrderIndexKey,
                static_cast<int>(workspace.order_index));

    if (workspace.icon.has_value()) {
      ws_dict.Set(kWorkspaceIconKey, *workspace.icon);
    }
    if (!workspace.description.empty()) {
      ws_dict.Set(kWorkspaceDescriptionKey, workspace.description);
    }
  }

  if (options.include_metadata) {
    ws_dict.Set(kWorkspaceCreatedTimeKey,
                workspace.created_time.ToDoubleT());
    if (!workspace.last_used_time.is_null()) {
      ws_dict.Set(kWorkspaceLastUsedTimeKey,
                  workspace.last_used_time.ToDoubleT());
    }
    ws_dict.Set(kWorkspaceIsHibernatedKey, workspace.is_hibernated);
  }

  if (options.include_tabs) {
    ws_dict.Set(kWorkspaceTabsKey,
                CollectTabsForWorkspace(workspace.id, options));
  }

  return ws_dict;
}

base::Value::List AstraWorkspaceImportExport::CollectTabsForWorkspace(
    const std::string& workspace_id,
    const AstraWorkspaceExportOptions& options) const {
  base::Value::List tabs;

  if (!profile_) {
    return tabs;
  }

  for (auto* browser : *BrowserList::GetInstance()) {
    if (browser->profile() != profile_) {
      continue;
    }

    TabStripModel* tab_strip = browser->tab_strip_model();
    if (!tab_strip) {
      continue;
    }

    int tab_count = tab_strip->count();
    for (int i = 0; i < tab_count; ++i) {
      content::WebContents* web_contents = tab_strip->GetWebContentsAt(i);
      if (!web_contents) {
        continue;
      }

      // Check if this tab belongs to the workspace.
      AstraTabFeatures* features =
          AstraTabFeatures::FromWebContents(web_contents);
      std::string tab_ws_id = "default";
      bool is_favorite = false;
      if (features) {
        tab_ws_id = features->workspace_id();
        is_favorite = features->is_favorite();
      }

      if (tab_ws_id != workspace_id) {
        continue;
      }

      base::Value::Dict tab_dict;
      tab_dict.Set(kTabTitleKey,
                   base::UTF16ToUTF8(web_contents->GetTitle()));
      tab_dict.Set(kTabUrlKey, web_contents->GetURL().spec());
      tab_dict.Set(kTabPinnedKey, tab_strip->IsTabPinned(i));

      if (options.include_favorites) {
        tab_dict.Set(kTabFavoriteKey, is_favorite);
      }

      tabs.Append(std::move(tab_dict));
    }
  }

  return tabs;
}

base::Value AstraWorkspaceImportExport::BuildExportValue(
    base::Value::List workspace_list,
    const AstraWorkspaceExportOptions& /*options*/) const {
  base::Value::Dict root;
  root.Set(kVersionKey, kCurrentVersion);
  root.Set(kExportedAtKey,
           base::Time::Now().ToISO8601(false /* include_milliseconds */));
  root.Set(kWorkspacesKey, std::move(workspace_list));
  return base::Value(std::move(root));
}

// =========================================================================
// Import
// =========================================================================

AstraWorkspaceImportResult AstraWorkspaceImportExport::ImportWorkspaces(
    const std::string& json,
    const AstraWorkspaceImportOptions& options) {
  AstraWorkspaceImportResult result;

  if (!profile_) {
    result.error_message = "No profile available";
    NotifyImportFailed(result.error_message);
    return result;
  }

  // Parse and validate first.
  auto validation = ValidateWorkspaceJsonWithError(json);
  if (!validation.is_valid) {
    result.error_message = validation.error_message;
    NotifyImportFailed(result.error_message);
    return result;
  }

  absl::optional<base::Value> value = ParseJson(json);
  if (!value.has_value()) {
    result.error_message = "Failed to parse JSON";
    NotifyImportFailed(result.error_message);
    return result;
  }

  const base::Value::Dict& root = value->GetDict();
  const base::Value::List* workspaces = root.FindList(kWorkspacesKey);
  if (!workspaces) {
    result.error_message = "No workspaces field in JSON";
    NotifyImportFailed(result.error_message);
    return result;
  }

  size_t total = workspaces->size();
  NotifyImportStarted(total);

  // If replacing, delete existing non-default workspaces first.
  if (options.mode == AstraWorkspaceImportMode::kReplace) {
    AstraWorkspaceService* service =
        AstraWorkspaceServiceFactory::GetForProfile(profile_);
    if (!service) {
      result.error_message = "Workspace service not available";
      NotifyImportFailed(result.error_message);
      return result;
    }

    std::vector<std::string> ids_to_delete;
    for (const auto& ws : service->workspaces()) {
      if (!ws.is_default) {
        ids_to_delete.push_back(ws.id);
      }
    }
    for (const auto& id : ids_to_delete) {
      service->DeleteWorkspace(id);
    }
  }

  // Calculate starting order index.
  size_t start_order = 0;
  AstraWorkspaceService* service =
      AstraWorkspaceServiceFactory::GetForProfile(profile_);
  if (service && !service->workspaces().empty()) {
    start_order = service->workspaces().size();
  }

  // Import each workspace.
  size_t current = 0;
  size_t order_index = start_order;

  for (const auto& item : *workspaces) {
    const base::Value::Dict* ws_dict = item.GetIfDict();
    if (!ws_dict) {
      result.failed_count++;
      current++;
      NotifyImportProgress(current, total);
      continue;
    }

    // Check for name conflict.
    const std::string* name_ptr = ws_dict->FindString(kWorkspaceNameKey);
    if (!name_ptr) {
      result.failed_count++;
      current++;
      NotifyImportProgress(current, total);
      continue;
    }

    std::string resolved_name = *name_ptr;
    if (WorkspaceNameExists(*name_ptr)) {
      resolved_name = ResolveNameConflict(*name_ptr, options.conflict_resolution);
      if (resolved_name.empty()) {
        result.skipped_count++;
        current++;
        NotifyImportProgress(current, total);
        continue;
      }
    }

    // Create a modified dict with the resolved name.
    base::Value::Dict modified_dict = ws_dict->Clone();
    modified_dict.Set(kWorkspaceNameKey, resolved_name);

    std::string new_id =
        ImportWorkspaceFromDict(modified_dict, options, order_index);

    if (!new_id.empty()) {
      result.imported_count++;
      order_index++;
    } else {
      result.failed_count++;
    }

    current++;
    NotifyImportProgress(current, total);
  }

  result.success = result.imported_count > 0;
  if (!result.success && result.error_message.empty()) {
    result.error_message = "No workspaces were successfully imported";
  }

  NotifyImportCompleted(result);
  return result;
}

std::string AstraWorkspaceImportExport::ImportWorkspaceFromDict(
    const base::Value::Dict& ws_dict,
    const AstraWorkspaceImportOptions& options,
    size_t order_index) {
  if (!profile_) {
    return std::string();
  }

  AstraWorkspaceService* service =
      AstraWorkspaceServiceFactory::GetForProfile(profile_);
  if (!service) {
    return std::string();
  }

  // Extract workspace fields.
  const std::string* name = ws_dict.FindString(kWorkspaceNameKey);
  if (!name) {
    return std::string();
  }

  const std::string* accent_color =
      ws_dict.FindString(kWorkspaceAccentColorKey);
  const std::string* icon = ws_dict.FindString(kWorkspaceIconKey);
  const std::string* description = ws_dict.FindString(kWorkspaceDescriptionKey);

  // Generate a fresh ID.
  std::string new_id = base::GenerateGUID();

  // Build the workspace.
  AstraWorkspace ws;
  ws.id = new_id;
  ws.name = *name;
  ws.accent_color = accent_color ? *accent_color : kDefaultImportAccentColor;
  ws.created_time = base::Time::Now();
  ws.is_default = false;
  ws.order_index = order_index;
  ws.last_used_time = base::Time::Now();
  ws.is_hibernated = false;

  if (icon && !icon->empty()) {
    ws.icon = *icon;
  }
  if (description) {
    ws.description = *description;
  }

  service->AddWorkspace(std::move(ws));

  // Verify it was created.
  if (!service->GetWorkspace(new_id)) {
    return std::string();
  }

  // Open tabs if requested.
  if (options.open_tabs) {
    const base::Value::List* tabs = ws_dict.FindList(kWorkspaceTabsKey);
    if (tabs && !tabs->empty()) {
      size_t opened = 0;

      // Find or create a browser.
      Browser* browser = FindBrowserForProfile(profile_);
      if (!browser) {
        // TODO(astra): Create a new browser window if none exists.
        // For now, tabs won't be opened without an existing browser.
        return new_id;
      }

      TabStripModel* tab_strip = browser->tab_strip_model();
      if (!tab_strip) {
        return new_id;
      }

      for (const auto& tab_item : *tabs) {
        const base::Value::Dict* tab_dict = tab_item.GetIfDict();
        if (!tab_dict) continue;

        const std::string* url_str = tab_dict->FindString(kTabUrlKey);
        if (!url_str || url_str->empty()) continue;

        GURL url(*url_str);
        if (!url.is_valid() || !IsSafeUrlScheme(url)) continue;

        content::OpenURLParams params(
            url, content::Referrer(),
            WindowOpenDisposition::NEW_BACKGROUND_TAB,
            ui::PAGE_TRANSITION_AUTO_TOPLEVEL,
            false /* is_renderer_initiated */);

        content::WebContents* web_contents =
            browser->OpenURL(params, /*navigate=*/true);
        if (!web_contents) continue;

        // Set Astra metadata.
        AstraTabFeatures* features =
            AstraTabFeatures::GetOrCreateForWebContents(web_contents);
        if (features) {
          features->set_workspace_id(new_id);

          if (options.apply_favorites) {
            absl::optional<bool> is_favorite =
                tab_dict->FindBool(kTabFavoriteKey);
            if (is_favorite.has_value()) {
              features->set_is_favorite(*is_favorite);
            }
          }

          // Set pinned state via AstraTabFeatures (sidebar pinning).
          // TODO(astra): Actual tab pinning should use TabStripModel.
          absl::optional<bool> is_pinned = tab_dict->FindBool(kTabPinnedKey);
          if (is_pinned.has_value()) {
            features->set_sidebar_pinned(*is_pinned);
          }
        }

        opened++;
      }

      // We'd track this in result but we're inside a helper.
      // The caller counts workspaces, not individual tabs.
    }
  }

  return new_id;
}

std::string AstraWorkspaceImportExport::ResolveNameConflict(
    const std::string& original_name,
    AstraWorkspaceConflictResolution resolution) const {
  switch (resolution) {
    case AstraWorkspaceConflictResolution::kSkip:
      return std::string();  // Empty means skip.
    case AstraWorkspaceConflictResolution::kRename:
      return GenerateUniqueName(original_name);
  }
  return std::string();
}

bool AstraWorkspaceImportExport::WorkspaceNameExists(
    const std::string& name) const {
  if (!profile_) {
    return false;
  }

  AstraWorkspaceService* service =
      AstraWorkspaceServiceFactory::GetForProfile(profile_);
  if (!service) {
    return false;
  }

  for (const auto& ws : service->workspaces()) {
    if (ws.name == name) {
      return true;
    }
  }
  return false;
}

std::string AstraWorkspaceImportExport::GenerateUniqueName(
    const std::string& base_name) const {
  // Try "Name (2)", "Name (3)", etc. until we find a unique name.
  for (int i = 2; i <= 1000; ++i) {
    std::string candidate =
        base::StringPrintf("%s (%d)", base_name.c_str(), i);
    if (!WorkspaceNameExists(candidate)) {
      return candidate;
    }
  }
  // If we somehow can't find a unique name after 1000 tries, append a GUID.
  return base_name + " (" + base::GenerateGUID().substr(0, 8) + ")";
}

// =========================================================================
// Validation
// =========================================================================

AstraWorkspaceValidationResult
AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError(
    const std::string& json) {
  absl::optional<base::Value> value = ParseJson(json);
  if (!value.has_value()) {
    AstraWorkspaceValidationResult result;
    result.error_message = "Invalid JSON: parse error";
    return result;
  }
  return ValidateParsedJson(*value);
}

bool AstraWorkspaceImportExport::ValidateWorkspaceJson(
    const std::string& json) {
  return ValidateWorkspaceJsonWithError(json).is_valid;
}

absl::optional<base::Value> AstraWorkspaceImportExport::ParseJson(
    const std::string& json) {
  auto result = base::JSONReader::ReadAndReturnValueWithError(
      json, base::JSONParserOptions::JSON_PARSE_RFC);
  if (!result.has_value()) {
    return absl::nullopt;
  }
  return std::move(*result);
}

AstraWorkspaceValidationResult
AstraWorkspaceImportExport::ValidateParsedJson(const base::Value& value) {
  AstraWorkspaceValidationResult result;

  if (!value.is_dict()) {
    result.error_message = "JSON root must be an object";
    return result;
  }

  const base::Value::Dict& dict = value.GetDict();

  // Check version.
  absl::optional<int> version = dict.FindInt(kVersionKey);
  if (!version.has_value()) {
    result.error_message = "Missing required field: 'version'";
    return result;
  }
  if (*version != kCurrentVersion) {
    result.error_message =
        base::StringPrintf("Unsupported version: %d (expected %d)",
                           *version, kCurrentVersion);
    return result;
  }

  // Check workspaces list.
  const base::Value::List* workspaces = dict.FindList(kWorkspacesKey);
  if (!workspaces) {
    result.error_message = "Missing required field: 'workspaces' (must be a list)";
    return result;
  }

  // Enforce workspace count limit.
  if (workspaces->size() > kMaxWorkspaces) {
    result.error_message = base::StringPrintf(
        "Too many workspaces: %zu (max %zu)",
        workspaces->size(), kMaxWorkspaces);
    return result;
  }

  result.workspace_count = workspaces->size();

  // Validate each workspace.
  std::string error_msg;
  for (size_t i = 0; i < workspaces->size(); ++i) {
    const auto& item = (*workspaces)[i];
    if (!item.is_dict()) {
      result.error_message = WsError(i, "must be an object");
      return result;
    }
    if (!ValidateWorkspaceDict(item.GetDict(), i, error_msg)) {
      result.error_message = error_msg;
      return result;
    }

    // Count tabs.
    const base::Value::List* tabs = item.GetDict().FindList(kWorkspaceTabsKey);
    if (tabs) {
      result.total_tab_count += tabs->size();
    }
  }

  result.is_valid = true;
  return result;
}

bool AstraWorkspaceImportExport::ValidateWorkspaceDict(
    const base::Value::Dict& ws_dict,
    size_t index,
    std::string& error_message) {
  // Required: id (non-empty string).
  const std::string* id = ws_dict.FindString(kWorkspaceIdKey);
  if (!id || id->empty()) {
    error_message = WsError(index, "missing or invalid 'id' (must be a non-empty string)");
    return false;
  }

  // Required: name (string, must exist).
  if (!ws_dict.FindString(kWorkspaceNameKey)) {
    error_message = WsError(index, "missing required field: 'name'");
    return false;
  }

  // Optional: accent_color (string).
  // Optional: icon (string).
  // Optional: description (string).
  // Optional: order_index (int).
  // Optional: created_time (double).
  // Optional: last_used_time (double).
  // Optional: is_hibernated (bool).
  // Optional: tabs (list).

  // Validate tabs if present.
  const base::Value::List* tabs = ws_dict.FindList(kWorkspaceTabsKey);
  if (tabs) {
    // Enforce tab count limit per workspace.
    if (tabs->size() > kMaxTabsPerWorkspace) {
      error_message = WsError(
          index,
          base::StringPrintf("too many tabs: %zu (max %zu)",
                             tabs->size(), kMaxTabsPerWorkspace));
      return false;
    }

    for (size_t j = 0; j < tabs->size(); ++j) {
      const auto& tab_item = (*tabs)[j];
      if (!tab_item.is_dict()) {
        error_message = TabError(index, j, "must be an object");
        return false;
      }
      if (!ValidateTabDict(tab_item.GetDict(), index, j, error_message)) {
        return false;
      }
    }
  }

  return true;
}

bool AstraWorkspaceImportExport::ValidateTabDict(
    const base::Value::Dict& tab_dict,
    size_t ws_index,
    size_t tab_index,
    std::string& error_message) {
  // Required: url (string, must be valid and safe).
  const std::string* url_str = tab_dict.FindString(kTabUrlKey);
  if (!url_str || url_str->empty()) {
    error_message = TabError(ws_index, tab_index,
                             "missing or invalid 'url' (must be a non-empty string)");
    return false;
  }

  GURL url(*url_str);
  if (!url.is_valid()) {
    error_message = TabError(ws_index, tab_index,
                             "invalid URL: " + *url_str);
    return false;
  }

  if (!IsSafeUrlScheme(url)) {
    error_message = TabError(ws_index, tab_index,
                             "unsafe URL scheme: " + url.scheme());
    return false;
  }

  // Optional: title (string).
  // Optional: pinned (bool).
  // Optional: favorite (bool).

  return true;
}

bool AstraWorkspaceImportExport::IsSafeUrlScheme(const GURL& url) {
  if (!url.is_valid()) {
    return false;
  }

  // Allow standard web-safe schemes.
  if (url.SchemeIsHTTPOrHTTPS()) {
    return true;
  }

  // Allow about:blank (common default / new tab).
  if (url.spec() == "about:blank") {
    return true;
  }

  // Allow chrome:// URLs (internal pages).
  if (url.SchemeIs("chrome")) {
    return true;
  }

  // Disallow everything else: javascript:, data:, file:, etc.
  return false;
}

// =========================================================================
// Settings / PrefService
// =========================================================================

AstraWorkspaceExportOptions
AstraWorkspaceImportExport::GetDefaultExportOptions() const {
  AstraWorkspaceExportOptions options;
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return options;
  }

  options.include_tabs =
      prefs->GetBoolean(prefs::kPrefWorkspaceExportIncludeTabs);
  options.include_settings =
      prefs->GetBoolean(prefs::kPrefWorkspaceExportIncludeSettings);
  options.include_favorites =
      prefs->GetBoolean(prefs::kPrefWorkspaceExportIncludeFavorites);
  options.include_metadata =
      prefs->GetBoolean(prefs::kPrefWorkspaceExportIncludeMetadata);

  return options;
}

void AstraWorkspaceImportExport::SetDefaultExportOptions(
    const AstraWorkspaceExportOptions& options) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefWorkspaceExportIncludeTabs,
                    options.include_tabs);
  prefs->SetBoolean(prefs::kPrefWorkspaceExportIncludeSettings,
                    options.include_settings);
  prefs->SetBoolean(prefs::kPrefWorkspaceExportIncludeFavorites,
                    options.include_favorites);
  prefs->SetBoolean(prefs::kPrefWorkspaceExportIncludeMetadata,
                    options.include_metadata);
}

AstraWorkspaceImportOptions
AstraWorkspaceImportExport::GetDefaultImportOptions() const {
  AstraWorkspaceImportOptions options;
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return options;
  }

  int mode_int = prefs->GetInteger(prefs::kPrefWorkspaceImportMode);
  switch (mode_int) {
    case 0:
      options.mode = AstraWorkspaceImportMode::kMerge;
      break;
    case 1:
      options.mode = AstraWorkspaceImportMode::kReplace;
      break;
    default:
      options.mode = AstraWorkspaceImportMode::kMerge;
      break;
  }

  int conflict_int =
      prefs->GetInteger(prefs::kPrefWorkspaceImportConflictResolution);
  switch (conflict_int) {
    case 0:
      options.conflict_resolution = AstraWorkspaceConflictResolution::kRename;
      break;
    case 1:
      options.conflict_resolution = AstraWorkspaceConflictResolution::kSkip;
      break;
    default:
      options.conflict_resolution = AstraWorkspaceConflictResolution::kRename;
      break;
  }

  options.open_tabs =
      prefs->GetBoolean(prefs::kPrefWorkspaceImportOpenTabs);
  options.apply_favorites =
      prefs->GetBoolean(prefs::kPrefWorkspaceImportApplyFavorites);

  return options;
}

void AstraWorkspaceImportExport::SetDefaultImportOptions(
    const AstraWorkspaceImportOptions& options) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  int mode_int = 0;
  switch (options.mode) {
    case AstraWorkspaceImportMode::kMerge:
      mode_int = 0;
      break;
    case AstraWorkspaceImportMode::kReplace:
      mode_int = 1;
      break;
  }
  prefs->SetInteger(prefs::kPrefWorkspaceImportMode, mode_int);

  int conflict_int = 0;
  switch (options.conflict_resolution) {
    case AstraWorkspaceConflictResolution::kRename:
      conflict_int = 0;
      break;
    case AstraWorkspaceConflictResolution::kSkip:
      conflict_int = 1;
      break;
  }
  prefs->SetInteger(prefs::kPrefWorkspaceImportConflictResolution, conflict_int);

  prefs->SetBoolean(prefs::kPrefWorkspaceImportOpenTabs, options.open_tabs);
  prefs->SetBoolean(prefs::kPrefWorkspaceImportApplyFavorites,
                    options.apply_favorites);
}

PrefService* AstraWorkspaceImportExport::GetPrefs() const {
  if (!profile_) {
    return nullptr;
  }
  return profile_->GetPrefs();
}

// =========================================================================
// File path helpers
// =========================================================================

std::string AstraWorkspaceImportExport::GenerateExportFileName() {
  std::string timestamp =
      base::Time::Now().ToISO8601(false /* include_milliseconds */);
  // Replace colons with underscores for file system compatibility.
  std::string safe_timestamp;
  for (char c : timestamp) {
    safe_timestamp += (c == ':' || c == 'T' || c == 'Z') ? '_' : c;
  }
  return kExportFileNamePrefix + safe_timestamp + kExportFileExtension;
}

std::string AstraWorkspaceImportExport::GetExportFileExtension() {
  return kExportFileExtension;
}

// =========================================================================
// Observer notification helpers
// =========================================================================

void AstraWorkspaceImportExport::NotifyImportStarted(size_t total_workspaces) {
  for (auto& observer : observers_) {
    observer.OnImportStarted(total_workspaces);
  }
}

void AstraWorkspaceImportExport::NotifyImportProgress(size_t current,
                                                       size_t total) {
  for (auto& observer : observers_) {
    observer.OnImportProgress(current, total);
  }
}

void AstraWorkspaceImportExport::NotifyImportCompleted(
    const AstraWorkspaceImportResult& result) {
  for (auto& observer : observers_) {
    observer.OnImportCompleted(result);
  }
}

void AstraWorkspaceImportExport::NotifyImportFailed(
    const std::string& error_message) {
  for (auto& observer : observers_) {
    observer.OnImportFailed(error_message);
  }
}

void AstraWorkspaceImportExport::NotifyExportStarted() {
  for (auto& observer : observers_) {
    observer.OnExportStarted();
  }
}

void AstraWorkspaceImportExport::NotifyExportCompleted(
    const AstraWorkspaceExportResult& result) {
  for (auto& observer : observers_) {
    observer.OnExportCompleted(result);
  }
}

}  // namespace astra
