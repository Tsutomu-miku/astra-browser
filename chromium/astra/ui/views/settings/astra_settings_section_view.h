#ifndef ASTRA_UI_VIEWS_SETTINGS_ASTRA_SETTINGS_SECTION_VIEW_H_
#define ASTRA_UI_VIEWS_SETTINGS_ASTRA_SETTINGS_SECTION_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "ui/views/view.h"

namespace ui {
class ComboboxModel;
}  // namespace ui

namespace views {
class Combobox;
class ImageButton;
class Label;
class Slider;
class ToggleButton;
class MdTextButton;
class ImageView;
}  // namespace views

namespace astra {

class AstraSettingsSectionViewObserver;

// =========================================================================
// AstraSettingsSectionView — reusable settings section container
// =========================================================================
//
// AstraSettingsSectionView is a reusable container for a settings section.
// It consists of a section header (with icon, title, description,
// expand/collapse button, and setting count badge), and a vertical list
// of setting rows.
//
// Each section has a unique section ID, searchable text, and supports
// expand/collapse behavior.
//
// Expand/collapse: Sections can be expanded or collapsed by clicking the
// header.  When collapsed, only the header is visible and the content
// rows are hidden.
//
// Observer pattern: Observers can be notified when the section's expanded
// state changes.  All observer methods have default empty implementations.
//
// This is a pure presentation helper — it owns no state.  Values are
// read from and written to PrefService / Astra services by the owning
// page view.
//
// Chromium subsystems reused:
//   - views::View (base class)
//   - views::BoxLayout (section layout)
//   - views::ToggleButton, views::Combobox, views::Slider (controls)
//
// Chromium pattern reference:
//   chrome/browser/ui/views/settings/ — Chrome's native settings sections
//   chrome/browser/ui/views/controls/ — shared controls
//
// TODO(astra): Consider using Chrome's SettingsPage UI framework if we
//   ever migrate settings to a full page.  For the bubble-based quick
//   settings, this lightweight section view is sufficient.
// =========================================================================

class AstraSettingsSectionView : public views::View {
 public:
  // Observer for section expand/collapse events.
  // All methods have default empty implementations.
  class Observer {
   public:
    virtual void OnSectionExpandedChanged(AstraSettingsSectionView* section,
                                          bool expanded) {}

   protected:
    virtual ~Observer() = default;
  };

  // Construct a section with the given title.
  explicit AstraSettingsSectionView(const std::u16string& title);
  ~AstraSettingsSectionView() override;

  AstraSettingsSectionView(const AstraSettingsSectionView&) = delete;
  AstraSettingsSectionView& operator=(const AstraSettingsSectionView&) = delete;

  // -- Observer management -------------------------------------------------

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // -- Section identity ----------------------------------------------------

  // Set the section ID, title, and description.
  void SetSection(const std::string& section_id,
                  const std::u16string& title,
                  const std::u16string& description);

  // Returns the section ID.
  const std::string& GetSectionId() const { return section_id_; }

  // -- Section metadata ----------------------------------------------------

  // Set an optional description text shown below the header.
  void SetDescription(const std::u16string& description);

  // Add searchable keywords for this section.  These are matched against
  // the search query along with the title and row labels.
  void AddSearchKeyword(const std::u16string& keyword);
  void AddSearchKeywords(const std::vector<std::u16string>& keywords);

  // Returns true if this section matches the given search query.
  // Matches against title, description, keywords, and all row labels.
  bool MatchesSearch(const std::u16string& query) const;

  // Set the icon name for this section.
  void SetIconName(const std::string& icon_name);
  const std::string& icon_name() const { return icon_name_; }

  // -- Expand / collapse ---------------------------------------------------

  // Get/set whether the section is expanded (content rows visible).
  bool expanded() const { return expanded_; }
  void SetExpanded(bool expanded);
  void ToggleExpanded();

  // Enable or disable the expand/collapse button in the header.
  // When disabled, the section is always expanded and the button is hidden.
  void SetExpandable(bool expandable);
  bool expandable() const { return expandable_; }

  // -- Setting count -------------------------------------------------------

  // Set the number of settings in this section (shown as a badge).
  void SetSettingCount(int count);
  int setting_count() const { return setting_count_; }

  // -- Setting views -------------------------------------------------------

  // Add a setting row view.  The view is added to the rows container.
  // The section view takes ownership of the view.
  void AddSettingView(views::View* setting_view);

  // Clear all setting views from the section.
  void ClearSettingViews();

  // -- Row builders --------------------------------------------------------

  // Adds a labeled toggle row.  Returns the toggle button (owned by the
  // view hierarchy).  |callback| is called when the toggle is toggled.
  views::ToggleButton* AddToggleRow(
      const std::u16string& label,
      bool initial_value,
      base::RepeatingClosure callback);

  // Adds a labeled combobox (dropdown) row.  Returns the combobox.
  // |callback| is called when the selection changes.
  views::Combobox* AddComboboxRow(
      const std::u16string& label,
      std::unique_ptr<ui::ComboboxModel> model,
      base::RepeatingClosure callback);

  // Adds a labeled slider row with a value label on the right.
  // Returns the slider.  |callback| is called with the slider value.
  // |value_formatter| converts the slider value to a display string.
  using ValueFormatter = base::RepeatingCallback<std::u16string(double)>;
  views::Slider* AddSliderRow(
      const std::u16string& label,
      double initial_value,
      ValueFormatter value_formatter,
      base::RepeatingCallback<void(double)> callback);

  // Adds a labeled informational row with a text value on the right.
  // Returns the value label.
  views::Label* AddInfoRow(const std::u16string& label,
                           const std::u16string& value);

  // Adds a labeled button row (label on left, button on right).
  // Returns the button.
  views::MdTextButton* AddButtonRow(
      const std::u16string& label,
      const std::u16string& button_text,
      base::RepeatingClosure callback);

  // Adds a divider line.
  void AddDivider();

  // -- Accessors -----------------------------------------------------------

  const std::u16string& title() const { return title_; }

  // The container view for rows — use this if you need to add custom
  // content to the section.
  views::View* rows_container() { return rows_container_; }

  // The header row view.
  views::View* header_row() { return header_row_; }

  // -- views::View ---------------------------------------------------------

  void OnThemeChanged() override;

 private:
  // Creates a labeled row container (label + control).
  // Returns the row view.  The control is added as a child.
  views::View* CreateLabeledRow(const std::u16string& label_text,
                                std::unique_ptr<views::View> control);

  // Registers a row's label text for search matching.
  void RegisterRowLabel(const std::u16string& label);

  // Updates all labels' colors when the theme changes.
  void UpdateLabelColors();

  // Builds the header row with icon, title, expand button, and count badge.
  void BuildHeader();

  // Updates the expand button state based on current expanded state.
  void UpdateExpandButton();

  // Updates the setting count badge display.
  void UpdateSettingCountBadge();

  // Updates the section icon display.
  void UpdateIcon();

  // Notifies observers that the expanded state changed.
  void NotifyExpandedChanged();

  // Handler for the expand/collapse button.
  void OnExpandButtonPressed();

  // Section ID string.
  std::string section_id_;

  // Icon name for the section.
  std::string icon_name_;

  // Section title and description.
  std::u16string title_;
  std::u16string description_;

  // Search keywords and row labels for search matching.
  std::vector<std::u16string> keywords_;
  std::vector<std::u16string> row_labels_;

  // Whether the section is expanded (content visible).
  bool expanded_ = true;

  // Whether the section can be expanded/collapsed.
  bool expandable_ = true;

  // Number of settings shown in the badge.
  int setting_count_ = 0;

  // Header child views.
  raw_ptr<views::View> header_row_ = nullptr;
  raw_ptr<views::ImageView> icon_view_ = nullptr;
  raw_ptr<views::Label> header_label_ = nullptr;
  raw_ptr<views::MdTextButton> expand_button_ = nullptr;
  raw_ptr<views::Label> setting_count_badge_ = nullptr;
  raw_ptr<views::Label> description_label_ = nullptr;

  // Rows container.
  raw_ptr<views::View> rows_container_ = nullptr;

  // Observers.
  base::ObserverList<Observer>::Unchecked observers_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SETTINGS_ASTRA_SETTINGS_SECTION_VIEW_H_
