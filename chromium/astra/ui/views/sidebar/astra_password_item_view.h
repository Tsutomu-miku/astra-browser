#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_PASSWORD_ITEM_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_PASSWORD_ITEM_VIEW_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "ui/views/view.h"

#include "astra/ui/views/sidebar/astra_sidebar_item_view.h"

namespace views {
class ImageButton;
class Label;
}  // namespace views

namespace astra {

// Delegate interface for AstraPasswordItemView actions.
class AstraPasswordItemDelegate {
 public:
  virtual ~AstraPasswordItemDelegate() = default;

  // Called when the user clicks the password item (primary action).
  virtual void OnPasswordItemClicked(const std::u16string& site,
                                     const std::u16string& username) = 0;

  // Called when the user clicks the copy button.
  virtual void OnPasswordCopyRequested(const std::u16string& site,
                                       const std::u16string& username) = 0;
};

// A single password item row in the sidebar passwords section.
// Shows a site icon, site name, username, and (on hover) a copy button.
//
// This is a pure presentation view — it does not own password state.
// Data is projected from Chromium's PasswordStore by the parent
// AstraSidebarPasswordsView.
//
// Security: The password value itself is never stored in this view.
// Only metadata (site, username) is projected.
//
// TODO(astra): Replace placeholder icons with real vector icons from
//   chrome/browser/ui/vector_icons/.
//   Chromium owner: PasswordManagerUI
//     (chrome/browser/ui/webui/password_manager/)
class AstraPasswordItemView : public AstraSidebarItemView {
 public:
  AstraPasswordItemView(const std::u16string& site,
                        const std::u16string& username,
                        bool is_compromised);
  AstraPasswordItemView(const AstraPasswordItemView&) = delete;
  AstraPasswordItemView& operator=(const AstraPasswordItemView&) = delete;
  ~AstraPasswordItemView() override;

  // -- Password info ------------------------------------------------------

  // Set all password info at once.
  void SetPasswordInfo(const std::u16string& site,
                       const std::u16string& username,
                       bool is_compromised);

  // Get the site name/display name.
  const std::u16string& GetSite() const { return site_; }

  // Get the username.
  const std::u16string& GetUsername() const { return username_; }

  // -- Compromised state --------------------------------------------------

  // Set whether this password is marked as compromised.
  void SetCompromised(bool compromised);
  bool IsCompromised() const { return is_compromised_; }

  // -- Blocked state ------------------------------------------------------

  // Set whether this site is blocked (password saving disabled).
  void SetIsBlocked(bool blocked);
  bool IsBlocked() const { return is_blocked_; }

  // -- Delegate -----------------------------------------------------------

  void set_delegate(AstraPasswordItemDelegate* delegate) {
    delegate_ = delegate;
  }

  // -- views::View --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;

 protected:
  // AstraSidebarItemView overrides.
  void BuildLayout() override;
  void OnItemClicked() override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;

 private:
  // Update the label texts from current state.
  void UpdateLabels();

  // Update the copy button visibility.
  void UpdateCopyButtonVisibility();

  // Update the icon based on state (compromised, blocked).
  void UpdateIcon();

  // Button action handlers.
  void OnCopyButtonPressed();

  // Password metadata. Never stores actual password value.
  std::u16string site_;
  std::u16string username_;
  bool is_compromised_ = false;
  bool is_blocked_ = false;

  // Action delegate. Not owned.
  raw_ptr<AstraPasswordItemDelegate> delegate_ = nullptr;

  // Child views.
  raw_ptr<views::ImageButton> copy_button_ = nullptr;

  // Hover state for showing/hiding the copy button.
  bool is_hovered_internal_ = false;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_PASSWORD_ITEM_VIEW_H_
