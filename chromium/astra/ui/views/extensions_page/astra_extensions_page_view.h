// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_EXTENSIONS_PAGE_ASTRA_EXTENSIONS_PAGE_VIEW_H_
#define ASTRA_UI_VIEWS_EXTENSIONS_PAGE_ASTRA_EXTENSIONS_PAGE_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/view.h"

namespace views {
class BoxLayout;
class Button;
class Combobox;
class FlexLayout;
class ImageButton;
class ImageView;
class Label;
class MdTextButton;
class ScrollView;
class Separator;
class Textfield;
class ToggleButton;
}  // namespace views

namespace astra {

class AstraExtensionsPageModel;
class AstraExtensionCategory;
enum class AstraExtensionFilter;

// Delegate interface for extensions page interactions.
class AstraExtensionsPageDelegate {
 public:
  virtual ~AstraExtensionsPageDelegate() = default;

  // Called when an extension should be opened.
  virtual void OnOpenExtension(const std::string& extension_id) {}

  // Called when an extension's options should be shown.
  virtual void OnExtensionOptions(const std::string& extension_id) {}

  // Called when an extension should be removed.
  virtual void OnExtensionRemoved(const std::string& extension_id) {}

  // Called to open the Chrome Web Store.
  virtual void OnOpenChromeWebStore() {}

  // Called to manage keyboard shortcuts.
  virtual void OnManageShortcuts() {}

  // Called when the user wants to install a new extension.
  virtual void OnInstallExtension() {}
};

// =========================================================================
// AstraExtensionCardView — a single extension card in the grid/list
// =========================================================================
//
// Displays an extension with icon, name, version, description,
// enable/disable toggle, and action buttons.
//
// Can be displayed in a grid layout (large cards) or list layout (compact).
// =========================================================================
class AstraExtensionCardView : public views::View {
 public:
  explicit AstraExtensionCardView(const std::string& extension_id);
  ~AstraExtensionCardView() override;

  AstraExtensionCardView(const AstraExtensionCardView&) = delete;
  AstraExtensionCardView& operator=(const AstraExtensionCardView&) = delete;

  // Update the card from a model entry.
  void UpdateFromModel(const AstraExtensionsPageModel* model);

  const std::string& extension_id() const { return extension_id_; }

  // Set display mode: true = compact (list), false = large (grid).
  void SetCompact(bool compact);
  bool compact() const { return compact_; }

  // views::View:
  void OnThemeChanged() override;
  void OnPaint(gfx::Canvas* canvas) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;

  // Accessors for testing.
  views::ToggleButton* enabled_toggle() { return enabled_toggle_; }
  views::ImageButton* pin_button() { return pin_button_; }
  views::ImageButton* details_button() { return details_button_; }
  views::Label* name_label() { return name_label_; }
  views::Label* desc_label() { return desc_label_; }

 private:
  void BuildLayout();
  void UpdateColors();
  void OnEnabledToggled();
  void OnPinClicked();
  void OnDetailsClicked();

  std::string extension_id_;
  bool compact_ = false;
  bool is_hovered_ = false;

  raw_ptr<views::ImageView> icon_view_ = nullptr;
  raw_ptr<views::Label> name_label_ = nullptr;
  raw_ptr<views::Label> version_label_ = nullptr;
  raw_ptr<views::Label> desc_label_ = nullptr;
  raw_ptr<views::Label> status_label_ = nullptr;
  raw_ptr<views::ToggleButton> enabled_toggle_ = nullptr;
  raw_ptr<views::ImageButton> pin_button_ = nullptr;
  raw_ptr<views::ImageButton> details_button_ = nullptr;
  raw_ptr<views::View> permission_badge_ = nullptr;
  raw_ptr<views::View> error_indicator_ = nullptr;
};

// =========================================================================
// AstraExtensionsPageView — full page extensions manager
// =========================================================================
//
// Full-page view for managing extensions. Layout:
//
//   +----------------------------------------------------+
//   |  [Search]   [Sort v] [View grid/list]  [+ Add]    |  <- Toolbar
//   +-----------+----------------------------------------+
//   | Categories|  Extension card grid / list            |
//   |  - All    |                                        |
//   |  - Productivity |                                  |
//   |  - Developer |                                    |
//   |  ...       |                                        |
//   +-----------+----------------------------------------+
//
// Chromium subsystems reused:
//   - ExtensionService / ExtensionRegistry (truth source)
//   - views framework (ScrollView, BoxLayout, etc.)
//
// TODO(astra): Wire to Chrome's chrome://extensions page pattern.
//   Reference: chrome/browser/ui/webui/extensions/extensions_ui.cc
// =========================================================================
class AstraExtensionsPageView : public views::View,
                                public views::TextfieldController,
                                public AstraExtensionsPageObserver {
 public:
  AstraExtensionsPageView();
  ~AstraExtensionsPageView() override;

  AstraExtensionsPageView(const AstraExtensionsPageView&) = delete;
  AstraExtensionsPageView& operator=(const AstraExtensionsPageView&) = delete;

  // Set the model to observe. The view does not own the model.
  void SetModel(AstraExtensionsPageModel* model);
  AstraExtensionsPageModel* model() { return model_; }

  // Set the delegate for page actions.
  void SetDelegate(AstraExtensionsPageDelegate* delegate) {
    delegate_ = delegate;
  }
  AstraExtensionsPageDelegate* delegate() { return delegate_; }

  // -- Layout mode ---------------------------------------------------------

  void SetDisplayMode(bool compact);
  bool IsCompactMode() const { return compact_mode_; }

  // Refresh everything from the model.
  void RefreshFromModel();

  // -- views::View ---------------------------------------------------------

  void OnThemeChanged() override;
  void Layout() override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;

  // -- views::TextfieldController ------------------------------------------

  void ContentsChanged(views::Textfield* sender,
                       const std::u16string& new_contents) override;
  bool HandleKeyEvent(views::Textfield* sender,
                      const ui::KeyEvent& key_event) override;

  // -- AstraExtensionsPageObserver -----------------------------------------

  void OnExtensionsChanged(AstraExtensionsPageModel* model) override;
  void OnFilterChanged(AstraExtensionsPageModel* model) override;
  void OnSearchChanged(AstraExtensionsPageModel* model,
                       const std::u16string& query) override;
  void OnExtensionsPageModelShutdown(AstraExtensionsPageModel* model) override;

  // Accessors for testing.
  views::Textfield* search_field() { return search_field_; }
  views::Combobox* sort_combobox() { return sort_combobox_; }
  views::View* categories_sidebar() { return categories_sidebar_; }
  views::ScrollView* content_scroll_view() { return content_scroll_view_; }
  views::View* extensions_container() { return extensions_container_; }
  views::View* empty_state() { return empty_state_; }
  views::MdTextButton* add_button() { return add_button_; }
  views::MdTextButton* webstore_button() { return webstore_button_; }
  int GetExtensionCardCount() const;
  AstraExtensionCardView* GetExtensionCardAt(int index) const;

 private:
  void BuildUI();
  void BuildToolbar();
  void BuildCategoriesSidebar();
  void BuildContentArea();
  void BuildEmptyState();

  // Refresh extension cards list.
  void RefreshExtensionCards();

  // Refresh categories sidebar.
  void RefreshCategories();

  // Update empty state visibility.
  void UpdateEmptyState();

  // Handle sort change.
  void OnSortChanged();

  // Handle display mode toggle.
  void OnDisplayModeToggled();

  // Handle category selection.
  void OnCategorySelected(const std::string& category_id);

  // Handle add extension button.
  void OnAddExtension();

  // Handle Chrome Web Store button.
  void OnOpenWebStore();

  // Handle manage shortcuts button.
  void OnManageShortcuts();

  // Draw icons.
  void DrawExtensionIcon(gfx::Canvas* canvas,
                         const gfx::Rect& bounds,
                         SkColor color,
                         const std::u16string& name);

  // Model (not owned).
  raw_ptr<AstraExtensionsPageModel> model_ = nullptr;

  // Delegate (not owned).
  raw_ptr<AstraExtensionsPageDelegate> delegate_ = nullptr;

  // Scoped observation.
  base::ScopedObservation<AstraExtensionsPageModel,
                          AstraExtensionsPageObserver>
      scoped_observation_{this};

  // Layout mode.
  bool compact_mode_ = false;

  // Toolbar.
  raw_ptr<views::View> toolbar_ = nullptr;
  raw_ptr<views::Textfield> search_field_ = nullptr;
  raw_ptr<views::Combobox> sort_combobox_ = nullptr;
  raw_ptr<views::ImageButton> view_mode_button_ = nullptr;
  raw_ptr<views::MdTextButton> add_button_ = nullptr;
  raw_ptr<views::MdTextButton> webstore_button_ = nullptr;

  // Sidebar.
  raw_ptr<views::View> categories_sidebar_ = nullptr;
  raw_ptr<views::View> categories_container_ = nullptr;
  raw_ptr<views::Label> categories_header_ = nullptr;

  // Content area.
  raw_ptr<views::ScrollView> content_scroll_view_ = nullptr;
  raw_ptr<views::View> extensions_container_ = nullptr;
  raw_ptr<views::View> empty_state_ = nullptr;
  raw_ptr<views::Label> empty_state_title_ = nullptr;
  raw_ptr<views::Label> empty_state_desc_ = nullptr;

  // Owned card views.
  std::vector<raw_ptr<AstraExtensionCardView, VectorExperimental>>
      extension_cards_;

  // Selected category (empty = all).
  std::string selected_category_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_EXTENSIONS_PAGE_ASTRA_EXTENSIONS_PAGE_VIEW_H_
