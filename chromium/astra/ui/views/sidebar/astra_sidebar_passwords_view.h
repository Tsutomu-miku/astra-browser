#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_PASSWORDS_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_PASSWORDS_VIEW_H_

#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "ui/views/view.h"
#include "url/gurl.h"

#include "astra/browser/astra_password_helper.h"
#include "astra/ui/views/sidebar/astra_password_item_view.h"

class Profile;

namespace views {
class Label;
class LabelButton;
class Textfield;
class ToggleButton;
class Combobox;
}  // namespace views

namespace astra {

// A sidebar section that shows saved passwords, projected from Chromium's
// PasswordManagerService and PasswordStore.
//
// This is a presentation-only view. It never stores or mutates password
// state — it only queries and displays Chromium's password data, and
// delegates actions (copy, open site, settings) to the password helper
// or the browser.
//
// Layout (top to bottom):
//   - Section header ("Passwords") with count badge
//   - Search text field (filter passwords by site/username)
//   - Toolbar row: sort menu, filter menu, group by toggle, reveal all toggle
//   - List of password items (site name + username + copy button)
//   - "Show more" button (if more items available)
//   - "Password settings" footer link (opens chrome://settings/passwords)
//
// The view observes the password helper for changes and refreshes its
// projection whenever passwords are added, removed, or updated.
//
// Chromium owner: PasswordManagerService
//   (chrome/browser/password_manager/password_manager_service.h)
// Chromium store: PasswordStore
//   (components/password_manager/core/browser/password_store.h)
// Chromium WebUI: chrome://settings/passwords
//   (chrome/browser/ui/webui/settings/password_manager_handler.h)
class AstraSidebarPasswordsView
    : public views::View,
      public AstraPasswordHelperObserver,
      public AstraPasswordItemDelegate {
 public:
  // Delegate interface for actions that need browser-level context,
  // such as opening a URL in a tab. The parent sidebar view implements
  // this so the passwords section doesn't need direct access to Browser.
  class Delegate {
   public:
    virtual ~Delegate() = default;

    // Open a URL from a password entry.
    // |in_new_tab| controls whether the URL opens in the active tab or
    // a new foreground tab.
    virtual void OpenPasswordURL(const GURL& url, bool in_new_tab) = 0;

    // Open the full password settings page (chrome://settings/passwords).
    virtual void OpenPasswordSettings() = 0;

    // Open the password check page.
    virtual void OpenPasswordCheck() = 0;

    // Called when a password entry is selected/clicked.
    virtual void OnPasswordSelected(const AstraPasswordEntry& entry) = 0;

    // Called when the user requests to show more passwords.
    virtual void OnShowMorePasswords() = 0;
  };

  explicit AstraSidebarPasswordsView(Profile* profile);
  AstraSidebarPasswordsView(const AstraSidebarPasswordsView&) = delete;
  AstraSidebarPasswordsView& operator=(const AstraSidebarPasswordsView&) = delete;
  ~AstraSidebarPasswordsView() override;

  // Set the delegate for navigation actions. Not owned.
  void set_delegate(Delegate* delegate) { delegate_ = delegate; }
  Delegate* delegate() const { return delegate_; }

  // Refresh the password list from the password helper.
  // Initiates a query; results arrive via OnPasswordsChanged.
  void Refresh();

  // Set the maximum number of password items to show before showing
  // the "Show more" button.
  void set_max_items(size_t max) { max_items_ = max; }
  size_t max_items() const { return max_items_; }

  // Show "Show more" button when there are more items than max_items_.
  void SetShowMoreVisible(bool visible);
  bool IsShowMoreVisible() const;

  // -- Sort / filter / group controls ------------------------------------

  // Set the current sort order. Updates the UI and refreshes the list.
  void SetSortOrder(AstraPasswordSortOrder order);
  AstraPasswordSortOrder GetSortOrder() const;

  // Set the current filter. Updates the UI and refreshes the list.
  void SetFilter(AstraPasswordFilter filter);
  AstraPasswordFilter GetFilter() const;

  // Set the current grouping mode.
  void SetGroupBy(AstraPasswordGroupBy group_by);
  AstraPasswordGroupBy GetGroupBy() const;

  // Toggle whether all passwords are revealed in the list.
  void SetAllRevealed(bool revealed);
  bool AreAllRevealed() const;

  // -- AstraPasswordHelperObserver ---------------------------------------

  void OnPasswordsChanged() override;
  void OnPasswordSettingsChanged() override;
  void OnPasswordHealthChanged() override;

  // -- AstraPasswordItemDelegate ----------------------------------------

  void OnPasswordItemClicked(const AstraPasswordEntry& entry) override;
  void OnPasswordCopyRequested(const AstraPasswordEntry& entry) override;
  void OnUsernameCopyRequested(const AstraPasswordEntry& entry) override;
  void OnPasswordRevealToggled(const AstraPasswordEntry& entry,
                               bool revealed) override;
  void OnPasswordOpenInNewTab(const AstraPasswordEntry& entry) override;

  // -- views::View --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

  // -- Loading and empty states -------------------------------------------

  // Show the loading state indicator.
  void ShowLoadingState();

  // Hide the loading state indicator.
  void HideLoadingState();

  // Show the empty state (no saved passwords).
  void ShowEmptyState();

  // Hide the empty state.
  void HideEmptyState();

  // Update the visibility of items, loading indicator, and empty state
  // based on the current number of passwords.
  void UpdateStateVisibility();

  // -- Health summary ----------------------------------------------------

  // Show or hide the password health summary row.
  void SetHealthSummaryVisible(bool visible);
  bool IsHealthSummaryVisible() const;

  // Update the health summary display from the helper.
  void UpdateHealthSummary();

  // -- Item access (for testing) -----------------------------------------

  size_t GetItemCount() const;
  AstraPasswordItemView* GetItemAt(size_t index) const;
  views::View* items_container() { return items_container_; }
  views::Textfield* search_field() { return search_field_; }
  views::Label* count_badge() { return count_badge_; }
  views::Label* empty_state_label() { return empty_state_label_; }
  views::Label* loading_label() { return loading_label_; }
  views::LabelButton* settings_link() { return settings_link_; }

 private:
  // Build the child views and layout. Called once from constructor.
  void BuildLayout();

  // Get the password helper from the profile. Returns nullptr if not available.
  // TODO(astra): Obtain via a proper factory pattern. For now, create
  //   a helper instance on-demand or use the profile's keyed service.
  AstraPasswordHelper* GetPasswordHelper();

  // Populate the items container with password entries.
  // Clears existing items and rebuilds from the current list.
  void PopulateItems(const std::vector<AstraPasswordEntry>& entries);

  // Clear all password items from the list.
  void ClearItems();

  // Update the header count badge with the current number of saved passwords.
  void UpdateCountBadge();

  // Update the header title to show search state (e.g. "Results: 3").
  void UpdateHeaderForSearch();

  // Callback for the "Password settings" link.
  void OnPasswordSettingsClicked();

  // Callback for the "Show more" button.
  void OnShowMoreClicked();

  // Callback for search text field changes.
  void OnSearchTextChanged();

  // Callback for sort order changes.
  void OnSortOrderChanged();

  // Callback for filter changes.
  void OnFilterChanged();

  // Callback for group by toggle.
  void OnGroupByToggled();

  // Callback for reveal all toggle.
  void OnRevealAllToggled();

  // The profile this view is associated with. Not owned.
  raw_ptr<Profile> profile_;

  // Delegate for navigation actions. Not owned.
  raw_ptr<Delegate> delegate_ = nullptr;

  // Password helper for querying and operating on passwords.
  // Not owned — obtained from the profile.
  raw_ptr<AstraPasswordHelper> password_helper_ = nullptr;

  // Maximum number of password items to display before "Show more".
  size_t max_items_ = 20;

  // Whether all passwords are currently revealed (shown in plain text).
  bool all_revealed_ = false;

  // Current search query text.
  std::u16string search_query_;

  // Loading state indicator.
  bool is_loading_ = false;

  // Child views (owned by the view hierarchy).
  raw_ptr<views::Label> header_label_ = nullptr;
  raw_ptr<views::Label> count_badge_ = nullptr;
  raw_ptr<views::Textfield> search_field_ = nullptr;

  // Toolbar row with sort/filter/group controls.
  raw_ptr<views::View> toolbar_row_ = nullptr;
  raw_ptr<views::Combobox> sort_combobox_ = nullptr;
  raw_ptr<views::Combobox> filter_combobox_ = nullptr;
  raw_ptr<views::ToggleButton> group_toggle_ = nullptr;
  raw_ptr<views::ToggleButton> reveal_all_toggle_ = nullptr;

  // Health summary row.
  raw_ptr<views::View> health_summary_row_ = nullptr;
  raw_ptr<views::Label> health_summary_label_ = nullptr;

  // Items container.
  raw_ptr<views::View> items_container_ = nullptr;

  // "Show more" button.
  raw_ptr<views::LabelButton> show_more_button_ = nullptr;

  // Footer: "Password settings" link.
  raw_ptr<views::LabelButton> settings_link_ = nullptr;

  // Loading state indicator.
  raw_ptr<views::Label> loading_label_ = nullptr;

  // Empty state message.
  raw_ptr<views::Label> empty_state_label_ = nullptr;

  // Weak pointer factory for async callbacks.
  base::WeakPtrFactory<AstraSidebarPasswordsView> weak_ptr_factory_{this};
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_PASSWORDS_VIEW_H_
