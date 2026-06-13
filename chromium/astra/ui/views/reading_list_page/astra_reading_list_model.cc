// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/reading_list_page/astra_reading_list_model.h"

#include <algorithm>
#include <set>

#include "base/i18n/case_conversion.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"

namespace astra {

namespace {

// Case-insensitive substring match.
bool CaseInsensitiveContains(const std::u16string& haystack,
                             const std::u16string& needle) {
  if (needle.empty()) {
    return true;
  }
  std::u16string haystack_lower = base::i18n::ToLower(haystack);
  std::u16string needle_lower = base::i18n::ToLower(needle);
  return haystack_lower.find(needle_lower) != std::u16string::npos;
}

// Extract site name from a URL (simple heuristic).
std::string GetSiteNameFromUrl(const std::string& url) {
  // Find the host part.
  size_t pos = url.find("://");
  if (pos == std::string::npos) {
    return url;
  }
  std::string host = url.substr(pos + 3);
  size_t slash = host.find('/');
  if (slash != std::string::npos) {
    host = host.substr(0, slash);
  }
  // Strip "www." prefix.
  if (host.substr(0, 4) == "www.") {
    host = host.substr(4);
  }
  return host;
}

}  // namespace

// ===========================================================================
// AstraReadingListModel
// ===========================================================================

AstraReadingListModel::AstraReadingListModel() = default;

AstraReadingListModel::~AstraReadingListModel() {
  for (AstraReadingListObserver& observer : observers_) {
    observer.OnReadingListModelShutdown();
  }
}

// -- Observer management ----------------------------------------------------

void AstraReadingListModel::AddObserver(AstraReadingListObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraReadingListModel::RemoveObserver(AstraReadingListObserver* observer) {
  observers_.RemoveObserver(observer);
}

// -- Entry access -----------------------------------------------------------

const std::vector<AstraReadingListEntry>& AstraReadingListModel::GetEntries()
    const {
  return all_entries_;
}

const AstraReadingListEntry* AstraReadingListModel::GetEntry(
    const std::string& id) const {
  for (const auto& entry : all_entries_) {
    if (entry.id == id) {
      return &entry;
    }
  }
  return nullptr;
}

size_t AstraReadingListModel::GetCount() const {
  return all_entries_.size();
}

size_t AstraReadingListModel::GetUnreadCount() const {
  size_t count = 0;
  for (const auto& entry : all_entries_) {
    if (!entry.is_read) {
      ++count;
    }
  }
  return count;
}

size_t AstraReadingListModel::GetFavoritesCount() const {
  size_t count = 0;
  for (const auto& entry : all_entries_) {
    if (entry.is_favorited) {
      ++count;
    }
  }
  return count;
}

// -- Search -----------------------------------------------------------------

void AstraReadingListModel::SetSearchQuery(const std::u16string& query) {
  if (search_query_ == query) {
    return;
  }
  search_query_ = query;
  filtered_dirty_ = true;
  NotifySearchChanged();
  NotifyReadingListChanged();
}

// -- Filtering --------------------------------------------------------------

void AstraReadingListModel::SetFilter(AstraReadingListFilter filter) {
  if (filter_ == filter) {
    return;
  }
  filter_ = filter;
  filtered_dirty_ = true;
  NotifyFilterChanged();
  NotifyReadingListChanged();
}

void AstraReadingListModel::SetCategoryFilter(const std::string& category) {
  if (category_filter_ == category) {
    return;
  }
  category_filter_ = category;
  filtered_dirty_ = true;
  NotifyReadingListChanged();
}

void AstraReadingListModel::SetFolderFilter(const std::string& folder) {
  if (folder_filter_ == folder) {
    return;
  }
  folder_filter_ = folder;
  filtered_dirty_ = true;
  NotifyReadingListChanged();
}

// -- Sorting ----------------------------------------------------------------

void AstraReadingListModel::SetSortType(AstraReadingListSortType sort_type) {
  if (sort_type_ == sort_type) {
    return;
  }
  sort_type_ = sort_type;
  filtered_dirty_ = true;
  NotifyReadingListChanged();
}

// -- Categories -------------------------------------------------------------

std::vector<std::string> AstraReadingListModel::GetCategories() const {
  std::set<std::string> unique_categories;
  for (const auto& entry : all_entries_) {
    if (!entry.category.empty()) {
      unique_categories.insert(entry.category);
    }
  }
  return std::vector<std::string>(unique_categories.begin(),
                                  unique_categories.end());
}

// -- Folders ----------------------------------------------------------------

const std::vector<AstraReadingListFolder>& AstraReadingListModel::GetFolders()
    const {
  return folders_;
}

AstraReadingListFolder* AstraReadingListModel::FindFolder(
    const std::string& id) {
  for (auto& folder : folders_) {
    if (folder.id == id) {
      return &folder;
    }
  }
  return nullptr;
}

const AstraReadingListFolder* AstraReadingListModel::FindFolder(
    const std::string& id) const {
  for (const auto& folder : folders_) {
    if (folder.id == id) {
      return &folder;
    }
  }
  return nullptr;
}

void AstraReadingListModel::UpdateFolderEntryCounts() {
  for (auto& folder : folders_) {
    int count = 0;
    for (const auto& entry : all_entries_) {
      if (entry.folder == folder.name) {
        ++count;
      }
    }
    folder.entry_count = count;
  }
}

// -- Entry manipulation -----------------------------------------------------

std::string AstraReadingListModel::AddEntry(const std::u16string& title,
                                            const std::string& url,
                                            const std::u16string& preview_text,
                                            int estimated_read_time,
                                            const std::string& category,
                                            const std::string& folder) {
  AstraReadingListEntry entry;
  entry.id = GenerateId();
  entry.title = title;
  entry.url = url;
  entry.preview_text = preview_text;
  entry.estimated_read_time_minutes = estimated_read_time;
  entry.category = category;
  entry.folder = folder;
  entry.date_added = base::Time::Now();
  entry.date_last_read = base::Time();
  entry.is_read = false;
  entry.is_favorited = false;
  entry.site_name = GetSiteNameFromUrl(url);
  entry.favicon_url = "https://" + entry.site_name + "/favicon.ico";

  all_entries_.push_back(std::move(entry));
  filtered_dirty_ = true;
  UpdateFolderEntryCounts();
  NotifyEntryAdded(all_entries_.back().id);
  NotifyReadingListChanged();
  return all_entries_.back().id;
}

void AstraReadingListModel::RemoveEntry(const std::string& id) {
  auto it = std::remove_if(all_entries_.begin(), all_entries_.end(),
                           [&id](const AstraReadingListEntry& e) {
                             return e.id == id;
                           });
  if (it == all_entries_.end()) {
    return;
  }
  all_entries_.erase(it, all_entries_.end());
  filtered_dirty_ = true;
  UpdateFolderEntryCounts();
  NotifyEntryRemoved(id);
  NotifyReadingListChanged();
}

void AstraReadingListModel::MarkAsRead(const std::string& id) {
  auto* entry = const_cast<AstraReadingListEntry*>(GetEntry(id));
  if (!entry || entry->is_read) {
    return;
  }
  entry->is_read = true;
  entry->date_last_read = base::Time::Now();
  filtered_dirty_ = true;
  NotifyEntryUpdated(id);
  NotifyReadingListChanged();
}

void AstraReadingListModel::MarkAsUnread(const std::string& id) {
  auto* entry = const_cast<AstraReadingListEntry*>(GetEntry(id));
  if (!entry || !entry->is_read) {
    return;
  }
  entry->is_read = false;
  filtered_dirty_ = true;
  NotifyEntryUpdated(id);
  NotifyReadingListChanged();
}

void AstraReadingListModel::ToggleFavorite(const std::string& id) {
  auto* entry = const_cast<AstraReadingListEntry*>(GetEntry(id));
  if (!entry) {
    return;
  }
  entry->is_favorited = !entry->is_favorited;
  filtered_dirty_ = true;
  NotifyEntryUpdated(id);
  NotifyReadingListChanged();
}

void AstraReadingListModel::MoveEntryToFolder(const std::string& id,
                                              const std::string& folder) {
  auto* entry = const_cast<AstraReadingListEntry*>(GetEntry(id));
  if (!entry || entry->folder == folder) {
    return;
  }
  entry->folder = folder;
  filtered_dirty_ = true;
  UpdateFolderEntryCounts();
  NotifyEntryUpdated(id);
  NotifyReadingListChanged();
}

// -- Folder manipulation ----------------------------------------------------

std::string AstraReadingListModel::AddFolder(const std::u16string& name) {
  AstraReadingListFolder folder;
  folder.id = GenerateId();
  folder.name = name;
  folder.entry_count = 0;

  folders_.push_back(std::move(folder));
  NotifyFolderAdded(folders_.back().id);
  NotifyReadingListChanged();
  return folders_.back().id;
}

void AstraReadingListModel::RemoveFolder(const std::string& id) {
  auto it =
      std::remove_if(folders_.begin(), folders_.end(),
                     [&id](const AstraReadingListFolder& f) {
                       return f.id == id;
                     });
  if (it == folders_.end()) {
    return;
  }

  std::u16string folder_name = it->name;
  folders_.erase(it, folders_.end());

  // Move entries from the removed folder to "Unsorted".
  for (auto& entry : all_entries_) {
    if (base::UTF16ToUTF8(entry.folder.empty() ? u"" : base::UTF8ToUTF16(entry.folder)) ==
        base::UTF16ToUTF8(folder_name)) {
      entry.folder = "Unsorted";
    }
  }

  if (base::UTF8ToUTF16(folder_filter_) == folder_name) {
    folder_filter_.clear();
  }

  UpdateFolderEntryCounts();
  filtered_dirty_ = true;
  NotifyFolderRemoved(id);
  NotifyReadingListChanged();
}

void AstraReadingListModel::RenameFolder(const std::string& id,
                                         const std::u16string& name) {
  auto* folder = FindFolder(id);
  if (!folder || folder->name == name) {
    return;
  }
  std::u16string old_name = folder->name;
  folder->name = name;

  // Update entries in this folder.
  for (auto& entry : all_entries_) {
    if (base::UTF8ToUTF16(entry.folder) == old_name) {
      entry.folder = base::UTF16ToUTF8(name);
    }
  }

  filtered_dirty_ = true;
  NotifyReadingListChanged();
}

// -- Sample data ------------------------------------------------------------

void AstraReadingListModel::PopulateSampleEntries() {
  all_entries_.clear();
  folders_.clear();
  next_id_ = 1;
  filtered_dirty_ = true;

  // Create sample folders.
  AddFolder(u"Must Read");
  AddFolder(u"Later");
  AddFolder(u"Tech Deep Dives");
  AddFolder(u"Design Inspiration");

  base::Time now = base::Time::Now();

  struct SampleEntry {
    const char* title;
    const char* url;
    const char* preview_text;
    int read_time;
    const char* category;
    const char* folder;
    const char* site_name;
    int days_ago;
    bool is_read;
    bool is_favorited;
  };

  SampleEntry samples[] = {
      // Technology category
      {"The Future of Web Browsers: What's Next in 2026",
       "https://techcrunch.com/future-of-browsers-2026",
       "An in-depth look at emerging browser technologies including AI-powered "
       "assistance, privacy enhancements, and new rendering capabilities.",
       12, "Technology", "Must Read", "TechCrunch", 1, false, true},
      {"Understanding Chromium's Multi-Process Architecture",
       "https://blog.chromium.org/multi-process-architecture",
       "A deep dive into how Chromium manages processes for tabs, extensions, "
       "and GPU acceleration, with diagrams and performance analysis.",
       25, "Technology", "Tech Deep Dives", "Chromium Blog", 3, true, true},
      {"Rust in Chromium: Memory Safety for the Modern Web",
       "https://security.googleblog.com/rust-in-chromium",
       "Google's plan to incrementally rewrite parts of Chromium in Rust for "
       "improved memory safety and reduced attack surface.",
       18, "Technology", "Tech Deep Dives", "Google Security Blog", 5, false,
       false},
      {"WebGPU: The Future of Graphics on the Web",
       "https://developer.mozilla.org/docs/WebGPU",
       "Everything you need to know about WebGPU, the next-generation graphics "
       "API that promises near-native performance in the browser.",
       20, "Technology", "Later", "MDN Web Docs", 7, false, false},
      {"The State of JavaScript Frameworks in 2026",
       "https://medium.com/javascript-frameworks-2026",
       "A comprehensive comparison of React, Vue, Svelte, Angular, and emerging "
       "frameworks, with benchmarks and real-world usage data.",
       15, "Technology", "Later", "Medium", 10, true, false},
      {"Progressive Web Apps: The Quiet Revolution",
       "https://web.dev/progressive-web-apps",
       "How PWAs are bridging the gap between web and native apps, with "
       "success stories from major companies.",
       10, "Technology", "Must Read", "web.dev", 14, false, true},

      // News category
      {"Global Tech Regulations: What You Need to Know",
       "https://www.nytimes.com/tech-regulations-2026",
       "A comprehensive overview of new technology regulations around the world, "
       "from EU's AI Act to new US privacy laws.",
       8, "News", "Later", "New York Times", 2, false, false},
      {"AI Breakthrough: New Model Achieves Human-Level Reasoning",
       "https://www.theverge.com/ai-breakthrough-2026",
       "Researchers announce a major AI milestone, but experts caution about "
       "real-world implications and safety considerations.",
       6, "News", "Must Read", "The Verge", 1, false, true},
      {"Climate Tech Startups Raise Record Funding",
       "https://techcrunch.com/climate-tech-funding",
       "Climate technology startups have raised over $50 billion this year, "
       "driven by increasing urgency around climate change solutions.",
       7, "News", "Later", "TechCrunch", 4, true, false},
      {"New Space Race: Private Companies Push Boundaries",
       "https://www.bloomberg.com/space-race-2026",
       "An analysis of the competitive landscape in private space exploration, "
       "with key players and future timelines.",
       9, "News", "Later", "Bloomberg", 6, false, false},

      // Design category
      {"The Principles of Effective UI Animation",
       "https://dribbble.com/stories/ui-animation-principles",
       "Learn how to use animation effectively in your designs to guide users, "
       "provide feedback, and create delightful experiences.",
       11, "Design", "Design Inspiration", "Dribbble", 8, false, true},
      {"Design Systems at Scale: Lessons from Top Companies",
       "https://medium.com/design-systems-at-scale",
       "How leading tech companies build and maintain design systems that scale "
       "across hundreds of products and thousands of designers.",
       16, "Design", "Design Inspiration", "Medium", 12, true, true},
      {"Accessibility First: Designing for Everyone",
       "https://www.smashingmagazine.com/accessibility-first-design",
       "A practical guide to building accessible interfaces from the start, "
       "covering WCAG guidelines, common pitfalls, and best practices.",
       14, "Design", "Must Read", "Smashing Magazine", 9, false, false},
      {"Color Theory for Digital Products",
       "https://www.figma.com/blog/color-theory-digital-products",
       "Understanding how color works in digital interfaces, from contrast "
       "ratios to emotional impact and brand consistency.",
       13, "Design", "Design Inspiration", "Figma Blog", 11, false, false},
      {"Microinteractions: Small Details, Big Impact",
       "https://uxdesign.cc/microinteractions-big-impact",
       "Exploring how tiny interactive details can transform the user experience "
       "and make products feel more human and engaging.",
       8, "Design", "Later", "UX Collective", 15, true, false},

      // Business category
      {"Remote Work is Here to Stay: Data from 2026",
       "https://hbr.org/remote-work-2026-data",
       "New research shows how remote and hybrid work models have evolved, "
       "with surprising findings about productivity and employee satisfaction.",
       10, "Business", "Must Read", "Harvard Business Review", 3, true, true},
      {"The Economics of Open Source Software",
       "https://www.economist.com/open-source-economics",
       "An analysis of how open source projects generate revenue, sustain "
       "development, and shape the software industry.",
       12, "Business", "Later", "The Economist", 16, false, false},
      {"Startup Valuations: What's Real and What's Hype?",
       "https://techcrunch.com/startup-valuations-2026",
       "A critical look at how startups are valued in the current market, with "
       "insights from investors and entrepreneurs.",
       9, "Business", "Later", "TechCrunch", 5, false, false},
      {"Building a Company Culture That Scales",
       "https://medium.com/building-culture-that-scales",
       "Practical advice from founders who have grown companies from 10 to "
       "10,000 employees while preserving their core culture.",
       17, "Business", "Must Read", "Medium", 18, false, true},

      // Science category
      {"Quantum Computing Reaches New Milestone",
       "https://www.nature.com/articles/quantum-milestone-2026",
       "Scientists achieve quantum advantage in practical problem solving, "
       "marking a significant step forward for the field.",
       15, "Science", "Later", "Nature", 7, false, false},
      {"The Human Brain: New Discoveries in Neuroscience",
       "https://www.science.org/news/brain-neuroscience-discoveries",
       "Recent breakthroughs in understanding how the brain processes "
       "information, with implications for AI and medicine.",
       13, "Science", "Later", "Science", 10, true, false},
      {"Climate Change: Latest Research Findings",
       "https://www.ipcc.ch/reports/2026-update",
       "The latest IPCC report summarizes current understanding of climate "
       "change, impacts, and mitigation strategies.",
       20, "Science", "Must Read", "IPCC", 2, false, true},
      {"CRISPR Gene Editing: From Lab to Clinic",
       "https://www.nejm.org/crispr-lab-to-clinic",
       "How CRISPR technology is moving from research labs to patient "
       "treatments, with successes, challenges, and ethical considerations.",
       18, "Science", "Tech Deep Dives", "New England Journal of Medicine",
       14, false, false},
  };

  for (const auto& sample : samples) {
    AstraReadingListEntry entry;
    entry.id = GenerateId();
    entry.title = base::UTF8ToUTF16(sample.title);
    entry.url = sample.url;
    entry.preview_text = base::UTF8ToUTF16(sample.preview_text);
    entry.estimated_read_time_minutes = sample.read_time;
    entry.category = sample.category;
    entry.folder = sample.folder;
    entry.site_name = sample.site_name;
    entry.date_added = now - base::Days(sample.days_ago);
    entry.date_last_read = sample.is_read ? entry.date_added : base::Time();
    entry.is_read = sample.is_read;
    entry.is_favorited = sample.is_favorited;
    entry.favicon_url = "https://" + entry.site_name + "/favicon.ico";

    all_entries_.push_back(std::move(entry));
  }

  UpdateFolderEntryCounts();
  ApplyFilters();
  NotifyReadingListChanged();
}

// -- State ------------------------------------------------------------------

void AstraReadingListModel::SetLoading(bool loading) {
  loading_ = loading;
  // TODO(astra): Add OnLoadingChanged to AstraReadingListObserver when
  // a loading state indicator is needed in the UI.
}

std::vector<AstraReadingListEntry> AstraReadingListModel::GetFilteredEntries()
    const {
  if (filtered_dirty_) {
    ApplyFilters();
    filtered_dirty_ = false;
  }
  return filtered_entries_;
}

// -- Private methods --------------------------------------------------------

bool AstraReadingListModel::MatchesSearch(
    const AstraReadingListEntry& entry) const {
  if (search_query_.empty()) {
    return true;
  }

  if (CaseInsensitiveContains(entry.title, search_query_)) {
    return true;
  }

  if (CaseInsensitiveContains(entry.preview_text, search_query_)) {
    return true;
  }

  std::u16string url_u16 = base::UTF8ToUTF16(entry.url);
  if (CaseInsensitiveContains(url_u16, search_query_)) {
    return true;
  }

  std::u16string site_u16 = base::UTF8ToUTF16(entry.site_name);
  if (CaseInsensitiveContains(site_u16, search_query_)) {
    return true;
  }

  return false;
}

bool AstraReadingListModel::MatchesFilter(
    const AstraReadingListEntry& entry) const {
  switch (filter_) {
    case AstraReadingListFilter::kAll:
      return true;
    case AstraReadingListFilter::kUnread:
      return !entry.is_read;
    case AstraReadingListFilter::kRead:
      return entry.is_read;
    case AstraReadingListFilter::kFavorites:
      return entry.is_favorited;
  }
  return true;
}

bool AstraReadingListModel::MatchesCategory(
    const AstraReadingListEntry& entry) const {
  if (category_filter_.empty()) {
    return true;
  }
  return entry.category == category_filter_;
}

bool AstraReadingListModel::MatchesFolder(
    const AstraReadingListEntry& entry) const {
  if (folder_filter_.empty()) {
    return true;
  }
  return entry.folder == folder_filter_;
}

void AstraReadingListModel::SortEntries(
    std::vector<AstraReadingListEntry>& entries) const {
  switch (sort_type_) {
    case AstraReadingListSortType::kNewestFirst:
      std::sort(entries.begin(), entries.end(),
                [](const AstraReadingListEntry& a,
                   const AstraReadingListEntry& b) {
                  return a.date_added > b.date_added;
                });
      break;
    case AstraReadingListSortType::kOldestFirst:
      std::sort(entries.begin(), entries.end(),
                [](const AstraReadingListEntry& a,
                   const AstraReadingListEntry& b) {
                  return a.date_added < b.date_added;
                });
      break;
    case AstraReadingListSortType::kAlphabetical:
      std::sort(entries.begin(), entries.end(),
                [](const AstraReadingListEntry& a,
                   const AstraReadingListEntry& b) {
                  return a.title < b.title;
                });
      break;
    case AstraReadingListSortType::kReadTime:
      std::sort(entries.begin(), entries.end(),
                [](const AstraReadingListEntry& a,
                   const AstraReadingListEntry& b) {
                  return a.estimated_read_time_minutes <
                         b.estimated_read_time_minutes;
                });
      break;
  }
}

void AstraReadingListModel::ApplyFilters() const {
  std::vector<AstraReadingListEntry> filtered;

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
    if (!MatchesFolder(entry)) {
      continue;
    }
    filtered.push_back(entry);
  }

  SortEntries(filtered);
  filtered_entries_ = std::move(filtered);
}

std::string AstraReadingListModel::GenerateId() const {
  return "rl_" + base::NumberToString(next_id_++);
}

// -- Notification helpers ---------------------------------------------------

void AstraReadingListModel::NotifyReadingListChanged() {
  for (AstraReadingListObserver& observer : observers_) {
    observer.OnReadingListChanged();
  }
}

void AstraReadingListModel::NotifyEntryAdded(const std::string& id) {
  for (AstraReadingListObserver& observer : observers_) {
    observer.OnEntryAdded(id);
  }
}

void AstraReadingListModel::NotifyEntryRemoved(const std::string& id) {
  for (AstraReadingListObserver& observer : observers_) {
    observer.OnEntryRemoved(id);
  }
}

void AstraReadingListModel::NotifyEntryUpdated(const std::string& id) {
  for (AstraReadingListObserver& observer : observers_) {
    observer.OnEntryUpdated(id);
  }
}

void AstraReadingListModel::NotifyFolderAdded(const std::string& id) {
  for (AstraReadingListObserver& observer : observers_) {
    observer.OnFolderAdded(id);
  }
}

void AstraReadingListModel::NotifyFolderRemoved(const std::string& id) {
  for (AstraReadingListObserver& observer : observers_) {
    observer.OnFolderRemoved(id);
  }
}

void AstraReadingListModel::NotifySearchChanged() {
  for (AstraReadingListObserver& observer : observers_) {
    observer.OnSearchChanged(search_query_);
  }
}

void AstraReadingListModel::NotifyFilterChanged() {
  for (AstraReadingListObserver& observer : observers_) {
    observer.OnFilterChanged();
  }
}

}  // namespace astra
