// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/downloads_page/astra_downloads_page_model.h"

#include <algorithm>
#include <set>

#include "base/i18n/case_conversion.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"

namespace astra {

namespace {

// Comparator for sorting downloads by start_time descending (newest first).
bool CompareByStartTimeDesc(const AstraDownloadEntry& a,
                            const AstraDownloadEntry& b) {
  return a.start_time > b.start_time;
}

// Determine category from mime type and file extension.
// TODO(astra): Use Chromium's download_util::DetermineDownloadCategory()
// or similar utility once wired up.
std::string DetermineCategory(const std::string& mime_type,
                              const std::u16string& file_name) {
  std::string lower_mime = base::ToLowerASCII(mime_type);

  if (base::StartsWith(lower_mime, "image/"))
    return "Images";
  if (base::StartsWith(lower_mime, "video/"))
    return "Videos";
  if (base::StartsWith(lower_mime, "audio/"))
    return "Audio";
  if (base::StartsWith(lower_mime, "application/pdf") ||
      base::StartsWith(lower_mime, "text/"))
    return "Documents";

  // Check by extension for common archive formats.
  std::u16 lower_name = base::i18n::ToLower(file_name);
  const std::vector<std::u16string> archive_exts = {
      u".zip", u".tar", u".gz", u".bz2", u".7z", u".rar", u".xz"};
  for (const auto& ext : archive_exts) {
    if (base::EndsWith(lower_name, ext))
      return "Archives";
  }

  // Check document extensions.
  const std::vector<std::u16string> doc_exts = {
      u".pdf",  u".doc",  u".docx", u".xls",  u".xlsx",
      u".ppt",  u".pptx", u".txt",  u".rtf",  u".odt",
      u".ods",  u".csv",  u".json", u".xml",  u".html"};
  for (const auto& ext : doc_exts) {
    if (base::EndsWith(lower_name, ext))
      return "Documents";
  }

  return "Other";
}

// Generate a sample download entry.
AstraDownloadEntry MakeSampleEntry(const std::string& id,
                                   const std::u16string& file_name,
                                   const std::string& url,
                                   const std::string& file_path,
                                   int64_t total_bytes,
                                   int64_t received_bytes,
                                   AstraDownloadPageState state,
                                   AstraDownloadDangerType danger_type,
                                   base::Time start_time,
                                   base::Time end_time,
                                   const std::string& mime_type,
                                   bool is_openable,
                                   int opener_tab_id,
                                   const std::string& workspace) {
  AstraDownloadEntry entry;
  entry.id = id;
  entry.file_name = file_name;
  entry.url = url;
  entry.file_path = file_path;
  entry.total_bytes = total_bytes;
  entry.received_bytes = received_bytes;
  entry.state = state;
  entry.danger_type = danger_type;
  entry.start_time = start_time;
  entry.end_time = end_time;
  entry.mime_type = mime_type;
  entry.is_openable = is_openable;
  entry.opener_tab_id = opener_tab_id;
  entry.workspace = workspace;
  entry.category = DetermineCategory(mime_type, file_name);
  return entry;
}

}  // namespace

// ===========================================================================
// AstraDownloadsPageModel
// ===========================================================================

AstraDownloadsPageModel::AstraDownloadsPageModel() = default;

AstraDownloadsPageModel::~AstraDownloadsPageModel() {
  NotifyShutdown();
}

// -- Observer management ----------------------------------------------------

void AstraDownloadsPageModel::AddObserver(
    AstraDownloadsPageObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraDownloadsPageModel::RemoveObserver(
    AstraDownloadsPageObserver* observer) {
  observers_.RemoveObserver(observer);
}

// -- Download data access ---------------------------------------------------

const std::vector<AstraDownloadEntry>& AstraDownloadsPageModel::GetDownloads()
    const {
  return filtered_downloads_;
}

const AstraDownloadEntry* AstraDownloadsPageModel::GetDownload(
    const std::string& id) const {
  for (const auto& entry : all_downloads_) {
    if (entry.id == id)
      return &entry;
  }
  return nullptr;
}

size_t AstraDownloadsPageModel::GetCount() const {
  return all_downloads_.size();
}

// -- Search -----------------------------------------------------------------

void AstraDownloadsPageModel::SetSearchQuery(const std::u16string& query) {
  if (search_query_ == query)
    return;
  search_query_ = query;
  ApplyFilters();
  NotifySearchChanged();
  NotifyDownloadsChanged();
}

// -- State filter -----------------------------------------------------------

void AstraDownloadsPageModel::SetFilter(AstraDownloadsPageFilter filter) {
  if (filter_ == filter)
    return;
  filter_ = filter;
  ApplyFilters();
  NotifyFilterChanged();
  NotifyDownloadsChanged();
}

std::vector<std::pair<AstraDownloadsPageFilter, std::u16string>>
AstraDownloadsPageModel::GetFilterOptions() const {
  return {
      {AstraDownloadsPageFilter::kAll, u"All"},
      {AstraDownloadsPageFilter::kInProgress, u"In progress"},
      {AstraDownloadsPageFilter::kCompleted, u"Completed"},
      {AstraDownloadsPageFilter::kCancelled, u"Cancelled"},
      {AstraDownloadsPageFilter::kInterrupted, u"Interrupted"},
  };
}

// -- Category filter --------------------------------------------------------

void AstraDownloadsPageModel::SetCategoryFilter(const std::string& category) {
  if (category_filter_ == category)
    return;
  category_filter_ = category;
  ApplyFilters();
  NotifyFilterChanged();
  NotifyDownloadsChanged();
}

std::vector<std::string> AstraDownloadsPageModel::GetCategories() const {
  std::set<std::string> categories;
  for (const auto& entry : all_downloads_) {
    if (!entry.category.empty())
      categories.insert(entry.category);
  }
  return std::vector<std::string>(categories.begin(), categories.end());
}

// -- Download actions -------------------------------------------------------

void AstraDownloadsPageModel::RemoveDownload(const std::string& id) {
  auto it = std::find_if(all_downloads_.begin(), all_downloads_.end(),
                         [&id](const AstraDownloadEntry& e) {
                           return e.id == id;
                         });
  if (it == all_downloads_.end())
    return;

  all_downloads_.erase(it);
  ApplyFilters();
  NotifyDownloadRemoved(id);
  NotifyDownloadsChanged();
}

void AstraDownloadsPageModel::ClearAllDownloads() {
  all_downloads_.clear();
  ApplyFilters();
  NotifyDownloadsChanged();
}

void AstraDownloadsPageModel::OpenDownload(const std::string& id) {
  // TODO(astra): Delegate to Chromium's DownloadItem::OpenDownload().
  // Chromium method: download::DownloadItem::OpenDownload()
}

void AstraDownloadsPageModel::ShowInFolder(const std::string& id) {
  // TODO(astra): Delegate to Chromium's DownloadItem::ShowDownloadInShell().
  // Chromium method: download::DownloadItem::ShowDownloadInShell()
}

void AstraDownloadsPageModel::PauseDownload(const std::string& id) {
  auto it = std::find_if(all_downloads_.begin(), all_downloads_.end(),
                         [&id](AstraDownloadEntry& e) {
                           return e.id == id;
                         });
  if (it == all_downloads_.end())
    return;

  if (it->state == AstraDownloadPageState::kInProgress) {
    it->state = AstraDownloadPageState::kPaused;
    ApplyFilters();
    NotifyDownloadUpdated(id);
    NotifyDownloadsChanged();
  }
}

void AstraDownloadsPageModel::ResumeDownload(const std::string& id) {
  auto it = std::find_if(all_downloads_.begin(), all_downloads_.end(),
                         [&id](AstraDownloadEntry& e) {
                           return e.id == id;
                         });
  if (it == all_downloads_.end())
    return;

  if (it->state == AstraDownloadPageState::kPaused) {
    it->state = AstraDownloadPageState::kInProgress;
    ApplyFilters();
    NotifyDownloadUpdated(id);
    NotifyDownloadsChanged();
  }
}

void AstraDownloadsPageModel::CancelDownload(const std::string& id) {
  auto it = std::find_if(all_downloads_.begin(), all_downloads_.end(),
                         [&id](AstraDownloadEntry& e) {
                           return e.id == id;
                         });
  if (it == all_downloads_.end())
    return;

  if (it->state == AstraDownloadPageState::kInProgress ||
      it->state == AstraDownloadPageState::kPaused) {
    it->state = AstraDownloadPageState::kCancelled;
    it->end_time = base::Time::Now();
    ApplyFilters();
    NotifyDownloadUpdated(id);
    NotifyDownloadsChanged();
  }
}

void AstraDownloadsPageModel::RetryDownload(const std::string& id) {
  // TODO(astra): Delegate retry to Chromium's download system.
  // In this scaffold we just reset the entry to in-progress.
  auto it = std::find_if(all_downloads_.begin(), all_downloads_.end(),
                         [&id](AstraDownloadEntry& e) {
                           return e.id == id;
                         });
  if (it == all_downloads_.end())
    return;

  if (it->state == AstraDownloadPageState::kCancelled ||
      it->state == AstraDownloadPageState::kInterrupted) {
    it->state = AstraDownloadPageState::kInProgress;
    it->received_bytes = 0;
    it->start_time = base::Time::Now();
    it->end_time = base::Time();
    ApplyFilters();
    NotifyDownloadUpdated(id);
    NotifyDownloadsChanged();
  }
}

// -- Sample data ------------------------------------------------------------

void AstraDownloadsPageModel::PopulateSampleDownloads() {
  base::Time now = base::Time::Now();
  base::TimeDelta one_hour = base::Hours(1);
  base::TimeDelta one_day = base::Days(1);

  all_downloads_.clear();

  // Today — in-progress downloads
  all_downloads_.push_back(MakeSampleEntry(
      "dl_001", u"project_source.zip",
      "https://example.com/downloads/project_source.zip",
      "/Users/user/Downloads/project_source.zip",
      100 * 1024 * 1024,   // 100 MB total
      35 * 1024 * 1024,    // 35 MB received
      AstraDownloadPageState::kInProgress,
      AstraDownloadDangerType::kSafe,
      now - base::Minutes(5),
      base::Time(),
      "application/zip",
      false,
      1,
      "Work"));

  all_downloads_.push_back(MakeSampleEntry(
      "dl_002", u"presentation_final.pptx",
      "https://company.com/docs/presentation_final.pptx",
      "/Users/user/Downloads/presentation_final.pptx",
      8 * 1024 * 1024,     // 8 MB total
      8 * 1024 * 1024,     // complete
      AstraDownloadPageState::kComplete,
      AstraDownloadDangerType::kSafe,
      now - base::Minutes(30),
      now - base::Minutes(28),
      "application/vnd.openxmlformats-officedocument.presentationml.presentation",
      true,
      2,
      "Work"));

  all_downloads_.push_back(MakeSampleEntry(
      "dl_003", u"vacation_photo.jpg",
      "https://photos.example.com/vacation_photo.jpg",
      "/Users/user/Downloads/vacation_photo.jpg",
      4 * 1024 * 1024,     // 4 MB
      4 * 1024 * 1024,
      AstraDownloadPageState::kComplete,
      AstraDownloadDangerType::kSafe,
      now - base::Minutes(45),
      now - base::Minutes(44),
      "image/jpeg",
      true,
      3,
      "Personal"));

  all_downloads_.push_back(MakeSampleEntry(
      "dl_004", u"large_installer.dmg",
      "https://software.example.com/large_installer.dmg",
      "/Users/user/Downloads/large_installer.dmg",
      2500 * 1024 * 1024,  // 2.5 GB
      0,
      AstraDownloadPageState::kPaused,
      AstraDownloadDangerType::kUncommon,
      now - base::Minutes(10),
      base::Time(),
      "application/x-apple-diskimage",
      false,
      4,
      "Personal"));

  // Today — dangerous download
  all_downloads_.push_back(MakeSampleEntry(
      "dl_005", u"setup.exe",
      "https://suspicious.example.com/setup.exe",
      "/Users/user/Downloads/setup.exe",
      15 * 1024 * 1024,
      15 * 1024 * 1024,
      AstraDownloadPageState::kComplete,
      AstraDownloadDangerType::kDangerous,
      now - base::Minutes(15),
      now - base::Minutes(14),
      "application/exe",
      false,
      5,
      "Personal"));

  // Yesterday
  base::Time yesterday = now - one_day;

  all_downloads_.push_back(MakeSampleEntry(
      "dl_006", u"report_q2.pdf",
      "https://company.com/reports/report_q2.pdf",
      "/Users/user/Downloads/report_q2.pdf",
      2 * 1024 * 1024,
      2 * 1024 * 1024,
      AstraDownloadPageState::kComplete,
      AstraDownloadDangerType::kSafe,
      yesterday + base::Hours(14),
      yesterday + base::Hours(14) + base::Minutes(1),
      "application/pdf",
      true,
      6,
      "Work"));

  all_downloads_.push_back(MakeSampleEntry(
      "dl_007", u"soundtrack.mp3",
      "https://music.example.com/soundtrack.mp3",
      "/Users/user/Downloads/soundtrack.mp3",
      5 * 1024 * 1024,
      5 * 1024 * 1024,
      AstraDownloadPageState::kComplete,
      AstraDownloadDangerType::kSafe,
      yesterday + base::Hours(10),
      yesterday + base::Hours(10) + base::Seconds(30),
      "audio/mpeg",
      true,
      7,
      "Personal"));

  all_downloads_.push_back(MakeSampleEntry(
      "dl_008", u"movie_trailer.mp4",
      "https://movies.example.com/trailer.mp4",
      "/Users/user/Downloads/movie_trailer.mp4",
      150 * 1024 * 1024,
      150 * 1024 * 1024,
      AstraDownloadPageState::kComplete,
      AstraDownloadDangerType::kSafe,
      yesterday + base::Hours(8),
      yesterday + base::Hours(8) + base::Minutes(2),
      "video/mp4",
      true,
      8,
      "Personal"));

  all_downloads_.push_back(MakeSampleEntry(
      "dl_009", u"failed_archive.tar.gz",
      "https://example.com/failed_archive.tar.gz",
      "/Users/user/Downloads/failed_archive.tar.gz",
      50 * 1024 * 1024,
      23 * 1024 * 1024,
      AstraDownloadPageState::kInterrupted,
      AstraDownloadDangerType::kSafe,
      yesterday + base::Hours(16),
      yesterday + base::Hours(16) + base::Minutes(3),
      "application/gzip",
      false,
      9,
      "Work"));

  // Last 7 days
  base::Time three_days_ago = now - base::Days(3);

  all_downloads_.push_back(MakeSampleEntry(
      "dl_010", u"design_mockup.png",
      "https://design.example.com/mockup.png",
      "/Users/user/Downloads/design_mockup.png",
      3 * 1024 * 1024,
      3 * 1024 * 1024,
      AstraDownloadPageState::kComplete,
      AstraDownloadDangerType::kSafe,
      three_days_ago + base::Hours(11),
      three_days_ago + base::Hours(11) + base::Seconds(10),
      "image/png",
      true,
      10,
      "Work"));

  all_downloads_.push_back(MakeSampleEntry(
      "dl_011", u"dataset.csv",
      "https://data.example.com/dataset.csv",
      "/Users/user/Downloads/dataset.csv",
      25 * 1024 * 1024,
      25 * 1024 * 1024,
      AstraDownloadPageState::kComplete,
      AstraDownloadDangerType::kSafe,
      three_days_ago + base::Hours(9),
      three_days_ago + base::Hours(9) + base::Seconds(45),
      "text/csv",
      true,
      11,
      "Work"));

  all_downloads_.push_back(MakeSampleEntry(
      "dl_012", u"cancelled_update.zip",
      "https://updates.example.com/cancelled_update.zip",
      "/Users/user/Downloads/cancelled_update.zip",
      200 * 1024 * 1024,
      45 * 1024 * 1024,
      AstraDownloadPageState::kCancelled,
      AstraDownloadDangerType::kSafe,
      three_days_ago + base::Hours(15),
      three_days_ago + base::Hours(15) + base::Minutes(1),
      "application/zip",
      false,
      12,
      "Personal"));

  // Older
  base::Time two_weeks_ago = now - base::Days(14);

  all_downloads_.push_back(MakeSampleEntry(
      "dl_013", u"old_report.docx",
      "https://archive.company.com/old_report.docx",
      "/Users/user/Downloads/old_report.docx",
      1 * 1024 * 1024,
      1 * 1024 * 1024,
      AstraDownloadPageState::kComplete,
      AstraDownloadDangerType::kSafe,
      two_weeks_ago + base::Hours(10),
      two_weeks_ago + base::Hours(10) + base::Seconds(5),
      "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
      true,
      13,
      "Work"));

  all_downloads_.push_back(MakeSampleEntry(
      "dl_014", u"code_library.tar.gz",
      "https://opensource.example.com/code_library.tar.gz",
      "/Users/user/Downloads/code_library.tar.gz",
      12 * 1024 * 1024,
      12 * 1024 * 1024,
      AstraDownloadPageState::kComplete,
      AstraDownloadDangerType::kSafe,
      two_weeks_ago - base::Days(3),
      two_weeks_ago - base::Days(3) + base::Seconds(20),
      "application/gzip",
      true,
      14,
      "Work"));

  all_downloads_.push_back(MakeSampleEntry(
      "dl_015", u"dangerous_host_file.exe",
      "https://dangerous-host.example.com/file.exe",
      "/Users/user/Downloads/dangerous_host_file.exe",
      8 * 1024 * 1024,
      8 * 1024 * 1024,
      AstraDownloadPageState::kComplete,
      AstraDownloadDangerType::kDangerousHost,
      two_weeks_ago - base::Days(5),
      two_weeks_ago - base::Days(5) + base::Seconds(15),
      "application/exe",
      false,
      15,
      "Personal"));

  all_downloads_.push_back(MakeSampleEntry(
      "dl_016", u"audiobook.m4a",
      "https://books.example.com/audiobook.m4a",
      "/Users/user/Downloads/audiobook.m4a",
      200 * 1024 * 1024,
      200 * 1024 * 1024,
      AstraDownloadPageState::kComplete,
      AstraDownloadDangerType::kSafe,
      two_weeks_ago - base::Days(7),
      two_weeks_ago - base::Days(7) + base::Minutes(5),
      "audio/mp4a-latm",
      true,
      16,
      "Personal"));

  // Sort all downloads by start time descending.
  std::sort(all_downloads_.begin(), all_downloads_.end(),
            CompareByStartTimeDesc);

  ApplyFilters();
  NotifyDownloadsChanged();
}

// -- Loading state ----------------------------------------------------------

void AstraDownloadsPageModel::SetLoading(bool loading) {
  if (loading_ == loading)
    return;
  loading_ = loading;
  NotifyDownloadsChanged();
}

// -- Statistics -------------------------------------------------------------

int64_t AstraDownloadsPageModel::GetTotalDownloadedBytes() const {
  int64_t total = 0;
  for (const auto& entry : all_downloads_) {
    if (entry.state == AstraDownloadPageState::kComplete) {
      total += entry.received_bytes;
    }
  }
  return total;
}

// -- Filtering helpers ------------------------------------------------------

bool AstraDownloadsPageModel::MatchesSearch(
    const AstraDownloadEntry& entry) const {
  if (search_query_.empty())
    return true;

  std::u16string query_lower = base::i18n::ToLower(search_query_);
  std::u16 name_lower = base::i18n::ToLower(entry.file_name);
  std::u16 url_lower = base::i18n::ToLower(base::UTF8ToUTF16(entry.url));

  return name_lower.find(query_lower) != std::u16string::npos ||
         url_lower.find(query_lower) != std::u16string::npos;
}

bool AstraDownloadsPageModel::MatchesFilter(
    const AstraDownloadEntry& entry) const {
  switch (filter_) {
    case AstraDownloadsPageFilter::kAll:
      return true;
    case AstraDownloadsPageFilter::kInProgress:
      return entry.state == AstraDownloadPageState::kInProgress ||
             entry.state == AstraDownloadPageState::kPaused;
    case AstraDownloadsPageFilter::kCompleted:
      return entry.state == AstraDownloadPageState::kComplete;
    case AstraDownloadsPageFilter::kCancelled:
      return entry.state == AstraDownloadPageState::kCancelled;
    case AstraDownloadsPageFilter::kInterrupted:
      return entry.state == AstraDownloadPageState::kInterrupted;
  }
  return true;
}

bool AstraDownloadsPageModel::MatchesCategory(
    const AstraDownloadEntry& entry) const {
  if (category_filter_.empty())
    return true;
  return entry.category == category_filter_;
}

void AstraDownloadsPageModel::ApplyFilters() {
  filtered_downloads_ = GetFilteredDownloads();
}

std::vector<AstraDownloadEntry>
AstraDownloadsPageModel::GetFilteredDownloads() const {
  std::vector<AstraDownloadEntry> result;
  result.reserve(all_downloads_.size());

  for (const auto& entry : all_downloads_) {
    if (MatchesSearch(entry) && MatchesFilter(entry) &&
        MatchesCategory(entry)) {
      result.push_back(entry);
    }
  }

  return result;
}

// -- Notification helpers ---------------------------------------------------

void AstraDownloadsPageModel::NotifyDownloadsChanged() {
  for (auto& observer : observers_)
    observer.OnDownloadsChanged();
}

void AstraDownloadsPageModel::NotifyDownloadAdded(const std::string& id) {
  for (auto& observer : observers_)
    observer.OnDownloadAdded(id);
}

void AstraDownloadsPageModel::NotifyDownloadRemoved(const std::string& id) {
  for (auto& observer : observers_)
    observer.OnDownloadRemoved(id);
}

void AstraDownloadsPageModel::NotifyDownloadUpdated(const std::string& id) {
  for (auto& observer : observers_)
    observer.OnDownloadUpdated(id);
}

void AstraDownloadsPageModel::NotifySearchChanged() {
  for (auto& observer : observers_)
    observer.OnSearchChanged(search_query_);
}

void AstraDownloadsPageModel::NotifyFilterChanged() {
  for (auto& observer : observers_)
    observer.OnFilterChanged();
}

void AstraDownloadsPageModel::NotifyShutdown() {
  for (auto& observer : observers_)
    observer.OnDownloadsPageModelShutdown();
}

}  // namespace astra
