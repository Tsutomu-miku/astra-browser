// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/clear_browsing_data/astra_clear_browsing_data_dialog.h"
#include "astra/ui/views/clear_browsing_data/astra_clear_browsing_data_model.h"

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

// ===========================================================================
// AstraClearBrowsingDataModelTest
// ===========================================================================

class AstraClearBrowsingDataModelTest : public testing::Test {
 protected:
  void SetUp() override {
    model_ = std::make_unique<AstraClearBrowsingDataModel>();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<AstraClearBrowsingDataModel> model_;
};

// Test model creation and default state.
TEST_F(AstraClearBrowsingDataModelTest, Creation) {
  EXPECT_EQ(AstraClearBrowsingDataTimeRange::kLast24Hours,
            model_->GetTimeRange());
  EXPECT_FALSE(model_->IsLoading());
  EXPECT_TRUE(model_->GetResultMessage().empty());
  EXPECT_GT(model_->GetSelectedDataTypesCount(), 0u);
}

// Test time range selection.
TEST_F(AstraClearBrowsingDataModelTest, TimeRangeSelection) {
  model_->SetTimeRange(AstraClearBrowsingDataTimeRange::kLastHour);
  EXPECT_EQ(AstraClearBrowsingDataTimeRange::kLastHour,
            model_->GetTimeRange());

  model_->SetTimeRange(AstraClearBrowsingDataTimeRange::kAllTime);
  EXPECT_EQ(AstraClearBrowsingDataTimeRange::kAllTime,
            model_->GetTimeRange());

  // Setting same range is a no-op.
  model_->SetTimeRange(AstraClearBrowsingDataTimeRange::kAllTime);
  EXPECT_EQ(AstraClearBrowsingDataTimeRange::kAllTime,
            model_->GetTimeRange());
}

// Test data type toggling.
TEST_F(AstraClearBrowsingDataModelTest, DataTypeToggling) {
  // Browsing history should be selected by default.
  EXPECT_TRUE(model_->IsDataTypeSelected(
      AstraClearBrowsingDataType::kBrowsingHistory));

  // Toggle it off.
  model_->ToggleDataType(AstraClearBrowsingDataType::kBrowsingHistory);
  EXPECT_FALSE(model_->IsDataTypeSelected(
      AstraClearBrowsingDataType::kBrowsingHistory));

  // Toggle it back on.
  model_->ToggleDataType(AstraClearBrowsingDataType::kBrowsingHistory);
  EXPECT_TRUE(model_->IsDataTypeSelected(
      AstraClearBrowsingDataType::kBrowsingHistory));
}

// Test selected data types count.
TEST_F(AstraClearBrowsingDataModelTest, SelectedDataTypesCount) {
  // Start with default selection.
  size_t initial_count = model_->GetSelectedDataTypesCount();
  EXPECT_GT(initial_count, 0u);

  // Toggle on a type that is off by default (passwords).
  model_->ToggleDataType(
      AstraClearBrowsingDataType::kPasswordsAndSigninData);
  EXPECT_EQ(initial_count + 1, model_->GetSelectedDataTypesCount());

  // Toggle it off again.
  model_->ToggleDataType(
      AstraClearBrowsingDataType::kPasswordsAndSigninData);
  EXPECT_EQ(initial_count, model_->GetSelectedDataTypesCount());
}

// Test all data types are accessible.
TEST_F(AstraClearBrowsingDataModelTest, AllDataTypesAccessible) {
  auto types = AstraClearBrowsingDataModel::GetAllDataTypes();
  EXPECT_EQ(8u, types.size());

  // Each type should have a non-empty name.
  for (const auto& type : types) {
    EXPECT_FALSE(AstraClearBrowsingDataModel::GetDataTypeName(type).empty());
    EXPECT_FALSE(
        AstraClearBrowsingDataModel::GetDataTypeDescription(type).empty());
    EXPECT_FALSE(
        AstraClearBrowsingDataModel::GetDataTypeIconName(type).empty());
  }
}

// Test all time ranges are accessible.
TEST_F(AstraClearBrowsingDataModelTest, AllTimeRangesAccessible) {
  auto ranges = AstraClearBrowsingDataModel::GetAllTimeRanges();
  EXPECT_EQ(5u, ranges.size());

  for (const auto& range : ranges) {
    EXPECT_FALSE(
        AstraClearBrowsingDataModel::GetTimeRangeName(range).empty());
  }
}

// Test clear data with selection.
TEST_F(AstraClearBrowsingDataModelTest, ClearDataWithSelection) {
  // Ensure at least one type is selected (default should have some).
  ASSERT_GT(model_->GetSelectedDataTypesCount(), 0u);

  model_->ClearData();

  // Loading state: current implementation is synchronous so is_loading
  // starts and ends within ClearData, but we can verify result message.
  EXPECT_FALSE(model_->IsLoading());
  EXPECT_FALSE(model_->GetResultMessage().empty());
}

// Test clear data with no selection shows error.
TEST_F(AstraClearBrowsingDataModelTest, ClearDataNoSelection) {
  // Deselect all types.
  auto types = AstraClearBrowsingDataModel::GetAllDataTypes();
  for (const auto& type : types) {
    if (model_->IsDataTypeSelected(type)) {
      model_->ToggleDataType(type);
    }
  }
  ASSERT_EQ(0u, model_->GetSelectedDataTypesCount());

  model_->ClearData();

  // Should show a message about selecting at least one type.
  EXPECT_FALSE(model_->GetResultMessage().empty());
  EXPECT_FALSE(model_->IsLoading());
}

// Test observer pattern for time range changes.
TEST_F(AstraClearBrowsingDataModelTest, ObserverTimeRangeChanged) {
  class TestObserver : public AstraClearBrowsingDataObserver {
   public:
    void OnTimeRangeChanged(AstraClearBrowsingDataModel* model,
                            AstraClearBrowsingDataTimeRange range) override {
      time_range_changed_called_ = true;
      last_range_ = range;
      last_model_ = model;
    }

    bool time_range_changed_called_ = false;
    AstraClearBrowsingDataTimeRange last_range_ =
        AstraClearBrowsingDataTimeRange::kLastHour;
    raw_ptr<AstraClearBrowsingDataModel> last_model_ = nullptr;
  };

  TestObserver observer;
  model_->AddObserver(&observer);

  model_->SetTimeRange(AstraClearBrowsingDataTimeRange::kLast7Days);
  EXPECT_TRUE(observer.time_range_changed_called_);
  EXPECT_EQ(AstraClearBrowsingDataTimeRange::kLast7Days,
            observer.last_range_);
  EXPECT_EQ(model_.get(), observer.last_model_);

  model_->RemoveObserver(&observer);
}

// Test observer pattern for data type toggles.
TEST_F(AstraClearBrowsingDataModelTest, ObserverDataTypeToggled) {
  class TestObserver : public AstraClearBrowsingDataObserver {
   public:
    void OnDataTypeToggled(AstraClearBrowsingDataModel* model,
                           AstraClearBrowsingDataType type,
                           bool selected) override {
      toggled_called_ = true;
      last_type_ = type;
      last_selected_ = selected;
      last_model_ = model;
    }

    bool toggled_called_ = false;
    AstraClearBrowsingDataType last_type_ =
        AstraClearBrowsingDataType::kBrowsingHistory;
    bool last_selected_ = false;
    raw_ptr<AstraClearBrowsingDataModel> last_model_ = nullptr;
  };

  TestObserver observer;
  model_->AddObserver(&observer);

  // Start by toggling browsing history (default on).
  bool initial =
      model_->IsDataTypeSelected(AstraClearBrowsingDataType::kBrowsingHistory);
  model_->ToggleDataType(AstraClearBrowsingDataType::kBrowsingHistory);

  EXPECT_TRUE(observer.toggled_called_);
  EXPECT_EQ(AstraClearBrowsingDataType::kBrowsingHistory, observer.last_type_);
  EXPECT_EQ(!initial, observer.last_selected_);
  EXPECT_EQ(model_.get(), observer.last_model_);

  model_->RemoveObserver(&observer);
}

// Test observer pattern for clear operations.
TEST_F(AstraClearBrowsingDataModelTest, ObserverClearStartedAndCompleted) {
  class TestObserver : public AstraClearBrowsingDataObserver {
   public:
    void OnClearStarted(AstraClearBrowsingDataModel* model) override {
      started_called_ = true;
    }
    void OnClearCompleted(AstraClearBrowsingDataModel* model,
                          bool success) override {
      completed_called_ = true;
      last_success_ = success;
    }
    void OnResultMessageChanged(AstraClearBrowsingDataModel* model,
                                const std::u16string& message) override {
      message_changed_called_ = true;
      last_message_ = message;
    }

    bool started_called_ = false;
    bool completed_called_ = false;
    bool last_success_ = false;
    bool message_changed_called_ = false;
    std::u16string last_message_;
  };

  TestObserver observer;
  model_->AddObserver(&observer);

  model_->ClearData();

  EXPECT_TRUE(observer.started_called_);
  EXPECT_TRUE(observer.completed_called_);
  EXPECT_TRUE(observer.last_success_);
  EXPECT_TRUE(observer.message_changed_called_);
  EXPECT_FALSE(observer.last_message_.empty());

  model_->RemoveObserver(&observer);
}

// Test model shutdown notification.
TEST_F(AstraClearBrowsingDataModelTest, ObserverModelShutdown) {
  class TestObserver : public AstraClearBrowsingDataObserver {
   public:
    void OnClearBrowsingDataModelShutdown(
        AstraClearBrowsingDataModel* model) override {
      shutdown_called_ = true;
    }
    bool shutdown_called_ = false;
  };

  TestObserver observer;
  model_->AddObserver(&observer);

  model_.reset();  // Destroy the model.
  EXPECT_TRUE(observer.shutdown_called_);
}

// ===========================================================================
// AstraClearBrowsingDataDialogTest
// ===========================================================================

class AstraClearBrowsingDataDialogTest : public testing::Test {
 protected:
  void SetUp() override {
    model_ = std::make_unique<AstraClearBrowsingDataModel>();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<AstraClearBrowsingDataModel> model_;
};

// Test model accessor.
TEST_F(AstraClearBrowsingDataDialogTest, ModelAccessor) {
  EXPECT_TRUE(model_);
  EXPECT_GT(model_->GetSelectedDataTypesCount(), 0u);
}

// Test time range can be changed through model.
TEST_F(AstraClearBrowsingDataDialogTest, TimeRangeChange) {
  model_->SetTimeRange(AstraClearBrowsingDataTimeRange::kLastHour);
  EXPECT_EQ(AstraClearBrowsingDataTimeRange::kLastHour,
            model_->GetTimeRange());

  model_->SetTimeRange(AstraClearBrowsingDataTimeRange::kAllTime);
  EXPECT_EQ(AstraClearBrowsingDataTimeRange::kAllTime,
            model_->GetTimeRange());
}

// Test data type toggling through model.
TEST_F(AstraClearBrowsingDataDialogTest, DataTypeToggle) {
  bool initial =
      model_->IsDataTypeSelected(AstraClearBrowsingDataType::kSiteSettings);

  model_->ToggleDataType(AstraClearBrowsingDataType::kSiteSettings);
  EXPECT_EQ(!initial,
            model_->IsDataTypeSelected(
                AstraClearBrowsingDataType::kSiteSettings));

  model_->ToggleDataType(AstraClearBrowsingDataType::kSiteSettings);
  EXPECT_EQ(initial,
            model_->IsDataTypeSelected(
                AstraClearBrowsingDataType::kSiteSettings));
}

// Test that clear data works with model.
TEST_F(AstraClearBrowsingDataDialogTest, ClearDataViaModel) {
  size_t initial_count = model_->GetSelectedDataTypesCount();
  ASSERT_GT(initial_count, 0u);

  model_->ClearData();

  EXPECT_FALSE(model_->IsLoading());
  EXPECT_FALSE(model_->GetResultMessage().empty());
}

// Test that all data type checkboxes correspond to model state.
TEST_F(AstraClearBrowsingDataDialogTest, AllDataTypesInModel) {
  auto types = AstraClearBrowsingDataModel::GetAllDataTypes();
  EXPECT_EQ(8u, types.size());

  // Each data type should be toggleable and report correct state.
  for (const auto& type : types) {
    bool initial = model_->IsDataTypeSelected(type);
    model_->ToggleDataType(type);
    EXPECT_EQ(!initial, model_->IsDataTypeSelected(type));
    // Toggle back.
    model_->ToggleDataType(type);
    EXPECT_EQ(initial, model_->IsDataTypeSelected(type));
  }
}

// Test time range names are non-empty.
TEST_F(AstraClearBrowsingDataDialogTest, TimeRangeNames) {
  auto ranges = AstraClearBrowsingDataModel::GetAllTimeRanges();
  for (const auto& range : ranges) {
    std::u16string name =
        AstraClearBrowsingDataModel::GetTimeRangeName(range);
    EXPECT_FALSE(name.empty());
  }
}

// Test data type metadata.
TEST_F(AstraClearBrowsingDataDialogTest, DataTypeMetadata) {
  auto types = AstraClearBrowsingDataModel::GetAllDataTypes();
  for (const auto& type : types) {
    EXPECT_FALSE(AstraClearBrowsingDataModel::GetDataTypeName(type).empty());
    EXPECT_FALSE(
        AstraClearBrowsingDataModel::GetDataTypeDescription(type).empty());
    EXPECT_FALSE(
        AstraClearBrowsingDataModel::GetDataTypeIconName(type).empty());
  }
}

}  // namespace astra
