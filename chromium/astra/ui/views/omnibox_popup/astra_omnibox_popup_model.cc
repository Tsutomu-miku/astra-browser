// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/omnibox_popup/astra_omnibox_popup_model.h"

#include <algorithm>
#include <map>

#include "base/check.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"

namespace astra {

AstraOmniboxPopupModel::AstraOmniboxPopupModel() = default;

AstraOmniboxPopupModel::~AstraOmniboxPopupModel() {
  for (auto& observer : observers_) {
    observer.OnOmniboxPopupModelShutdown(this);
  }
}

void AstraOmniboxPopupModel::AddObserver(
    AstraOmniboxPopupObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraOmniboxPopupModel::RemoveObserver(
    AstraOmniboxPopupObserver* observer) {
  observers_.RemoveObserver(observer);
}

const std::vector<AstraOmniboxMatch>& AstraOmniboxPopupModel::GetMatches()
    const {
  return matches_;
}

size_t AstraOmniboxPopupModel::GetMatchCount() const {
  return matches_.size();
}

const AstraOmniboxMatch* AstraOmniboxPopupModel::GetMatchAt(
    size_t index) const {
  if (index >= matches_.size()) {
    return nullptr;
  }
  return &matches_[index];
}

size_t AstraOmniboxPopupModel::GetSelectedIndex() const {
  return selected_index_;
}

const AstraOmniboxMatch* AstraOmniboxPopupModel::GetSelectedMatch() const {
  if (selected_index_ >= matches_.size()) {
    return nullptr;
  }
  return &matches_[selected_index_];
}

void AstraOmniboxPopupModel::SetSelectedIndex(size_t index) {
  if (matches_.empty()) {
    selected_index_ = 0;
    return;
  }
  size_t new_index = std::min(index, matches_.size() - 1);
  if (new_index == selected_index_) {
    return;
  }
  selected_index_ = new_index;
  NotifySelectedMatchChanged();
}

void AstraOmniboxPopupModel::SelectNext() {
  if (matches_.empty()) {
    return;
  }
  size_t new_index = selected_index_ + 1;
  if (new_index >= matches_.size()) {
    new_index = 0;  // Wrap around.
  }
  SetSelectedIndex(new_index);
}

void AstraOmniboxPopupModel::SelectPrevious() {
  if (matches_.empty()) {
    return;
  }
  if (selected_index_ == 0) {
    SetSelectedIndex(matches_.size() - 1);  // Wrap around.
  } else {
    SetSelectedIndex(selected_index_ - 1);
  }
}

void AstraOmniboxPopupModel::SelectFirst() {
  SetSelectedIndex(0);
}

void AstraOmniboxPopupModel::SelectLast() {
  if (matches_.empty()) {
    return;
  }
  SetSelectedIndex(matches_.size() - 1);
}

void AstraOmniboxPopupModel::Show() {
  if (visible_) {
    return;
  }
  visible_ = true;
  NotifyVisibilityChanged();
}

void AstraOmniboxPopupModel::Hide() {
  if (!visible_) {
    return;
  }
  visible_ = false;
  NotifyVisibilityChanged();
}

bool AstraOmniboxPopupModel::IsVisible() const {
  return visible_;
}

void AstraOmniboxPopupModel::SetMatches(
    const std::vector<AstraOmniboxMatch>& matches) {
  matches_ = matches;
  UpdateDefaultSelection();
  NotifySuggestionsChanged();
}

void AstraOmniboxPopupModel::AddMatch(const AstraOmniboxMatch& match) {
  matches_.push_back(match);
  // Keep sorted by relevance (descending).
  std::sort(matches_.begin(), matches_.end(),
            [](const AstraOmniboxMatch& a, const AstraOmniboxMatch& b) {
              return a.relevance > b.relevance;
            });
  NotifySuggestionsChanged();
}

void AstraOmniboxPopupModel::ClearMatches() {
  if (matches_.empty()) {
    return;
  }
  matches_.clear();
  selected_index_ = 0;
  NotifySuggestionsChanged();
}

void AstraOmniboxPopupModel::RemoveMatchAt(size_t index) {
  if (index >= matches_.size()) {
    return;
  }
  matches_.erase(matches_.begin() + index);
  if (selected_index_ >= matches_.size()) {
    selected_index_ = matches_.empty() ? 0 : matches_.size() - 1;
  }
  NotifySuggestionsChanged();
}

void AstraOmniboxPopupModel::UpdateDefaultSelection() {
  if (matches_.empty()) {
    selected_index_ = 0;
    return;
  }
  // The first match is usually the default.
  selected_index_ = 0;
  // Mark as default.
  if (!matches_.empty()) {
    matches_[0].is_default = true;
    for (size_t i = 1; i < matches_.size(); i++) {
      matches_[i].is_default = false;
    }
  }
}

void AstraOmniboxPopupModel::SetQuery(const std::u16string& query) {
  if (query_ == query) {
    return;
  }
  query_ = query;
  // In a real implementation, this would trigger an autocomplete request.
}

const std::u16string& AstraOmniboxPopupModel::GetQuery() const {
  return query_;
}

std::vector<std::pair<std::u16string, std::vector<AstraOmniboxMatch>>>
AstraOmniboxPopupModel::GetGroupedMatches() const {
  std::map<AstraOmniboxMatchType, std::vector<AstraOmniboxMatch>> groups;
  for (const auto& match : matches_) {
    groups[match.type].push_back(match);
  }

  std::vector<std::pair<std::u16string, std::vector<AstraOmniboxMatch>>> result;

  // Define section order and titles.
  struct SectionInfo {
    AstraOmniboxMatchType type;
    std::u16string title;
  };

  std::vector<SectionInfo> sections = {
      {AstraOmniboxMatchType::kSearchWhatYouTyped, u"Search"},
      {AstraOmniboxMatchType::kSearchSuggestion, u"Searches"},
      {AstraOmniboxMatchType::kSearchHistory, u"Search history"},
      {AstraOmniboxMatchType::kUrlOpenTab, u"Switch to tab"},
      {AstraOmniboxMatchType::kUrlBookmark, u"Bookmarks"},
      {AstraOmniboxMatchType::kUrlHistory, u"History"},
      {AstraOmniboxMatchType::kAnswer, u"Answers"},
      {AstraOmniboxMatchType::kUrlNavsuggest, u"Navigation"},
      {AstraOmniboxMatchType::kOmniboxAction, u"Actions"},
      {AstraOmniboxMatchType::kClipboard, u"Clipboard"},
      {AstraOmniboxMatchType::kDocument, u"Documents"},
      {AstraOmniboxMatchType::kExtensionCommand, u"Extensions"},
  };

  for (const auto& section : sections) {
    auto it = groups.find(section.type);
    if (it != groups.end() && !it->second.empty()) {
      result.push_back({section.title, it->second});
    }
  }

  return result;
}

void AstraOmniboxPopupModel::PopulateSampleSuggestions(
    const std::u16string& query) {
  matches_.clear();

  if (query.empty()) {
    // No query = no suggestions (in a real implementation, there might be
    // zero-input suggestions like most visited sites).
    selected_index_ = 0;
    NotifySuggestionsChanged();
    return;
  }

  std::u16string query_lower = base::ToLowerASCII(query);
  int relevance_base = 1000;

  // "What you typed" search suggestion.
  AstraOmniboxMatch what_you_typed;
  what_you_typed.type = AstraOmniboxMatchType::kSearchWhatYouTyped;
  what_you_typed.contents = query;
  what_you_typed.description = u"Search Google";
  what_you_typed.icon_name = "search";
  what_you_typed.relevance = relevance_base;
  what_you_typed.is_default = true;
  matches_.push_back(what_you_typed);

  // URL match for history.
  if (query.find(u"wiki") != std::u16string::npos ||
      query.find(u"example") != std::u16string::npos) {
    AstraOmniboxMatch history_match;
    history_match.type = AstraOmniboxMatchType::kUrlHistory;
    history_match.contents = u"https://en.wikipedia.org/wiki/Special:Search";
    history_match.description = u"Wikipedia - The Free Encyclopedia";
    history_match.destination_url = "https://en.wikipedia.org/";
    history_match.icon_name = "globe";
    history_match.relevance = 800;
    history_match.is_bookmarked = true;
    matches_.push_back(history_match);
  }

  // Open tab match.
  AstraOmniboxMatch tab_match;
  tab_match.type = AstraOmniboxMatchType::kUrlOpenTab;
  tab_match.contents = u"example.com";
  tab_match.description = u"Example Domain";
  tab_match.destination_url = "https://example.com";
  tab_match.icon_name = "tab";
  tab_match.relevance = 900;
  tab_match.is_tab_switch = true;
  tab_match.tab_id = "tab_123";
  tab_match.action_label = u"Switch";
  matches_.push_back(tab_match);

  // Search suggestions.
  std::vector<std::u16string> suggestions = {
      query + u" translate",
      query + u" meaning",
      query + u" wikipedia",
      query + u" google",
      query + u" 翻译",
  };

  for (size_t i = 0; i < suggestions.size(); i++) {
    AstraOmniboxMatch suggestion;
    suggestion.type = AstraOmniboxMatchType::kSearchSuggestion;
    suggestion.contents = suggestions[i];
    suggestion.description = u"Search suggestion";
    suggestion.icon_name = "search";
    suggestion.relevance = 700 - static_cast<int>(i) * 50;
    matches_.push_back(suggestion);
  }

  // Bookmark match.
  AstraOmniboxMatch bookmark_match;
  bookmark_match.type = AstraOmniboxMatchType::kUrlBookmark;
  bookmark_match.contents = u"github.com/astra-browser";
  bookmark_match.description = u"Astra Browser · GitHub";
  bookmark_match.destination_url = "https://github.com/astra-browser";
  bookmark_match.icon_name = "star";
  bookmark_match.relevance = 600;
  bookmark_match.is_bookmarked = true;
  matches_.push_back(bookmark_match);

  // History match.
  AstraOmniboxMatch history2;
  history2.type = AstraOmniboxMatchType::kUrlHistory;
  history2.contents = u"news.ycombinator.com";
  history2.description = u"Hacker News";
  history2.destination_url = "https://news.ycombinator.com";
  history2.icon_name = "history";
  history2.relevance = 500;
  matches_.push_back(history2);

  // Sort by relevance.
  std::sort(matches_.begin(), matches_.end(),
            [](const AstraOmniboxMatch& a, const AstraOmniboxMatch& b) {
              return a.relevance > b.relevance;
            });

  UpdateDefaultSelection();
  NotifySuggestionsChanged();
}

void AstraOmniboxPopupModel::NotifySuggestionsChanged() {
  for (auto& observer : observers_) {
    observer.OnSuggestionsChanged(this);
  }
}

void AstraOmniboxPopupModel::NotifySelectedMatchChanged() {
  for (auto& observer : observers_) {
    observer.OnSelectedMatchChanged(this);
  }
}

void AstraOmniboxPopupModel::NotifyVisibilityChanged() {
  for (auto& observer : observers_) {
    observer.OnPopupVisibilityChanged(this, visible_);
  }
}

}  // namespace astra
