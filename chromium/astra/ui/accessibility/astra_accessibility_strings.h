// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_ACCESSIBILITY_ASTRA_ACCESSIBILITY_STRINGS_H_
#define ASTRA_UI_ACCESSIBILITY_ASTRA_ACCESSIBILITY_STRINGS_H_

#include "base/strings/string_piece.h"

namespace astra {
namespace accessibility {

// =========================================================================
// Accessibility String Constants
// =========================================================================
//
// Screen reader announcement strings and accessibility labels used by
// Astra UI components.  These follow Chromium's l10n pattern where
// strings are defined as constants and can be replaced with localized
// strings from a GRD file at build time.
//
// For now, these are stub strings in English.  In the full Chromium
// build, these would be generated from
// //chrome/app/chromeos_strings.grd or a similar Astra-specific GRD file.
//
// TODO(astra): Replace stub strings with proper l10n strings from a GRD
//   file.  Use IDS_* constants and base::i18n to look up translations.
// Chromium pattern: chrome/app/chrome_strings.grd -> chrome/grit/chromium_strings.h
// Chromium utility: ui/base/l10n/l10n_util.h (GetStringUTF16, GetStringFUTF16)
// =========================================================================

// -- Sidebar -----------------------------------------------------------------

// Accessible name for the sidebar container region.
// Announced when focus enters the sidebar area.
constexpr base::StringPiece16 kSidebarAccessibleName = u"Sidebar";

// Role description for sidebar sections (groups of items).
constexpr base::StringPiece16 kSidebarSectionRoleDescription =
    u"Sidebar section";

// Accessible name for the favorites section in the sidebar.
constexpr base::StringPiece16 kSidebarFavoritesSectionName = u"Favorites";

// Accessible name for the history section in the sidebar.
constexpr base::StringPiece16 kSidebarHistorySectionName = u"History";

// Accessible name for the downloads section in the sidebar.
constexpr base::StringPiece16 kSidebarDownloadsSectionName = u"Downloads";

// Accessible name for the extensions section in the sidebar.
constexpr base::StringPiece16 kSidebarExtensionsSectionName = u"Extensions";

// Announcement when a sidebar item is activated (opened).
// TODO(astra): Make this a format string with the item name.
//   Pattern: "{item name} opened"
constexpr base::StringPiece16 kSidebarItemOpenedAnnouncement = u"Item opened";

// Announcement when a sidebar item is added to favorites.
constexpr base::StringPiece16 kSidebarItemAddedToFavorites =
    u"Added to favorites";

// Announcement when a sidebar item is removed from favorites.
constexpr base::StringPiece16 kSidebarItemRemovedFromFavorites =
    u"Removed from favorites";

// -- Spaces / workspaces -----------------------------------------------------

// Accessible name for the space selector widget.
constexpr base::StringPiece16 kSpaceSelectorAccessibleName = u"Space switcher";

// Role description for a space (workspace).
constexpr base::StringPiece16 kSpaceRoleDescription = u"Space";

// Announcement when switching to a different space.
// Format: "Switched to {space name} space"
// TODO(astra): Add format string variant with placeholder.
constexpr base::StringPiece16 kSpaceSwitchedAnnouncement = u"Space switched";

// Announcement when a new space is created.
constexpr base::StringPiece16 kSpaceCreatedAnnouncement = u"New space created";

// Announcement when a space is closed.
constexpr base::StringPiece16 kSpaceClosedAnnouncement = u"Space closed";

// -- Split view --------------------------------------------------------------

// Accessible name for the split view container.
constexpr base::StringPiece16 kSplitViewAccessibleName = u"Split view";

// Role description for a split view divider (resizer).
constexpr base::StringPiece16 kSplitViewDividerRoleDescription =
    u"Split divider";

// Announcement when split view is activated.
constexpr base::StringPiece16 kSplitViewActivatedAnnouncement =
    u"Split view activated";

// Announcement when split view is deactivated.
constexpr base::StringPiece16 kSplitViewDeactivatedAnnouncement =
    u"Split view deactivated";

// Announcement when resizing the split divider.
// Format: "Split at {percent} percent"
constexpr base::StringPiece16 kSplitResizeAnnouncement =
    u"Split view resized";

// -- Tabs --------------------------------------------------------------------

// Announcement when a tab is pinned.
constexpr base::StringPiece16 kTabPinnedAnnouncement = u"Tab pinned";

// Announcement when a tab is unpinned.
constexpr base::StringPiece16 kTabUnpinnedAnnouncement = u"Tab unpinned";

// Announcement when a tab is moved to another space.
// Format: "Tab moved to {space name}"
constexpr base::StringPiece16 kTabMovedToSpaceAnnouncement = u"Tab moved";

// -- Glance / preview --------------------------------------------------------

// Accessible name for a glance preview popup.
constexpr base::StringPiece16 kGlancePreviewAccessibleName = u"Tab preview";

// Announcement when a glance preview is shown.
constexpr base::StringPiece16 kGlancePreviewShown = u"Preview shown";

// -- Command palette ---------------------------------------------------------

// Accessible name for the command palette.
constexpr base::StringPiece16 kCommandPaletteAccessibleName = u"Command palette";

// Hint text for the command palette input field.
constexpr base::StringPiece16 kCommandPaletteHintText =
    u"Type a command or search";

// Announcement for command palette results count.
// Format: "{count} results"
// TODO(astra): Add plural-aware format string.
constexpr base::StringPiece16 kCommandPaletteResultsCount = u"results";

// -- Focus and navigation ----------------------------------------------------

// Announcement when focus moves to a new section.
// Format: "{section name}, {position} of {total}"
constexpr base::StringPiece16 kFocusSectionAnnouncement = u"section";

// Announcement when a bubble dialog opens and receives focus.
constexpr base::StringPiece16 kBubbleOpenedAnnouncement = u"Dialog opened";

// Announcement when a bubble dialog closes and focus is restored.
constexpr base::StringPiece16 kBubbleClosedAnnouncement = u"Dialog closed";

// -- Status announcements ----------------------------------------------------

// Generic status announcement prefix.
constexpr base::StringPiece16 kStatusPrefix = u"Status";

// Alert announcement prefix (for assertive live regions).
constexpr base::StringPiece16 kAlertPrefix = u"Alert";

// Success announcement suffix.
constexpr base::StringPiece16 kSuccessSuffix = u"success";

// Error announcement suffix.
constexpr base::StringPiece16 kErrorSuffix = u"error";

// -- Keyboard navigation hints -----------------------------------------------

// Hint for activating a focused item (Enter/Space).
constexpr base::StringPiece16 kActivateHint =
    u"Press Enter or Space to activate";

// Hint for opening the context menu on a focused item.
constexpr base::StringPiece16 kContextMenuHint =
    u"Press Menu or Shift+F10 for context menu";

// Hint for navigating a list with arrow keys.
constexpr base::StringPiece16 kArrowKeyNavigationHint =
    u"Use arrow keys to navigate";

// =========================================================================
// Format string helpers (stub)
// =========================================================================
//
// TODO(astra): Implement proper format string helpers using Chromium's
//   l10n_util pattern.  These functions will eventually call
//   l10n_util::GetStringFUTF16() with IDS_ constants.
//
// For now, these are simple concatenation stubs that demonstrate the API.
// Chromium pattern: ui/base/l10n/l10n_util.h
// =========================================================================

// Returns the results count announcement string.
// Stub implementation: returns "{count} results"
//
// TODO(astra): Replace with proper l10n format string from GRD.
//   Chromium component: ui/base/l10n/l10n_util.h (GetPluralStringFUTF16)
std::u16string GetResultsCountString(int count);

// Returns the space switched announcement string with the space name.
//
// TODO(astra): Replace with l10n_util::GetStringFUTF16.
std::u16string GetSpaceSwitchedString(const std::u16string& space_name);

// Returns the tab moved announcement string with the destination space name.
//
// TODO(astra): Replace with l10n_util::GetStringFUTF16.
std::u16string GetTabMovedToString(const std::u16string& space_name);

// Returns the split resize announcement with the current percentage.
//
// TODO(astra): Replace with l10n_util::GetStringFUTF16Int.
std::u16string GetSplitResizeString(int percent);

}  // namespace accessibility
}  // namespace astra

#endif  // ASTRA_UI_ACCESSIBILITY_ASTRA_ACCESSIBILITY_STRINGS_H_
