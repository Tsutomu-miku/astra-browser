// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_OMNIBOX_POPUP_ASTRA_OMNIBOX_POPUP_MODEL_H_
#define ASTRA_UI_VIEWS_OMNIBOX_POPUP_ASTRA_OMNIBOX_POPUP_MODEL_H_

#include <string>
#include <vector>

#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/image/image_skia.h"

namespace astra {

// Types of omnibox suggestion matches.
enum class AstraOmniboxMatchType {
  kSearchWhatYouTyped,   // "Search Google for..."
  kSearchHistory,        // Previous search
  kSearchSuggestion,     // Search engine suggestion
  kUrlHistory,           // History URL match
  kUrlBookmark,          // Bookmark match
  kUrlOpenTab,           // Switch to open tab
  kUrlNavsuggest,        // Navigation suggestion
  kClipboard,            // Clipboard URL
  kDocument,             // Document suggestion (Drive, etc.)
  kAnswer,               // Direct answer (e.g. weather, calculator)
  kOmniboxAction,        // Omnibox action (e.g. "Clear browsing data")
  kExtensionCommand,     // Extension command
};

// Content type for answers.
enum class AstraOmniboxAnswerType {
  kNone,
  kCalculator,
  kWeather,
  kDictionary,
  kStock,
  kTranslation,
  kSunriseSunset,
  kFlightStatus,
  kSports,
  kTimeZone,
  kUnitConversion,
};

// A single omnibox suggestion match.
struct AstraOmniboxMatch {
  // Match type (determines icon and section).
  AstraOmniboxMatchType type = AstraOmniboxMatchType::kSearchWhatYouTyped;

  // Display content.
  std::u16string contents;       // Primary text (URL or search query)
  std::u16string description;    // Secondary text (title or description)

  // URL for navigation matches.
  std::string destination_url;

  // Answer fields (for answer type matches).
  AstraOmniboxAnswerType answer_type = AstraOmniboxAnswerType::kNone;
  std::u16string answer_text;
  std::u16string answer_additional_text;
  gfx::ImageSkia answer_image;

  // Icon.
  gfx::ImageSkia icon;
  std::string icon_name;  // Name of default icon if no image.

  // Relevance score (higher = more relevant).
  int relevance = 0;

  // Whether this is the default match (first result).
  bool is_default = false;

  // Whether to show a star (bookmarked).
  bool is_bookmarked = false;

  // Whether this suggests switching to an open tab.
  bool is_tab_switch = false;
  std::string tab_id;

  // Answer badge text (e.g. "Calculator", "Weather").
  std::u16string answer_badge;

  // Action button (e.g. "Remove", "Preview").
  std::u16string action_label;
  std::string action_type;
};

// Observer for AstraOmniboxPopupModel.
class AstraOmniboxPopupObserver : public base::CheckedObserver {
 public:
  // Called when the list of suggestions changes.
  virtual void OnSuggestionsChanged(AstraOmniboxPopupModel* model) {}

  // Called when the selected/highlighted match changes.
  virtual void OnSelectedMatchChanged(AstraOmniboxPopupModel* model) {}

  // Called when the popup should be shown or hidden.
  virtual void OnPopupVisibilityChanged(AstraOmniboxPopupModel* model,
                                        bool visible) {}

  // Called when the model is about to be destroyed.
  virtual void OnOmniboxPopupModelShutdown(AstraOmniboxPopupModel* model) {}

 protected:
  ~AstraOmniboxPopupObserver() override = default;
};

// Model for the omnibox popup.
//
// Owns the list of omnibox suggestions and the currently selected match.
// Suggestions come from Chromium's AutocompleteController — this model
// projects those results into a form suitable for the Astra popup UI.
//
// Chromium owner: OmniboxPopupModel / AutocompleteController
//   (chrome/browser/ui/omnibox/omnibox_popup_model.h)
//   (components/omnibox/browser/autocomplete_controller.h)
//
// TODO(astra): Wire up to Chromium's AutocompleteController via
// OmniboxController and OmniboxEditModel.  Patch point:
// chrome/browser/ui/omnibox/omnibox_edit_model.cc
class AstraOmniboxPopupModel {
 public:
  AstraOmniboxPopupModel();
  ~AstraOmniboxPopupModel();

  AstraOmniboxPopupModel(const AstraOmniboxPopupModel&) = delete;
  AstraOmniboxPopupModel& operator=(const AstraOmniboxPopupModel&) = delete;

  // -- Observer management --------------------------------------------------

  void AddObserver(AstraOmniboxPopupObserver* observer);
  void RemoveObserver(AstraOmniboxPopupObserver* observer);

  // -- Suggestions ----------------------------------------------------------

  // Get all matches.
  const std::vector<AstraOmniboxMatch>& GetMatches() const;

  // Get the number of matches.
  size_t GetMatchCount() const;

  // Get a match by index. Returns nullptr if out of bounds.
  const AstraOmniboxMatch* GetMatchAt(size_t index) const;

  // -- Selection ------------------------------------------------------------

  // Get the index of the currently selected match.
  size_t GetSelectedIndex() const;

  // Get the currently selected match. Returns nullptr if no selection.
  const AstraOmniboxMatch* GetSelectedMatch() const;

  // Set the selected index. Clamps to valid range.
  void SetSelectedIndex(size_t index);

  // Move selection down (next match). Wraps around.
  void SelectNext();

  // Move selection up (previous match). Wraps around.
  void SelectPrevious();

  // Select the first match.
  void SelectFirst();

  // Select the last match.
  void SelectLast();

  // -- Visibility -----------------------------------------------------------

  // Show the popup.
  void Show();

  // Hide the popup.
  void Hide();

  // Whether the popup is currently visible.
  bool IsVisible() const;

  // -- Suggestion manipulation ----------------------------------------------

  // Set the full list of matches.
  void SetMatches(const std::vector<AstraOmniboxMatch>& matches);

  // Add a match.
  void AddMatch(const AstraOmniboxMatch& match);

  // Clear all matches.
  void ClearMatches();

  // Remove a match at the given index.
  void RemoveMatchAt(size_t index);

  // Update the default selection based on current matches.
  void UpdateDefaultSelection();

  // -- Query ----------------------------------------------------------------

  // Set the current input query.
  void SetQuery(const std::u16string& query);
  const std::u16string& GetQuery() const;

  // -- Grouping / sections --------------------------------------------------

  // Get matches grouped by type.
  // Returns pairs of (section_title, vector_of_matches).
  std::vector<std::pair<std::u16string, std::vector<AstraOmniboxMatch>>>
  GetGroupedMatches() const;

  // -- Sample data ----------------------------------------------------------

  // Populate with sample suggestions for testing/development.
  void PopulateSampleSuggestions(const std::u16string& query);

 private:
  // Notify observers that suggestions changed.
  void NotifySuggestionsChanged();

  // Notify observers that selection changed.
  void NotifySelectedMatchChanged();

  // Notify observers that visibility changed.
  void NotifyVisibilityChanged();

  std::vector<AstraOmniboxMatch> matches_;
  size_t selected_index_ = 0;
  bool visible_ = false;
  std::u16string query_;

  base::ObserverList<AstraOmniboxPopupObserver> observers_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_OMNIBOX_POPUP_ASTRA_OMNIBOX_POPUP_MODEL_H_
