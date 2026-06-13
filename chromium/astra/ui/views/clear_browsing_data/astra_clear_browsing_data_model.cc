// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/clear_browsing_data/astra_clear_browsing_data_model.h"

#include "base/strings/utf_string_conversions.h"

namespace astra {

namespace {

// Time range metadata.
struct TimeRangeInfo {
  AstraClearBrowsingDataTimeRange range;
  const char* name;
};

const TimeRangeInfo kTimeRangeInfos[] = {
    {AstraClearBrowsingDataTimeRange::kLastHour, "Last hour"},
    {AstraClearBrowsingDataTimeRange::kLast24Hours, "Last 24 hours"},
    {AstraClearBrowsingDataTimeRange::kLast7Days, "Last 7 days"},
    {AstraClearBrowsingDataTimeRange::kLast4Weeks, "Last 4 weeks"},
    {AstraClearBrowsingDataTimeRange::kAllTime, "All time"},
};

// Data type metadata.
struct DataTypeInfo {
  AstraClearBrowsingDataType type;
  const char* name;
  const char* description;
  const char* icon_name;
  bool default_selected;
};

const DataTypeInfo kDataTypeInfos[] = {
    {AstraClearBrowsingDataType::kBrowsingHistory,
     "Browsing history",
     "Websites you've visited",
     "history",
     true},
    {AstraClearBrowsingDataType::kDownloadHistory,
     "Download history",
     "Files you've downloaded",
     "download",
     true},
    {AstraClearBrowsingDataType::kCookiesAndSiteData,
     "Cookies and other site data",
     "Sign-in data and site preferences",
     "cookie",
     true},
    {AstraClearBrowsingDataType::kCachedImagesAndFiles,
     "Cached images and files",
     "Stored content for faster loading",
     "cache",
     true},
    {AstraClearBrowsingDataType::kAutofillFormData,
     "Autofill form data",
     "Addresses, phone numbers, and form entries",
     "form",
     false},
    {AstraClearBrowsingDataType::kPasswordsAndSigninData,
     "Passwords and sign-in data",
     "Saved passwords and sign-in information",
     "password",
     false},
    {AstraClearBrowsingDataType::kSiteSettings,
     "Site settings",
     "Permissions and preferences for sites",
     "settings",
     false},
    {AstraClearBrowsingDataType::kHostedAppData,
     "Hosted app data",
     "Data stored by hosted apps",
     "app",
     false},
};

const TimeRangeInfo* FindTimeRangeInfo(AstraClearBrowsingDataTimeRange range) {
  for (const auto& info : kTimeRangeInfos) {
    if (info.range == range) {
      return &info;
    }
  }
  return nullptr;
}

const DataTypeInfo* FindDataTypeInfo(AstraClearBrowsingDataType type) {
  for (const auto& info : kDataTypeInfos) {
    if (info.type == type) {
      return &info;
    }
  }
  return nullptr;
}

size_t DataTypeIndex(AstraClearBrowsingDataType type) {
  return static_cast<size_t>(type);
}

}  // namespace

// ===========================================================================
// Static helpers
// ===========================================================================

std::u16string AstraClearBrowsingDataModel::GetTimeRangeName(
    AstraClearBrowsingDataTimeRange range) {
  const auto* info = FindTimeRangeInfo(range);
  return info ? base::UTF8ToUTF16(info->name) : std::u16string();
}

std::vector<AstraClearBrowsingDataTimeRange>
AstraClearBrowsingDataModel::GetAllTimeRanges() {
  std::vector<AstraClearBrowsingDataTimeRange> ranges;
  for (const auto& info : kTimeRangeInfos) {
    ranges.push_back(info.range);
  }
  return ranges;
}

std::u16string AstraClearBrowsingDataModel::GetDataTypeName(
    AstraClearBrowsingDataType type) {
  const auto* info = FindDataTypeInfo(type);
  return info ? base::UTF8ToUTF16(info->name) : std::u16string();
}

std::u16string AstraClearBrowsingDataModel::GetDataTypeDescription(
    AstraClearBrowsingDataType type) {
  const auto* info = FindDataTypeInfo(type);
  return info ? base::UTF8ToUTF16(info->description) : std::u16string();
}

std::string AstraClearBrowsingDataModel::GetDataTypeIconName(
    AstraClearBrowsingDataType type) {
  const auto* info = FindDataTypeInfo(type);
  return info ? info->icon_name : std::string();
}

std::vector<AstraClearBrowsingDataType>
AstraClearBrowsingDataModel::GetAllDataTypes() {
  std::vector<AstraClearBrowsingDataType> types;
  for (const auto& info : kDataTypeInfos) {
    types.push_back(info.type);
  }
  return types;
}

// ===========================================================================
// AstraClearBrowsingDataModel
// ===========================================================================

AstraClearBrowsingDataModel::AstraClearBrowsingDataModel() {
  // Initialize selected types with defaults.
  size_t num_types = std::size(kDataTypeInfos);
  selected_types_.resize(num_types, false);
  for (size_t i = 0; i < num_types; ++i) {
    selected_types_[i] = kDataTypeInfos[i].default_selected;
  }
}

AstraClearBrowsingDataModel::~AstraClearBrowsingDataModel() {
  for (auto& observer : observers_) {
    observer.OnClearBrowsingDataModelShutdown(this);
  }
}

void AstraClearBrowsingDataModel::AddObserver(
    AstraClearBrowsingDataObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraClearBrowsingDataModel::RemoveObserver(
    AstraClearBrowsingDataObserver* observer) {
  observers_.RemoveObserver(observer);
}

void AstraClearBrowsingDataModel::SetTimeRange(
    AstraClearBrowsingDataTimeRange range) {
  if (time_range_ == range) {
    return;
  }
  time_range_ = range;
  NotifyTimeRangeChanged();
}

void AstraClearBrowsingDataModel::ToggleDataType(
    AstraClearBrowsingDataType type) {
  size_t idx = DataTypeIndex(type);
  if (idx >= selected_types_.size()) {
    return;
  }
  selected_types_[idx] = !selected_types_[idx];
  NotifyDataTypeToggled(type, selected_types_[idx]);
}

bool AstraClearBrowsingDataModel::IsDataTypeSelected(
    AstraClearBrowsingDataType type) const {
  size_t idx = DataTypeIndex(type);
  if (idx >= selected_types_.size()) {
    return false;
  }
  return selected_types_[idx];
}

size_t AstraClearBrowsingDataModel::GetSelectedDataTypesCount() const {
  size_t count = 0;
  for (bool selected : selected_types_) {
    if (selected) {
      ++count;
    }
  }
  return count;
}

void AstraClearBrowsingDataModel::ClearData() {
  if (is_loading_) {
    return;
  }
  if (GetSelectedDataTypesCount() == 0) {
    result_message_ = u"Select at least one type of data to clear.";
    NotifyResultMessageChanged();
    return;
  }

  is_loading_ = true;
  result_message_ = u"";
  NotifyClearStarted();

  // TODO(astra): Wire to BrowsingDataRemover::Remove() for actual data removal.
  // Reference: chrome/browser/browsing_data/browsing_data_remover.h
  // Patch point: chrome/browser/browsing_data/browsing_data_remover.cc
  // This is a simulated operation.

  // Simulate completion immediately (in a real implementation, this would
  // be asynchronous via BrowsingDataRemover::Observer).
  is_loading_ = false;
  result_message_ = u"Clearing browsing data complete.";
  NotifyClearCompleted(true);
  NotifyResultMessageChanged();
}

// ===========================================================================
// Private helpers
// ===========================================================================

void AstraClearBrowsingDataModel::NotifyTimeRangeChanged() {
  for (auto& observer : observers_) {
    observer.OnTimeRangeChanged(this, time_range_);
  }
}

void AstraClearBrowsingDataModel::NotifyDataTypeToggled(
    AstraClearBrowsingDataType type,
    bool selected) {
  for (auto& observer : observers_) {
    observer.OnDataTypeToggled(this, type, selected);
  }
}

void AstraClearBrowsingDataModel::NotifyClearStarted() {
  for (auto& observer : observers_) {
    observer.OnClearStarted(this);
  }
}

void AstraClearBrowsingDataModel::NotifyClearCompleted(bool success) {
  for (auto& observer : observers_) {
    observer.OnClearCompleted(this, success);
  }
}

void AstraClearBrowsingDataModel::NotifyResultMessageChanged() {
  for (auto& observer : observers_) {
    observer.OnResultMessageChanged(this, result_message_);
  }
}

}  // namespace astra
