// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/history_page/astra_history_page_model.h"

#include <algorithm>
#include <set>

#include "base/i18n/time_formatting.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"

namespace astra {

namespace {

// Helper to get the start of the day (midnight) for a given time.
base::Time StartOfDay(base::Time time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  exploded.hour = 0;
  exploded.minute = 0;
  exploded.second = 0;
  exploded.millisecond = 0;
  base::Time result;
  bool success = base::Time::FromLocalExploded(exploded, &result);
  DCHECK(success);
  return result;
}

// Helper to get the start of the current month.
base::Time StartOfMonth(base::Time time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  exploded.day_of_month = 1;
  exploded.hour = 0;
  exploded.minute = 0;
  exploded.second = 0;
  exploded.millisecond = 0;
  base::Time result;
  bool success = base::Time::FromLocalExploded(exploded, &result);
  DCHECK(success);
  return result;
}

// Generate a date label for a given day, relative to "now".
std::u16string GetDateLabel(base::Time day_date, base::Time now) {
  base::Time today_start = StartOfDay(now);
  base::Time yesterday_start = today_start - base::Days(1);
  base::Time day_start = StartOfDay(day_date);

  if (day_start == today_start) {
    return u"Today";
  }
  if (day_start == yesterday_start) {
    return u"Yesterday";
  }

  // Format as "Monday, June 9".
  // TODO(astra): Use proper i18n date formatting via
  // base::TimeFormatWithPattern or icu::DateFormat.
  base::Time::Exploded exploded;
  day_date.LocalExplode(&exploded);

  const char* weekdays[] = {
      "Sunday",   "Monday", "Tuesday", "Wednesday",
      "Thursday", "Friday", "Saturday"};
  const char* months[] = {
      "January", "February", "March",     "April",   "May",      "June",
      "July",    "August",   "September", "October", "November", "December"};

  std::string label = std::string(weekdays[exploded.day_of_week]) + ", " +
                      std::string(months[exploded.month - 1]) + " " +
                      base::NumberToString(exploded.day_of_month);
  return base::UTF8ToUTF16(label);
}

// Get the time range for a given filter, relative to "now".
void GetFilterTimeRange(AstraHistoryFilter filter,
                        base::Time now,
                        base::Time* out_start,
                        base::Time* out_end) {
  base::Time today_start = StartOfDay(now);

  *out_start = base::Time();   // null = no start bound
  *out_end = base::Time::Max();  // far future = no end bound

  switch (filter) {
    case AstraHistoryFilter::kAll:
      // No bounds.
      break;
    case AstraHistoryFilter::kToday:
      *out_start = today_start;
      *out_end = now;
      break;
    case AstraHistoryFilter::kYesterday:
      *out_start = today_start - base::Days(1);
      *out_end = today_start;
      break;
    case AstraHistoryFilter::kLast7Days:
      *out_start = today_start - base::Days(6);
      *out_end = now;
      break;
    case AstraHistoryFilter::kLast30Days:
      *out_start = today_start - base::Days(29);
      *out_end = now;
      break;
    case AstraHistoryFilter::kThisMonth:
      *out_start = StartOfMonth(now);
      *out_end = now;
      break;
  }
}

// Case-insensitive substring match of |needle| in |haystack|.
bool CaseInsensitiveContains(const std::u16string& haystack,
                             const std::u16string& needle) {
  if (needle.empty()) {
    return true;
  }
  return base::StrContains(base::ToLowerASCII(haystack),
                           base::ToLowerASCII(needle));
}

}  // namespace

// ===========================================================================
// AstraHistoryPageModel
// ===========================================================================

AstraHistoryPageModel::AstraHistoryPageModel() = default;

AstraHistoryPageModel::~AstraHistoryPageModel() {
  for (AstraHistoryPageObserver& observer : observers_) {
    observer.OnHistoryPageModelShutdown(this);
  }
}

// -- Observer management ----------------------------------------------------

void AstraHistoryPageModel::AddObserver(AstraHistoryPageObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraHistoryPageModel::RemoveObserver(AstraHistoryPageObserver* observer) {
  observers_.RemoveObserver(observer);
}

// -- History data -----------------------------------------------------------

const std::vector<AstraHistoryDay>& AstraHistoryPageModel::GetDays() const {
  return filtered_days_;
}

size_t AstraHistoryPageModel::GetTotalEntryCount() const {
  return all_entries_.size();
}

const AstraHistoryEntry* AstraHistoryPageModel::GetEntry(
    const std::string& id) const {
  for (const auto& entry : all_entries_) {
    if (entry.id == id) {
      return &entry;
    }
  }
  return nullptr;
}

// -- Filtering --------------------------------------------------------------

void AstraHistoryPageModel::SetFilter(AstraHistoryFilter filter) {
  if (filter_ == filter) {
    return;
  }
  filter_ = filter;
  ApplyFilters();
  NotifyFilterChanged();
  NotifyHistoryChanged();
}

std::vector<std::pair<AstraHistoryFilter, std::u16string>>
AstraHistoryPageModel::GetFilterOptions() const {
  return {
      {AstraHistoryFilter::kAll, u"All time"},
      {AstraHistoryFilter::kToday, u"Today"},
      {AstraHistoryFilter::kYesterday, u"Yesterday"},
      {AstraHistoryFilter::kLast7Days, u"Last 7 days"},
      {AstraHistoryFilter::kLast30Days, u"Last 30 days"},
      {AstraHistoryFilter::kThisMonth, u"This month"},
  };
}

// -- Search -----------------------------------------------------------------

void AstraHistoryPageModel::SetSearchQuery(const std::u16string& query) {
  if (search_query_ == query) {
    return;
  }
  search_query_ = query;
  ApplyFilters();
  NotifySearchChanged();
  NotifyHistoryChanged();
}

// -- Categories -------------------------------------------------------------

std::vector<std::string> AstraHistoryPageModel::GetCategories() const {
  std::set<std::string> unique_categories;
  for (const auto& entry : all_entries_) {
    if (!entry.category.empty()) {
      unique_categories.insert(entry.category);
    }
  }
  return std::vector<std::string>(unique_categories.begin(),
                                  unique_categories.end());
}

void AstraHistoryPageModel::SetCategoryFilter(const std::string& category) {
  if (category_filter_ == category) {
    return;
  }
  category_filter_ = category;
  ApplyFilters();
  NotifyHistoryChanged();
}

// -- Entry manipulation -----------------------------------------------------

void AstraHistoryPageModel::RemoveEntry(const std::string& id) {
  auto it = std::remove_if(all_entries_.begin(), all_entries_.end(),
                           [&id](const AstraHistoryEntry& e) {
                             return e.id == id;
                           });
  if (it == all_entries_.end()) {
    return;
  }
  all_entries_.erase(it, all_entries_.end());
  ApplyFilters();

  for (AstraHistoryPageObserver& observer : observers_) {
    observer.OnHistoryEntryRemoved(this, id);
  }
  NotifyHistoryChanged();
}

void AstraHistoryPageModel::RemoveAllInRange() {
  if (filter_ == AstraHistoryFilter::kAll && search_query_.empty() &&
      category_filter_.empty()) {
    ClearAllHistory();
    return;
  }

  base::Time now = base::Time::Now();
  base::Time range_start;
  base::Time range_end;
  GetFilterTimeRange(filter_, now, &range_start, &range_end);

  std::vector<std::string> removed_ids;
  all_entries_.erase(
      std::remove_if(all_entries_.begin(), all_entries_.end(),
                     [&](const AstraHistoryEntry& entry) {
                       bool in_time =
                           (range_start.is_null() ||
                            entry.visit_time >= range_start) &&
                           (range_end.is_null() ||
                            entry.visit_time < range_end);
                       bool matches_search = MatchesSearch(entry);
                       bool matches_category = MatchesCategory(entry);

                       if (in_time && matches_search && matches_category) {
                         removed_ids.push_back(entry.id);
                         return true;
                       }
                       return false;
                     }),
      all_entries_.end());

  ApplyFilters();

  for (const auto& id : removed_ids) {
    for (AstraHistoryPageObserver& observer : observers_) {
      observer.OnHistoryEntryRemoved(this, id);
    }
  }
  NotifyHistoryChanged();
}

void AstraHistoryPageModel::ClearAllHistory() {
  all_entries_.clear();
  ApplyFilters();
  NotifyHistoryChanged();
}

// -- Sample data ------------------------------------------------------------

void AstraHistoryPageModel::PopulateSampleHistory() {
  all_entries_.clear();

  base::Time now = base::Time::Now();
  base::Time today_start = StartOfDay(now);

  struct SampleEntry {
    const char* id;
    const char* title;
    const char* url;
    const char* host;
    const char* category;
    const char* workspace;
    int days_ago;
    int hours_offset;  // hours from start of that day
    bool is_bookmarked;
    int visit_count;
  };

  // 30+ sample entries spanning multiple time periods.
  SampleEntry samples[] = {
      // Today entries (8 entries)
      {"h001", "Astra Browser Design Doc - Google Docs",
       "https://docs.google.com/document/d/astra-design/edit",
       "docs.google.com", "Work", "Product", 0, 9, true, 12},
      {"h002", "GitHub - astra-browser/astra: Astra Browser",
       "https://github.com/astra-browser/astra", "github.com", "Work",
       "Engineering", 0, 10, false, 8},
      {"h003", "Chromium Docs - Architecture Overview",
       "https://www.chromium.org/developers/design-documents/multi-process-architecture",
       "www.chromium.org", "Work", "Engineering", 0, 11, true, 5},
      {"h004", "Gmail - Inbox",
       "https://mail.google.com/mail/u/0/#inbox", "mail.google.com", "Work",
       "Communication", 0, 8, false, 25},
      {"h005", "YouTube - Building a Chromium-based browser",
       "https://www.youtube.com/watch?v=abc123", "www.youtube.com",
       "Entertainment", "Personal", 0, 14, false, 3},
      {"h006", "Stack Overflow - C++ smart pointers best practices",
       "https://stackoverflow.com/questions/123456/smart-pointers",
       "stackoverflow.com", "Work", "Engineering", 0, 13, true, 7},
      {"h007", "Figma - Astra UI Mockups",
       "https://www.figma.com/file/astra-ui-mockups", "www.figma.com", "Work",
       "Design", 0, 15, false, 4},
      {"h008", "Twitter / X - Tech news feed",
       "https://twitter.com/home", "twitter.com", "Social", "Personal", 0, 7,
       false, 15},

      // Yesterday entries (7 entries)
      {"h009", "Notion - Sprint Planning Board",
       "https://www.notion.so/astra/Sprint-24-Planning", "www.notion.so",
       "Work", "Product", 1, 10, true, 9},
      {"h010", "Reddit - r/programming - Daily discussion",
       "https://www.reddit.com/r/programming/comments/daily", "www.reddit.com",
       "Social", "Personal", 1, 8, false, 11},
      {"h011", "MDN Web Docs - CSS Grid Layout",
       "https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_Grid_Layout",
       "developer.mozilla.org", "Work", "Engineering", 1, 14, true, 6},
      {"h012", "Amazon - Wireless headphones",
       "https://www.amazon.com/s?k=wireless+headphones", "www.amazon.com",
       "Shopping", "Personal", 1, 16, false, 2},
      {"h013", "Slack - Astra team channel",
       "https://app.slack.com/client/T0ABC/astra-team", "app.slack.com",
       "Work", "Communication", 1, 9, false, 30},
      {"h014", "Netflix - Continue watching",
       "https://www.netflix.com/browse", "www.netflix.com", "Entertainment",
       "Personal", 1, 20, false, 4},
      {"h015", "Wikipedia - Chromium (web browser)",
       "https://en.wikipedia.org/wiki/Chromium_(web_browser)",
       "en.wikipedia.org", "News", "Research", 1, 11, false, 2},

      // Last 7 days entries (7 entries)
      {"h016", "Google Calendar - Weekly view",
       "https://calendar.google.com/calendar/u/0/r/week", "calendar.google.com",
       "Work", "Productivity", 2, 9, false, 20},
      {"h017", "Medium - The future of browser architecture",
       "https://medium.com/@author/future-of-browsers", "medium.com", "News",
       "Research", 2, 15, true, 1},
      {"h018", "Spotify Web Player - Work playlist",
       "https://open.spotify.com/playlist/work", "open.spotify.com",
       "Entertainment", "Personal", 2, 10, false, 18},
      {"h019", "Jira - Backlog view",
       "https://astra.atlassian.net/jira/software/projects/DEV/boards/1",
       "astra.atlassian.net", "Work", "Engineering", 3, 11, false, 14},
      {"h020", "Hacker News - Front page",
       "https://news.ycombinator.com/", "news.ycombinator.com", "News",
       "Research", 3, 8, false, 6},
      {"h021", "GitHub Copilot - Chat",
       "https://github.com/copilot", "github.com", "Work", "Engineering", 4,
       13, true, 10},
      {"h022", "Dribbble - Browser UI inspiration",
       "https://dribbble.com/search/browser-ui", "dribbble.com", "Work",
       "Design", 5, 16, false, 3},

      // Last 30 days / older entries (8 entries)
      {"h023", "LeetCode - Problem of the day",
       "https://leetcode.com/problems/two-sum", "leetcode.com", "Work",
       "Engineering", 8, 19, false, 15},
      {"h024", "Google Drive - Project files",
       "https://drive.google.com/drive/u/0/my-drive", "drive.google.com",
       "Work", "Productivity", 10, 10, false, 8},
      {"h025", "eBay - Vintage camera collection",
       "https://www.ebay.com/sch/vintage-cameras", "www.ebay.com", "Shopping",
       "Personal", 12, 14, false, 5},
      {"h026", "The Verge - Tech news",
       "https://www.theverge.com/", "www.theverge.com", "News", "Research", 14,
       7, false, 22},
      {"h027", "LinkedIn - Network feed",
       "https://www.linkedin.com/feed/", "www.linkedin.com", "Social",
       "Professional", 15, 9, true, 12},
      {"h028", "Coursera - Advanced C++ course",
       "https://www.coursera.org/learn/advanced-cpp", "www.coursera.org",
       "Work", "Learning", 20, 18, false, 7},
      {"h029", "Airbnb - Tokyo accommodations",
       "https://www.airbnb.com/s/Tokyo--Japan", "www.airbnb.com", "Shopping",
       "Travel", 25, 12, false, 3},
      {"h030", "CNN - World news",
       "https://www.cnn.com/world", "www.cnn.com", "News", "Personal", 28, 8,
       false, 10},
      {"h031", "Bloomberg - Markets",
       "https://www.bloomberg.com/markets", "www.bloomberg.com", "News",
       "Research", 18, 6, false, 4},
  };

  for (const auto& sample : samples) {
    AstraHistoryEntry entry;
    entry.id = sample.id;
    entry.title = base::UTF8ToUTF16(sample.title);
    entry.url = sample.url;
    entry.host = sample.host;
    entry.category = sample.category;
    entry.workspace = sample.workspace;
    entry.visit_count = sample.visit_count;
    entry.is_bookmarked = sample.is_bookmarked;
    entry.favicon_url = std::string("https://") + sample.host + "/favicon.ico";

    base::Time day_start = today_start - base::Days(sample.days_ago);
    entry.visit_time = day_start + base::Hours(sample.hours_offset);

    all_entries_.push_back(std::move(entry));
  }

  // Sort all entries by visit time descending (newest first).
  std::sort(all_entries_.begin(), all_entries_.end(),
            [](const AstraHistoryEntry& a, const AstraHistoryEntry& b) {
              return a.visit_time > b.visit_time;
            });

  ApplyFilters();
  NotifyHistoryChanged();
}

// -- State ------------------------------------------------------------------

void AstraHistoryPageModel::SetLoading(bool loading) {
  loading_ = loading;
  // TODO(astra): Notify observers of loading state change when a
  // dedicated notification method is added to AstraHistoryPageObserver.
}

// -- Private methods --------------------------------------------------------

void AstraHistoryPageModel::NotifyHistoryChanged() {
  for (AstraHistoryPageObserver& observer : observers_) {
    observer.OnHistoryChanged(this);
  }
}

void AstraHistoryPageModel::NotifyFilterChanged() {
  for (AstraHistoryPageObserver& observer : observers_) {
    observer.OnFilterChanged(this, filter_);
  }
}

void AstraHistoryPageModel::NotifySearchChanged() {
  for (AstraHistoryPageObserver& observer : observers_) {
    observer.OnSearchChanged(this, search_query_);
  }
}

void AstraHistoryPageModel::ApplyFilters() {
  std::vector<AstraHistoryEntry> filtered;

  for (const auto& entry : all_entries_) {
    if (!MatchesFilter(entry)) {
      continue;
    }
    if (!MatchesSearch(entry)) {
      continue;
    }
    if (!MatchesCategory(entry)) {
      continue;
    }
    filtered.push_back(entry);
  }

  filtered_days_ = GroupEntriesByDay(filtered);
}

std::vector<AstraHistoryDay> AstraHistoryPageModel::GroupEntriesByDay(
    const std::vector<AstraHistoryEntry>& entries) const {
  std::vector<AstraHistoryDay> days;
  base::Time now = base::Time::Now();

  if (entries.empty()) {
    return days;
  }

  base::Time current_day_start;
  AstraHistoryDay* current_day = nullptr;

  for (const auto& entry : entries) {
    base::Time day_start = StartOfDay(entry.visit_time);

    if (!current_day || day_start != current_day_start) {
      days.emplace_back();
      current_day = &days.back();
      current_day->date = day_start;
      current_day->date_label = GetDateLabel(day_start, now);
      current_day_start = day_start;
    }

    current_day->entries.push_back(entry);
    current_day->total_visits += entry.visit_count;
  }

  return days;
}

bool AstraHistoryPageModel::MatchesSearch(
    const AstraHistoryEntry& entry) const {
  if (search_query_.empty()) {
    return true;
  }

  if (CaseInsensitiveContains(entry.title, search_query_)) {
    return true;
  }

  std::u16string url_u16 = base::UTF8ToUTF16(entry.url);
  if (CaseInsensitiveContains(url_u16, search_query_)) {
    return true;
  }

  std::u16string host_u16 = base::UTF8ToUTF16(entry.host);
  if (CaseInsensitiveContains(host_u16, search_query_)) {
    return true;
  }

  return false;
}

bool AstraHistoryPageModel::MatchesFilter(
    const AstraHistoryEntry& entry) const {
  if (filter_ == AstraHistoryFilter::kAll) {
    return true;
  }

  base::Time now = base::Time::Now();
  base::Time range_start;
  base::Time range_end;
  GetFilterTimeRange(filter_, now, &range_start, &range_end);

  bool after_start = range_start.is_null() || entry.visit_time >= range_start;
  bool before_end = range_end.is_null() || entry.visit_time < range_end;

  return after_start && before_end;
}

bool AstraHistoryPageModel::MatchesCategory(
    const AstraHistoryEntry& entry) const {
  if (category_filter_.empty()) {
    return true;
  }
  return entry.category == category_filter_;
}

}  // namespace astra
