// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_PASSWORD_MANAGER_ASTRA_PASSWORD_MANAGER_VIEW_H_
#define ASTRA_UI_VIEWS_PASSWORD_MANAGER_ASTRA_PASSWORD_MANAGER_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/metadata/view_factory.h"
#include "ui/views/view.h"

#include "astra/ui/views/password_manager/astra_password_manager_model.h"

namespace views {
class BoxLayout;
class FlexLayout;
class ImageButton;
class ImageView;
class Label;
class ScrollView;
class Textfield;
}  // namespace views

namespace astra {

class AstraPasswordEntryRow;
class AstraPasswordGroupSection;
class AstraPasswordSidebarItem;

// Delegate for password manager page view interactions.
class AstraPasswordManagerDelegate {
 public:
  virtual ~AstraPasswordManagerDelegate() = default;

  // Called when a password entry is clicked (open URL).
  virtual void OnPasswordEntryClicked(const std::string& id) = 0;

  // Called when the "Add password" button is clicked.
  virtual void OnAddPassword() = 0;

  // Called when a password is requested to be removed.
  virtual void OnRemovePassword(const std::string& id) = 0;

  // Called when a password's favorite status is toggled.
  virtual void OnFavoriteToggled(const std::string& id, bool favorited) = 0;

  // Called when copy password is requested.
  virtual void OnCopyPassword(const std::string& id) = 0;

  // Called when the search query changes.
  virtual void OnSearchQueryChanged(const std::u16string& query) = 0;

  // Called when the filter changes.
  virtual void OnFilterChanged(AstraPasswordFilter filter) = 0;

  // Called when a category filter is selected.
  virtual void OnCategoryFilterChanged(const std::string& category) = 0;
};

// The full-page password manager view.
//
// Layout:
//   +-----------------------------------------------+
//   |  Title    Search          Add  Sort  More    |  <- toolbar
//   +---------+-------------------------------------+
//   |         |                                     |
//   | Sidebar |   Password list (grouped by letter) |  <- main content
//   |  (filters     Password detail panel (right)   |
//   |  + cats) |                                     |
//   |         |                                     |
//   +---------+-------------------------------------+
//   |  Status bar (count + issues)                  |  <- bottom
//   +-----------------------------------------------+
//
// This is a Views-based alternative to Chromium's password manager WebUI.
//
// Chromium owner: PasswordManagerUI / PasswordManagerPageHandler
//   (chrome/browser/ui/webui/password_manager/password_manager_ui.h)
//
// TODO(astra): Integrate with Chromium's PasswordStore via a KeyedService.
// Patch point: chrome/browser/ui/webui/password_manager/password_manager_ui.cc
// — replace WebUI with this Views page, or embed it in a constrained
// WebUI container.
class AstraPasswordManagerView : public views::View,
                                 public AstraPasswordManagerObserver,
                                 public views::TextfieldController {
 public:
  METADATA_HEADER(AstraPasswordManagerView);

  AstraPasswordManagerView();
  explicit AstraPasswordManagerView(AstraPasswordManagerModel* model);
  AstraPasswordManagerView(const AstraPasswordManagerView&) = delete;
  AstraPasswordManagerView& operator=(const AstraPasswordManagerView&) = delete;
  ~AstraPasswordManagerView() override;

  // -- Model binding --------------------------------------------------------

  void SetModel(AstraPasswordManagerModel* model);
  AstraPasswordManagerModel* model() const { return model_; }

  // -- Delegate -------------------------------------------------------------

  void SetDelegate(AstraPasswordManagerDelegate* delegate) {
    delegate_ = delegate;
  }
  AstraPasswordManagerDelegate* delegate() const { return delegate_; }

  // -- Controls access (for testing) ----------------------------------------

  views::Textfield* search_field_for_test() { return search_field_; }
  views::ImageButton* add_button_for_test() { return add_button_; }
  views::View* sidebar_for_test() { return sidebar_scroll_; }
  views::ScrollView* content_scroll_for_test() { return content_scroll_; }
  views::Label* status_label_for_test() { return status_label_; }
  views::View* detail_panel_for_test() { return detail_panel_; }

  size_t password_row_count_for_test() const { return password_rows_.size(); }

  // -- Refresh --------------------------------------------------------------

  // Rebuild the entire password list from the model.
  void RefreshFromModel();

  // -- views::View: --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void Layout() override;
  void OnThemeChanged() override;

  // -- AstraPasswordManagerObserver: ---------------------------------------

  void OnPasswordsChanged() override;
  void OnPasswordAdded(const std::string& id) override;
  void OnPasswordRemoved(const std::string& id) override;
  void OnPasswordUpdated(const std::string& id) override;
  void OnSearchChanged(const std::u16string& query) override;
  void OnFilterChanged() override;
  void OnPasswordManagerModelShutdown() override;

  // -- views::TextfieldController: -----------------------------------------

  void ContentsChanged(views::Textfield* sender,
                       const std::u16string& new_contents) override;

 private:
  // Build the entire UI.
  void Build();

  // Build the top toolbar (search, add, sort, more).
  void BuildToolbar();

  // Build the left sidebar (filters + categories).
  void BuildSidebar();

  // Build the main content area.
  void BuildContent();

  // Build the bottom status bar.
  void BuildStatusBar();

  // Build the detail panel (right side).
  void BuildDetailPanel();

  // Rebuild the sidebar filter items from the model.
  void RebuildSidebar();

  // Rebuild the password list from the model's filtered data.
  void RebuildPasswordList();

  // Update the status bar text.
  void UpdateStatusBar();

  // Show or hide the empty state.
  void UpdateEmptyState();

  // Update the detail panel for the selected password.
  void UpdateDetailPanel(const std::string& id);

  // Clear the detail panel (no selection).
  void ClearDetailPanel();

  // -- Event handlers -------------------------------------------------------

  void OnAddButtonClicked();
  void OnSortButtonClicked();
  void OnMoreButtonClicked();
  void OnSidebarFilterClicked(AstraPasswordFilter filter);
  void OnSidebarCategoryClicked(const std::string& category);
  void OnPasswordRowClicked(const std::string& id);
  void OnPasswordRowFavoriteClicked(const std::string& id);
  void OnPasswordRowCopyClicked(const std::string& id);
  void OnPasswordRowDeleteClicked(const std::string& id);
  void OnDetailEditClicked();
  void OnDetailDeleteClicked();

  // -- Custom icon painting -------------------------------------------------

  static void DrawKeyIcon(gfx::Canvas* canvas,
                          const gfx::Rect& bounds,
                          SkColor color);
  static void DrawLockIcon(gfx::Canvas* canvas,
                           const gfx::Rect& bounds,
                           SkColor color);
  static void DrawCopyIcon(gfx::Canvas* canvas,
                           const gfx::Rect& bounds,
                           SkColor color);
  static void DrawEyeIcon(gfx::Canvas* canvas,
                          const gfx::Rect& bounds,
                          SkColor color,
                          bool visible);
  static void DrawStarIcon(gfx::Canvas* canvas,
                           const gfx::Rect& bounds,
                           SkColor color,
                           bool filled);
  static void DrawWarningIcon(gfx::Canvas* canvas,
                              const gfx::Rect& bounds,
                              SkColor color);
  static void DrawSearchIcon(gfx::Canvas* canvas,
                             const gfx::Rect& bounds,
                             SkColor color);
  static void DrawAddIcon(gfx::Canvas* canvas,
                          const gfx::Rect& bounds,
                          SkColor color);
  static void DrawMoreIcon(gfx::Canvas* canvas,
                           const gfx::Rect& bounds,
                           SkColor color);
  static void DrawDeleteIcon(gfx::Canvas* canvas,
                             const gfx::Rect& bounds,
                             SkColor color);
  static void DrawEditIcon(gfx::Canvas* canvas,
                           const gfx::Rect& bounds,
                           SkColor color);
  static void DrawSortIcon(gfx::Canvas* canvas,
                           const gfx::Rect& bounds,
                           SkColor color);

  // -- Data -----------------------------------------------------------------

  raw_ptr<AstraPasswordManagerModel> model_ = nullptr;
  raw_ptr<AstraPasswordManagerDelegate> delegate_ = nullptr;

  base::ScopedObservation<AstraPasswordManagerModel,
                          AstraPasswordManagerObserver>
      model_observation_{this};

  // Child views.
  raw_ptr<views::View> toolbar_ = nullptr;
  raw_ptr<views::Textfield> search_field_ = nullptr;
  raw_ptr<views::ImageButton> add_button_ = nullptr;
  raw_ptr<views::ImageButton> sort_button_ = nullptr;
  raw_ptr<views::ImageButton> more_button_ = nullptr;

  raw_ptr<views::ScrollView> sidebar_scroll_ = nullptr;
  raw_ptr<views::View> sidebar_content_ = nullptr;
  raw_ptr<views::View> filter_section_ = nullptr;
  raw_ptr<views::View> category_section_ = nullptr;

  raw_ptr<views::ScrollView> content_scroll_ = nullptr;
  raw_ptr<views::View> content_container_ = nullptr;
  raw_ptr<views::View> empty_state_view_ = nullptr;

  raw_ptr<views::View> detail_panel_ = nullptr;

  raw_ptr<views::View> status_bar_ = nullptr;
  raw_ptr<views::Label> status_label_ = nullptr;
  raw_ptr<views::Label> issues_label_ = nullptr;

  // Tracked password rows (owned by view hierarchy).
  std::vector<raw_ptr<AstraPasswordEntryRow, VectorExperimental>>
      password_rows_;

  // Tracked sidebar items (owned by view hierarchy).
  std::vector<raw_ptr<AstraPasswordSidebarItem, VectorExperimental>>
      sidebar_items_;

  // Currently selected password ID (empty = none).
  std::string selected_password_id_;

  // Currently selected sidebar filter.
  AstraPasswordFilter selected_filter_ = AstraPasswordFilter::kAll;

  // Currently selected category (empty = all).
  std::string selected_category_;

  // Layout constants.
  static constexpr int kToolbarHeight = 56;
  static constexpr int kSidebarWidth = 240;
  static constexpr int kDetailPanelWidth = 320;
  static constexpr int kStatusBarHeight = 32;
  static constexpr int kSidePadding = 16;
  static constexpr int kToolbarSpacing = 8;
  static constexpr int kButtonSize = 32;
  static constexpr int kSearchFieldWidth = 280;
  static constexpr int kRowHeight = 64;
  static constexpr int kGroupSpacing = 16;
};

// A single password entry row in the list.
class AstraPasswordEntryRow : public views::View {
 public:
  METADATA_HEADER(AstraPasswordEntryRow);

  explicit AstraPasswordEntryRow(const AstraPasswordEntry& entry);
  AstraPasswordEntryRow(const AstraPasswordEntryRow&) = delete;
  AstraPasswordEntryRow& operator=(const AstraPasswordEntryRow&) = delete;
  ~AstraPasswordEntryRow() override;

  // Accessors.
  const std::string& entry_id() const { return entry_.id; }
  const AstraPasswordEntry& entry() const { return entry_; }

  views::ImageButton* favorite_button() { return favorite_button_; }
  views::ImageButton* copy_button() { return copy_button_; }
  views::ImageButton* more_button() { return more_button_; }

  // Update the entry data and refresh display.
  void SetEntry(const AstraPasswordEntry& entry);

  // Set selected state.
  void SetSelected(bool selected);
  bool selected() const { return selected_; }

  // -- views::View: --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void Layout() override;
  void OnThemeChanged() override;
  void OnPaint(gfx::Canvas* canvas) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;

  // Callback types.
  using ClickCallback = base::RepeatingCallback<void(const std::string&)>;
  using FavoriteCallback = base::RepeatingCallback<void(const std::string&)>;
  using CopyCallback = base::RepeatingCallback<void(const std::string&)>;
  using DeleteCallback = base::RepeatingCallback<void(const std::string&)>;

  void SetClickCallback(ClickCallback callback) {
    click_callback_ = std::move(callback);
  }
  void SetFavoriteCallback(FavoriteCallback callback) {
    favorite_callback_ = std::move(callback);
  }
  void SetCopyCallback(CopyCallback callback) {
    copy_callback_ = std::move(callback);
  }
  void SetDeleteCallback(DeleteCallback callback) {
    delete_callback_ = std::move(callback);
  }

 private:
  // Draw the favicon placeholder (colored circle with first letter).
  void DrawFaviconPlaceholder(gfx::Canvas* canvas);

  // Draw warning badges.
  void DrawWarningBadges(gfx::Canvas* canvas);

  AstraPasswordEntry entry_;
  bool hovered_ = false;
  bool selected_ = false;

  raw_ptr<views::Label> site_name_label_ = nullptr;
  raw_ptr<views::Label> username_label_ = nullptr;
  raw_ptr<views::ImageButton> favorite_button_ = nullptr;
  raw_ptr<views::ImageButton> copy_button_ = nullptr;
  raw_ptr<views::ImageButton> more_button_ = nullptr;

  ClickCallback click_callback_;
  FavoriteCallback favorite_callback_;
  CopyCallback copy_callback_;
  DeleteCallback delete_callback_;

  static constexpr int kFaviconSize = 32;
  static constexpr int kFaviconSpacing = 12;
  static constexpr int kButtonSize = 28;
  static constexpr int kButtonSpacing = 4;
  static constexpr int kRowPadding = 12;
  static constexpr int kBadgeSize = 16;
  static constexpr int kBadgeSpacing = 4;
};

// A group section (by first letter) in the password list.
class AstraPasswordGroupSection : public views::View {
 public:
  METADATA_HEADER(AstraPasswordGroupSection);

  explicit AstraPasswordGroupSection(const AstraPasswordGroup& group_data);
  AstraPasswordGroupSection(const AstraPasswordGroupSection&) = delete;
  AstraPasswordGroupSection& operator=(const AstraPasswordGroupSection&) =
      delete;
  ~AstraPasswordGroupSection() override;

  size_t GetEntryCount() const { return entry_rows_.size(); }
  AstraPasswordEntryRow* GetEntryRow(size_t index) const;

  // -- views::View: --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void Layout() override;
  void OnThemeChanged() override;

 private:
  void Build();

  raw_ptr<views::Label> group_label_ = nullptr;
  raw_ptr<views::View> entries_container_ = nullptr;

  std::vector<raw_ptr<AstraPasswordEntryRow, VectorExperimental>> entry_rows_;

  AstraPasswordGroup group_data_;

  static constexpr int kLabelHeight = 28;
  static constexpr int kEntrySpacing = 2;
};

// A sidebar item (filter or category).
class AstraPasswordSidebarItem : public views::View {
 public:
  METADATA_HEADER(AstraPasswordSidebarItem);

  AstraPasswordSidebarItem(const std::u16string& label,
                           int count,
                           bool is_filter = true);
  AstraPasswordSidebarItem(const AstraPasswordSidebarItem&) = delete;
  AstraPasswordSidebarItem& operator=(const AstraPasswordSidebarItem&) = delete;
  ~AstraPasswordSidebarItem() override;

  void SetSelected(bool selected);
  bool selected() const { return selected_; }

  void SetCount(int count);
  void SetLabel(const std::u16string& label);

  // -- views::View: --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void Layout() override;
  void OnThemeChanged() override;
  void OnPaint(gfx::Canvas* canvas) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;

  using ClickCallback = base::RepeatingClosure;
  void SetClickCallback(ClickCallback callback) {
    click_callback_ = std::move(callback);
  }

 private:
  std::u16string label_;
  int count_ = 0;
  bool is_filter_ = true;
  bool selected_ = false;
  bool hovered_ = false;

  raw_ptr<views::Label> label_view_ = nullptr;
  raw_ptr<views::Label> count_view_ = nullptr;

  ClickCallback click_callback_;

  static constexpr int kItemHeight = 36;
  static constexpr int kItemPadding = 12;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_PASSWORD_MANAGER_ASTRA_PASSWORD_MANAGER_VIEW_H_
