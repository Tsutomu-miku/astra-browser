#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_PASSWORD_ITEM_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_PASSWORD_ITEM_VIEW_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "ui/views/view.h"

#include "astra/browser/astra_password_helper.h"
#include "astra/ui/views/sidebar/astra_sidebar_item_view.h"

namespace views {
class ImageButton;
class ImageView;
class Label;
class ProgressBar;
}  // namespace views

namespace astra {

// Delegate interface for AstraPasswordItemView actions.
class AstraPasswordItemDelegate {
 public:
  virtual ~AstraPasswordItemDelegate() = default;

  // Called when the user clicks the password item (primary action).
  virtual void OnPasswordItemClicked(const AstraPasswordEntry& entry) = 0;

  // Called when the user clicks the copy password button.
  virtual void OnPasswordCopyRequested(const AstraPasswordEntry& entry) = 0;

  // Called when the user clicks the copy username button.
  virtual void OnUsernameCopyRequested(const AstraPasswordEntry& entry) = 0;

  // Called when the user clicks the reveal/hide password toggle.
  virtual void OnPasswordRevealToggled(const AstraPasswordEntry& entry,
                                       bool revealed) = 0;

  // Called when the user clicks "open in new tab" from the item.
  virtual void OnPasswordOpenInNewTab(const AstraPasswordEntry& entry) = 0;
};

// Type of warning badge to show on a password item.
enum class AstraPasswordWarningType {
  kNone,         // No warning
  kCompromised,  // Compromised / breached password
  kWeak,         // Weak password
  kReused,       // Reused password
};

// A single password item row in the sidebar passwords section.
// Shows a site icon, site name, username, password (hidden/revealable),
// strength indicator, warning badges, and hover action buttons.
//
// This is a pure presentation view — it does not own password state.
// Data is projected from Chromium's PasswordStore by the parent
// AstraSidebarPasswordsView.
//
// Security: The password value itself is never stored in this view.
// Only metadata (site, username, strength, health flags) is projected.
// The password display shows dots or the revealed state indicator only.
//
// TODO(astra): Replace placeholder icons with real vector icons from
//   chrome/browser/ui/vector_icons/.
//   Chromium owner: PasswordManagerUI
//     (chrome/browser/ui/webui/password_manager/)
class AstraPasswordItemView : public AstraSidebarItemView {
 public:
  // Construct from a full password entry.
  explicit AstraPasswordItemView(const AstraPasswordEntry& entry);

  // Legacy constructor for backward compatibility.
  AstraPasswordItemView(const std::u16string& site,
                        const std::u16string& username,
                        bool is_compromised);

  AstraPasswordItemView(const AstraPasswordItemView&) = delete;
  AstraPasswordItemView& operator=(const AstraPasswordItemView&) = delete;
  ~AstraPasswordItemView() override;

  // -- Password entry ----------------------------------------------------

  // Set all password info from an entry struct.
  void SetPasswordEntry(const AstraPasswordEntry& entry);

  // Get the underlying password entry data (projected copy).
  const AstraPasswordEntry& entry() const { return entry_; }

  // -- Password info (legacy getters for compatibility) ------------------

  const std::u16string& GetSite() const { return entry_.site_display_name; }
  const std::u16string& GetUsername() const { return entry_.username; }

  // -- Compromised / warning state ---------------------------------------

  // Set whether this password is marked as compromised.
  void SetCompromised(bool compromised);
  bool IsCompromised() const { return entry_.is_compromised; }

  // Set the primary warning type shown in the badge.
  void SetWarningType(AstraPasswordWarningType type);
  AstraPasswordWarningType GetWarningType() const { return warning_type_; }

  // -- Blocked state ------------------------------------------------------

  void SetIsBlocked(bool blocked);
  bool IsBlocked() const { return entry_.is_blocked; }

  // -- Password visibility (reveal) ---------------------------------------

  // Set whether the password is currently revealed (shown in plain text).
  // Note: This only controls the visual state — the actual password value
  // is never stored in this view.
  void SetPasswordRevealed(bool revealed);
  bool IsPasswordRevealed() const { return is_password_revealed_; }
  void TogglePasswordRevealed();

  // -- Strength indicator ------------------------------------------------

  // Set the password strength level.
  void SetStrength(AstraPasswordStrength strength);
  AstraPasswordStrength GetStrength() const { return entry_.strength; }

  // Set strength as a percentage (0-100).
  void SetStrengthPercent(int percent);
  int GetStrengthPercent() const { return entry_.strength_percent; }

  // Show or hide the strength indicator bar.
  void SetStrengthIndicatorVisible(bool visible);
  bool IsStrengthIndicatorVisible() const { return strength_visible_; }

  // -- Last used time ----------------------------------------------------

  // Set the last used time as a human-readable label (e.g. "Last used 2h ago").
  void SetLastUsedLabel(const std::u16string& label);
  const std::u16string& GetLastUsedLabel() const { return last_used_label_; }

  // Show or hide the last used time display.
  void SetLastUsedVisible(bool visible);
  bool IsLastUsedVisible() const { return last_used_visible_; }

  // -- Delegate -----------------------------------------------------------

  void set_delegate(AstraPasswordItemDelegate* delegate) {
    delegate_ = delegate;
  }
  AstraPasswordItemDelegate* delegate() const { return delegate_; }

  // -- views::View --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

 protected:
  // AstraSidebarItemView overrides.
  void BuildLayout() override;
  void OnItemClicked() override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;

 private:
  // Update the label texts from current entry state.
  void UpdateLabels();

  // Update the copy buttons' visibility based on hover/focus state.
  void UpdateActionButtonsVisibility();

  // Update the icon based on state (compromised, blocked).
  void UpdateIcon();

  // Update the strength indicator bar appearance.
  void UpdateStrengthIndicator();

  // Update the warning badge visibility and text.
  void UpdateWarningBadge();

  // Update the password display (dots or revealed indicator).
  void UpdatePasswordDisplay();

  // Update the last used label display.
  void UpdateLastUsedDisplay();

  // Button action handlers.
  void OnCopyPasswordButtonPressed();
  void OnCopyUsernameButtonPressed();
  void OnRevealButtonPressed();
  void OnOpenInNewTabButtonPressed();

  // The projected password entry data.
  // Note: The actual password value is never stored here.
  AstraPasswordEntry entry_;

  // Primary warning type shown in the badge.
  AstraPasswordWarningType warning_type_ = AstraPasswordWarningType::kNone;

  // Whether the password is currently revealed (visual state only).
  bool is_password_revealed_ = false;

  // Whether the strength indicator bar is visible.
  bool strength_visible_ = true;

  // Whether the last used time is visible.
  bool last_used_visible_ = true;

  // Human-readable last used label.
  std::u16string last_used_label_;

  // Action delegate. Not owned.
  raw_ptr<AstraPasswordItemDelegate> delegate_ = nullptr;

  // Child views (owned by the view hierarchy).
  raw_ptr<views::ImageButton> copy_password_button_ = nullptr;
  raw_ptr<views::ImageButton> copy_username_button_ = nullptr;
  raw_ptr<views::ImageButton> reveal_button_ = nullptr;
  raw_ptr<views::ImageButton> open_in_new_tab_button_ = nullptr;
  raw_ptr<views::View> strength_bar_container_ = nullptr;
  raw_ptr<views::Label> password_label_ = nullptr;
  raw_ptr<views::Label> warning_badge_ = nullptr;
  raw_ptr<views::Label> last_used_label_view_ = nullptr;

  // Internal hover state for showing/hiding action buttons.
  bool is_hovered_internal_ = false;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_PASSWORD_ITEM_VIEW_H_
