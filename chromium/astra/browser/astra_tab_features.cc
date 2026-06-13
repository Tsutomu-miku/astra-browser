// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_tab_features.h"

#include "base/time/time.h"
#include "base/unguessable_token.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "third_party/skia/include/core/SkColor.h"

#include "astra/browser/astra_incognito_handler.h"

namespace astra {

AstraTabFeatures::AstraTabFeatures(content::WebContents* web_contents)
    : content::WebContentsUserData<AstraTabFeatures>(*web_contents) {
  // Generate a unique tab identity.
  // Each tab gets a stable Astra tab ID at creation time.
  // TODO(astra): For session restore, the tab_unique_id should be restored
  //   from serialized session data instead of generating a new one.
  //   Patch point: session restore pipeline — deserialize tab_unique_id
  //   and call set_tab_unique_id() after creating the WebContents.
  GenerateNewTabUniqueId();

  // Record creation time.
  created_time_ = base::Time::Now();

  // Initialize favorite state based on profile type.
  // In incognito, tabs always start as non-favorite and cannot be favorited.
  // See AstraIncognitoHandler for the design rationale.
  //
  // Chromium owner: Profile::IsOffTheRecord()
  // (chrome/browser/profiles/profile.h)
  content::BrowserContext* browser_context = web_contents->GetBrowserContext();
  if (browser_context && browser_context->IsOffTheRecord()) {
    // Incognito tab: favorite state is locked to false.
    // The is_favorite_ field is already false by default (see header),
    // but we explicitly set it here to document the incognito behavior.
    is_favorite_ = false;
    favorite_read_only_ = true;
  }
}

AstraTabFeatures::~AstraTabFeatures() = default;

// static
AstraTabFeatures* AstraTabFeatures::GetOrCreateForWebContents(
    content::WebContents* web_contents) {
  AstraTabFeatures* data = FromWebContents(web_contents);
  if (data) {
    return data;
  }
  CreateForWebContents(web_contents);
  return FromWebContents(web_contents);
}

// static
AstraTabFeatures* AstraTabFeatures::GetForWebContents(
    content::WebContents* web_contents) {
  if (!web_contents) {
    return nullptr;
  }
  return FromWebContents(web_contents);
}

// static
std::string AstraTabFeatures::GetTabId(content::WebContents* web_contents) {
  AstraTabFeatures* features = GetForWebContents(web_contents);
  if (!features) {
    return std::string();
  }
  return features->tab_unique_id();
}

// static
void AstraTabFeatures::AssignToWorkspace(content::WebContents* web_contents,
                                         const std::string& workspace_id) {
  AstraTabFeatures* features = GetOrCreateForWebContents(web_contents);
  if (features) {
    features->set_workspace_id(workspace_id);
  }
}

// static
std::string AstraTabFeatures::GetWorkspaceId(
    content::WebContents* web_contents) {
  AstraTabFeatures* features = GetForWebContents(web_contents);
  if (!features) {
    return "default";
  }
  return features->workspace_id();
}

void AstraTabFeatures::GenerateNewTabUniqueId() {
  tab_unique_id_ = base::UnguessableToken::Create().ToString();
}

void AstraTabFeatures::Reset() {
  // IMPORTANT: tab_unique_id_ is NOT reset.
  // Tab identity persists across tab reuse / session restore.
  // Use GenerateNewTabUniqueId() explicitly if you need a new identity.

  workspace_id_ = "default";
  source_workspace_id_.clear();

  is_favorite_ = false;
  favorite_order_index_ = 0;
  favorite_folder_id_ = "root";

  // Reset tab color.
  tab_color_ = SK_ColorTRANSPARENT;

  is_in_split_view_ = false;
  split_view_partner_id_.clear();
  split_view_ratio_ = 0.5f;
  split_view_orientation_ = SplitViewOrientation::kHorizontal;

  is_glance_tab_ = false;
  glance_source_tab_id_.clear();

  sidebar_pinned_ = false;
  sidebar_hidden_ = false;

  // Reset named tab stack membership.
  stack_id_.clear();

  // Reset tab stacking state (hierarchical).
  is_in_stack_ = false;
  stack_parent_id_.clear();
  is_stack_collapsed_ = false;

  // Reset tab stack / grouping.
  tab_stack_id_.clear();
  stack_position_ = 0;

  // Reset picture-in-picture (PiP) state.
  // A reused / restored tab starts as a normal tab, not in PiP mode.
  // The actual PiP window is owned by Chromium and would be destroyed
  // before tab reuse.
  //
  // Chromium owner: PictureInPictureWindowController
  //   (chrome/browser/picture_in_picture/picture_in_picture_window_controller.h)
  is_pip_tab_ = false;
  pip_window_size_ = gfx::Size();

  // Reset reading list state.
  is_in_reading_list_ = false;
  reading_list_added_time_ = base::Time();

  // Reset read later (Astra-specific).
  read_later_ = false;

  // Reset notes association.
  has_note_ = false;
  note_id_.clear();

  // Reset snooze state.
  is_snoozed_ = false;
  snooze_time_ = base::Time();

  // Reset hibernation state.
  is_hibernated_ = false;

  // Created time is NOT reset — it represents when the logical tab was created.
  // last_activated_time_ is NOT reset.

  // Reset view state projections.
  // is_pinned_ resets to false — a reused tab starts as not pinned.
  // The actual pinned state is owned by TabStripModel and should be
  // synced by an observer after Reset().
  is_pinned_ = false;
  // last_active_time_ is left as-is intentionally: the caller should update it
  // when the tab becomes active after reset.
  tab_index_hint_ = -1;

  // Reset thumbnail state.
  has_thumbnail_ = false;
  thumbnail_last_updated_ = base::Time();

  // Reset discard state.
  // A reused / restored tab starts as not discarded.
  is_discarded_ = false;
  // discard_count_ is NOT reset — it's a lifetime counter for the tab.
  // TODO(astra): Decide whether discard_count_ should persist across
  //   tab reuse or reset.  For telemetry purposes, keeping it makes
  //   sense if the "same tab" concept is preserved across navigations.
  //   For session restore, it should probably be restored from session
  //   storage along with other metadata.

  // Reset memory saver / suspension state.
  // Suspended state resets to false — a reused / restored tab starts as active.
  is_suspended_ = false;
  suspended_url_ = GURL();
  // last_active_time_ is left as-is (see above).

  // Re-evaluate read-only state based on the current WebContents' profile.
  // If this WebContents was repurposed (e.g. session restore into a different
  // profile context), the read-only flag needs to be recomputed.
  //
  // TODO(astra): Verify that Reset() is called in all paths where a
  // WebContents changes profile context.  In Chromium, WebContents typically
  // stay with one BrowserContext for their lifetime, but some edge cases
  // (e.g. prerender activation) may need verification.
  // Chromium owner: content::WebContents / WebContentsImpl.
  content::WebContents* web_contents = &GetWebContents();
  content::BrowserContext* browser_context = web_contents->GetBrowserContext();
  favorite_read_only_ =
      browser_context && browser_context->IsOffTheRecord();
}

bool AstraTabFeatures::ToggleFavorite() {
  // In incognito mode, favorite state is read-only — toggle is a no-op.
  // See AstraIncognitoHandler::AreFavoritesMutable for design rationale.
  if (favorite_read_only_) {
    // Always returns false (not favorited) in incognito.
    // The state is locked to false.
    return false;
  }
  is_favorite_ = !is_favorite_;
  return is_favorite_;
}

// =========================================================================
// Tab stack / grouping
// =========================================================================

void AstraTabFeatures::SetStackInfo(const std::string& stack_id,
                                    size_t position) {
  tab_stack_id_ = stack_id;
  stack_position_ = position;
}

void AstraTabFeatures::ClearStackInfo() {
  tab_stack_id_.clear();
  stack_position_ = 0;
}

// =========================================================================
// Reading list
// =========================================================================

void AstraTabFeatures::AddToReadingList() {
  if (is_in_reading_list_) {
    return;
  }
  is_in_reading_list_ = true;
  reading_list_added_time_ = base::Time::Now();
}

void AstraTabFeatures::RemoveFromReadingList() {
  if (!is_in_reading_list_) {
    return;
  }
  is_in_reading_list_ = false;
  reading_list_added_time_ = base::Time();
}

// =========================================================================
// Snooze
// =========================================================================

void AstraTabFeatures::SnoozeUntil(base::Time time) {
  is_snoozed_ = true;
  snooze_time_ = time;
}

void AstraTabFeatures::CancelSnooze() {
  if (!is_snoozed_) {
    return;
  }
  is_snoozed_ = false;
  snooze_time_ = base::Time();
}

bool AstraTabFeatures::IsSnoozeDue() const {
  if (!is_snoozed_) {
    return false;
  }
  if (snooze_time_.is_null()) {
    return false;
  }
  return base::Time::Now() >= snooze_time_;
}

// =========================================================================
// Hibernation
// =========================================================================

void AstraTabFeatures::Hibernate() {
  if (is_hibernated_) {
    return;
  }
  is_hibernated_ = true;

  // TODO(astra): Trigger actual tab discard via Chromium's discard mechanism.
  //   The actual unload is done by Chromium — we just mark the state here.
  //   Patch point: content::WebContents::Discard() or
  //   resource_coordinator::TabManager::DiscardTab().
  //   Chromium subsystem: resource_coordinator / TabManager.
}

void AstraTabFeatures::Wake() {
  if (!is_hibernated_) {
    return;
  }
  is_hibernated_ = false;

  // TODO(astra): Trigger tab reload / restore if the WebContents was
  //   discarded.  The actual reload is done by Chromium when the tab
  //   is activated.
  //   Chromium subsystem: content::WebContents / NavigationController.
}

// =========================================================================
// Created / activated times
// =========================================================================

void AstraTabFeatures::MarkActivated() {
  last_active_time_ = base::TimeTicks::Now();
  last_activated_time_ = base::Time::Now();
}

// =========================================================================
// Memory saver / suspension
// =========================================================================

void AstraTabFeatures::SetSuspended(bool suspended) {
  if (is_suspended_ == suspended) {
    return;
  }

  is_suspended_ = suspended;

  if (suspended) {
    // When suspending, cache the current URL so we know what to reload.
    // The actual discard is done by Chromium's TabManager — we just save
    // the URL for quick UI access and for the restore flow.
    //
    // If the WebContents is already discarded, the URL should have been
    // set explicitly by the caller (e.g. from the NavigationController).
    //
    // TODO(astra): Read the URL from the NavigationController's pending
    // entry or last committed entry instead of relying on WebContents::GetURL(),
    // which may not be available for a discarded tab.
    // Chromium subsystem: content::NavigationController
    //   (content/public/browser/navigation_controller.h)
    content::WebContents* web_contents = &GetWebContents();
    if (web_contents && !web_contents->GetURL().is_empty()) {
      suspended_url_ = web_contents->GetURL();
    }
  } else {
    // When restoring, clear the cached URL — it will be re-established
    // when the tab reloads.
    suspended_url_ = GURL();
    // Update last active time to "now" since the tab is being woken up.
    last_active_time_ = base::TimeTicks::Now();
    last_activated_time_ = base::Time::Now();
  }
}

// =========================================================================
// Utility / query methods
// =========================================================================

bool AstraTabFeatures::IsInAnyStack() const {
  return !stack_id_.empty() || is_in_stack_ || !tab_stack_id_.empty();
}

bool AstraTabFeatures::IsUntouched() const {
  return last_active_time_.is_null();
}

base::TimeDelta AstraTabFeatures::GetTimeSinceLastActive() const {
  if (last_active_time_.is_null()) {
    return base::TimeDelta();
  }
  return base::TimeTicks::Now() - last_active_time_;
}

bool AstraTabFeatures::IsStale(base::TimeDelta threshold) const {
  if (last_active_time_.is_null()) {
    // Never been active — not "stale" in the sense of aging, it's
    // just untouched.  Callers that care about untouched tabs should
    // check IsUntouched() separately.
    return false;
  }
  return (base::TimeTicks::Now() - last_active_time_) >= threshold;
}

void AstraTabFeatures::CopyFrom(const AstraTabFeatures& other) {
  // Note: we do NOT copy favorite_read_only_ because that is determined
  // by the current WebContents' own BrowserContext (incognito state).
  // A tab in a regular profile can't become incognito by copying metadata.

  // Note: we do NOT copy tab_unique_id_ because each tab has its own
  // identity.  Use CloneFrom() if you need to copy identity too.

  // Note: we do NOT copy created_time_ because each tab has its own
  // creation timestamp.

  workspace_id_ = other.workspace_id_;
  source_workspace_id_ = other.source_workspace_id_;

  // Only copy favorite state if this tab is not read-only (not incognito).
  if (!favorite_read_only_) {
    is_favorite_ = other.is_favorite_;
    favorite_order_index_ = other.favorite_order_index_;
    favorite_folder_id_ = other.favorite_folder_id_;
  }

  tab_color_ = other.tab_color_;

  is_in_split_view_ = other.is_in_split_view_;
  split_view_partner_id_ = other.split_view_partner_id_;
  split_view_ratio_ = other.split_view_ratio_;
  split_view_orientation_ = other.split_view_orientation_;

  is_glance_tab_ = other.is_glance_tab_;
  glance_source_tab_id_ = other.glance_source_tab_id_;

  sidebar_pinned_ = other.sidebar_pinned_;
  sidebar_hidden_ = other.sidebar_hidden_;

  stack_id_ = other.stack_id_;

  is_in_stack_ = other.is_in_stack_;
  stack_parent_id_ = other.stack_parent_id_;
  is_stack_collapsed_ = other.is_stack_collapsed_;

  tab_stack_id_ = other.tab_stack_id_;
  stack_position_ = other.stack_position_;

  is_pip_tab_ = other.is_pip_tab_;
  pip_window_size_ = other.pip_window_size_;

  is_in_reading_list_ = other.is_in_reading_list_;
  reading_list_added_time_ = other.reading_list_added_time_;

  read_later_ = other.read_later_;

  has_note_ = other.has_note_;
  note_id_ = other.note_id_;

  is_snoozed_ = other.is_snoozed_;
  snooze_time_ = other.snooze_time_;

  is_hibernated_ = other.is_hibernated_;

  is_pinned_ = other.is_pinned_;
  // last_active_time_ is NOT copied — each tab has its own activity timeline.
  // last_activated_time_ is NOT copied.
  tab_index_hint_ = other.tab_index_hint_;

  has_thumbnail_ = other.has_thumbnail_;
  thumbnail_last_updated_ = other.thumbnail_last_updated_;

  is_discarded_ = other.is_discarded_;
  // discard_count_ is NOT copied — it's a per-tab lifetime counter.

  is_suspended_ = other.is_suspended_;
  suspended_url_ = other.suspended_url_;
  // last_active_time_ is NOT copied (see above).
}

void AstraTabFeatures::CloneFrom(const AstraTabFeatures& other) {
  // Start with a regular copy (copies all metadata except identity).
  CopyFrom(other);

  // CloneFrom also copies tab identity, which CopyFrom does not.
  // This is used in session restore and tab duplication scenarios
  // where the new tab should carry the same logical identity.
  //
  // TODO(astra): Define exact semantics for when CloneFrom should be
  //   used vs CopyFrom.  For session restore, the tab should keep its
  //   identity; for "duplicate tab", it should get a new identity but
  //   copy metadata.
  tab_unique_id_ = other.tab_unique_id_;
  created_time_ = other.created_time_;
}

void AstraTabFeatures::ClearAllAstraMetadata() {
  workspace_id_ = "default";
  source_workspace_id_.clear();

  is_favorite_ = false;
  favorite_order_index_ = 0;
  favorite_folder_id_ = "root";
  // Note: favorite_read_only_ is preserved because it's a function of the
  // WebContents' BrowserContext, not Astra metadata.

  tab_color_ = SK_ColorTRANSPARENT;

  is_in_split_view_ = false;
  split_view_partner_id_.clear();
  split_view_ratio_ = 0.5f;
  split_view_orientation_ = SplitViewOrientation::kHorizontal;

  is_glance_tab_ = false;
  glance_source_tab_id_.clear();

  sidebar_pinned_ = false;
  sidebar_hidden_ = false;

  stack_id_.clear();

  is_in_stack_ = false;
  stack_parent_id_.clear();
  is_stack_collapsed_ = false;

  tab_stack_id_.clear();
  stack_position_ = 0;

  is_pip_tab_ = false;
  pip_window_size_ = gfx::Size();

  is_in_reading_list_ = false;
  reading_list_added_time_ = base::Time();

  read_later_ = false;

  has_note_ = false;
  note_id_.clear();

  is_snoozed_ = false;
  snooze_time_ = base::Time();

  is_hibernated_ = false;

  is_pinned_ = false;
  // last_active_time_ is preserved — it's Chromium-proximal state, not
  // Astra metadata per se.
  // last_activated_time_ is preserved.
  tab_index_hint_ = -1;

  has_thumbnail_ = false;
  thumbnail_last_updated_ = base::Time();

  is_discarded_ = false;
  // discard_count_ is NOT cleared — it's a lifetime counter.

  is_suspended_ = false;
  suspended_url_ = GURL();
  // last_active_time_ is preserved (see above).

  // Note: tab_unique_id_ is NOT cleared — tab identity is fundamental.
  // Note: created_time_ is NOT cleared.
}

bool AstraTabFeatures::HasAnyAstraMetadata() const {
  // Check if any Astra-specific field has a non-default value.
  // We don't count fields that are projections of Chromium state
  // (like is_pinned_, is_discarded_, is_suspended_, last_active_time_,
  // tab_index_hint_, last_activated_time_, created_time_) because those
  // are mirrors or identity fields, not Astra metadata.

  // Tab identity is not "Astra metadata" per se — every tab has it.
  // (tab_unique_id_ is always set, so it would always return true.)

  // Workspace
  if (workspace_id_ != "default") {
    return true;
  }
  if (!source_workspace_id_.empty()) {
    return true;
  }

  // Favorites
  if (is_favorite_) {
    return true;
  }
  if (favorite_order_index_ != 0) {
    return true;
  }
  if (favorite_folder_id_ != "root") {
    return true;
  }

  // Tab color
  if (tab_color_ != SK_ColorTRANSPARENT) {
    return true;
  }

  // Split view
  if (is_in_split_view_) {
    return true;
  }
  if (!split_view_partner_id_.empty()) {
    return true;
  }
  if (split_view_ratio_ != 0.5f) {
    return true;
  }
  if (split_view_orientation_ != SplitViewOrientation::kHorizontal) {
    return true;
  }

  // Glance
  if (is_glance_tab_) {
    return true;
  }
  if (!glance_source_tab_id_.empty()) {
    return true;
  }

  // Sidebar
  if (sidebar_pinned_) {
    return true;
  }
  if (sidebar_hidden_) {
    return true;
  }

  // Named stack
  if (!stack_id_.empty()) {
    return true;
  }

  // Hierarchical stack
  if (is_in_stack_) {
    return true;
  }
  if (!stack_parent_id_.empty()) {
    return true;
  }
  if (is_stack_collapsed_) {
    return true;
  }

  // Tab stack / grouping
  if (!tab_stack_id_.empty()) {
    return true;
  }
  // stack_position_ is only meaningful when in a stack, so we don't
  // check it independently.

  // PiP
  if (is_pip_tab_) {
    return true;
  }
  if (!pip_window_size_.IsEmpty()) {
    return true;
  }

  // Reading list
  if (is_in_reading_list_) {
    return true;
  }

  // Read later
  if (read_later_) {
    return true;
  }

  // Notes
  if (has_note_) {
    return true;
  }
  if (!note_id_.empty()) {
    return true;
  }

  // Snooze
  if (is_snoozed_) {
    return true;
  }

  // Hibernation
  if (is_hibernated_) {
    return true;
  }

  // Thumbnail
  if (has_thumbnail_) {
    return true;
  }

  return false;
}

// =========================================================================
// Session restore integration (stubs)
// =========================================================================

std::string AstraTabFeatures::SerializeForSessionRestore() const {
  // TODO(astra): Implement actual serialization for session restore.
  //   The format should include all Astra-specific metadata that needs
  //   to survive browser restart:
  //     - tab_unique_id (stable tab identity)
  //     - workspace_id
  //     - is_favorite, favorite_order_index, favorite_folder_id
  //     - tab_color
  //     - sidebar_pinned, sidebar_hidden
  //     - stack_id (named stack membership)
  //     - is_in_stack, stack_parent_id, is_stack_collapsed
  //     - tab_stack_id, stack_position
  //     - is_in_split_view, split_view_partner_id, split_view_ratio,
  //       split_view_orientation
  //     - is_glance_tab, glance_source_tab_id
  //     - is_in_reading_list, reading_list_added_time
  //     - read_later
  //     - has_note, note_id
  //     - is_snoozed, snooze_time
  //     - is_hibernated
  //     - created_time
  //     - last_activated_time
  //     - discard_count
  //     - has_thumbnail, thumbnail_last_updated
  //     - source_workspace_id
  //
  //   Patch point: chrome/browser/sessions/session_service.cc — add
  //     an extra data blob to each tab's session entry.
  //   Chromium component: sessions / SessionService / TabRestoreService.
  //
  //   Format options:
  //     - JSON (human-readable, easy to debug)
  //     - protobuf (more compact, type-safe)
  //     - base::Value dictionary (Chromium-native)
  //
  // For now, return an empty string as a placeholder.
  return std::string();
}

void AstraTabFeatures::DeserializeFromSessionRestore(const std::string& data) {
  // TODO(astra): Implement actual deserialization for session restore.
  //   This should parse the serialized data and apply all the saved
  //   metadata to this tab features object.
  //
  //   Important: deserialize tab_unique_id BEFORE any other fields so
  //   that the tab has its correct identity from the start.
  //
  //   Patch point: AstraSessionRestoreHelper should call this method
  //     when restoring a tab from session data.
  //   Chromium component: sessions / SessionService.
  //
  // For now, this is a no-op.
  if (data.empty()) {
    return;
  }
  // TODO(astra): Implement deserialization.
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(AstraTabFeatures);

}  // namespace astra
