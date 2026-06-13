// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_SITE_SETTINGS_ASTRA_SITE_SETTINGS_VIEW_H_
#define ASTRA_UI_VIEWS_SITE_SETTINGS_ASTRA_SITE_SETTINGS_VIEW_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "ui/views/view.h"
#include "astra/ui/views/site_settings/astra_site_settings_model.h"

namespace views {
class BoxLayout;
class Button;
class Combobox;
class ImageButton;
class ImageView;
class Label;
class MdTextButton;
class Textfield;
}  // namespace views

namespace astra {

// =========================================================================
// AstraSiteSettingsPageView — site settings / permissions page
// =========================================================================
//
// Full-page view for managing site permissions and site data.
// Corresponds to Chrome's "Site settings" and "All sites" settings pages.
//
// Layout:
//
//   +-----------------------------------------------------------+
//   |  [←]  Site settings               [🔍 search...] [⋮]     |
//   +-----------+-----------------------------------------------+
//   |  All sites |  Header: "12 sites"    [Sort v] [Filter v] |
//   |  Cookies   |                                               |
//   |  Location  |  ┌───────────────────────────────────────┐  |
//   |  Camera    |  │ 🔒 Google               15 MB  42 🍪  │  |
//   |  Mic       |  │    Camera: allowed  Notifications: allowed │
//   |  ...       |  └───────────────────────────────────────┘  |
//   |           |  ┌───────────────────────────────────────┐  |
//   |           |  │ 📺 YouTube               8 MB  18 🍪  │  |
//   |           |  │    Autoplay: allowed                   │  |
//   |           |  └───────────────────────────────────────┘  |
//   +-----------+-----------------------------------------------+
//
// Chromium subsystems reused:
//   - HostContentSettingsMap (truth source for permissions)
//   - views::View (base view class)
//   - views::Combobox / views::Textfield (controls)
//
// TODO(astra): Wire to HostContentSettingsMap for real permission data.
//   Patch point: chrome/browser/ui/webui/settings/site_settings_handler.h
//   Chromium owner: components/content_settings/core/browser
// =========================================================================

// Forward declarations.
class AstraSiteSettingsObserver;

// A single site row in the site list.
class AstraSiteRowView : public views::View {
 public:
  explicit AstraSiteRowView(const AstraSiteSettingsEntry& site);
  ~AstraSiteRowView() override;

  AstraSiteRowView(const AstraSiteRowView&) = delete;
  AstraSiteRowView& operator=(const AstraSiteRowView&) = delete;

  // Update the row with new site data.
  void Update(const AstraSiteSettingsEntry& site);

  // Accessors.
  const std::string& site_id() const { return site_id_; }

  // views::View:
  void OnThemeChanged() override;

 private:
  void BuildUI();
  void DrawSiteIcon(gfx::Canvas* canvas, const gfx::Rect& bounds);
  void DrawFaviconFallback(gfx::Canvas* canvas, const gfx::Rect& bounds);

  std::string site_id_;
  std::u16string display_name_;
  std::string origin_;

  raw_ptr<views::ImageView> icon_view_ = nullptr;
  raw_ptr<views::Label> name_label_ = nullptr;
  raw_ptr<views::Label> origin_label_ = nullptr;
  raw_ptr<views::Label> storage_label_ = nullptr;
  raw_ptr<views::Label> permissions_label_ = nullptr;
  raw_ptr<views::ImageButton> more_button_ = nullptr;
};

// Sidebar category item.
class AstraSiteSettingsCategoryView : public views::View {
 public:
  AstraSiteSettingsCategoryView(AstraSiteSettingsCategory category,
                                const std::u16string& name,
                                bool is_active);
  ~AstraSiteSettingsCategoryView() override;

  AstraSiteSettingsCategoryView(const AstraSiteSettingsCategoryView&) = delete;
  AstraSiteSettingsCategoryView& operator=(
      const AstraSiteSettingsCategoryView&) = delete;

  void SetActive(bool active);
  AstraSiteSettingsCategory category() const { return category_; }

  // views::View:
  void OnThemeChanged() override;

 private:
  void BuildUI();
  void DrawIcon(gfx::Canvas* canvas, const gfx::Rect& bounds);

  AstraSiteSettingsCategory category_;
  std::u16string name_;
  bool is_active_ = false;

  raw_ptr<views::ImageView> icon_view_ = nullptr;
  raw_ptr<views::Label> label_ = nullptr;
};

// Main page view.
class AstraSiteSettingsPageView : public views::View,
                                  public AstraSiteSettingsObserver {
 public:
  explicit AstraSiteSettingsPageView(AstraSiteSettingsModel* model);
  ~AstraSiteSettingsPageView() override;

  AstraSiteSettingsPageView(const AstraSiteSettingsPageView&) = delete;
  AstraSiteSettingsPageView& operator=(const AstraSiteSettingsPageView&) =
      delete;

  // Set the model.
  void SetModel(AstraSiteSettingsModel* model);
  AstraSiteSettingsModel* model() { return model_; }

  // Refresh from model.
  void RefreshFromModel();

  // -- AstraSiteSettingsObserver -------------------------------------------

  void OnSitesChanged(AstraSiteSettingsModel* model) override;
  void OnCategoryChanged(AstraSiteSettingsModel* model,
                         AstraSiteSettingsCategory category) override;
  void OnSearchQueryChanged(AstraSiteSettingsModel* model,
                            const std::string& query) override;
  void OnSitePermissionChanged(AstraSiteSettingsModel* model,
                               const std::string& site_id,
                               AstraSitePermissionType type,
                               AstraContentSetting setting) override;
  void OnSiteSettingsModelShutdown(
      AstraSiteSettingsModel* model) override;

  // -- views::View ---------------------------------------------------------

  void OnThemeChanged() override;

  // Accessors for testing.
  views::Textfield* search_field() { return search_field_; }
  views::Combobox* sort_combobox() { return sort_combobox_; }
  views::Combobox* filter_combobox() { return filter_combobox_; }
  views::View* sidebar_ = nullptr;
  views::View* site_list_ = nullptr;
  views::Label* header_label() { return header_label_; }
  views::MdTextButton* reset_permissions_button() {
    return reset_permissions_button_;
  }
  views::MdTextButton* clear_data_button() { return clear_data_button_; }
  views::ImageButton* back_button() { return back_button_; }

 private:
  void BuildUI();
  void BuildHeader();
  void BuildSidebar();
  void BuildContent();
  void BuildSiteList();
  void BuildEmptyState();

  // Update content from model.
  void UpdateSiteList();
  void UpdateSidebar();
  void UpdateHeader();

  // Event handlers.
  void OnSearchChanged();
  void OnSortChanged();
  void OnFilterChanged();
  void OnCategoryClicked(AstraSiteSettingsCategory category);
  void OnBackClicked();
  void OnResetPermissionsClicked();
  void OnClearDataClicked();

  // Draw sidebar icons.
  static void DrawCategoryIcon(gfx::Canvas* canvas,
                               const gfx::Rect& bounds,
                               AstraSiteSettingsCategory category,
                               SkColor color);

  // Model (not owned).
  raw_ptr<AstraSiteSettingsModel> model_ = nullptr;

  // Scoped observation.
  base::ScopedObservation<AstraSiteSettingsModel,
                          AstraSiteSettingsObserver>
      scoped_observation_{this};

  // Header.
  raw_ptr<views::ImageButton> back_button_ = nullptr;
  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::Textfield> search_field_ = nullptr;
  raw_ptr<views::ImageButton> more_button_ = nullptr;

  // Sidebar.
  raw_ptr<views::View> sidebar_view_ = nullptr;

  // Content header.
  raw_ptr<views::View> content_header_ = nullptr;
  raw_ptr<views::Label> header_label_ = nullptr;
  raw_ptr<views::Combobox> sort_combobox_ = nullptr;
  raw_ptr<views::Combobox> filter_combobox_ = nullptr;

  // Content.
  raw_ptr<views::View> content_view_ = nullptr;
  raw_ptr<views::ScrollView> scroll_view_ = nullptr;
  raw_ptr<views::View> site_list_view_ = nullptr;
  raw_ptr<views::View> empty_view_ = nullptr;

  // Action buttons (in detail view or toolbar).
  raw_ptr<views::MdTextButton> reset_permissions_button_ = nullptr;
  raw_ptr<views::MdTextButton> clear_data_button_ = nullptr;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SITE_SETTINGS_ASTRA_SITE_SETTINGS_VIEW_H_
