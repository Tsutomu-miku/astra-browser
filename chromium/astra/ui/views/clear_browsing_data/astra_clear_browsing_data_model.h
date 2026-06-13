// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_CLEAR_BROWSING_DATA_ASTRA_CLEAR_BROWSING_DATA_MODEL_H_
#define ASTRA_UI_VIEWS_CLEAR_BROWSING_DATA_ASTRA_CLEAR_BROWSING_DATA_MODEL_H_

#include <string>
#include <vector>

#include "base/observer_list.h"
#include "base/observer_list_types.h"

namespace astra {

// Time range options for clearing browsing data.
enum class AstraClearBrowsingDataTimeRange {
  kLastHour,
  kLast24Hours,
  kLast7Days,
  kLast4Weeks,
  kAllTime,
};

// Data types that can be cleared.
enum class AstraClearBrowsingDataType {
  kBrowsingHistory,
  kDownloadHistory,
  kCookiesAndSiteData,
  kCachedImagesAndFiles,
  kAutofillFormData,
  kPasswordsAndSigninData,
  kSiteSettings,
  kHostedAppData,
};

// Observer for AstraClearBrowsingDataModel.
class AstraClearBrowsingDataObserver : public base::CheckedObserver {
 public:
  // Called when the selected time range changes.
  virtual void OnTimeRangeChanged(AstraClearBrowsingDataModel* model,
                                  AstraClearBrowsingDataTimeRange range) {}

  // Called when a data type selection is toggled.
  virtual void OnDataTypeToggled(AstraClearBrowsingDataModel* model,
                                 AstraClearBrowsingDataType type,
                                 bool selected) {}

  // Called when the clear operation starts.
  virtual void OnClearStarted(AstraClearBrowsingDataModel* model) {}

  // Called when the clear operation completes.
  virtual void OnClearCompleted(AstraClearBrowsingDataModel* model,
                                bool success) {}

  // Called when the result message changes.
  virtual void OnResultMessageChanged(AstraClearBrowsingDataModel* model,
                                      const std::u16string& message) {}

  // Called when the model is about to be destroyed.
  virtual void OnClearBrowsingDataModelShutdown(
      AstraClearBrowsingDataModel* model) {}

 protected:
  ~AstraClearBrowsingDataObserver() override = default;
};

// Model for the clear browsing data dialog.
//
// Manages the state of the clear browsing data dialog: selected time range,
// selected data types, loading state, and result messages.
//
// Chromium subsystem reused:
//   BrowsingDataRemover (chrome/browser/browsing_data/browsing_data_remover.h)
//   is the truth source for actually removing data. This model projects and
//   augments it with Astra-specific UX patterns.
//
// TODO(astra): Wire to BrowsingDataRemover.
//   Reference: chrome/browser/browsing_data/browsing_data_remover.h
//   Patch point: chrome/browser/ui/browser_browsing_data_remover.cc
class AstraClearBrowsingDataModel {
 public:
  AstraClearBrowsingDataModel();
  ~AstraClearBrowsingDataModel();

  AstraClearBrowsingDataModel(const AstraClearBrowsingDataModel&) = delete;
  AstraClearBrowsingDataModel& operator=(
      const AstraClearBrowsingDataModel&) = delete;

  // -- Observer management --------------------------------------------------

  void AddObserver(AstraClearBrowsingDataObserver* observer);
  void RemoveObserver(AstraClearBrowsingDataObserver* observer);

  // -- Time range -----------------------------------------------------------

  // Set the selected time range.
  void SetTimeRange(AstraClearBrowsingDataTimeRange range);
  AstraClearBrowsingDataTimeRange GetTimeRange() const { return time_range_; }

  // Get the display name for a time range.
  static std::u16string GetTimeRangeName(AstraClearBrowsingDataTimeRange range);

  // Get all time range options in display order.
  static std::vector<AstraClearBrowsingDataTimeRange> GetAllTimeRanges();

  // -- Data types -----------------------------------------------------------

  // Toggle a data type selection.
  void ToggleDataType(AstraClearBrowsingDataType type);

  // Check if a data type is selected.
  bool IsDataTypeSelected(AstraClearBrowsingDataType type) const;

  // Get the number of selected data types.
  size_t GetSelectedDataTypesCount() const;

  // Get display name for a data type.
  static std::u16string GetDataTypeName(AstraClearBrowsingDataType type);

  // Get description for a data type.
  static std::u16string GetDataTypeDescription(AstraClearBrowsingDataType type);

  // Get icon name for a data type.
  static std::string GetDataTypeIconName(AstraClearBrowsingDataType type);

  // Get all data types in display order.
  static std::vector<AstraClearBrowsingDataType> GetAllDataTypes();

  // -- Clear operation ------------------------------------------------------

  // Start clearing selected data.
  void ClearData();

  // -- State ----------------------------------------------------------------

  bool IsLoading() const { return is_loading_; }
  const std::u16string& GetResultMessage() const { return result_message_; }

 private:
  // Notify helpers.
  void NotifyTimeRangeChanged();
  void NotifyDataTypeToggled(AstraClearBrowsingDataType type, bool selected);
  void NotifyClearStarted();
  void NotifyClearCompleted(bool success);
  void NotifyResultMessageChanged();

  // Selected time range.
  AstraClearBrowsingDataTimeRange time_range_ =
      AstraClearBrowsingDataTimeRange::kLast24Hours;

  // Selected data types stored as a bitfield (indices match the enum).
  std::vector<bool> selected_types_;

  // Loading state.
  bool is_loading_ = false;

  // Result message to display.
  std::u16string result_message_;

  base::ObserverList<AstraClearBrowsingDataObserver> observers_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_CLEAR_BROWSING_DATA_ASTRA_CLEAR_BROWSING_DATA_MODEL_H_
