// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_EXTENSIONS_MENU_ASTRA_EXTENSIONS_MENU_VIEW_H_
#define ASTRA_UI_VIEWS_EXTENSIONS_MENU_ASTRA_EXTENSIONS_MENU_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/metadata/view_factory.h"
#include "ui/views/view.h"

#include "astra/ui/views/extensions_menu/astra_extensions_menu_model.h"

namespace views {
class ImageButton;
class Label;
class Textfield;
class ScrollView;
}  // namespace views

namespace astra {

// Delegate for AstraExtensionsMenuView events.
class AstraExtensionsMenuDelegate {
 public:
  virtual ~AstraExtensionsMenuDelegate() = default;

  // Called when an extension is clicked (primary action / open popup).
  virtual void OnExtensionClicked(const std::string& extension_id) = 0;

  // Called when the pin/unpin button is clicked for an extension.
  virtual void OnExtensionPinToggled(const std::string& extension_id,
                                     bool pinned) = 0;

  // Called when "Manage extension" is selected from the context menu.
  virtual void OnManageExtension(const std::string& extension_id) = 0;

  // Called when "Remove extension" is selected from the context menu.
  virtual void OnRemoveExtension(const std::string& extension_id) = 0;

  // Called when "Visit Chrome Web Store" is selected.
  virtual void OnVisitChromeWebStore() {}

  // Called when "Manage extensions" (settings page) is selected.
  virtual void OnManageExtensionsPage() {}
};

// Individual extension item in the extensions menu.
//
// Shows: icon, name, description (optional), pin button, badge.
//
// Chromium owner: ExtensionsMenuItemView
//   (chrome/browser/ui/views/extensions/extensions_menu_item_view.h)
class AstraExtensionsMenuItemView : public views::View {
 public:
  METADATA_HEADER(AstraExtensionsMenuItemView);

  AstraExtensionsMenuItemView(const AstraExtensionMenuEntry& entry,
                              AstraExtensionsMenuDelegate* delegate);
  AstraExtensionsMenuItemView(const AstraExtensionsMenuItemView&) = delete;
  AstraExtensionsMenuItemView& operator=(const AstraExtensionsMenuItemView&) =
      delete;
  ~AstraExtensionsMenuItemView() override;

  // -- Entry update ---------------------------------------------------------

  void UpdateFromEntry(const AstraExtensionMenuEntry& entry);

  // -- Accessors ------------------------------------------------------------

  const std::string& extension_id() const { return extension_id_; }

  // -- views::View: --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void Layout() override;
  void OnThemeChanged() override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;
  void OnGestureEvent(ui::GestureEvent* event) override;

 private:
  // Handle primary click on the extension item.
  void HandlePrimaryClick();

  // Toggle pin state.
  void TogglePinned();

  // Update visual state.
  void UpdateVisuals();

  // Update icon display.
  void UpdateIcon();

  std::string extension_id_;
  std::u16string name_;
  std::u16string description_;
  gfx::ImageSkia icon_;
  AstraExtensionState state_;
  bool pinned_ = false;
  bool has_badge_ = false;
  std::u16string badge_text_;
  SkColor badge_color_ = SK_ColorRED;

  raw_ptr<AstraExtensionsMenuDelegate> delegate_ = nullptr;

  // Child views.
  raw_ptr<views::ImageView> icon_view_ = nullptr;
  raw_ptr<views::Label> name_label_ = nullptr;
  raw_ptr<views::Label> description_label_ = nullptr;
  raw_ptr<views::ImageButton> pin_button_ = nullptr;
  raw_ptr<views::ImageView> badge_view_ = nullptr;

  bool hovered_ = false;

  static constexpr int kIconSize = 28;
  static constexpr int kItemHeight = 48;
  static constexpr int kItemPadding = 12;
};

// The main extensions menu bubble view.
//
// Features:
//   - Search box at top
//   - Section: Pinned extensions
//   - Section: Active extensions
//   - Section: Inactive extensions
//   - Section: Blocked extensions
//   - Footer: "Manage extensions" + "Visit Chrome Web Store"
//
// Chromium owner: ExtensionsMenuBubbleView
//   (chrome/browser/ui/views/extensions/extensions_menu_bubble_view.h)
//
// TODO(astra): Integrate with toolbar via a patch to
// chrome/browser/ui/views/toolbar/toolbar_view.h — add Astra's
// extensions menu button and bubble delegate.
class AstraExtensionsMenuView : public views::BubbleDialogDelegateView,
                                public views::TextfieldController,
                                public AstraExtensionsMenuObserver,
                                public AstraExtensionsMenuDelegate {
 public:
  METADATA_HEADER(AstraExtensionsMenuView);

  explicit AstraExtensionsMenuView(views::View* anchor_view);
  AstraExtensionsMenuView(const AstraExtensionsMenuView&) = delete;
  AstraExtensionsMenuView& operator=(const AstraExtensionsMenuView&) = delete;
  ~AstraExtensionsMenuView() override;

  // -- Model binding --------------------------------------------------------

  void SetModel(AstraExtensionsMenuModel* model);
  AstraExtensionsMenuModel* model() const { return model_; }

  // -- Delegate -------------------------------------------------------------

  void SetDelegate(AstraExtensionsMenuDelegate* delegate) {
    outer_delegate_ = delegate;
  }

  // -- Search ---------------------------------------------------------------

  void SetSearchQuery(const std::u16string& query);
  std::u16string GetSearchQuery() const;

  // -- views::BubbleDialogDelegateView: -----------------------------------

  std::u16string GetWindowTitle() const override;

  // -- views::View: --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;

  // -- views::TextfieldController: -----------------------------------------

  void ContentsChanged(views::Textfield* sender,
                       const std::u16string& new_contents) override;
  bool HandleKeyEvent(views::Textfield* sender,
                      const ui::KeyEvent& key_event) override;

  // -- AstraExtensionsMenuObserver: ----------------------------------------

  void OnExtensionsChanged(AstraExtensionsMenuModel* model) override;
  void OnExtensionChanged(AstraExtensionsMenuModel* model,
                          const std::string& extension_id) override;
  void OnExtensionsMenuModelShutdown(
      AstraExtensionsMenuModel* model) override;

  // -- AstraExtensionsMenuDelegate: ----------------------------------------

  void OnExtensionClicked(const std::string& extension_id) override;
  void OnExtensionPinToggled(const std::string& extension_id,
                             bool pinned) override;
  void OnManageExtension(const std::string& extension_id) override;
  void OnRemoveExtension(const std::string& extension_id) override;
  void OnVisitChromeWebStore() override;
  void OnManageExtensionsPage() override;

  // -- Test helpers ---------------------------------------------------------

  views::Textfield* search_box_for_test() { return search_box_; }
  views::ScrollView* scroll_view_for_test() { return scroll_view_; }
  size_t GetItemViewCountForTest() const;

 private:
  // Build the UI.
  void Build();

  // Rebuild all extension items from the model.
  void RebuildItems();

  // Update all extension items from the model.
  void UpdateItems();

  // Update a specific extension item.
  void UpdateExtensionItem(const std::string& extension_id);

  // Build a section header.
  std::unique_ptr<views::View> CreateSectionHeader(const std::u16string& title,
                                                    size_t count);

  // Build extension items for a category.
  void BuildCategoryItems(AstraExtensionCategory category,
                          const std::u16string& title,
                          views::View* container);

  // Find the item view for a specific extension ID.
  AstraExtensionsMenuItemView* FindItemView(
      const std::string& extension_id) const;

  // Called when "Manage extensions" is clicked.
  void OnManageExtensionsClicked();

  // Called when "Chrome Web Store" is clicked.
  void OnChromeWebStoreClicked();

  raw_ptr<AstraExtensionsMenuModel> model_ = nullptr;
  raw_ptr<AstraExtensionsMenuDelegate> outer_delegate_ = nullptr;

  base::ScopedObservation<AstraExtensionsMenuModel,
                          AstraExtensionsMenuObserver>
      model_observation_{this};

  // Child views.
  raw_ptr<views::Textfield> search_box_ = nullptr;
  raw_ptr<views::ScrollView> scroll_view_ = nullptr;
  raw_ptr<views::View> content_container_ = nullptr;
  raw_ptr<views::View> footer_ = nullptr;

  // All item views (owned by the view hierarchy).
  std::vector<raw_ptr<AstraExtensionsMenuItemView>> item_views_;

  static constexpr int kBubbleWidth = 320;
  static constexpr int kBubbleMaxHeight = 480;
  static constexpr int kSearchBoxHeight = 36;
  static constexpr int kSectionHeaderHeight = 32;
  static constexpr int kFooterHeight = 40;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_EXTENSIONS_MENU_ASTRA_EXTENSIONS_MENU_VIEW_H_
