// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_CLEAR_BROWSING_DATA_ASTRA_CLEAR_BROWSING_DATA_DIALOG_H_
#define ASTRA_UI_VIEWS_CLEAR_BROWSING_DATA_ASTRA_CLEAR_BROWSING_DATA_DIALOG_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/view.h"
#include "astra/ui/views/clear_browsing_data/astra_clear_browsing_data_model.h"

namespace views {
class Checkbox;
class Combobox;
class ImageView;
class Label;
class MdTextButton;
}  // namespace views

namespace astra {

// =========================================================================
// AstraClearBrowsingDataDialog — clear browsing data dialog
// =========================================================================
//
// A dialog for clearing browsing data: history, cookies, cache, etc.
//
// Layout:
//
//   +-------------------------------------------+
//   |  [trash icon]  Clear browsing data         |
//   |                                           |
//   |  Time range:    [ Last 24 hours  \/]      |
//   |                                           |
//   |  [ ] Browsing history                     |
//   |      Websites you've visited              |
//   |                                           |
//   |  [ ] Download history                     |
//   |      Files you've downloaded              |
//   |  ... (more data types)                    |
//   |                                           |
//   |  [status message area]                    |
//   |                                           |
//   |  Note: Signed-in data ...                 |
//   |                                           |
//   |                  [ Cancel ] [ Clear data ]|
//   +-------------------------------------------+
//
// Chromium subsystems reused:
//   - BrowsingDataRemover (truth source for actual removal)
//   - views::BubbleDialogDelegateView (dialog pattern)
//
// TODO(astra): Wire to Chrome's clear browsing data dialog system.
//   Reference: chrome/browser/ui/views/clear_browsing_data/clear_browsing_data_dialog.h
//   Patch point: chrome/browser/browsing_data/browsing_data_remover.cc
// =========================================================================
class AstraClearBrowsingDataDialog
    : public views::BubbleDialogDelegateView,
      public AstraClearBrowsingDataObserver {
 public:
  // Construct a clear browsing data dialog anchored to a view.
  explicit AstraClearBrowsingDataDialog(views::View* anchor_view,
                                        AstraClearBrowsingDataModel* model);
  ~AstraClearBrowsingDataDialog() override;

  AstraClearBrowsingDataDialog(const AstraClearBrowsingDataDialog&) = delete;
  AstraClearBrowsingDataDialog& operator=(
      const AstraClearBrowsingDataDialog&) = delete;

  // Set the model.
  void SetModel(AstraClearBrowsingDataModel* model);
  AstraClearBrowsingDataModel* model() { return model_; }

  // Refresh the view from the model.
  void RefreshFromModel();

  // -- views::BubbleDialogDelegateView -------------------------------------

  std::u16string GetWindowTitle() const override;
  void OnThemeChanged() override;

  // -- AstraClearBrowsingDataObserver --------------------------------------

  void OnTimeRangeChanged(AstraClearBrowsingDataModel* model,
                          AstraClearBrowsingDataTimeRange range) override;
  void OnDataTypeToggled(AstraClearBrowsingDataModel* model,
                         AstraClearBrowsingDataType type,
                         bool selected) override;
  void OnClearStarted(AstraClearBrowsingDataModel* model) override;
  void OnClearCompleted(AstraClearBrowsingDataModel* model,
                        bool success) override;
  void OnResultMessageChanged(AstraClearBrowsingDataModel* model,
                              const std::u16string& message) override;
  void OnClearBrowsingDataModelShutdown(
      AstraClearBrowsingDataModel* model) override;

  // Accessors for testing.
  views::ImageView* header_icon() { return header_icon_; }
  views::Label* title_label() { return title_label_; }
  views::Combobox* time_range_combobox() { return time_range_combobox_; }
  views::MdTextButton* clear_button() { return clear_button_; }
  views::MdTextButton* cancel_button() { return cancel_button_; }
  views::Label* result_label() { return result_label_; }
  views::Label* footer_label() { return footer_label_; }

  // Get the checkbox for a specific data type (for testing).
  views::Checkbox* GetDataTypeCheckbox(AstraClearBrowsingDataType type);

 private:
  void BuildUI();
  void BuildHeader();
  void BuildTimeRangeRow();
  void BuildDataTypeCheckboxes();
  void BuildResultArea();
  void BuildFooter();
  void BuildButtons();

  // Update checkbox states from the model.
  void UpdateCheckboxStates();

  // Update button enabled states.
  void UpdateButtonStates();

  // Button handlers.
  void OnClearClicked();
  void OnCancelClicked();
  void OnTimeRangeChanged();
  void OnDataTypeToggled(AstraClearBrowsingDataType type);

  // Icon drawing helpers.
  void DrawTrashIcon(gfx::Canvas* canvas,
                     const gfx::Rect& bounds,
                     SkColor color);
  void DrawClockIcon(gfx::Canvas* canvas,
                     const gfx::Rect& bounds,
                     SkColor color);
  void DrawHistoryIcon(gfx::Canvas* canvas,
                       const gfx::Rect& bounds,
                       SkColor color);
  void DrawDownloadIcon(gfx::Canvas* canvas,
                        const gfx::Rect& bounds,
                        SkColor color);
  void DrawCookieIcon(gfx::Canvas* canvas,
                      const gfx::Rect& bounds,
                      SkColor color);
  void DrawCacheIcon(gfx::Canvas* canvas,
                     const gfx::Rect& bounds,
                     SkColor color);
  void DrawFormIcon(gfx::Canvas* canvas,
                    const gfx::Rect& bounds,
                    SkColor color);
  void DrawPasswordIcon(gfx::Canvas* canvas,
                        const gfx::Rect& bounds,
                        SkColor color);
  void DrawSettingsIcon(gfx::Canvas* canvas,
                        const gfx::Rect& bounds,
                        SkColor color);
  void DrawAppIcon(gfx::Canvas* canvas,
                   const gfx::Rect& bounds,
                   SkColor color);

  // Draw a data type icon by type.
  void DrawDataTypeIcon(gfx::Canvas* canvas,
                        const gfx::Rect& bounds,
                        AstraClearBrowsingDataType type,
                        SkColor color);

  // Model (not owned).
  raw_ptr<AstraClearBrowsingDataModel> model_ = nullptr;

  // Scoped observation.
  base::ScopedObservation<AstraClearBrowsingDataModel,
                          AstraClearBrowsingDataObserver>
      scoped_observation_{this};

  // Header.
  raw_ptr<views::ImageView> header_icon_ = nullptr;
  raw_ptr<views::Label> title_label_ = nullptr;

  // Time range.
  raw_ptr<views::Combobox> time_range_combobox_ = nullptr;

  // Data type checkboxes.
  struct DataTypeCheckboxRow {
    raw_ptr<views::Checkbox> checkbox = nullptr;
    AstraClearBrowsingDataType type;
  };
  std::vector<DataTypeCheckboxRow> checkbox_rows_;

  // Result / status area.
  raw_ptr<views::Label> result_label_ = nullptr;

  // Footer warning.
  raw_ptr<views::Label> footer_label_ = nullptr;

  // Buttons.
  raw_ptr<views::MdTextButton> clear_button_ = nullptr;
  raw_ptr<views::MdTextButton> cancel_button_ = nullptr;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_CLEAR_BROWSING_DATA_ASTRA_CLEAR_BROWSING_DATA_DIALOG_H_
