#include "astra/browser/astra_session_metadata.h"

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"

#include "astra/browser/astra_tab_features.h"
#include "astra/browser/astra_window_features.h"
#include "content/public/browser/web_contents.h"

namespace astra {

base::Value::Dict ExtractAstraMetadataFromWebContents(
    content::WebContents* web_contents) {
  base::Value::Dict metadata;

  AstraTabFeatures* features = AstraTabFeatures::FromWebContents(web_contents);
  if (!features) {
    return metadata;
  }

  metadata.Set(kMetaKeyWorkspaceId, features->workspace_id());
  metadata.Set(kMetaKeyIsFavorite, features->is_favorite());
  metadata.Set(kMetaKeyFavoriteFolderId, features->favorite_folder_id());
  metadata.Set(kMetaKeyFavoriteFolder, features->favorite_folder_id());
  metadata.Set(kMetaKeyFavoriteOrderIndex,
               static_cast<int>(features->favorite_order_index()));
  metadata.Set(kMetaKeySidebarPinned, features->sidebar_pinned());
  metadata.Set(kMetaKeyIsPinned, features->sidebar_pinned());
  metadata.Set(kMetaKeySidebarHidden, features->sidebar_hidden());
  metadata.Set(kMetaKeyIsInSplitView, features->is_in_split_view());
  metadata.Set(kMetaKeySplitViewPartnerId, features->split_view_partner_id());
  metadata.Set(kMetaKeySplitViewPartner, features->split_view_partner_id());
  metadata.Set(kMetaKeySplitViewRatio, features->split_view_ratio());
  metadata.Set(kMetaKeySplitViewOrientation,
               static_cast<int>(features->split_view_orientation()));

  // -- Tab stack metadata ---------------------------------------------------

  if (!features->stack_id().empty()) {
    metadata.Set(kMetaKeyTabStackId, features->stack_id());
  }
  if (features->is_in_stack()) {
    metadata.Set(kMetaKeyStackParentId, features->stack_parent_id());
    metadata.Set(kMetaKeyIsStackCollapsed, features->is_stack_collapsed());
  }

  // -- Notes metadata -------------------------------------------------------
  //
  // TODO(astra): Integrate with AstraNoteService to look up the note
  // associated with this tab's URL and store its ID + preview here.
  // Currently we only store the note_id if one has been set on
  // AstraTabFeatures.
  // Chromium owner: AstraNoteService (ProfileKeyedService).

  // -- Glance / Peek metadata -----------------------------------------------

  if (features->is_glance_tab()) {
    metadata.Set(kMetaKeyIsGlanceTab, true);
    if (!features->glance_source_tab_id().empty()) {
      metadata.Set(kMetaKeyGlanceSourceTabId,
                   features->glance_source_tab_id());
    }
  }

  // -- Picture-in-Picture metadata ------------------------------------------

  if (features->is_pip_tab()) {
    metadata.Set(kMetaKeyIsPipTab, true);
    if (!features->pip_window_size().IsEmpty()) {
      metadata.Set(kMetaKeyPipWindowWidth,
                   features->pip_window_size().width());
      metadata.Set(kMetaKeyPipWindowHeight,
                   features->pip_window_size().height());
    }
  }

  // -- Tab identity ---------------------------------------------------------

  if (!features->tab_unique_id().is_empty()) {
    metadata.Set(kMetaKeyTabUniqueId,
                 features->tab_unique_id().ToString());
  }

  if (!features->source_workspace_id().empty()) {
    metadata.Set(kMetaKeySourceWorkspaceId,
                 features->source_workspace_id());
  }

  if (features->has_tab_color()) {
    metadata.Set(kMetaKeyTabColor,
                 static_cast<int>(features->tab_color()));
  }

  // -- Astra-native tab states ----------------------------------------------

  if (features->read_later()) {
    metadata.Set(kMetaKeyReadLater, true);
  }

  if (features->is_snoozed()) {
    metadata.Set(kMetaKeyIsSnoozed, true);
    if (!features->snooze_time().is_null()) {
      double time_us = static_cast<double>(
          features->snooze_time().ToDeltaSinceWindowsEpoch().InMicroseconds());
      metadata.Set(kMetaKeySnoozeTime, time_us);
    }
  }

  if (features->is_hibernated()) {
    metadata.Set(kMetaKeyIsHibernatedTab, true);
  }

  if (!features->created_time().is_null()) {
    double time_us = static_cast<double>(
        features->created_time().ToDeltaSinceWindowsEpoch().InMicroseconds());
    metadata.Set(kMetaKeyCreatedTime, time_us);
  }

  return metadata;
}

void ApplyAstraMetadataToWebContents(const base::Value::Dict& metadata,
                                     content::WebContents* web_contents) {
  if (metadata.empty()) {
    return;
  }

  AstraTabFeatures* features =
      AstraTabFeatures::GetOrCreateForWebContents(web_contents);

  if (const std::string* workspace_id =
          metadata.FindString(kMetaKeyWorkspaceId)) {
    features->set_workspace_id(*workspace_id);
  }

  if (std::optional<bool> is_favorite =
          metadata.FindBool(kMetaKeyIsFavorite)) {
    features->set_is_favorite(*is_favorite);
  }

  if (const std::string* folder_id =
          metadata.FindString(kMetaKeyFavoriteFolderId)) {
    features->set_favorite_folder_id(*folder_id);
  }

  // kMetaKeyFavoriteFolder is an alias for kMetaKeyFavoriteFolderId.
  if (const std::string* folder = metadata.FindString(kMetaKeyFavoriteFolder)) {
    features->set_favorite_folder_id(*folder);
  }

  if (std::optional<int> order_index =
          metadata.FindInt(kMetaKeyFavoriteOrderIndex)) {
    features->set_favorite_order_index(
        static_cast<size_t>(std::max(0, *order_index)));
  }

  if (std::optional<bool> sidebar_pinned =
          metadata.FindBool(kMetaKeySidebarPinned)) {
    features->set_sidebar_pinned(*sidebar_pinned);
  }

  // kMetaKeyIsPinned is an alias for kMetaKeySidebarPinned.
  if (std::optional<bool> is_pinned = metadata.FindBool(kMetaKeyIsPinned)) {
    features->set_sidebar_pinned(*is_pinned);
  }

  if (std::optional<bool> sidebar_hidden =
          metadata.FindBool(kMetaKeySidebarHidden)) {
    features->set_sidebar_hidden(*sidebar_hidden);
  }

  if (std::optional<bool> in_split =
          metadata.FindBool(kMetaKeyIsInSplitView)) {
    features->set_is_in_split_view(*in_split);
  }

  if (const std::string* partner_id =
          metadata.FindString(kMetaKeySplitViewPartnerId)) {
    features->set_split_view_partner_id(*partner_id);
  }
  // kMetaKeySplitViewPartner is an alias for kMetaKeySplitViewPartnerId.
  if (const std::string* partner =
          metadata.FindString(kMetaKeySplitViewPartner)) {
    features->set_split_view_partner_id(*partner);
  }

  if (std::optional<double> ratio =
          metadata.FindDouble(kMetaKeySplitViewRatio)) {
    // Clamp ratio to [0.0, 1.0] to be defensive against corrupt session data.
    float clamped =
        std::max(0.0f, std::min(1.0f, static_cast<float>(*ratio)));
    features->set_split_view_ratio(clamped);
  }

  if (std::optional<int> orientation =
          metadata.FindInt(kMetaKeySplitViewOrientation)) {
    // Clamp to valid enum range to be defensive against corrupt session data.
    int clamped = std::max(
        0, std::min(*orientation,
                    static_cast<int>(SplitViewOrientation::kVertical)));
    features->set_split_view_orientation(
        static_cast<SplitViewOrientation>(clamped));
  }

  // -- Tab stack metadata ---------------------------------------------------

  if (const std::string* stack_id = metadata.FindString(kMetaKeyTabStackId)) {
    features->set_stack_id(*stack_id);
  }

  if (const std::string* stack_parent_id =
          metadata.FindString(kMetaKeyStackParentId)) {
    features->set_stack_parent_id(*stack_parent_id);
  }

  if (std::optional<bool> stack_collapsed =
          metadata.FindBool(kMetaKeyIsStackCollapsed)) {
    features->set_stack_collapsed(*stack_collapsed);
  }

  // -- Notes metadata -------------------------------------------------------

  // TODO(astra): Apply note_id to AstraTabFeatures when note association
  // is added to the tab features.
  // Currently notes are URL-keyed in AstraNoteService, not tab-keyed.
  // Per-tab note reference requires a note_id field on AstraTabFeatures.
  // Chromium owner: AstraNoteService (ProfileKeyedService).

  // -- Glance / Peek metadata -----------------------------------------------

  if (std::optional<bool> is_glance = metadata.FindBool(kMetaKeyIsGlanceTab)) {
    features->set_is_glance_tab(*is_glance);
  }

  if (const std::string* glance_source =
          metadata.FindString(kMetaKeyGlanceSourceTabId)) {
    features->set_glance_source_tab_id(*glance_source);
  }

  // -- Picture-in-Picture metadata ------------------------------------------

  if (std::optional<bool> is_pip = metadata.FindBool(kMetaKeyIsPipTab)) {
    features->set_is_pip_tab(*is_pip);
  }

  std::optional<int> pip_width = metadata.FindInt(kMetaKeyPipWindowWidth);
  std::optional<int> pip_height = metadata.FindInt(kMetaKeyPipWindowHeight);
  if (pip_width.has_value() && pip_height.has_value() &&
      *pip_width > 0 && *pip_height > 0) {
    features->set_pip_window_size(gfx::Size(*pip_width, *pip_height));
  }

  // -- Tab identity ---------------------------------------------------------

  if (const std::string* unique_id_str =
          metadata.FindString(kMetaKeyTabUniqueId)) {
    base::UnguessableToken token =
        base::UnguessableToken::Deserialize(0, 0);
    // Try to parse the string representation.
    // TODO(astra): Use base::UnguessableToken::CreateFromString when available,
    // or a proper deserialization method.  For now, we accept the string as-is
    // and store it; the tab_unique_id setter on AstraTabFeatures takes a
    // base::UnguessableToken.
    //
    // For session restore round-trip correctness, we serialize with ToString()
    // and deserialize here.  UnguessableToken string format is
    // "(%016llX,%016llX)" (high, low in hex).
    if (!unique_id_str->empty()) {
      // Simple parsing: try to extract high and low from the string format.
      uint64_t high = 0, low = 0;
      if (sscanf(unique_id_str->c_str(), "(%llX,%llX)",
                 (unsigned long long*)&high,
                 (unsigned long long*)&low) == 2) {
        token = base::UnguessableToken::Deserialize(high, low);
      }
    }
    if (!token.is_empty()) {
      features->set_tab_unique_id(token);
    }
  }

  if (const std::string* source_ws =
          metadata.FindString(kMetaKeySourceWorkspaceId)) {
    features->set_source_workspace_id(*source_ws);
  }

  if (std::optional<int> color_int = metadata.FindInt(kMetaKeyTabColor)) {
    if (*color_int != 0) {
      features->set_tab_color(static_cast<SkColor>(*color_int));
    }
  }

  // -- Astra-native tab states ----------------------------------------------

  if (std::optional<bool> read_later = metadata.FindBool(kMetaKeyReadLater)) {
    features->set_read_later(*read_later);
  }

  if (std::optional<bool> is_snoozed = metadata.FindBool(kMetaKeyIsSnoozed)) {
    if (*is_snoozed) {
      if (auto v = metadata.FindDouble(kMetaKeySnoozeTime)) {
        base::Time snooze_time = base::Time::FromDeltaSinceWindowsEpoch(
            base::Microseconds(static_cast<int64_t>(*v)));
        features->SnoozeUntil(snooze_time);
      } else {
        // Snoozed but no time set — use a default far-future time.
        // TODO(astra): Handle this edge case more gracefully.
      }
    } else {
      features->CancelSnooze();
    }
  }

  if (std::optional<bool> is_hibernated =
          metadata.FindBool(kMetaKeyIsHibernatedTab)) {
    if (*is_hibernated) {
      features->Hibernate();
    } else {
      features->Wake();
    }
  }

  if (auto v = metadata.FindDouble(kMetaKeyCreatedTime)) {
    base::Time created_time = base::Time::FromDeltaSinceWindowsEpoch(
        base::Microseconds(static_cast<int64_t>(*v)));
    features->set_created_time(created_time);
  }
}

bool HasAstraMetadata(const base::Value::Dict& metadata) {
  // Check for the workspace_id key as a sentinel — if it's present, we have
  // Astra metadata.  We could check all keys, but workspace_id is always set
  // (defaults to "default") so it's sufficient as a presence check.
  return metadata.contains(kMetaKeyWorkspaceId);
}

// ---------------------------------------------------------------------------
// Tab metadata utility functions
// ---------------------------------------------------------------------------

bool IsEmptyAstraTabMetadata(const base::Value::Dict& metadata) {
  if (metadata.empty()) {
    return true;
  }
  return GetAstraTabMetadataFieldCount(metadata) == 0;
}

base::Value::Dict CloneAstraTabMetadata(const base::Value::Dict& metadata) {
  base::Value::Dict result;

  // Workspace
  if (const std::string* v = metadata.FindString(kMetaKeyWorkspaceId)) {
    result.Set(kMetaKeyWorkspaceId, *v);
  }

  // Favorites
  if (std::optional<bool> v = metadata.FindBool(kMetaKeyIsFavorite)) {
    result.Set(kMetaKeyIsFavorite, *v);
  }
  if (const std::string* v = metadata.FindString(kMetaKeyFavoriteFolderId)) {
    result.Set(kMetaKeyFavoriteFolderId, *v);
  }
  if (const std::string* v = metadata.FindString(kMetaKeyFavoriteFolder)) {
    result.Set(kMetaKeyFavoriteFolder, *v);
  }
  if (std::optional<int> v = metadata.FindInt(kMetaKeyFavoriteOrderIndex)) {
    result.Set(kMetaKeyFavoriteOrderIndex, *v);
  }

  // Sidebar
  if (std::optional<bool> v = metadata.FindBool(kMetaKeySidebarPinned)) {
    result.Set(kMetaKeySidebarPinned, *v);
  }
  if (std::optional<bool> v = metadata.FindBool(kMetaKeyIsPinned)) {
    result.Set(kMetaKeyIsPinned, *v);
  }
  if (std::optional<bool> v = metadata.FindBool(kMetaKeySidebarHidden)) {
    result.Set(kMetaKeySidebarHidden, *v);
  }

  // Split view
  if (std::optional<bool> v = metadata.FindBool(kMetaKeyIsInSplitView)) {
    result.Set(kMetaKeyIsInSplitView, *v);
  }
  if (const std::string* v = metadata.FindString(kMetaKeySplitViewPartnerId)) {
    result.Set(kMetaKeySplitViewPartnerId, *v);
  }
  if (const std::string* v = metadata.FindString(kMetaKeySplitViewPartner)) {
    result.Set(kMetaKeySplitViewPartner, *v);
  }
  if (std::optional<double> v = metadata.FindDouble(kMetaKeySplitViewRatio)) {
    result.Set(kMetaKeySplitViewRatio, *v);
  }
  if (std::optional<int> v = metadata.FindInt(kMetaKeySplitViewOrientation)) {
    result.Set(kMetaKeySplitViewOrientation, *v);
  }

  // Tab stacks
  if (const std::string* v = metadata.FindString(kMetaKeyTabStackId)) {
    result.Set(kMetaKeyTabStackId, *v);
  }
  if (const std::string* v = metadata.FindString(kMetaKeyStackParentId)) {
    result.Set(kMetaKeyStackParentId, *v);
  }
  if (std::optional<bool> v = metadata.FindBool(kMetaKeyIsStackCollapsed)) {
    result.Set(kMetaKeyIsStackCollapsed, *v);
  }

  // Notes
  if (const std::string* v = metadata.FindString(kMetaKeyNoteId)) {
    result.Set(kMetaKeyNoteId, *v);
  }
  if (const std::string* v = metadata.FindString(kMetaKeyNotePreview)) {
    result.Set(kMetaKeyNotePreview, *v);
  }
  // kMetaKeyNotes is a list key
  if (const base::Value::List* v = metadata.FindList(kMetaKeyNotes)) {
    result.Set(kMetaKeyNotes, v->Clone());
  }

  // Glance
  if (std::optional<bool> v = metadata.FindBool(kMetaKeyIsGlanceTab)) {
    result.Set(kMetaKeyIsGlanceTab, *v);
  }
  if (const std::string* v = metadata.FindString(kMetaKeyGlanceSourceTabId)) {
    result.Set(kMetaKeyGlanceSourceTabId, *v);
  }

  // PiP
  if (std::optional<bool> v = metadata.FindBool(kMetaKeyIsPipTab)) {
    result.Set(kMetaKeyIsPipTab, *v);
  }
  if (std::optional<int> v = metadata.FindInt(kMetaKeyPipWindowWidth)) {
    result.Set(kMetaKeyPipWindowWidth, *v);
  }
  if (std::optional<int> v = metadata.FindInt(kMetaKeyPipWindowHeight)) {
    result.Set(kMetaKeyPipWindowHeight, *v);
  }

  // Reading list
  if (std::optional<bool> v = metadata.FindBool(kMetaKeyIsInReadingList)) {
    result.Set(kMetaKeyIsInReadingList, *v);
  }

  // Tab lifecycle
  if (std::optional<double> v = metadata.FindDouble(kMetaKeyLastActiveTime)) {
    result.Set(kMetaKeyLastActiveTime, *v);
  }
  if (std::optional<int> v = metadata.FindInt(kMetaKeyDiscardCount)) {
    result.Set(kMetaKeyDiscardCount, *v);
  }

  // Tab identity
  if (const std::string* v = metadata.FindString(kMetaKeyTabUniqueId)) {
    result.Set(kMetaKeyTabUniqueId, *v);
  }
  if (const std::string* v = metadata.FindString(kMetaKeySourceWorkspaceId)) {
    result.Set(kMetaKeySourceWorkspaceId, *v);
  }
  if (std::optional<int> v = metadata.FindInt(kMetaKeyTabColor)) {
    result.Set(kMetaKeyTabColor, *v);
  }

  // Astra-native tab states
  if (std::optional<bool> v = metadata.FindBool(kMetaKeyReadLater)) {
    result.Set(kMetaKeyReadLater, *v);
  }
  if (std::optional<bool> v = metadata.FindBool(kMetaKeyIsSnoozed)) {
    result.Set(kMetaKeyIsSnoozed, *v);
  }
  if (std::optional<double> v = metadata.FindDouble(kMetaKeySnoozeTime)) {
    result.Set(kMetaKeySnoozeTime, *v);
  }
  if (std::optional<bool> v = metadata.FindBool(kMetaKeyIsHibernatedTab)) {
    result.Set(kMetaKeyIsHibernatedTab, *v);
  }
  if (std::optional<double> v = metadata.FindDouble(kMetaKeyCreatedTime)) {
    result.Set(kMetaKeyCreatedTime, *v);
  }

  return result;
}

void MergeAstraTabMetadata(const base::Value::Dict& source,
                           base::Value::Dict& target) {
  base::Value::Dict source_only = CloneAstraTabMetadata(source);
  target.Merge(std::move(source_only));
}

size_t GetAstraTabMetadataFieldCount(const base::Value::Dict& metadata) {
  size_t count = 0;

  const char* kTabKeys[] = {
    kMetaKeyWorkspaceId,
    kMetaKeyIsFavorite,
    kMetaKeyFavoriteFolderId,
    kMetaKeyFavoriteFolder,
    kMetaKeyFavoriteOrderIndex,
    kMetaKeySidebarPinned,
    kMetaKeyIsPinned,
    kMetaKeySidebarHidden,
    kMetaKeyIsInSplitView,
    kMetaKeySplitViewPartnerId,
    kMetaKeySplitViewPartner,
    kMetaKeySplitViewRatio,
    kMetaKeySplitViewOrientation,
    kMetaKeyTabStackId,
    kMetaKeyStackParentId,
    kMetaKeyIsStackCollapsed,
    kMetaKeyNotes,
    kMetaKeyNoteId,
    kMetaKeyNotePreview,
    kMetaKeyIsGlanceTab,
    kMetaKeyGlanceSourceTabId,
    kMetaKeyIsPipTab,
    kMetaKeyPipWindowWidth,
    kMetaKeyPipWindowHeight,
    kMetaKeyIsInReadingList,
    kMetaKeyLastActiveTime,
    kMetaKeyDiscardCount,
    kMetaKeyTabUniqueId,
    kMetaKeySourceWorkspaceId,
    kMetaKeyTabColor,
    kMetaKeyReadLater,
    kMetaKeyIsSnoozed,
    kMetaKeySnoozeTime,
    kMetaKeyIsHibernatedTab,
    kMetaKeyCreatedTime,
  };

  for (const char* key : kTabKeys) {
    if (metadata.contains(key)) {
      count++;
    }
  }

  return count;
}

void NormalizeAstraTabMetadata(base::Value::Dict& metadata) {
  // Workspace: default to "default"
  if (!metadata.contains(kMetaKeyWorkspaceId)) {
    metadata.Set(kMetaKeyWorkspaceId, "default");
  }

  // Favorites: default to not favorited, root folder, index 0
  if (!metadata.contains(kMetaKeyIsFavorite)) {
    metadata.Set(kMetaKeyIsFavorite, false);
  }
  if (!metadata.contains(kMetaKeyFavoriteFolderId)) {
    metadata.Set(kMetaKeyFavoriteFolderId, "root");
  }
  if (!metadata.contains(kMetaKeyFavoriteOrderIndex)) {
    metadata.Set(kMetaKeyFavoriteOrderIndex, 0);
  }

  // Sidebar: default to not pinned, not hidden
  if (!metadata.contains(kMetaKeySidebarPinned)) {
    metadata.Set(kMetaKeySidebarPinned, false);
  }
  if (!metadata.contains(kMetaKeySidebarHidden)) {
    metadata.Set(kMetaKeySidebarHidden, false);
  }

  // Split view: default to not in split view, 0.5 ratio, horizontal
  if (!metadata.contains(kMetaKeyIsInSplitView)) {
    metadata.Set(kMetaKeyIsInSplitView, false);
  }
  if (!metadata.contains(kMetaKeySplitViewRatio)) {
    metadata.Set(kMetaKeySplitViewRatio, 0.5);
  }
  if (!metadata.contains(kMetaKeySplitViewOrientation)) {
    metadata.Set(kMetaKeySplitViewOrientation,
                 static_cast<int>(SplitViewOrientation::kHorizontal));
  }

  // Tab stacks: default to not in a stack
  if (!metadata.contains(kMetaKeyIsStackCollapsed)) {
    metadata.Set(kMetaKeyIsStackCollapsed, false);
  }

  // Glance: default to not a glance tab
  if (!metadata.contains(kMetaKeyIsGlanceTab)) {
    metadata.Set(kMetaKeyIsGlanceTab, false);
  }

  // PiP: default to not a PiP tab
  if (!metadata.contains(kMetaKeyIsPipTab)) {
    metadata.Set(kMetaKeyIsPipTab, false);
  }

  // Reading list: default to not in reading list
  if (!metadata.contains(kMetaKeyIsInReadingList)) {
    metadata.Set(kMetaKeyIsInReadingList, false);
  }

  // Discard count: default to 0
  if (!metadata.contains(kMetaKeyDiscardCount)) {
    metadata.Set(kMetaKeyDiscardCount, 0);
  }

  // Tab color: default to 0 (no color)
  if (!metadata.contains(kMetaKeyTabColor)) {
    metadata.Set(kMetaKeyTabColor, 0);
  }

  // Read later: default to false
  if (!metadata.contains(kMetaKeyReadLater)) {
    metadata.Set(kMetaKeyReadLater, false);
  }

  // Snooze: default to not snoozed
  if (!metadata.contains(kMetaKeyIsSnoozed)) {
    metadata.Set(kMetaKeyIsSnoozed, false);
  }

  // Hibernation: default to not hibernated
  if (!metadata.contains(kMetaKeyIsHibernatedTab)) {
    metadata.Set(kMetaKeyIsHibernatedTab, false);
  }
}

bool AreAstraTabMetadataEqual(const base::Value::Dict& a,
                              const base::Value::Dict& b) {
  // Clone both to strip non-Astra keys, then compare.
  base::Value::Dict a_clone = CloneAstraTabMetadata(a);
  base::Value::Dict b_clone = CloneAstraTabMetadata(b);
  return a_clone == b_clone;
}

// ---------------------------------------------------------------------------
// Window-level metadata
// ---------------------------------------------------------------------------

base::Value::Dict ExtractAstraWindowMetadataFromBrowser(Browser* browser) {
  base::Value::Dict metadata;

  AstraWindowFeatures* features = AstraWindowFeatures::FromBrowser(browser);
  if (!features) {
    return metadata;
  }

  metadata.Set(kMetaKeyWindowWorkspaceId, features->workspace_id());

  // Save window bounds (for multi-monitor workspace restore).
  const gfx::Rect& bounds = features->saved_bounds();
  if (!bounds.IsEmpty()) {
    metadata.Set(kMetaKeyWindowSavedBoundsX, bounds.x());
    metadata.Set(kMetaKeyWindowSavedBoundsY, bounds.y());
    metadata.Set(kMetaKeyWindowSavedBoundsWidth, bounds.width());
    metadata.Set(kMetaKeyWindowSavedBoundsHeight, bounds.height());
  }

  metadata.Set(kMetaKeyWindowIsMinimized, features->is_minimized());
  metadata.Set(kMetaKeyWindowIsMaximized, features->is_maximized());

  // -- Window-level sidebar state -------------------------------------------
  //
  // Currently sidebar state is profile-level (see astra_prefs.h).
  // When per-window sidebar state is implemented, these values will be
  // populated from AstraWindowFeatures.
  //
  // TODO(astra): Add per-window sidebar state to AstraWindowFeatures and
  // populate these values during session save.
  // Chromium component: AstraWindowFeatures (Browser user data).

  return metadata;
}

void ApplyAstraWindowMetadataToBrowser(const base::Value::Dict& metadata,
                                       Browser* browser) {
  if (metadata.empty() || !browser) {
    return;
  }

  AstraWindowFeatures* features =
      AstraWindowFeatures::GetOrCreateForBrowser(browser);
  DCHECK(features);

  if (features->workspace_read_only()) {
    // Incognito windows: skip workspace_id assignment — it's locked to default.
    // But we still apply saved bounds and window state (those are per-window).
  } else {
    if (const std::string* workspace_id =
            metadata.FindString(kMetaKeyWindowWorkspaceId)) {
      features->set_workspace_id(*workspace_id);
    }
  }

  // Restore saved bounds (for multi-monitor workspace restore).
  std::optional<int> x = metadata.FindInt(kMetaKeyWindowSavedBoundsX);
  std::optional<int> y = metadata.FindInt(kMetaKeyWindowSavedBoundsY);
  std::optional<int> width = metadata.FindInt(kMetaKeyWindowSavedBoundsWidth);
  std::optional<int> height = metadata.FindInt(kMetaKeyWindowSavedBoundsHeight);
  if (x.has_value() && y.has_value() && width.has_value() && height.has_value()) {
    features->set_saved_bounds(
        gfx::Rect(*x, *y, *width, *height));
  }

  if (std::optional<bool> is_minimized =
          metadata.FindBool(kMetaKeyWindowIsMinimized)) {
    features->set_is_minimized(*is_minimized);
  }

  if (std::optional<bool> is_maximized =
          metadata.FindBool(kMetaKeyWindowIsMaximized)) {
    features->set_is_maximized(*is_maximized);
  }

  // -- Window-level sidebar state -------------------------------------------
  //
  // TODO(astra): Apply per-window sidebar state to AstraWindowFeatures
  // when per-window sidebar state is implemented.
  // Currently sidebar state is profile-level (astra_prefs.h).
  // Chromium component: AstraWindowFeatures (Browser user data).
}

bool HasAstraWindowMetadata(const base::Value::Dict& metadata) {
  // Check for the window_workspace_id key as a sentinel.
  return metadata.contains(kMetaKeyWindowWorkspaceId);
}

// ---------------------------------------------------------------------------
// Window metadata utility functions
// ---------------------------------------------------------------------------

bool IsEmptyAstraWindowMetadata(const base::Value::Dict& metadata) {
  if (metadata.empty()) {
    return true;
  }
  return GetAstraWindowMetadataFieldCount(metadata) == 0;
}

base::Value::Dict CloneAstraWindowMetadata(const base::Value::Dict& metadata) {
  base::Value::Dict result;

  // Workspace
  if (const std::string* v = metadata.FindString(kMetaKeyWindowWorkspaceId)) {
    result.Set(kMetaKeyWindowWorkspaceId, *v);
  }

  // Saved bounds
  if (std::optional<int> v = metadata.FindInt(kMetaKeyWindowSavedBoundsX)) {
    result.Set(kMetaKeyWindowSavedBoundsX, *v);
  }
  if (std::optional<int> v = metadata.FindInt(kMetaKeyWindowSavedBoundsY)) {
    result.Set(kMetaKeyWindowSavedBoundsY, *v);
  }
  if (std::optional<int> v = metadata.FindInt(kMetaKeyWindowSavedBoundsWidth)) {
    result.Set(kMetaKeyWindowSavedBoundsWidth, *v);
  }
  if (std::optional<int> v = metadata.FindInt(kMetaKeyWindowSavedBoundsHeight)) {
    result.Set(kMetaKeyWindowSavedBoundsHeight, *v);
  }

  // Window state
  if (std::optional<bool> v = metadata.FindBool(kMetaKeyWindowIsMinimized)) {
    result.Set(kMetaKeyWindowIsMinimized, *v);
  }
  if (std::optional<bool> v = metadata.FindBool(kMetaKeyWindowIsMaximized)) {
    result.Set(kMetaKeyWindowIsMaximized, *v);
  }

  // Sidebar
  if (std::optional<bool> v = metadata.FindBool(kMetaKeyWindowSidebarVisible)) {
    result.Set(kMetaKeyWindowSidebarVisible, *v);
  }
  if (std::optional<int> v = metadata.FindInt(kMetaKeyWindowSidebarWidth)) {
    result.Set(kMetaKeyWindowSidebarWidth, *v);
  }
  if (std::optional<bool> v = metadata.FindBool(kMetaKeyWindowSidebarPinned)) {
    result.Set(kMetaKeyWindowSidebarPinned, *v);
  }

  // Window order
  if (std::optional<int> v = metadata.FindInt(kMetaKeyWindowOrderIndex)) {
    result.Set(kMetaKeyWindowOrderIndex, *v);
  }

  // Split view (window-level)
  if (std::optional<bool> v = metadata.FindBool(kMetaKeyWindowSplitViewActive)) {
    result.Set(kMetaKeyWindowSplitViewActive, *v);
  }
  if (std::optional<int> v = metadata.FindInt(kMetaKeyWindowSplitViewOrientation)) {
    result.Set(kMetaKeyWindowSplitViewOrientation, *v);
  }
  if (std::optional<double> v = metadata.FindDouble(kMetaKeyWindowSplitViewRatio)) {
    result.Set(kMetaKeyWindowSplitViewRatio, *v);
  }

  // Hibernation
  if (std::optional<bool> v = metadata.FindBool(kMetaKeyWindowIsHibernated)) {
    result.Set(kMetaKeyWindowIsHibernated, *v);
  }

  return result;
}

void MergeAstraWindowMetadata(const base::Value::Dict& source,
                              base::Value::Dict& target) {
  base::Value::Dict source_only = CloneAstraWindowMetadata(source);
  target.Merge(std::move(source_only));
}

size_t GetAstraWindowMetadataFieldCount(const base::Value::Dict& metadata) {
  size_t count = 0;

  const char* kWindowKeys[] = {
    kMetaKeyWindowWorkspaceId,
    kMetaKeyWindowSavedBoundsX,
    kMetaKeyWindowSavedBoundsY,
    kMetaKeyWindowSavedBoundsWidth,
    kMetaKeyWindowSavedBoundsHeight,
    kMetaKeyWindowIsMinimized,
    kMetaKeyWindowIsMaximized,
    kMetaKeyWindowSidebarVisible,
    kMetaKeyWindowSidebarWidth,
    kMetaKeyWindowSidebarPinned,
    kMetaKeyWindowOrderIndex,
    kMetaKeyWindowSplitViewActive,
    kMetaKeyWindowSplitViewOrientation,
    kMetaKeyWindowSplitViewRatio,
    kMetaKeyWindowIsHibernated,
  };

  for (const char* key : kWindowKeys) {
    if (metadata.contains(key)) {
      count++;
    }
  }

  return count;
}

void NormalizeAstraWindowMetadata(base::Value::Dict& metadata) {
  // Workspace: default to "default"
  if (!metadata.contains(kMetaKeyWindowWorkspaceId)) {
    metadata.Set(kMetaKeyWindowWorkspaceId, "default");
  }

  // Window state: default to not minimized, not maximized
  if (!metadata.contains(kMetaKeyWindowIsMinimized)) {
    metadata.Set(kMetaKeyWindowIsMinimized, false);
  }
  if (!metadata.contains(kMetaKeyWindowIsMaximized)) {
    metadata.Set(kMetaKeyWindowIsMaximized, false);
  }

  // Hibernation: default to not hibernated
  if (!metadata.contains(kMetaKeyWindowIsHibernated)) {
    metadata.Set(kMetaKeyWindowIsHibernated, false);
  }

  // Window order: default to 0
  if (!metadata.contains(kMetaKeyWindowOrderIndex)) {
    metadata.Set(kMetaKeyWindowOrderIndex, 0);
  }

  // Sidebar: default to visible, not pinned, 300px width
  if (!metadata.contains(kMetaKeyWindowSidebarVisible)) {
    metadata.Set(kMetaKeyWindowSidebarVisible, true);
  }
  if (!metadata.contains(kMetaKeyWindowSidebarPinned)) {
    metadata.Set(kMetaKeyWindowSidebarPinned, false);
  }
  if (!metadata.contains(kMetaKeyWindowSidebarWidth)) {
    metadata.Set(kMetaKeyWindowSidebarWidth, 300);
  }

  // Split view (window-level): default to inactive, 0.5 ratio, horizontal
  if (!metadata.contains(kMetaKeyWindowSplitViewActive)) {
    metadata.Set(kMetaKeyWindowSplitViewActive, false);
  }
  if (!metadata.contains(kMetaKeyWindowSplitViewRatio)) {
    metadata.Set(kMetaKeyWindowSplitViewRatio, 0.5);
  }
  if (!metadata.contains(kMetaKeyWindowSplitViewOrientation)) {
    metadata.Set(kMetaKeyWindowSplitViewOrientation, 0);
  }
}

bool AreAstraWindowMetadataEqual(const base::Value::Dict& a,
                                 const base::Value::Dict& b) {
  base::Value::Dict a_clone = CloneAstraWindowMetadata(a);
  base::Value::Dict b_clone = CloneAstraWindowMetadata(b);
  return a_clone == b_clone;
}

// ---------------------------------------------------------------------------
// Validation
// ---------------------------------------------------------------------------

bool ValidateAstraTabMetadata(const base::Value::Dict& metadata) {
  // If there's no Astra metadata at all, it's trivially valid.
  if (metadata.empty()) {
    return true;
  }

  // -- Workspace -----------------------------------------------------------

  if (metadata.contains(kMetaKeyWorkspaceId)) {
    if (!metadata.FindString(kMetaKeyWorkspaceId)) {
      return false;
    }
  }

  // -- Favorites -----------------------------------------------------------

  if (metadata.contains(kMetaKeyIsFavorite) &&
      !metadata.FindBool(kMetaKeyIsFavorite)) {
    return false;
  }

  if (metadata.contains(kMetaKeyFavoriteFolderId) &&
      !metadata.FindString(kMetaKeyFavoriteFolderId)) {
    return false;
  }

  if (metadata.contains(kMetaKeyFavoriteFolder) &&
      !metadata.FindString(kMetaKeyFavoriteFolder)) {
    return false;
  }

  if (metadata.contains(kMetaKeyFavoriteOrderIndex)) {
    std::optional<int> val = metadata.FindInt(kMetaKeyFavoriteOrderIndex);
    if (!val.has_value() || *val < 0) {
      return false;
    }
  }

  // -- Sidebar presentation ------------------------------------------------

  if (metadata.contains(kMetaKeySidebarPinned) &&
      !metadata.FindBool(kMetaKeySidebarPinned)) {
    return false;
  }

  if (metadata.contains(kMetaKeyIsPinned) &&
      !metadata.FindBool(kMetaKeyIsPinned)) {
    return false;
  }

  if (metadata.contains(kMetaKeySidebarHidden) &&
      !metadata.FindBool(kMetaKeySidebarHidden)) {
    return false;
  }

  // -- Split view ----------------------------------------------------------

  if (metadata.contains(kMetaKeyIsInSplitView) &&
      !metadata.FindBool(kMetaKeyIsInSplitView)) {
    return false;
  }

  if (metadata.contains(kMetaKeySplitViewPartnerId) &&
      !metadata.FindString(kMetaKeySplitViewPartnerId)) {
    return false;
  }

  if (metadata.contains(kMetaKeySplitViewPartner) &&
      !metadata.FindString(kMetaKeySplitViewPartner)) {
    return false;
  }

  if (metadata.contains(kMetaKeySplitViewRatio)) {
    std::optional<double> val = metadata.FindDouble(kMetaKeySplitViewRatio);
    if (!val.has_value() || *val < 0.0 || *val > 1.0) {
      return false;
    }
  }

  if (metadata.contains(kMetaKeySplitViewOrientation)) {
    std::optional<int> val = metadata.FindInt(kMetaKeySplitViewOrientation);
    if (!val.has_value() ||
        *val < 0 ||
        *val > static_cast<int>(SplitViewOrientation::kVertical)) {
      return false;
    }
  }

  // -- Tab stacks ----------------------------------------------------------

  if (metadata.contains(kMetaKeyTabStackId) &&
      !metadata.FindString(kMetaKeyTabStackId)) {
    return false;
  }

  if (metadata.contains(kMetaKeyStackParentId) &&
      !metadata.FindString(kMetaKeyStackParentId)) {
    return false;
  }

  if (metadata.contains(kMetaKeyIsStackCollapsed) &&
      !metadata.FindBool(kMetaKeyIsStackCollapsed)) {
    return false;
  }

  // -- Notes ---------------------------------------------------------------

  if (metadata.contains(kMetaKeyNoteId) &&
      !metadata.FindString(kMetaKeyNoteId)) {
    return false;
  }

  if (metadata.contains(kMetaKeyNotePreview) &&
      !metadata.FindString(kMetaKeyNotePreview)) {
    return false;
  }

  // -- Glance / Peek -------------------------------------------------------

  if (metadata.contains(kMetaKeyIsGlanceTab) &&
      !metadata.FindBool(kMetaKeyIsGlanceTab)) {
    return false;
  }

  if (metadata.contains(kMetaKeyGlanceSourceTabId) &&
      !metadata.FindString(kMetaKeyGlanceSourceTabId)) {
    return false;
  }

  // -- Picture-in-Picture --------------------------------------------------

  if (metadata.contains(kMetaKeyIsPipTab) &&
      !metadata.FindBool(kMetaKeyIsPipTab)) {
    return false;
  }

  if (metadata.contains(kMetaKeyPipWindowWidth)) {
    std::optional<int> val = metadata.FindInt(kMetaKeyPipWindowWidth);
    if (!val.has_value() || *val < 0) {
      return false;
    }
  }

  if (metadata.contains(kMetaKeyPipWindowHeight)) {
    std::optional<int> val = metadata.FindInt(kMetaKeyPipWindowHeight);
    if (!val.has_value() || *val < 0) {
      return false;
    }
  }

  // -- Reading list --------------------------------------------------------

  if (metadata.contains(kMetaKeyIsInReadingList) &&
      !metadata.FindBool(kMetaKeyIsInReadingList)) {
    return false;
  }

  // -- Tab lifecycle -------------------------------------------------------

  if (metadata.contains(kMetaKeyLastActiveTime)) {
    if (!metadata.FindDouble(kMetaKeyLastActiveTime)) {
      return false;
    }
  }

  if (metadata.contains(kMetaKeyDiscardCount)) {
    std::optional<int> val = metadata.FindInt(kMetaKeyDiscardCount);
    if (!val.has_value() || *val < 0) {
      return false;
    }
  }

  // -- Tab identity --------------------------------------------------------

  if (metadata.contains(kMetaKeyTabUniqueId) &&
      !metadata.FindString(kMetaKeyTabUniqueId)) {
    return false;
  }

  if (metadata.contains(kMetaKeySourceWorkspaceId) &&
      !metadata.FindString(kMetaKeySourceWorkspaceId)) {
    return false;
  }

  if (metadata.contains(kMetaKeyTabColor)) {
    std::optional<int> val = metadata.FindInt(kMetaKeyTabColor);
    if (!val.has_value() || *val < 0) {
      return false;
    }
  }

  // -- Astra-native tab states ---------------------------------------------

  if (metadata.contains(kMetaKeyReadLater) &&
      !metadata.FindBool(kMetaKeyReadLater)) {
    return false;
  }

  if (metadata.contains(kMetaKeyIsSnoozed) &&
      !metadata.FindBool(kMetaKeyIsSnoozed)) {
    return false;
  }

  if (metadata.contains(kMetaKeySnoozeTime)) {
    if (!metadata.FindDouble(kMetaKeySnoozeTime)) {
      return false;
    }
  }

  if (metadata.contains(kMetaKeyIsHibernatedTab) &&
      !metadata.FindBool(kMetaKeyIsHibernatedTab)) {
    return false;
  }

  if (metadata.contains(kMetaKeyCreatedTime)) {
    if (!metadata.FindDouble(kMetaKeyCreatedTime)) {
      return false;
    }
  }

  // Unknown keys are allowed (forward compatibility with newer versions).
  return true;
}

bool ValidateAstraWindowMetadata(const base::Value::Dict& metadata) {
  if (metadata.empty()) {
    return true;
  }

  // -- Workspace -----------------------------------------------------------

  if (metadata.contains(kMetaKeyWindowWorkspaceId) &&
      !metadata.FindString(kMetaKeyWindowWorkspaceId)) {
    return false;
  }

  // -- Saved bounds --------------------------------------------------------

  if (metadata.contains(kMetaKeyWindowSavedBoundsX) &&
      !metadata.FindInt(kMetaKeyWindowSavedBoundsX)) {
    return false;
  }

  if (metadata.contains(kMetaKeyWindowSavedBoundsY) &&
      !metadata.FindInt(kMetaKeyWindowSavedBoundsY)) {
    return false;
  }

  if (metadata.contains(kMetaKeyWindowSavedBoundsWidth)) {
    std::optional<int> val =
        metadata.FindInt(kMetaKeyWindowSavedBoundsWidth);
    if (!val.has_value() || *val < 0) {
      return false;
    }
  }

  if (metadata.contains(kMetaKeyWindowSavedBoundsHeight)) {
    std::optional<int> val =
        metadata.FindInt(kMetaKeyWindowSavedBoundsHeight);
    if (!val.has_value() || *val < 0) {
      return false;
    }
  }

  // -- Window state --------------------------------------------------------

  if (metadata.contains(kMetaKeyWindowIsMinimized) &&
      !metadata.FindBool(kMetaKeyWindowIsMinimized)) {
    return false;
  }

  if (metadata.contains(kMetaKeyWindowIsMaximized) &&
      !metadata.FindBool(kMetaKeyWindowIsMaximized)) {
    return false;
  }

  // -- Sidebar -------------------------------------------------------------

  if (metadata.contains(kMetaKeyWindowSidebarVisible) &&
      !metadata.FindBool(kMetaKeyWindowSidebarVisible)) {
    return false;
  }

  if (metadata.contains(kMetaKeyWindowSidebarWidth)) {
    std::optional<int> val = metadata.FindInt(kMetaKeyWindowSidebarWidth);
    if (!val.has_value() || *val < 0) {
      return false;
    }
  }

  if (metadata.contains(kMetaKeyWindowSidebarPinned) &&
      !metadata.FindBool(kMetaKeyWindowSidebarPinned)) {
    return false;
  }

  // -- Window order --------------------------------------------------------

  if (metadata.contains(kMetaKeyWindowOrderIndex)) {
    std::optional<int> val = metadata.FindInt(kMetaKeyWindowOrderIndex);
    if (!val.has_value() || *val < 0) {
      return false;
    }
  }

  // -- Window-level split view ---------------------------------------------

  if (metadata.contains(kMetaKeyWindowSplitViewActive) &&
      !metadata.FindBool(kMetaKeyWindowSplitViewActive)) {
    return false;
  }

  if (metadata.contains(kMetaKeyWindowSplitViewOrientation)) {
    std::optional<int> val =
        metadata.FindInt(kMetaKeyWindowSplitViewOrientation);
    if (!val.has_value() || *val < 0 || *val > 1) {
      return false;
    }
  }

  if (metadata.contains(kMetaKeyWindowSplitViewRatio)) {
    std::optional<double> val =
        metadata.FindDouble(kMetaKeyWindowSplitViewRatio);
    if (!val.has_value() || *val < 0.0 || *val > 1.0) {
      return false;
    }
  }

  // -- Hibernation ---------------------------------------------------------

  if (metadata.contains(kMetaKeyWindowIsHibernated) &&
      !metadata.FindBool(kMetaKeyWindowIsHibernated)) {
    return false;
  }

  // Unknown keys are allowed (forward compatibility with newer versions).
  return true;
}

// ---------------------------------------------------------------------------
// Structured tab metadata implementation
// ---------------------------------------------------------------------------

base::Value::Dict AstraTabSessionMetadata::ToDict() const {
  base::Value::Dict dict;

  if (!tab_id.empty()) {
    dict.Set("tab_id", tab_id);
  }
  if (!workspace_id.empty()) {
    dict.Set(kMetaKeyWorkspaceId, workspace_id);
  }

  dict.Set(kMetaKeyIsFavorite, is_favorite);
  if (!favorite_folder_id.empty()) {
    dict.Set(kMetaKeyFavoriteFolderId, favorite_folder_id);
  }
  dict.Set(kMetaKeyFavoriteOrderIndex, favorite_order_index);

  if (!tab_stack_id.empty()) {
    dict.Set(kMetaKeyTabStackId, tab_stack_id);
  }
  if (!stack_parent_id.empty()) {
    dict.Set(kMetaKeyStackParentId, stack_parent_id);
  }
  dict.Set(kMetaKeyIsStackCollapsed, is_stack_collapsed);

  dict.Set(kMetaKeyIsPinned, is_pinned);
  dict.Set(kMetaKeySidebarPinned, sidebar_pinned);
  dict.Set(kMetaKeySidebarHidden, sidebar_hidden);

  dict.Set(kMetaKeyIsInSplitView, is_in_split_view);
  if (!split_view_partner_id.empty()) {
    dict.Set(kMetaKeySplitViewPartnerId, split_view_partner_id);
  }
  dict.Set(kMetaKeySplitViewRatio, split_view_ratio);
  dict.Set(kMetaKeySplitViewOrientation, split_view_orientation);

  if (!note_id.empty()) {
    dict.Set(kMetaKeyNoteId, note_id);
  }
  if (!note_preview.empty()) {
    dict.Set(kMetaKeyNotePreview, note_preview);
  }

  dict.Set(kMetaKeyIsGlanceTab, is_glance_tab);
  if (!glance_source_tab_id.empty()) {
    dict.Set(kMetaKeyGlanceSourceTabId, glance_source_tab_id);
  }

  dict.Set(kMetaKeyIsPipTab, is_pip_tab);
  if (pip_window_width > 0 && pip_window_height > 0) {
    dict.Set(kMetaKeyPipWindowWidth, pip_window_width);
    dict.Set(kMetaKeyPipWindowHeight, pip_window_height);
  }

  dict.Set(kMetaKeyIsInReadingList, is_in_reading_list);

  if (!last_active_time.is_null()) {
    double time_us = static_cast<double>(
        last_active_time.ToDeltaSinceWindowsEpoch().InMicroseconds());
    dict.Set(kMetaKeyLastActiveTime, time_us);
  }
  if (discard_count > 0) {
    dict.Set(kMetaKeyDiscardCount, discard_count);
  }

  // -- Tab identity ---------------------------------------------------------

  if (!tab_unique_id.empty()) {
    dict.Set(kMetaKeyTabUniqueId, tab_unique_id);
  }
  if (!source_workspace_id.empty()) {
    dict.Set(kMetaKeySourceWorkspaceId, source_workspace_id);
  }
  if (tab_color != 0) {
    dict.Set(kMetaKeyTabColor, static_cast<int>(tab_color));
  }

  // -- Astra-native tab states ----------------------------------------------

  dict.Set(kMetaKeyReadLater, read_later);

  dict.Set(kMetaKeyIsSnoozed, is_snoozed);
  if (!snooze_time.is_null()) {
    double time_us = static_cast<double>(
        snooze_time.ToDeltaSinceWindowsEpoch().InMicroseconds());
    dict.Set(kMetaKeySnoozeTime, time_us);
  }

  dict.Set(kMetaKeyIsHibernatedTab, is_hibernated);

  if (!created_time.is_null()) {
    double time_us = static_cast<double>(
        created_time.ToDeltaSinceWindowsEpoch().InMicroseconds());
    dict.Set(kMetaKeyCreatedTime, time_us);
  }

  return dict;
}

bool AstraTabSessionMetadata::FromDict(const base::Value::Dict& dict) {
  if (const std::string* v = dict.FindString("tab_id")) {
    tab_id = *v;
  }
  if (const std::string* v = dict.FindString(kMetaKeyWorkspaceId)) {
    workspace_id = *v;
  }

  if (auto v = dict.FindBool(kMetaKeyIsFavorite)) {
    is_favorite = *v;
  }
  if (const std::string* v = dict.FindString(kMetaKeyFavoriteFolderId)) {
    favorite_folder_id = *v;
  }
  if (auto v = dict.FindInt(kMetaKeyFavoriteOrderIndex)) {
    favorite_order_index = std::max(0, *v);
  }

  if (const std::string* v = dict.FindString(kMetaKeyTabStackId)) {
    tab_stack_id = *v;
  }
  if (const std::string* v = dict.FindString(kMetaKeyStackParentId)) {
    stack_parent_id = *v;
  }
  if (auto v = dict.FindBool(kMetaKeyIsStackCollapsed)) {
    is_stack_collapsed = *v;
  }

  if (auto v = dict.FindBool(kMetaKeyIsPinned)) {
    is_pinned = *v;
  }
  if (auto v = dict.FindBool(kMetaKeySidebarPinned)) {
    sidebar_pinned = *v;
  }
  if (auto v = dict.FindBool(kMetaKeySidebarHidden)) {
    sidebar_hidden = *v;
  }

  if (auto v = dict.FindBool(kMetaKeyIsInSplitView)) {
    is_in_split_view = *v;
  }
  if (const std::string* v = dict.FindString(kMetaKeySplitViewPartnerId)) {
    split_view_partner_id = *v;
  }
  if (auto v = dict.FindDouble(kMetaKeySplitViewRatio)) {
    split_view_ratio = std::max(0.0, std::min(1.0, *v));
  }
  if (auto v = dict.FindInt(kMetaKeySplitViewOrientation)) {
    split_view_orientation = std::max(0, std::min(1, *v));
  }

  if (const std::string* v = dict.FindString(kMetaKeyNoteId)) {
    note_id = *v;
  }
  if (const std::string* v = dict.FindString(kMetaKeyNotePreview)) {
    note_preview = *v;
  }

  if (auto v = dict.FindBool(kMetaKeyIsGlanceTab)) {
    is_glance_tab = *v;
  }
  if (const std::string* v = dict.FindString(kMetaKeyGlanceSourceTabId)) {
    glance_source_tab_id = *v;
  }

  if (auto v = dict.FindBool(kMetaKeyIsPipTab)) {
    is_pip_tab = *v;
  }
  if (auto v = dict.FindInt(kMetaKeyPipWindowWidth)) {
    pip_window_width = std::max(0, *v);
  }
  if (auto v = dict.FindInt(kMetaKeyPipWindowHeight)) {
    pip_window_height = std::max(0, *v);
  }

  if (auto v = dict.FindBool(kMetaKeyIsInReadingList)) {
    is_in_reading_list = *v;
  }

  if (auto v = dict.FindDouble(kMetaKeyLastActiveTime)) {
    last_active_time = base::Time::FromDeltaSinceWindowsEpoch(
        base::Microseconds(static_cast<int64_t>(*v)));
  }
  if (auto v = dict.FindInt(kMetaKeyDiscardCount)) {
    discard_count = std::max(0, *v);
  }

  // -- Tab identity ---------------------------------------------------------

  if (const std::string* v = dict.FindString(kMetaKeyTabUniqueId)) {
    tab_unique_id = *v;
  }
  if (const std::string* v = dict.FindString(kMetaKeySourceWorkspaceId)) {
    source_workspace_id = *v;
  }
  if (auto v = dict.FindInt(kMetaKeyTabColor)) {
    tab_color = static_cast<uint32_t>(std::max(0, *v));
  }

  // -- Astra-native tab states ----------------------------------------------

  if (auto v = dict.FindBool(kMetaKeyReadLater)) {
    read_later = *v;
  }

  if (auto v = dict.FindBool(kMetaKeyIsSnoozed)) {
    is_snoozed = *v;
  }
  if (auto v = dict.FindDouble(kMetaKeySnoozeTime)) {
    snooze_time = base::Time::FromDeltaSinceWindowsEpoch(
        base::Microseconds(static_cast<int64_t>(*v)));
  }

  if (auto v = dict.FindBool(kMetaKeyIsHibernatedTab)) {
    is_hibernated = *v;
  }

  if (auto v = dict.FindDouble(kMetaKeyCreatedTime)) {
    created_time = base::Time::FromDeltaSinceWindowsEpoch(
        base::Microseconds(static_cast<int64_t>(*v)));
  }

  return true;
}

bool AstraTabSessionMetadata::Validate() const {
  if (favorite_order_index < 0) return false;
  if (split_view_ratio < 0.0 || split_view_ratio > 1.0) return false;
  if (split_view_orientation < 0 || split_view_orientation > 1) return false;
  if (pip_window_width < 0) return false;
  if (pip_window_height < 0) return false;
  if (discard_count < 0) return false;
  if (is_snoozed && snooze_time.is_null()) return false;
  return true;
}

void AstraTabSessionMetadata::MergeFrom(const AstraTabSessionMetadata& other) {
  if (!other.tab_id.empty()) tab_id = other.tab_id;
  if (!other.workspace_id.empty()) workspace_id = other.workspace_id;

  is_favorite = other.is_favorite;
  if (!other.favorite_folder_id.empty()) {
    favorite_folder_id = other.favorite_folder_id;
  }
  favorite_order_index = other.favorite_order_index;

  if (!other.tab_stack_id.empty()) tab_stack_id = other.tab_stack_id;
  if (!other.stack_parent_id.empty()) stack_parent_id = other.stack_parent_id;
  is_stack_collapsed = other.is_stack_collapsed;

  is_pinned = other.is_pinned;
  sidebar_pinned = other.sidebar_pinned;
  sidebar_hidden = other.sidebar_hidden;

  is_in_split_view = other.is_in_split_view;
  if (!other.split_view_partner_id.empty()) {
    split_view_partner_id = other.split_view_partner_id;
  }
  split_view_ratio = other.split_view_ratio;
  split_view_orientation = other.split_view_orientation;

  if (!other.note_id.empty()) note_id = other.note_id;
  if (!other.note_preview.empty()) note_preview = other.note_preview;

  is_glance_tab = other.is_glance_tab;
  if (!other.glance_source_tab_id.empty()) {
    glance_source_tab_id = other.glance_source_tab_id;
  }

  is_pip_tab = other.is_pip_tab;
  pip_window_width = other.pip_window_width;
  pip_window_height = other.pip_window_height;

  is_in_reading_list = other.is_in_reading_list;

  if (!other.last_active_time.is_null()) {
    last_active_time = other.last_active_time;
  }
  discard_count = other.discard_count;

  // -- Tab identity ---------------------------------------------------------

  if (!other.tab_unique_id.empty()) {
    tab_unique_id = other.tab_unique_id;
  }
  if (!other.source_workspace_id.empty()) {
    source_workspace_id = other.source_workspace_id;
  }
  if (other.tab_color != 0) {
    tab_color = other.tab_color;
  }

  // -- Astra-native tab states ----------------------------------------------

  read_later = other.read_later;
  is_snoozed = other.is_snoozed;
  if (!other.snooze_time.is_null()) {
    snooze_time = other.snooze_time;
  }
  is_hibernated = other.is_hibernated;
  if (!other.created_time.is_null()) {
    created_time = other.created_time;
  }
}

size_t AstraTabSessionMetadata::EstimateSizeBytes() const {
  size_t size = sizeof(AstraTabSessionMetadata);
  size += tab_id.capacity();
  size += workspace_id.capacity();
  size += favorite_folder_id.capacity();
  size += tab_stack_id.capacity();
  size += stack_parent_id.capacity();
  size += split_view_partner_id.capacity();
  size += note_id.capacity();
  size += note_preview.capacity();
  size += glance_source_tab_id.capacity();
  size += tab_unique_id.capacity();
  size += source_workspace_id.capacity();
  return size;
}

// ---------------------------------------------------------------------------
// Structured window metadata implementation
// ---------------------------------------------------------------------------

base::Value::Dict AstraWindowSessionMetadata::ToDict() const {
  base::Value::Dict dict;

  if (!window_id.empty()) {
    dict.Set("window_id", window_id);
  }
  if (!workspace_id.empty()) {
    dict.Set(kMetaKeyWindowWorkspaceId, workspace_id);
  }

  dict.Set(kMetaKeyWindowOrderIndex, window_order_index);

  if (!saved_bounds.IsEmpty()) {
    dict.Set(kMetaKeyWindowSavedBoundsX, saved_bounds.x());
    dict.Set(kMetaKeyWindowSavedBoundsY, saved_bounds.y());
    dict.Set(kMetaKeyWindowSavedBoundsWidth, saved_bounds.width());
    dict.Set(kMetaKeyWindowSavedBoundsHeight, saved_bounds.height());
  }

  dict.Set(kMetaKeyWindowIsMinimized, is_minimized);
  dict.Set(kMetaKeyWindowIsMaximized, is_maximized);
  dict.Set(kMetaKeyWindowIsHibernated, is_hibernated);

  dict.Set(kMetaKeyWindowSidebarVisible, sidebar_visible);
  dict.Set(kMetaKeyWindowSidebarPinned, sidebar_pinned);
  dict.Set(kMetaKeyWindowSidebarWidth, sidebar_width);

  dict.Set(kMetaKeyWindowSplitViewActive, split_view_active);
  dict.Set(kMetaKeyWindowSplitViewOrientation, split_view_orientation);
  dict.Set(kMetaKeyWindowSplitViewRatio, split_view_ratio);

  base::Value::List tabs_list;
  for (const auto& tab : tabs) {
    tabs_list.Append(tab.ToDict());
  }
  if (!tabs_list.empty()) {
    dict.Set("tabs", std::move(tabs_list));
  }

  return dict;
}

bool AstraWindowSessionMetadata::FromDict(const base::Value::Dict& dict) {
  if (const std::string* v = dict.FindString("window_id")) {
    window_id = *v;
  }
  if (const std::string* v = dict.FindString(kMetaKeyWindowWorkspaceId)) {
    workspace_id = *v;
  }

  if (auto v = dict.FindInt(kMetaKeyWindowOrderIndex)) {
    window_order_index = std::max(0, *v);
  }

  auto x = dict.FindInt(kMetaKeyWindowSavedBoundsX);
  auto y = dict.FindInt(kMetaKeyWindowSavedBoundsY);
  auto w = dict.FindInt(kMetaKeyWindowSavedBoundsWidth);
  auto h = dict.FindInt(kMetaKeyWindowSavedBoundsHeight);
  if (x.has_value() && y.has_value() && w.has_value() && h.has_value()) {
    saved_bounds = gfx::Rect(std::max(0, *x), std::max(0, *y),
                              std::max(0, *w), std::max(0, *h));
  }

  if (auto v = dict.FindBool(kMetaKeyWindowIsMinimized)) {
    is_minimized = *v;
  }
  if (auto v = dict.FindBool(kMetaKeyWindowIsMaximized)) {
    is_maximized = *v;
  }
  if (auto v = dict.FindBool(kMetaKeyWindowIsHibernated)) {
    is_hibernated = *v;
  }

  if (auto v = dict.FindBool(kMetaKeyWindowSidebarVisible)) {
    sidebar_visible = *v;
  }
  if (auto v = dict.FindBool(kMetaKeyWindowSidebarPinned)) {
    sidebar_pinned = *v;
  }
  if (auto v = dict.FindInt(kMetaKeyWindowSidebarWidth)) {
    sidebar_width = std::max(0, *v);
  }

  if (auto v = dict.FindBool(kMetaKeyWindowSplitViewActive)) {
    split_view_active = *v;
  }
  if (auto v = dict.FindInt(kMetaKeyWindowSplitViewOrientation)) {
    split_view_orientation = std::max(0, std::min(1, *v));
  }
  if (auto v = dict.FindDouble(kMetaKeyWindowSplitViewRatio)) {
    split_view_ratio = std::max(0.0, std::min(1.0, *v));
  }

  // Parse tabs list.
  const base::Value::List* tabs_list = dict.FindList("tabs");
  if (tabs_list) {
    tabs.clear();
    tabs.reserve(tabs_list->size());
    for (const auto& tab_val : *tabs_list) {
      if (tab_val.is_dict()) {
        AstraTabSessionMetadata tab;
        tab.FromDict(tab_val.GetDict());
        tabs.push_back(std::move(tab));
      }
    }
  }

  return true;
}

bool AstraWindowSessionMetadata::Validate() const {
  if (window_order_index < 0) return false;
  if (sidebar_width < 0) return false;
  if (split_view_orientation < 0 || split_view_orientation > 1) return false;
  if (split_view_ratio < 0.0 || split_view_ratio > 1.0) return false;
  if (saved_bounds.width() < 0 || saved_bounds.height() < 0) return false;

  for (const auto& tab : tabs) {
    if (!tab.Validate()) return false;
  }

  return true;
}

void AstraWindowSessionMetadata::MergeFrom(
    const AstraWindowSessionMetadata& other) {
  if (!other.window_id.empty()) window_id = other.window_id;
  if (!other.workspace_id.empty()) workspace_id = other.workspace_id;

  window_order_index = other.window_order_index;
  if (!other.saved_bounds.IsEmpty()) saved_bounds = other.saved_bounds;

  is_minimized = other.is_minimized;
  is_maximized = other.is_maximized;
  is_hibernated = other.is_hibernated;

  sidebar_visible = other.sidebar_visible;
  sidebar_pinned = other.sidebar_pinned;
  sidebar_width = other.sidebar_width;

  split_view_active = other.split_view_active;
  split_view_orientation = other.split_view_orientation;
  split_view_ratio = other.split_view_ratio;

  // Merge tabs by ID, or append if not found.
  for (const auto& other_tab : other.tabs) {
    bool found = false;
    for (auto& our_tab : tabs) {
      if (!other_tab.tab_id.empty() && our_tab.tab_id == other_tab.tab_id) {
        our_tab.MergeFrom(other_tab);
        found = true;
        break;
      }
    }
    if (!found) {
      tabs.push_back(other_tab);
    }
  }
}

size_t AstraWindowSessionMetadata::GetTabCount() const {
  return tabs.size();
}

size_t AstraWindowSessionMetadata::GetWorkspaceTabCount(
    const std::string& ws_id) const {
  size_t count = 0;
  for (const auto& tab : tabs) {
    if (tab.workspace_id == ws_id) {
      count++;
    }
  }
  return count;
}

std::vector<std::string> AstraWindowSessionMetadata::GetTabsByWorkspace(
    const std::string& ws_id) const {
  std::vector<std::string> result;
  for (const auto& tab : tabs) {
    if (tab.workspace_id == ws_id) {
      result.push_back(tab.tab_id);
    }
  }
  return result;
}

size_t AstraWindowSessionMetadata::GetFavoriteTabCount() const {
  size_t count = 0;
  for (const auto& tab : tabs) {
    if (tab.is_favorite) count++;
  }
  return count;
}

size_t AstraWindowSessionMetadata::GetPinnedTabCount() const {
  size_t count = 0;
  for (const auto& tab : tabs) {
    if (tab.is_pinned || tab.sidebar_pinned) count++;
  }
  return count;
}

size_t AstraWindowSessionMetadata::GetStackedTabCount() const {
  size_t count = 0;
  for (const auto& tab : tabs) {
    if (!tab.tab_stack_id.empty() || !tab.stack_parent_id.empty()) {
      count++;
    }
  }
  return count;
}

size_t AstraWindowSessionMetadata::EstimateSizeBytes() const {
  size_t size = sizeof(AstraWindowSessionMetadata);
  size += window_id.capacity();
  size += workspace_id.capacity();
  for (const auto& tab : tabs) {
    size += tab.EstimateSizeBytes();
  }
  return size;
}

// ---------------------------------------------------------------------------
// Structured session metadata implementation
// ---------------------------------------------------------------------------

base::Value::Dict AstraSessionMetadata::ToDict() const {
  base::Value::Dict dict;

  if (!session_name.empty()) {
    dict.Set("session_name", session_name);
  }
  if (!save_time.is_null()) {
    double time_us = static_cast<double>(
        save_time.ToDeltaSinceWindowsEpoch().InMicroseconds());
    dict.Set("save_time", time_us);
  }

  base::Value::List windows_list;
  for (const auto& window : windows) {
    windows_list.Append(window.ToDict());
  }
  dict.Set("windows", std::move(windows_list));

  return dict;
}

bool AstraSessionMetadata::FromDict(const base::Value::Dict& dict) {
  if (const std::string* v = dict.FindString("session_name")) {
    session_name = *v;
  }

  if (auto v = dict.FindDouble("save_time")) {
    save_time = base::Time::FromDeltaSinceWindowsEpoch(
        base::Microseconds(static_cast<int64_t>(*v)));
  }

  const base::Value::List* windows_list = dict.FindList("windows");
  if (windows_list) {
    windows.clear();
    windows.reserve(windows_list->size());
    for (const auto& win_val : *windows_list) {
      if (win_val.is_dict()) {
        AstraWindowSessionMetadata window;
        window.FromDict(win_val.GetDict());
        windows.push_back(std::move(window));
      }
    }
  }

  return true;
}

bool AstraSessionMetadata::Validate() const {
  for (const auto& window : windows) {
    if (!window.Validate()) return false;
  }
  return true;
}

void AstraSessionMetadata::MergeFrom(const AstraSessionMetadata& other) {
  if (!other.session_name.empty()) {
    session_name = other.session_name;
  }
  if (!other.save_time.is_null()) {
    save_time = other.save_time;
  }

  // Merge windows by ID, or append if not found.
  for (const auto& other_win : other.windows) {
    bool found = false;
    for (auto& our_win : windows) {
      if (!other_win.window_id.empty() && our_win.window_id == other_win.window_id) {
        our_win.MergeFrom(other_win);
        found = true;
        break;
      }
    }
    if (!found) {
      windows.push_back(other_win);
    }
  }
}

size_t AstraSessionMetadata::GetTabCount() const {
  size_t count = 0;
  for (const auto& window : windows) {
    count += window.GetTabCount();
  }
  return count;
}

size_t AstraSessionMetadata::GetWindowCount() const {
  return windows.size();
}

size_t AstraSessionMetadata::GetWorkspaceCount() const {
  return GetWorkspaceIds().size();
}

size_t AstraSessionMetadata::GetFavoriteTabCount() const {
  size_t count = 0;
  for (const auto& window : windows) {
    count += window.GetFavoriteTabCount();
  }
  return count;
}

size_t AstraSessionMetadata::GetPinnedTabCount() const {
  size_t count = 0;
  for (const auto& window : windows) {
    count += window.GetPinnedTabCount();
  }
  return count;
}

size_t AstraSessionMetadata::GetStackedTabCount() const {
  size_t count = 0;
  for (const auto& window : windows) {
    count += window.GetStackedTabCount();
  }
  return count;
}

size_t AstraSessionMetadata::GetEstimatedMemoryUsage() const {
  size_t size = sizeof(AstraSessionMetadata);
  size += session_name.capacity();
  for (const auto& window : windows) {
    size += window.EstimateSizeBytes();
  }
  return size;
}

size_t AstraSessionMetadata::GetSessionSizeBytes() const {
  // Approximate serialized size by estimating the dict size.
  // This is a rough estimate based on the in-memory size.
  return GetEstimatedMemoryUsage() * 2 / 3;
}

size_t AstraSessionMetadata::GetWorkspaceTabCount(
    const std::string& workspace_id) const {
  size_t count = 0;
  for (const auto& window : windows) {
    count += window.GetWorkspaceTabCount(workspace_id);
  }
  return count;
}

std::vector<std::string> AstraSessionMetadata::GetTabsByWorkspace(
    const std::string& workspace_id) const {
  std::vector<std::string> result;
  for (const auto& window : windows) {
    auto tab_ids = window.GetTabsByWorkspace(workspace_id);
    result.insert(result.end(), tab_ids.begin(), tab_ids.end());
  }
  return result;
}

size_t AstraSessionMetadata::GetWindowTabCount(
    const std::string& window_id) const {
  for (const auto& window : windows) {
    if (window.window_id == window_id) {
      return window.GetTabCount();
    }
  }
  return 0;
}

std::set<std::string> AstraSessionMetadata::GetWorkspaceIds() const {
  std::set<std::string> ids;
  for (const auto& window : windows) {
    if (!window.workspace_id.empty()) {
      ids.insert(window.workspace_id);
    }
    for (const auto& tab : window.tabs) {
      if (!tab.workspace_id.empty()) {
        ids.insert(tab.workspace_id);
      }
    }
  }
  return ids;
}

// ---------------------------------------------------------------------------
// Sessions type serialization helpers
// ---------------------------------------------------------------------------
//
// These helpers operate on Chromium sessions types (SessionTab,
// SerializedNavigationEntry, SessionWindow) by reading/writing their
// extra_data dictionaries.  The exact field names depend on the Chromium
// version; we use the standard pattern of extra_data / extended_info.
//
// TODO(astra): Verify exact field names against the Chromium version we're
// building against.  The extra_data field name may be `extra_data` or
// `platform_specific_data` depending on the session type.
// Chromium component: sessions::SessionTab / SerializedNavigationEntry.
// Patch point: chromium/astra/patches/0006-session-restore-metadata.md

base::Value::Dict ExtractAstraMetadataFromSessionTab(
    const sessions::SessionTab& /* session_tab */) {
  // TODO(astra): Extract from session_tab.extra_data or equivalent field.
  // The exact API depends on the Chromium version.
  // Chromium component: sessions::SessionTab (components/sessions/session_tab.h)
  return base::Value::Dict();
}

void ApplyAstraMetadataToSessionTab(const base::Value::Dict& /* metadata */,
                                    sessions::SessionTab* /* session_tab */) {
  // TODO(astra): Merge metadata into session_tab.extra_data or equivalent.
  // Chromium component: sessions::SessionTab (components/sessions/session_tab.h)
}

base::Value::Dict ExtractAstraMetadataFromNavigationEntry(
    const sessions::SerializedNavigationEntry& /* entry */) {
  // TODO(astra): Extract from entry's extended_info or extra_headers.
  // Chromium component: sessions::SerializedNavigationEntry
  //   (components/sessions/serialized_navigation_entry.h)
  return base::Value::Dict();
}

void ApplyAstraMetadataToNavigationEntry(
    const base::Value::Dict& /* metadata */,
    sessions::SerializedNavigationEntry* /* entry */) {
  // TODO(astra): Merge metadata into entry's extended_info or equivalent.
  // Chromium component: sessions::SerializedNavigationEntry
  //   (components/sessions/serialized_navigation_entry.h)
}

base::Value::Dict ExtractAstraMetadataFromSessionWindow(
    const sessions::SessionWindow& /* session_window */) {
  // TODO(astra): Extract from session_window.extra_data or equivalent.
  // Chromium component: sessions::SessionWindow
  //   (components/sessions/session_window.h)
  return base::Value::Dict();
}

void ApplyAstraMetadataToSessionWindow(
    const base::Value::Dict& /* metadata */,
    sessions::SessionWindow* /* session_window */) {
  // TODO(astra): Merge metadata into session_window.extra_data or equivalent.
  // Chromium component: sessions::SessionWindow
  //   (components/sessions/session_window.h)
}

}  // namespace astra
