// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_BROWSER_ASTRA_TAB_FEATURES_H_
#define ASTRA_BROWSER_ASTRA_TAB_FEATURES_H_

#include <string>
#include <utility>

#include "base/time/time.h"
#include "content/public/browser/web_contents_user_data.h"
#include "ui/gfx/geometry/size.h"
#include "url/gurl.h"

namespace content {
class WebContents;
}

namespace astra {

// Orientation of a split view pair.
enum class SplitViewOrientation {
  kHorizontal,  // Tabs side-by-side (left/right).
  kVertical,    // Tabs stacked (top/bottom).
};

// Astra-only metadata attached to Chromium-owned WebContents.
//
// PRINCIPLE: This class stores ONLY product metadata that Chromium does not
// track.  Do NOT mirror navigation state, title, URL, favicon, zoom, mute,
// loading, history, or crash state here — those belong to WebContents,
// NavigationController, and TabStripModel.
//
// Fields that *appear* to duplicate Chromium state (e.g. is_pinned,
// is_discarded) are projections — they exist so that Astra UI layers can
// read tab state quickly without reaching into Chromium's browser layer,
// and they are kept in sync by observer hooks in Astra services.  The truth
// always lives on the Chromium side; these are caches / projections.
//
// Persistence model:
//   Per-tab Astra metadata does NOT persist via PrefService directly.
//   Instead, it survives across browser restarts through Chromium's
//   SessionService (session restore) + tab restore mechanisms.
//   Chromium owns session restore; Astra attaches extra data to each tab's
//   session entry via a session restore patch point.
//
//   This is the correct architecture because:
//     - Tab lifecycle is owned by Chromium (TabStripModel, SessionService).
//     - Tab metadata should travel with the tab it describes, not with
//       profile-level preferences.
//     - Session restore handles tab discarding, lazy loading, and crash
//       recovery — we want Astra metadata to participate in all of that.
//
//   TODO(astra): Attach AstraTabFeatures data to Chromium's session restore
//   pipeline so per-tab metadata (workspace_id, favorite state, split view
//   config, etc.) survives browser restart and tab restore.
//   Patch point: chrome/browser/sessions/session_service.cc or
//   sessions/tab_restore_service.cc — add extra data keys for Astra metadata.
//   Chromium component: sessions / SessionService / TabRestoreService.
//
// Observers: intentionally NOT provided on this class. WebContentsUserData
// objects are lightweight data attachments; UI and services that need to
// react to Astra metadata changes should observe the higher-level models
// that mutate this data (e.g., AstraWorkspaceService, sidebar controller,
// split view controller). Adding an ObserverList here would duplicate the
// observation pattern that already exists on TabStripModel and WebContents.
// TODO(astra): if we later need per-tab feature change events, evaluate
// patching TabStripModelObserver with an Astra hook instead of observing
// WebContentsUserData directly.
class AstraTabFeatures final
    : public content::WebContentsUserData<AstraTabFeatures> {
 public:
  ~AstraTabFeatures() override;

  // Creates the user data for |web_contents| if it does not already exist,
  // then returns a pointer to it.
  static AstraTabFeatures* GetOrCreateForWebContents(
      content::WebContents* web_contents);

  // Resets all Astra metadata to defaults. Used when a WebContents is reused
  // (e.g., session restore, tab discarding replacement) and old product
  // metadata should not carry over.
  void Reset();

  // -- Workspace ----------------------------------------------------------

  const std::string& workspace_id() const { return workspace_id_; }
  void set_workspace_id(std::string workspace_id) {
    workspace_id_ = std::move(workspace_id);
  }

  bool IsInDefaultWorkspace() const { return workspace_id_ == "default"; }

  // -- Favorites ----------------------------------------------------------

  bool is_favorite() const { return is_favorite_; }

  // Sets the favorite state.  In incognito mode, this is a no-op — favorite
  // state is read-only and always false.  See AstraIncognitoHandler for the
  // design rationale.
  //
  // Chromium owner for incognito detection: Profile::IsOffTheRecord()
  void set_is_favorite(bool is_favorite) {
    if (favorite_read_only_) {
      return;
    }
    is_favorite_ = is_favorite;
  }

  // Returns true if the favorite state is read-only (incognito mode).
  bool favorite_read_only() const { return favorite_read_only_; }

  // Toggles the favorite state and returns the new value.
  bool ToggleFavorite();

  // Position within the favorites folder (0-based).
  size_t favorite_order_index() const { return favorite_order_index_; }
  void set_favorite_order_index(size_t index) {
    favorite_order_index_ = index;
  }

  // ID of the favorite folder this tab belongs to. "root" for the top-level
  // favorites bar.
  const std::string& favorite_folder_id() const {
    return favorite_folder_id_;
  }
  void set_favorite_folder_id(std::string folder_id) {
    favorite_folder_id_ = std::move(folder_id);
  }

  // -- Split View ---------------------------------------------------------

  bool is_in_split_view() const { return is_in_split_view_; }
  void set_is_in_split_view(bool is_in_split_view) {
    is_in_split_view_ = is_in_split_view;
  }

  // Opaque identifier of the partner tab in a split view pair. Empty when
  // not in split view. This is stored as a string to avoid coupling to any
  // particular tab handle type; the actual lookup is done by the split view
  // controller using WebContents or TabStripModel indices.
  // TODO(astra): consider using base::Token instead of string for the
  // partner identifier once Astra tab identity stabilizes.
  const std::string& split_view_partner_id() const {
    return split_view_partner_id_;
  }
  void set_split_view_partner_id(std::string partner_id) {
    split_view_partner_id_ = std::move(partner_id);
  }

  // Split ratio in [0.0, 1.0]. 0.5 means equal split. For horizontal
  // orientation this is the width fraction of the left tab; for vertical
  // orientation this is the height fraction of the top tab.
  float split_view_ratio() const { return split_view_ratio_; }
  void set_split_view_ratio(float ratio) { split_view_ratio_ = ratio; }

  SplitViewOrientation split_view_orientation() const {
    return split_view_orientation_;
  }
  void set_split_view_orientation(SplitViewOrientation orientation) {
    split_view_orientation_ = orientation;
  }

  // -- Glance / Peek ------------------------------------------------------

  // True if this tab is displayed as a "glance" (peek preview overlay)
  // rather than a full tab in the tab strip.
  bool is_glance_tab() const { return is_glance_tab_; }
  void set_is_glance_tab(bool is_glance_tab) {
    is_glance_tab_ = is_glance_tab;
  }

  // Identifier of the source tab that triggered this glance. Empty when
  // not a glance tab. Used to dismiss the glance when navigating back to
  // the source.
  const std::string& glance_source_tab_id() const {
    return glance_source_tab_id_;
  }
  void set_glance_source_tab_id(std::string source_tab_id) {
    glance_source_tab_id_ = std::move(source_tab_id);
  }

  // -- Sidebar presentation -----------------------------------------------

  // True if this tab is pinned in the Astra sidebar. Pinned tabs appear
  // above the regular tab list and have fixed positions.
  bool sidebar_pinned() const { return sidebar_pinned_; }
  void set_sidebar_pinned(bool pinned) { sidebar_pinned_ = pinned; }

  // True if this tab should be hidden from the sidebar tab list. Used for
  // tabs that are part of the browser UI but not user-visible as tabs,
  // e.g., DevTools WebContents, internal pages.
  bool sidebar_hidden() const { return sidebar_hidden_; }
  void set_sidebar_hidden(bool hidden) { sidebar_hidden_ = hidden; }

  // -- Tab stacking (named stacks) -----------------------------------------

  // True if this tab belongs to a named tab stack.
  // Tab stacks are an Astra feature — named groups of tabs (like Arc's
  // tab orphans or Vivaldi's tab stacks) that appear as collapsible
  // sections in the sidebar.
  //
  // This is distinct from hierarchical parent/child stacking
  // (stack_parent_id_) — named stacks have their own identity (name,
  // color, order) and tabs are members of the stack by ID reference.
  //
  // Truth: AstraTabFeatures stores the stack_id reference;
  //   AstraTabStackService owns stack metadata (name, color, order).
  // Chromium analog: Tab groups (chrome/browser/ui/tabs/tab_group.h),
  //   but stacks are an Astra-native concept with richer metadata.
  bool is_in_named_stack() const { return !stack_id_.empty(); }

  // ID of the named tab stack this tab belongs to.  Empty when not in
  // a named stack.
  //
  // The stack ID references an AstraTabStack in AstraTabStackService.
  // Stack metadata (name, color, order) lives on the service; per-tab
  // membership lives here.
  const std::string& stack_id() const { return stack_id_; }
  void set_stack_id(std::string id) { stack_id_ = std::move(id); }

  // -- Tab stacking (hierarchical / parent-child) ---------------------------

  // True if this tab is currently stacked under a parent tab.
  // Stacked tabs appear as nested children in the sidebar tree view.
  //
  // This is the hierarchical stacking model (parent tab → child tabs).
  // For named stacks (with name, color, order), use stack_id() instead.
  //
  // Chromium analog: Tab groups (chrome/browser/ui/tabs/tab_group.h),
  // but tab groups are flat (label/color based) whereas stacks are
  // hierarchical (parent/child tree).
  // TODO(astra): Consider migrating to Chromium tab groups with sidebar
  //   tree projection, or keeping stacks as a separate Astra concept.
  //   Chromium owner: TabGroupModel / TabGroup (chrome/browser/ui/tabs/)
  bool is_in_stack() const { return is_in_stack_; }
  void set_is_in_stack(bool in_stack) { is_in_stack_ = in_stack; }

  // ID of the parent tab in a hierarchical stack.  Empty when not stacked.
  // The ID is an opaque string identifier that can be resolved to a
  // WebContents by the stack service.
  //
  // Truth lives on AstraTabFeatures (per-tab metadata).  The sidebar
  // projects parent/child relationships into a tree view.
  const std::string& stack_parent_id() const { return stack_parent_id_; }
  void set_stack_parent_id(std::string parent_id) {
    stack_parent_id_ = std::move(parent_id);
    is_in_stack_ = !stack_parent_id_.empty();
  }

  // Whether the hierarchical stack headed by this tab is collapsed
  // (children hidden) in the sidebar.  Only meaningful for tabs that
  // are stack parents (i.e., tabs that have children stacked under them).
  //
  // Collapsed state is stored on the parent tab's metadata.
  // Child tabs do not have their own collapsed state.
  //
  // For named stack collapse state, see AstraTabStackService.
  bool is_stack_collapsed() const { return is_stack_collapsed_; }
  void set_stack_collapsed(bool collapsed) { is_stack_collapsed_ = collapsed; }

  // -- Tab stack / grouping -----------------------------------------------

  // ID of the tab stack (group) this tab belongs to.  Empty if the tab
  // is not in any stack.
  //
  // This is the tab-level stack identifier, used for grouping tabs
  // together in the tab strip and sidebar.  It is distinct from both
  // named stacks (stack_id_) and hierarchical parent/child stacking
  // (stack_parent_id_).
  //
  // Chromium analog: TabGroupId (chrome/browser/ui/tabs/tab_group.h)
  // TODO(astra): Consider replacing with direct use of Chromium tab groups
  //   plus an Astra projection layer.  For now, we maintain our own ID
  //   for flexibility.
  const std::string& tab_stack_id() const { return tab_stack_id_; }
  void set_tab_stack_id(std::string id) { tab_stack_id_ = std::move(id); }

  // Position of this tab within its tab stack.  0-based.  Only meaningful
  // when tab_stack_id() is non-empty.
  size_t stack_position() const { return stack_position_; }
  void set_stack_position(size_t position) { stack_position_ = position; }

  // Sets both stack ID and position in a single call.
  void SetStackInfo(const std::string& stack_id, size_t position);

  // Clears both stack ID and position, removing the tab from its stack.
  void ClearStackInfo();

  // -- Picture-in-Picture (PiP) -------------------------------------------

  // True if this tab is currently displayed as a picture-in-picture window.
  //
  // This is Astra metadata only — the actual PiP window state is owned by
  // Chromium's PictureInPictureWindowController.  This flag is Astra's
  // projection of that state, used for sidebar indicators and quick lookup.
  //
  // Truth model:
  //   - Chromium owns the actual PiP window (PictureInPictureWindowController).
  //   - This flag is Astra's projection, kept in sync by AstraPipService
  //     via observer hooks into Chromium's PiP controller.
  //   - pip_window_size_ caches the current PiP window dimensions.
  //
  // Chromium owner: PictureInPictureWindowController
  //   (chrome/browser/picture_in_picture/picture_in_picture_window_controller.h)
  // TODO(astra): PiP tab state detection via PiP service.
  //   The service should observe PictureInPictureWindowController and keep
  //   this flag in sync with actual PiP state, rather than being set directly.
  //   Patch point: picture_in_picture_window_controller.cc — add Astra
  //   observer hook in OnWindowCreated / OnWindowDestroyed.
  bool is_pip_tab() const { return is_pip_tab_; }
  void set_is_pip_tab(bool is_pip) { is_pip_tab_ = is_pip; }

  // The size of the PiP window for this tab.  Empty size means the tab is
  // not in PiP mode, or the size hasn't been determined yet.
  //
  // The actual window size is owned by Chromium's views::Widget for the
  // PiP window.  We cache it here for quick UI access (sidebar indicators,
  // command enablement checks) without needing to query the PiP controller.
  //
  // Chromium owner: views::Widget (ui/views/widget/widget.h)
  // Chromium owner: PictureInPictureWindowController
  const gfx::Size& pip_window_size() const { return pip_window_size_; }
  void set_pip_window_size(const gfx::Size& size) { pip_window_size_ = size; }

  // -- Reading list -------------------------------------------------------

  // True if this tab is in the reading list.
  //
  // This is Astra metadata only — the actual reading list is owned by
  // Chromium's ReadingListModel.  This flag is Astra's projection, kept
  // in sync by AstraReadingListService.
  //
  // Chromium owner: ReadingListModel (components/reading_list/core/reading_list_model.h)
  bool is_in_reading_list() const { return is_in_reading_list_; }

  // Time when the tab was added to the reading list.  Only meaningful when
  // is_in_reading_list() is true.
  base::Time reading_list_added_time() const {
    return reading_list_added_time_;
  }

  // Adds the tab to the reading list and records the current time.
  // No-op if already in the reading list.
  void AddToReadingList();

  // Removes the tab from the reading list and clears the added time.
  // No-op if not in the reading list.
  void RemoveFromReadingList();

  // -- Notes --------------------------------------------------------------

  // True if this tab has an attached note.
  //
  // Note content and full metadata are owned by AstraNoteService.
  // Here we only track whether a note exists and its ID for quick lookup.
  //
  // Truth: AstraNoteService owns note content; AstraTabFeatures stores
  //   the association reference on the tab.
  bool has_note() const { return has_note_; }

  // ID of the note associated with this tab.  Empty if no note is attached.
  const std::string& note_id() const { return note_id_; }

  // Sets the note ID.  Passing an empty string removes the note association.
  void set_note_id(std::string id) {
    note_id_ = std::move(id);
    has_note_ = !note_id_.empty();
  }

  // -- View state ---------------------------------------------------------

  // True if the tab is pinned in the tab strip.
  //
  // This is a projection — Chromium owns the actual pinned state via
  // TabStripModel.  We mirror it here for quick access from Astra UI
  // layers without reaching into the browser layer.
  //
  // Truth: TabStripModel::IsTabPinned()
  //   (chrome/browser/ui/tabs/tab_strip_model.h)
  // Synced by: AstraTabFeatures observer or TabStripModelObserver patch.
  bool is_pinned() const { return is_pinned_; }
  void set_is_pinned(bool pinned) { is_pinned_ = pinned; }

  // Timestamp of the last time this tab was active (foreground / user
  // interaction). Used by the memory saver service and tab freshness
  // calculations.
  //
  // This is Astra's tracking of activity — Chromium's TabManager also
  // tracks last active time internally. We maintain our own for policy
  // decisions (e.g., workspace-aware suspension).
  //
  // Chromium owner: resource_coordinator::TabManager::last_active_time()
  //   (chrome/browser/resource_coordinator/tab_manager.h)
  base::TimeTicks last_active_time() const { return last_active_time_; }
  void set_last_active_time(base::TimeTicks time) { last_active_time_ = time; }

  // Hint about the tab's current index in the tab strip.  Updated by an
  // observer hook.  -1 means unknown or not applicable.
  //
  // This is a hint, not the source of truth.  The actual index is owned by
  // TabStripModel.  We cache it here for performance in UI layers that
  // need quick index lookups.
  //
  // Truth: TabStripModel::GetIndexOfWebContents()
  //   (chrome/browser/ui/tabs/tab_strip_model.h)
  int tab_index_hint() const { return tab_index_hint_; }
  void set_tab_index_hint(int index) { tab_index_hint_ = index; }

  // -- Screenshot / thumbnail ---------------------------------------------

  // True if a tab thumbnail / screenshot is available for this tab.
  //
  // The actual thumbnail image is owned by AstraScreenshotService.
  // Here we only track availability and last update time.
  //
  // Truth: AstraScreenshotService owns the actual image data.
  bool has_thumbnail() const { return has_thumbnail_; }
  void set_has_thumbnail(bool has_thumbnail) {
    has_thumbnail_ = has_thumbnail;
  }

  // Time when the thumbnail was last captured / updated.
  // Only meaningful when has_thumbnail() is true.
  base::Time thumbnail_last_updated() const {
    return thumbnail_last_updated_;
  }
  void set_thumbnail_last_updated(base::Time time) {
    thumbnail_last_updated_ = time;
  }

  // -- Tab discard state --------------------------------------------------

  // True if the tab has been discarded (memory saver / tab unloading).
  //
  // This is a projection — Chromium owns the actual discard state via
  // WebContents::IsDiscarded() and resource_coordinator::TabManager.
  //
  // Note: is_discarded_ is distinct from is_suspended_.  is_suspended_
  // tracks Astra's memory-saver decision (whether Astra has chosen to
  // suspend this tab), while is_discarded_ mirrors Chromium's actual
  // discard state.  In practice they should be in sync most of the time,
  // but they serve different conceptual layers.
  //
  // Truth: content::WebContents::IsDiscarded()
  //   (content/public/browser/web_contents.h)
  // Chromium owner: resource_coordinator::TabManager
  //   (chrome/browser/resource_coordinator/tab_manager.h)
  bool is_discarded() const { return is_discarded_; }
  void set_is_discarded(bool discarded) { is_discarded_ = discarded; }

  // Number of times this tab has been discarded.
  // Used for heuristics and telemetry.
  int discard_count() const { return discard_count_; }
  void set_discard_count(int count) { discard_count_ = count; }

  // Increments the discard count by 1.  Convenience method.
  void IncrementDiscardCount() { ++discard_count_; }

  // -- Memory saver / suspension ------------------------------------------

  // Whether this tab is currently suspended (discarded) from a memory-saver
  // perspective. This is Astra metadata only — the actual tab discard is
  // performed by Chromium's TabManager / resource_coordinator subsystem.
  //
  // Truth model:
  //   - Chromium owns the actual discard state (WebContents::IsDiscarded()).
  //   - This flag is Astra's projection of that state plus additional
  //     metadata (suspended_url_, last_active_time_).
  //   - The memory saver service keeps this in sync with Chromium's
  //     discard state via TabManager / WebContents observation.
  //
  // Chromium owner: TabManager / resource_coordinator::TabManager
  //   (chrome/browser/resource_coordinator/tab_manager.h)
  // Chromium owner: content::WebContents::IsDiscarded()
  //   (content/public/browser/web_contents.h)
  bool is_suspended() const { return is_suspended_; }

  // Sets the suspended state. Called by AstraMemorySaverService when it
  // observes a tab being discarded or restored by Chromium, or when
  // initiating a manual suspend/restore.
  //
  // Note: This does NOT actually discard the tab. The actual discard is
  // done by Chromium's TabManager. This only updates Astra's metadata.
  // TODO(astra): Integrate with Chromium's TabManager observer to keep
  // this flag in sync with actual discard state, rather than relying on
  // the memory saver service to set it.
  // Patch point: resource_coordinator::TabManagerObserver or
  // chrome/browser/resource_coordinator/tab_manager_delegate.cc.
  void SetSuspended(bool suspended);

  // The URL that was displayed when the tab was suspended. Used to reload
  // the tab when it is restored (clicked / woken up).
  //
  // This mirrors Chromium's NavigationController::GetVisibleURL() for a
  // discarded tab, but we cache it here for quick UI access without having
  // to read from the NavigationController of a discarded WebContents.
  const GURL& suspended_url() const { return suspended_url_; }
  void set_suspended_url(const GURL& url) { suspended_url_ = url; }

  // Convenience: returns true if this tab is eligible for auto-suspension
  // given the current timeout. Does NOT check policy exceptions (audio,
  // pinned, active workspace) — those are checked by the memory saver
  // service before suspending.
  bool IsEligibleForSuspend(base::TimeDelta timeout) const {
    return !is_suspended_ &&
           !last_active_time_.is_null() &&
           (base::TimeTicks::Now() - last_active_time_) >= timeout;
  }

  // ========================================================================
  // Utility / query methods
  // ========================================================================

  // Returns true if the tab is in any kind of stack (named stack,
  // hierarchical stack, or tab group).
  bool IsInAnyStack() const;

  // Returns true if the tab has never been active after creation
  // (last_active_time_ is null).
  bool IsUntouched() const;

  // Returns the time delta since the tab was last active.
  // Returns a zero delta if the tab has never been active.
  base::TimeDelta GetTimeSinceLastActive() const;

  // Returns true if the tab has been inactive for longer than |threshold|.
  // Returns false if the tab has never been active (IsUntouched()).
  bool IsStale(base::TimeDelta threshold) const;

  // Copies all Astra metadata from |other| to this tab.
  // Used when duplicating a tab or restoring tab state.
  void CopyFrom(const AstraTabFeatures& other);

  // Clears all Astra-specific metadata, resetting it to default values.
  // This is similar to Reset() but does NOT re-evaluate incognito read-only
  // state from the WebContents (it preserves the current state).
  // Use Reset() when reusing a WebContents; use ClearAllAstraMetadata()
  // when you just want to wipe Astra metadata.
  void ClearAllAstraMetadata();

  // Returns true if any Astra metadata is set to a non-default value.
  // Used to determine if a tab has any Astra-specific customization.
  bool HasAnyAstraMetadata() const;

 private:
  friend class content::WebContentsUserData<AstraTabFeatures>;

  explicit AstraTabFeatures(content::WebContents* web_contents);

  // Workspace membership.
  std::string workspace_id_ = "default";

  // Favorite state and ordering.
  bool is_favorite_ = false;
  size_t favorite_order_index_ = 0;
  std::string favorite_folder_id_ = "root";
  // When true, is_favorite_ cannot be changed (incognito mode).
  // Set during construction based on the WebContents' BrowserContext.
  bool favorite_read_only_ = false;

  // Split view configuration.
  bool is_in_split_view_ = false;
  std::string split_view_partner_id_;
  float split_view_ratio_ = 0.5f;
  SplitViewOrientation split_view_orientation_ =
      SplitViewOrientation::kHorizontal;

  // Glance / peek state.
  bool is_glance_tab_ = false;
  std::string glance_source_tab_id_;

  // Sidebar presentation flags.
  bool sidebar_pinned_ = false;
  bool sidebar_hidden_ = false;

  // Named tab stack membership.
  //
  // stack_id_: ID of the named stack this tab belongs to.  Empty if the
  //   tab is not in a named stack.  References an AstraTabStack in
  //   AstraTabStackService.
  //
  // Chromium analog: TabGroupId (tab_groups/tab_group_id.h)
  // TODO(astra): Consider using a dedicated ID type (like base::UnguessableToken)
  //   instead of a plain string for type safety.
  std::string stack_id_;

  // Tab stacking / tree organization (hierarchical, parent-child).
  //
  // Stack parent/child relationships are Astra metadata layered on top of
  // Chromium's flat TabStripModel.  The sidebar projects these relationships
  // as a tree view with collapsible parent nodes.
  //
  // is_in_stack_: true when this tab has a parent (is a child in a stack).
  // stack_parent_id_: opaque string ID of the parent tab.  Empty if not
  //   stacked.  Can be resolved to WebContents by AstraTabStackService.
  // is_stack_collapsed_: whether the stack headed by this tab is collapsed.
  //   Only meaningful for stack parents (tabs that have children).
  //
  // Chromium owner: TabGroupModel (for the analogous group concept)
  //   (chrome/browser/ui/tabs/tab_group_model.h)
  // TODO(astra): Consider replacing with Chromium tab groups plus a sidebar
  //   tree projection, if hierarchical grouping can be mapped cleanly.
  bool is_in_stack_ = false;
  std::string stack_parent_id_;
  bool is_stack_collapsed_ = false;

  // Tab stack / grouping (flat tab groups).
  //
  // tab_stack_id_: ID of the tab stack (group) this tab belongs to.
  //   Empty if not in any stack.
  // stack_position_: 0-based position within the stack.
  //
  // This represents flat tab grouping (like Chromium tab groups).
  // Distinct from named stacks (stack_id_, which have richer metadata)
  // and hierarchical stacking (stack_parent_id_).
  //
  // Chromium analog: TabGroup / TabGroupId
  //   (chrome/browser/ui/tabs/tab_group.h)
  std::string tab_stack_id_;
  size_t stack_position_ = 0;

  // Picture-in-Picture (PiP) tab metadata.
  //
  // is_pip_tab_: true when this tab is displayed as a PiP window.
  //   This is Astra's projection of Chromium's PiP state.
  //   Truth source: PictureInPictureWindowController.
  //
  // pip_window_size_: cached size of the PiP window for this tab.
  //   Empty when not in PiP mode.
  //
  // Chromium owner: PictureInPictureWindowController
  //   (chrome/browser/picture_in_picture/picture_in_picture_window_controller.h)
  // Patch point: PiP window controller lifecycle hooks to sync state.
  bool is_pip_tab_ = false;
  gfx::Size pip_window_size_;

  // Reading list state.
  //
  // is_in_reading_list_: true if the tab is in the reading list.
  //   This is Astra's projection of Chromium's ReadingListModel state.
  //
  // reading_list_added_time_: when the tab was added to the reading list.
  //
  // Chromium owner: ReadingListModel
  //   (components/reading_list/core/reading_list_model.h)
  bool is_in_reading_list_ = false;
  base::Time reading_list_added_time_;

  // Notes association.
  //
  // has_note_: true if this tab has an attached note.
  // note_id_: ID of the associated note.  Empty when no note.
  //
  // Truth: AstraNoteService owns the actual note content.
  bool has_note_ = false;
  std::string note_id_;

  // View state projections.
  //
  // is_pinned_: projection of TabStripModel pinned state.
  //   Truth: TabStripModel::IsTabPinned()
  //
  // last_active_time_: when the tab was last active.
  //   Used for memory saver and staleness checks.
  //   (Note: uses TimeTicks for elapsed time calculations; if wall-clock
  //    time is needed, convert via Time::Now() and TimeTicks::Now().)
  //
  // tab_index_hint_: cached tab strip index.  -1 means unknown.
  //   Truth: TabStripModel::GetIndexOfWebContents()
  bool is_pinned_ = false;
  base::TimeTicks last_active_time_;
  int tab_index_hint_ = -1;

  // Thumbnail / screenshot state.
  //
  // has_thumbnail_: whether a thumbnail is available.
  // thumbnail_last_updated_: when the thumbnail was last captured.
  //
  // Truth: AstraScreenshotService owns the actual image data.
  bool has_thumbnail_ = false;
  base::Time thumbnail_last_updated_;

  // Tab discard state projection.
  //
  // is_discarded_: projection of Chromium's discard state.
  //   Truth: content::WebContents::IsDiscarded()
  //
  // discard_count_: how many times the tab has been discarded.
  //   Used for heuristics and telemetry.
  //
  // Chromium owner: resource_coordinator::TabManager
  //   (chrome/browser/resource_coordinator/tab_manager.h)
  bool is_discarded_ = false;
  int discard_count_ = 0;

  // Memory saver / suspended tab metadata.
  //
  // is_suspended_ tracks whether the tab is currently in a suspended
  // (discarded) state from Astra's memory saver perspective. This is
  // Astra metadata — the actual discard is owned by Chromium's
  // resource_coordinator::TabManager.
  //
  // suspended_url_ caches the URL to reload when the tab is restored,
  // so the sidebar can show it without touching a discarded WebContents.
  //
  // last_active_time_ records when the tab was last the active tab or
  // had user interaction. Used by the memory saver to determine eligibility.
  //
  // Chromium owner: resource_coordinator::TabManager
  //   (chrome/browser/resource_coordinator/tab_manager.h)
  // Patch point: TabManager observer to sync is_suspended_ with real state.
  bool is_suspended_ = false;
  GURL suspended_url_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_TAB_FEATURES_H_
