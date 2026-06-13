#ifndef ASTRA_BROWSER_ASTRA_SESSION_METADATA_H_
#define ASTRA_BROWSER_ASTRA_SESSION_METADATA_H_

#include <cstddef>
#include <cstdint>
#include <set>
#include <string>
#include <vector>

#include "base/time/time.h"
#include "base/values.h"
#include "ui/gfx/geometry/rect.h"

namespace content {
class WebContents;
}

namespace astra {

// Per-tab Astra metadata that rides along with Chromium's session restore data.
//
// Chromium owns all session restore logic (SessionService, TabRestoreService,
// session persistence on disk, tab creation order, lazy loading, crash
// recovery).  Astra only adds a small dictionary of extra per-tab metadata
// that is saved alongside each tab's session entry and re-applied when the
// tab is restored.
//
// Three tiers of Astra persistence:
//
//   1. Profile-level  — stored in PrefService, survives all sessions.
//        - Workspace definitions (id, name, color, icon, order).
//        - User preferences (default sidebar width, etc.).
//
//   2. Window-level   — stored in session window data, restored per window.
//        - Active workspace ID (which workspace the sidebar shows).
//        - Sidebar visibility and width.
//      TODO(astra): Window-level session data patch point.
//      Chromium component: sessions::SessionWindow / BaseSessionService.
//
//   3. Tab-level      — stored in session tab data, restored per tab.
//        - Workspace ID (which workspace this tab belongs to).
//        - Favorite state and folder membership.
//        - Split view configuration.
//        - Sidebar presentation flags (pinned).
//      This file handles tab-level metadata.
//
// The data format is a base::Value::Dict so it can be easily attached to
// Chromium's SerializedNavigationEntry or SessionTab extra_data fields.
// All keys are namespaced under "astra." to avoid collisions.

// -- Dict key constants -------------------------------------------------------
//
// These are exposed (not in an anonymous namespace) because the session
// restore patch point and tests need to read/write the same keys.

// Tab-level metadata keys.
//
// All keys are namespaced under "astra." to avoid collisions with Chromium's
// own session data keys.  These keys describe per-tab Astra metadata that
// rides along with Chromium's session restore pipeline.
inline constexpr char kMetaKeyWorkspaceId[] = "astra.workspace_id";
inline constexpr char kMetaKeyIsFavorite[] = "astra.is_favorite";
inline constexpr char kMetaKeyFavoriteFolderId[] = "astra.favorite_folder_id";
inline constexpr char kMetaKeyFavoriteFolder[] = "astra.favorite_folder";
inline constexpr char kMetaKeyFavoriteOrderIndex[] =
    "astra.favorite_order_index";
inline constexpr char kMetaKeySidebarPinned[] = "astra.sidebar_pinned";
inline constexpr char kMetaKeyIsPinned[] = "astra.is_pinned";
inline constexpr char kMetaKeyIsInSplitView[] = "astra.is_in_split_view";
inline constexpr char kMetaKeySplitViewPartnerId[] =
    "astra.split_view_partner_id";
inline constexpr char kMetaKeySplitViewPartner[] = "astra.split_view_partner";
inline constexpr char kMetaKeySplitViewRatio[] = "astra.split_view_ratio";
inline constexpr char kMetaKeySplitViewOrientation[] =
    "astra.split_view_orientation";

// -- Tab stack metadata keys ------------------------------------------------
//
// Named tab stacks: tabs can be members of a named stack (like Arc's tab
// orphans or Vivaldi's tab stacks).  Stack metadata (name, color, order)
// lives in PrefService via AstraTabStackService; per-tab membership is
// stored here and travels with the tab through session restore.
//
// Hierarchical stacks: tabs can also have a parent/child relationship
// (tree-style stacking).  The parent tab ID and collapsed state travel
// with each tab.
inline constexpr char kMetaKeyTabStackId[] = "astra.tab_stack_id";
inline constexpr char kMetaKeyStackParentId[] = "astra.stack_parent_id";
inline constexpr char kMetaKeyIsStackCollapsed[] = "astra.is_stack_collapsed";

// -- Notes metadata keys ---------------------------------------------------
//
// Per-tab note reference.  Notes are stored in PrefService (AstraNoteService);
// the session stores just a note ID so that the note association survives
// restarts.  We also store a truncated preview for UI display without
// needing to look up the full note.
inline constexpr char kMetaKeyNotes[] = "astra.notes";
inline constexpr char kMetaKeyNoteId[] = "astra.note_id";
inline constexpr char kMetaKeyNotePreview[] = "astra.note_preview";

// -- Glance / Peek metadata keys -------------------------------------------
//
// Glance tabs are temporary peek previews (similar to Arc's peek or Safari's
// tab preview).  The glance flag and source tab ID persist through session
// restore so that glance state is preserved across restarts.
inline constexpr char kMetaKeyIsGlanceTab[] = "astra.is_glance_tab";
inline constexpr char kMetaKeyGlanceSourceTabId[] =
    "astra.glance_source_tab_id";

// -- Picture-in-Picture metadata keys ---------------------------------------
//
// PiP tab flag indicates whether this tab is displayed as a picture-in-picture
// window.  The actual PiP window is owned by Chromium's
// PictureInPictureWindowController; this flag is Astra's projection.
inline constexpr char kMetaKeyIsPipTab[] = "astra.is_pip_tab";
inline constexpr char kMetaKeyPipWindowWidth[] = "astra.pip_window_width";
inline constexpr char kMetaKeyPipWindowHeight[] = "astra.pip_window_height";

// -- Reading list metadata keys --------------------------------------------
//
// Per-tab reading list state.  The actual reading list is owned by Chromium's
// ReadingListModel; this flag is Astra's projection of reading list membership
// for quick UI access during session restore.
inline constexpr char kMetaKeyIsInReadingList[] = "astra.is_in_reading_list";

// -- Tab lifecycle metadata keys --------------------------------------------
//
// Tab lifecycle tracking that persists through session restore.
// These are used for statistics, memory management, and UX decisions.
inline constexpr char kMetaKeyLastActiveTime[] = "astra.last_active_time";
inline constexpr char kMetaKeyDiscardCount[] = "astra.discard_count";

// -- Sidebar presentation metadata keys ------------------------------------
//
// Per-tab sidebar presentation flags.  sidebar_hidden indicates the tab
// should be hidden from the sidebar tab list (used for internal pages,
// DevTools, etc.).
inline constexpr char kMetaKeySidebarHidden[] = "astra.sidebar_hidden";

// Window-level metadata keys.
//
// These are attached to each window's session entry.
// Per-window Astra metadata rides along with Chromium's SessionWindow data.
inline constexpr char kMetaKeyWindowWorkspaceId[] = "astra.window_workspace_id";
inline constexpr char kMetaKeyWindowSavedBoundsX[] = "astra.window_saved_bounds_x";
inline constexpr char kMetaKeyWindowSavedBoundsY[] = "astra.window_saved_bounds_y";
inline constexpr char kMetaKeyWindowSavedBoundsWidth[] =
    "astra.window_saved_bounds_width";
inline constexpr char kMetaKeyWindowSavedBoundsHeight[] =
    "astra.window_saved_bounds_height";
inline constexpr char kMetaKeyWindowIsMinimized[] = "astra.window_is_minimized";
inline constexpr char kMetaKeyWindowIsMaximized[] = "astra.window_is_maximized";

// Window-level sidebar state.
// Per-window sidebar visibility and width override the profile-level defaults.
// When sidebar state is profile-level (not per-window), these keys are not
// present in the window metadata.
inline constexpr char kMetaKeyWindowSidebarVisible[] =
    "astra.window_sidebar_visible";
inline constexpr char kMetaKeyWindowSidebarWidth[] =
    "astra.window_sidebar_width";
inline constexpr char kMetaKeyWindowSidebarPinned[] =
    "astra.window_sidebar_pinned";

// Window order and workspace metadata.
inline constexpr char kMetaKeyWindowOrderIndex[] =
    "astra.window_order_index";

// Window-level split view state.
// When a window has an active split view, these keys describe the configuration.
// Per-tab split view data (partner_id, etc.) is stored at the tab level.
inline constexpr char kMetaKeyWindowSplitViewActive[] =
    "astra.window_split_view_active";
inline constexpr char kMetaKeyWindowSplitViewOrientation[] =
    "astra.window_split_view_orientation";
inline constexpr char kMetaKeyWindowSplitViewRatio[] =
    "astra.window_split_view_ratio";

// Hibernation state for the window.
// Hibernated windows are saved but not immediately restored on startup.
inline constexpr char kMetaKeyWindowIsHibernated[] =
    "astra.window_is_hibernated";

// -- Serialization ------------------------------------------------------------

// Extracts Astra tab metadata from |web_contents| (via AstraTabFeatures) and
// returns it as a base::Value::Dict suitable for attaching to a session tab.
// If the WebContents has no AstraTabFeatures, returns an empty dict.
//
// Called during session save from the Chromium session service patch point.
// Chromium component: sessions::BaseSessionService / TabRestoreService.
base::Value::Dict ExtractAstraMetadataFromWebContents(
    content::WebContents* web_contents);

// Applies Astra tab metadata from |metadata| to |web_contents|.
// Creates AstraTabFeatures on the WebContents if it does not already exist.
// Unknown or missing keys are ignored — only present values are applied.
//
// Called during session restore from the Chromium session service patch point.
// Chromium component: sessions::SessionRestore / TabRestoreService.
void ApplyAstraMetadataToWebContents(const base::Value::Dict& metadata,
                                     content::WebContents* web_contents);

// Returns true if |metadata| contains any Astra metadata keys.
// Used by the patch point to decide whether to attach extra data.
bool HasAstraMetadata(const base::Value::Dict& metadata);

// Validates that Astra tab metadata in |metadata| is well-formed.
//
// Checks type correctness for all known keys and range validity for numeric
// fields (e.g. split_view_ratio must be in [0.0, 1.0]).  Unknown keys are
// ignored — they may be from a newer version of Astra.
//
// Returns true if all present Astra keys have valid types and values.
// Returns false if any key has a wrong type or an out-of-range value.
//
// This is a safety check for session data that may have been corrupted or
// written by an older/newer version of the browser.
//
// Chromium component: sessions::BaseSessionService — called before saving
//   or after restoring metadata to catch corrupt data early.
bool ValidateAstraTabMetadata(const base::Value::Dict& metadata);

// Returns true if |metadata| contains no Astra tab metadata keys.
// An empty dict is trivially empty; a dict with only non-Astra keys
// (from Chromium or other extensions) is also considered empty.
bool IsEmptyAstraTabMetadata(const base::Value::Dict& metadata);

// Returns a deep copy of |metadata| containing only the Astra tab keys.
// Non-Astra keys (e.g. Chromium's own session data) are stripped.
// Returns an empty dict if |metadata| has no Astra keys.
base::Value::Dict CloneAstraTabMetadata(const base::Value::Dict& metadata);

// Merges |source| into |target|, overwriting any keys that exist in both.
// Only Astra tab metadata keys are merged — other keys in |source| are ignored.
//
// Use this to combine metadata from multiple sources (e.g. base session data
// plus an override set).
void MergeAstraTabMetadata(const base::Value::Dict& source,
                           base::Value::Dict& target);

// Returns the number of Astra tab metadata keys present in |metadata|.
// Non-Astra keys are not counted.
size_t GetAstraTabMetadataFieldCount(const base::Value::Dict& metadata);

// Fills in default values for any missing Astra tab metadata keys in |metadata|.
// Defaults match AstraTabFeatures' initial values (workspace = "default",
// is_favorite = false, etc.).
//
// This is useful when you need a complete metadata dict but the source
// data may be partial (e.g. from an older version of the browser).
void NormalizeAstraTabMetadata(base::Value::Dict& metadata);

// Returns true if two tab metadata dicts have the same values for all
// Astra keys.  Non-Astra keys are ignored.
//
// This does a value-wise comparison of all known Astra tab metadata keys.
bool AreAstraTabMetadataEqual(const base::Value::Dict& a,
                              const base::Value::Dict& b);

// -- Tab-level serialization helpers for sessions types ---------------------
//
// These helpers extract/apply Astra metadata from/to Chromium session types.
// They are convenience wrappers that operate on the extra_data dictionary
// stored in each session object.
//
// Chromium component: sessions::SessionTab
//   (components/sessions/session_tab.h)
// Chromium component: sessions::SerializedNavigationEntry
//   (components/sessions/serialized_navigation_entry.h)

namespace sessions {
class SessionTab;
class SerializedNavigationEntry;
}  // namespace sessions

// Extracts Astra metadata from a SessionTab's extra_data dict.
// Returns an empty dict if the tab has no Astra metadata.
//
// Chromium component: sessions::SessionTab::extra_data
base::Value::Dict ExtractAstraMetadataFromSessionTab(
    const sessions::SessionTab& session_tab);

// Applies Astra metadata to a SessionTab's extra_data dict.
// Merges |metadata| into the tab's existing extra_data.
//
// Chromium component: sessions::SessionTab::extra_data
void ApplyAstraMetadataToSessionTab(const base::Value::Dict& metadata,
                                    sessions::SessionTab* session_tab);

// Extracts Astra metadata from a SerializedNavigationEntry's extra data.
// Returns an empty dict if the entry has no Astra metadata.
//
// Chromium component: sessions::SerializedNavigationEntry
base::Value::Dict ExtractAstraMetadataFromNavigationEntry(
    const sessions::SerializedNavigationEntry& entry);

// Applies Astra metadata to a SerializedNavigationEntry's extra data.
//
// Chromium component: sessions::SerializedNavigationEntry
void ApplyAstraMetadataToNavigationEntry(
    const base::Value::Dict& metadata,
    sessions::SerializedNavigationEntry* entry);

// -- Window-level metadata ----------------------------------------------------
//
// Per-window Astra metadata that rides along with Chromium's session restore
// window data.
//
// Chromium owns all session restore logic (SessionService, session persistence
// on disk, window creation order, crash recovery).  Astra only adds a small
// dictionary of extra per-window metadata that is saved alongside each
// window's session entry and re-applied when the window is restored.
//
// Window-level data includes:
//   - Workspace ID (which workspace this window belongs to).
//   - Saved window bounds (for multi-monitor restore).
//   - Minimized/maximized state.
//
// The data format is a base::Value::Dict so it can be easily attached to
// Chromium's SessionWindow extra_data field.
// All keys are namespaced under "astra." to avoid collisions.

class Browser;

// Extracts Astra window metadata from |browser| (via AstraWindowFeatures) and
// returns it as a base::Value::Dict suitable for attaching to a session window.
// If the Browser has no AstraWindowFeatures, returns an empty dict.
//
// Called during session save from the Chromium session service patch point.
// Chromium component: sessions::BaseSessionService / SessionWindow.
base::Value::Dict ExtractAstraWindowMetadataFromBrowser(Browser* browser);

// Applies Astra window metadata from |metadata| to |browser|.
// Creates AstraWindowFeatures on the Browser if it does not already exist.
// Unknown or missing keys are ignored — only present values are applied.
//
// Called during session restore from the Chromium session service patch point.
// Chromium component: sessions::SessionRestore — where Browser windows are
// created from SessionWindow data.
void ApplyAstraWindowMetadataToBrowser(const base::Value::Dict& metadata,
                                       Browser* browser);

// Returns true if |metadata| contains any Astra window metadata keys.
// Used by the patch point to decide whether to attach extra data.
bool HasAstraWindowMetadata(const base::Value::Dict& metadata);

// Validates that Astra window metadata in |metadata| is well-formed.
//
// Checks type correctness for all known window-level keys and range
// validity for numeric fields.  Unknown keys are ignored.
//
// Returns true if all present Astra window keys have valid types and values.
// Returns false if any key has a wrong type or an out-of-range value.
bool ValidateAstraWindowMetadata(const base::Value::Dict& metadata);

// Returns true if |metadata| contains no Astra window metadata keys.
bool IsEmptyAstraWindowMetadata(const base::Value::Dict& metadata);

// Returns a deep copy of |metadata| containing only the Astra window keys.
// Non-Astra keys are stripped.
base::Value::Dict CloneAstraWindowMetadata(const base::Value::Dict& metadata);

// Merges |source| into |target| for window metadata keys.
// Only Astra window metadata keys are merged — other keys are ignored.
void MergeAstraWindowMetadata(const base::Value::Dict& source,
                              base::Value::Dict& target);

// Returns the number of Astra window metadata keys present in |metadata|.
size_t GetAstraWindowMetadataFieldCount(const base::Value::Dict& metadata);

// Fills in default values for missing Astra window metadata keys.
void NormalizeAstraWindowMetadata(base::Value::Dict& metadata);

// Returns true if two window metadata dicts have the same values for all
// Astra window keys.
bool AreAstraWindowMetadataEqual(const base::Value::Dict& a,
                                 const base::Value::Dict& b);

// -- Structured session metadata --------------------------------------------
//
// Type-safe struct representations of Astra session metadata.
// These structs provide a convenient API for working with session data
// and can be serialized to/from base::Value::Dict for persistence.
//
// The dict-based functions above are the low-level serialization layer;
// these structs are the higher-level typed API.

// Per-tab structured metadata.
//
// This is the type-safe version of the tab-level metadata dict.
// All fields correspond to keys in the "astra.*" namespace.
struct AstraTabSessionMetadata {
  // Tab identifier (used for cross-references like split view partners).
  // This is an Astra-level unique ID, not Chromium's tab handle.
  std::string tab_id;

  // Workspace membership.
  std::string workspace_id;

  // Favorite state.
  bool is_favorite = false;
  std::string favorite_folder_id;
  int favorite_order_index = 0;

  // Tab stack membership.
  std::string tab_stack_id;
  std::string stack_parent_id;
  bool is_stack_collapsed = false;

  // Pinned state projection (sidebar pinning).
  bool is_pinned = false;
  bool sidebar_pinned = false;

  // Sidebar presentation.
  bool sidebar_hidden = false;

  // Split view state.
  bool is_in_split_view = false;
  std::string split_view_partner_id;
  double split_view_ratio = 0.5;
  int split_view_orientation = 0;

  // Note association.
  std::string note_id;
  std::string note_preview;

  // Glance / peek state.
  bool is_glance_tab = false;
  std::string glance_source_tab_id;

  // PiP state.
  bool is_pip_tab = false;
  int pip_window_width = 0;
  int pip_window_height = 0;

  // Reading list state.
  bool is_in_reading_list = false;

  // Tab lifecycle tracking.
  base::Time last_active_time;
  int discard_count = 0;

  // -- Methods ---------------------------------------------------------------

  // Serializes this metadata to a base::Value::Dict.
  base::Value::Dict ToDict() const;

  // Deserializes from a base::Value::Dict into this struct.
  // Returns true if deserialization succeeded (all known fields parsed).
  // Missing fields are left at their default values.
  bool FromDict(const base::Value::Dict& dict);

  // Returns true if this tab metadata is valid (all fields within range).
  bool Validate() const;

  // Merges metadata from another tab into this one.
  // Fields that are set in |other| overwrite values in this struct.
  void MergeFrom(const AstraTabSessionMetadata& other);

  // Returns the estimated memory footprint of this tab's metadata in bytes.
  size_t EstimateSizeBytes() const;
};

// Per-window structured metadata.
//
// This is the type-safe version of the window-level metadata dict.
// Contains a list of tab metadata for all tabs in the window.
struct AstraWindowSessionMetadata {
  // Window identifier.
  std::string window_id;

  // Workspace the window belongs to.
  std::string workspace_id;

  // Order within the workspace (0-based).
  int window_order_index = 0;

  // Saved window bounds.
  gfx::Rect saved_bounds;

  // Window state.
  bool is_minimized = false;
  bool is_maximized = false;
  bool is_hibernated = false;

  // Sidebar state.
  bool sidebar_visible = true;
  bool sidebar_pinned = false;
  int sidebar_width = 300;

  // Window-level split view state.
  bool split_view_active = false;
  int split_view_orientation = 0;
  double split_view_ratio = 0.5;

  // Tabs in this window (in order).
  std::vector<AstraTabSessionMetadata> tabs;

  // -- Methods ---------------------------------------------------------------

  // Serializes this window metadata (including all tabs) to a dict.
  base::Value::Dict ToDict() const;

  // Deserializes window metadata from a dict.
  // Returns true if window-level fields parsed successfully.
  bool FromDict(const base::Value::Dict& dict);

  // Validates window metadata and all contained tab metadata.
  bool Validate() const;

  // Merges another window's metadata into this one.
  void MergeFrom(const AstraWindowSessionMetadata& other);

  // Returns the number of tabs in this window.
  size_t GetTabCount() const;

  // Returns the number of tabs with a given workspace_id in this window.
  size_t GetWorkspaceTabCount(const std::string& workspace_id) const;

  // Returns list of tab IDs for a given workspace in this window.
  std::vector<std::string> GetTabsByWorkspace(
      const std::string& workspace_id) const;

  // Returns the number of favorite tabs in this window.
  size_t GetFavoriteTabCount() const;

  // Returns the number of pinned tabs in this window.
  size_t GetPinnedTabCount() const;

  // Returns the number of tabs in stacks in this window.
  size_t GetStackedTabCount() const;

  // Returns estimated memory usage of this window's metadata in bytes.
  size_t EstimateSizeBytes() const;
};

// Full session structured metadata.
//
// Contains all windows and their tabs for a saved session.
// Provides aggregate statistics and utility methods.
struct AstraSessionMetadata {
  // Session name / identifier.
  std::string session_name;

  // When the session was saved.
  base::Time save_time;

  // All windows in the session.
  std::vector<AstraWindowSessionMetadata> windows;

  // -- Methods ---------------------------------------------------------------

  // Serializes the full session metadata to a dict.
  base::Value::Dict ToDict() const;

  // Deserializes session metadata from a dict.
  // Returns true if parsing succeeded for window-level structure.
  bool FromDict(const base::Value::Dict& dict);

  // Validates all windows and tabs in the session.
  bool Validate() const;

  // Merges another session's metadata into this one.
  void MergeFrom(const AstraSessionMetadata& other);

  // -- Statistics ------------------------------------------------------------

  // Total number of tabs across all windows.
  size_t GetTabCount() const;

  // Total number of windows.
  size_t GetWindowCount() const;

  // Number of unique workspaces represented in the session.
  size_t GetWorkspaceCount() const;

  // Total number of favorite tabs.
  size_t GetFavoriteTabCount() const;

  // Total number of pinned tabs.
  size_t GetPinnedTabCount() const;

  // Total number of tabs in stacks.
  size_t GetStackedTabCount() const;

  // Rough estimate of memory usage for all metadata in bytes.
  size_t GetEstimatedMemoryUsage() const;

  // Serialized size estimate (approximate dict size in bytes).
  size_t GetSessionSizeBytes() const;

  // -- Per-workspace queries -------------------------------------------------

  // Returns the number of tabs in a given workspace across all windows.
  size_t GetWorkspaceTabCount(const std::string& workspace_id) const;

  // Returns list of tab IDs for a given workspace across all windows.
  std::vector<std::string> GetTabsByWorkspace(
      const std::string& workspace_id) const;

  // Returns the number of tabs in a specific window.
  size_t GetWindowTabCount(const std::string& window_id) const;

  // Returns the set of unique workspace IDs in the session.
  std::set<std::string> GetWorkspaceIds() const;
};

// -- Window-level serialization helpers for sessions types ------------------

namespace sessions {
class SessionWindow;
}  // namespace sessions

// Extracts Astra metadata from a SessionWindow's extra_data dict.
// Returns an empty dict if the window has no Astra metadata.
//
// Chromium component: sessions::SessionWindow
base::Value::Dict ExtractAstraMetadataFromSessionWindow(
    const sessions::SessionWindow& session_window);

// Applies Astra metadata to a SessionWindow's extra_data dict.
// Merges |metadata| into the window's existing extra_data.
//
// Chromium component: sessions::SessionWindow
void ApplyAstraMetadataToSessionWindow(const base::Value::Dict& metadata,
                                       sessions::SessionWindow* session_window);

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_SESSION_METADATA_H_
