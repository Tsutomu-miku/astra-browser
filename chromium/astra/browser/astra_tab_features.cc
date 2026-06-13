// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_tab_features.h"

#include "base/time/time.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"

#include "astra/browser/astra_incognito_handler.h"

namespace astra {

AstraTabFeatures::AstraTabFeatures(content::WebContents* web_contents)
    : content::WebContentsUserData<AstraTabFeatures>(*web_contents) {
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

void AstraTabFeatures::Reset() {
  workspace_id_ = "default";

  is_favorite_ = false;
  favorite_order_index_ = 0;
  favorite_folder_id_ = "root";

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

  // Reset notes association.
  has_note_ = false;
  note_id_.clear();

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

  workspace_id_ = other.workspace_id_;

  // Only copy favorite state if this tab is not read-only (not incognito).
  if (!favorite_read_only_) {
    is_favorite_ = other.is_favorite_;
    favorite_order_index_ = other.favorite_order_index_;
    favorite_folder_id_ = other.favorite_folder_id_;
  }

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

  has_note_ = other.has_note_;
  note_id_ = other.note_id_;

  is_pinned_ = other.is_pinned_;
  // last_active_time_ is NOT copied — each tab has its own activity timeline.
  tab_index_hint_ = other.tab_index_hint_;

  has_thumbnail_ = other.has_thumbnail_;
  thumbnail_last_updated_ = other.thumbnail_last_updated_;

  is_discarded_ = other.is_discarded_;
  // discard_count_ is NOT copied — it's a per-tab lifetime counter.

  is_suspended_ = other.is_suspended_;
  suspended_url_ = other.suspended_url_;
  // last_active_time_ is NOT copied (see above).
}

void AstraTabFeatures::ClearAllAstraMetadata() {
  workspace_id_ = "default";

  is_favorite_ = false;
  favorite_order_index_ = 0;
  favorite_folder_id_ = "root";
  // Note: favorite_read_only_ is preserved because it's a function of the
  // WebContents' BrowserContext, not Astra metadata.

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

  has_note_ = false;
  note_id_.clear();

  is_pinned_ = false;
  // last_active_time_ is preserved — it's Chromium-proximal state, not
  // Astra metadata per se.
  tab_index_hint_ = -1;

  has_thumbnail_ = false;
  thumbnail_last_updated_ = base::Time();

  is_discarded_ = false;
  // discard_count_ is NOT cleared — it's a lifetime counter.

  is_suspended_ = false;
  suspended_url_ = GURL();
  // last_active_time_ is preserved (see above).
}

bool AstraTabFeatures::HasAnyAstraMetadata() const {
  // Check if any Astra-specific field has a non-default value.
  // We don't count fields that are projections of Chromium state
  // (like is_pinned_, is_discarded_, is_suspended_, last_active_time_,
  // tab_index_hint_) because those are mirrors, not Astra metadata.

  // Workspace
  if (workspace_id_ != "default") {
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

  // Notes
  if (has_note_) {
    return true;
  }
  if (!note_id_.empty()) {
    return true;
  }

  // Thumbnail
  if (has_thumbnail_) {
    return true;
  }

  return false;
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(AstraTabFeatures);

}  // namespace astra
