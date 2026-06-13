#include "astra/ui/views/sidebar/astra_sidebar_notes_view.h"

#include <memory>
#include <utility>

#include "astra/ui/color/astra_color_ids.h"
#include "base/i18n/time_formatting.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"
#include "url/gurl.h"

namespace astra {

namespace {

// Layout constants.
constexpr int kNotesSectionTopPadding = 0;
constexpr int kNotesSectionBottomPadding = 8;
constexpr int kNotesHeaderHeight = 28;
constexpr int kNotesHeaderHorizontalPadding = 12;
constexpr int kNotesHeaderVerticalPadding = 8;
constexpr int kNotesHeaderFontSizeDelta = 1;
constexpr int kNotesGroupSpacing = 8;
constexpr int kNotesItemSpacing = 2;
constexpr int kSearchFieldHeight = 28;
constexpr int kSearchFieldHorizontalPadding = 12;
constexpr int kSubHeaderFontSizeDelta = 0;
constexpr int kSubHeaderVerticalPadding = 6;
constexpr int kSubHeaderHorizontalPadding = 12;
constexpr int kNotesEditorTopSpacing = 8;

// Astra color IDs for the notes panel.
// Uses the Astra sidebar color system from astra/ui/color/astra_color_ids.h.
// Chromium subsystem: ui::ColorProvider (ui/color/color_provider.h)
constexpr ui::ColorId kNotesHeaderTextColorId =
    kColorAstraSidebarSectionHeaderText;
constexpr ui::ColorId kNotesSubHeaderTextColorId =
    kColorAstraSidebarSectionHeaderText;
constexpr ui::ColorId kNotesEmptyTextColorId =
    kColorAstraSidebarItemSecondaryText;

// Titles.
const char16_t kNotesHeaderTitle[] = u"Notes";
const char16_t kNewNoteButtonLabel[] = u"+";
const char16_t kPageNotesTitle[] = u"Page notes";
const char16_t kAllNotesTitle[] = u"All notes";
const char16_t kSearchPlaceholder[] = u"Search notes...";
const char16_t kNoNotesLabel[] = u"No notes yet";
const char16_t kNoPageNotesLabel[] = u"No notes for this page";

// Max characters for the content snippet.
constexpr size_t kMaxSnippetLength = 80;

}  // namespace

// =========================================================================
// Construction / destruction
// =========================================================================

AstraSidebarNotesView::AstraSidebarNotesView(AstraNoteService* note_service)
    : note_service_(note_service) {
  BuildLayout();

  if (note_service_) {
    service_observation_.Observe(note_service_);
    // Initial population.
    UpdateFromService();
  }
}

AstraSidebarNotesView::~AstraSidebarNotesView() = default;

// =========================================================================
// Layout
// =========================================================================

void AstraSidebarNotesView::BuildLayout() {
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(kNotesSectionTopPadding, 0),
      kNotesGroupSpacing));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // -- Header row: title + new note button --

  header_row_ = AddChildView(std::make_unique<views::View>());
  auto* header_layout = header_row_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets::VH(kNotesHeaderVerticalPadding,
                          kNotesHeaderHorizontalPadding),
          8));
  header_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  header_label_ = header_row_->AddChildView(
      std::make_unique<views::Label>(kNotesHeaderTitle));
  header_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  header_label_->SetAutoColorReadabilityEnabled(false);
  header_label_->SetFontList(
      header_label_->font_list().DeriveWithSizeDelta(
          kNotesHeaderFontSizeDelta));
  header_layout->SetFlexForView(header_label_, 1);

  new_note_button_ = header_row_->AddChildView(
      std::make_unique<views::LabelButton>(
          base::BindRepeating(
              &AstraSidebarNotesView::OnNewNoteButtonPressed,
              base::Unretained(this)),
          kNewNoteButtonLabel));
  new_note_button_->SetTooltipText(u"New note");
  new_note_button_->SetAccessibleName(u"New note");

  // -- Search field --

  search_field_ = AddChildView(std::make_unique<views::Textfield>());
  search_field_->SetPlaceholderText(kSearchPlaceholder);
  search_field_->SetController(this);
  search_field_->SetAccessibleName(u"Search notes");
  search_field_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(0, kSearchFieldHorizontalPadding)));
  search_field_->SetPreferredSize(gfx::Size(0, kSearchFieldHeight));

  // -- List container (page notes + all notes) --

  list_container_ = AddChildView(std::make_unique<views::View>());
  auto* list_layout = list_container_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical, gfx::Insets(),
          kNotesGroupSpacing));
  list_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // Page notes section.
  page_notes_section_ =
      list_container_->AddChildView(std::make_unique<views::View>());
  auto* page_section_layout = page_notes_section_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical, gfx::Insets(),
          kNotesItemSpacing));
  page_section_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  page_notes_label_ = page_notes_section_->AddChildView(
      std::make_unique<views::Label>(kPageNotesTitle));
  page_notes_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  page_notes_label_->SetAutoColorReadabilityEnabled(false);
  page_notes_label_->SetFontList(
      page_notes_label_->font_list().DeriveWithSizeDelta(
          kSubHeaderFontSizeDelta));
  page_notes_label_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(kSubHeaderVerticalPadding,
                      kSubHeaderHorizontalPadding)));

  page_notes_container_ = page_notes_section_->AddChildView(
      std::make_unique<views::View>());
  auto* page_items_layout = page_notes_container_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical, gfx::Insets(),
          kNotesItemSpacing));
  page_items_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // Page notes empty state.
  page_notes_empty_label_ = page_notes_section_->AddChildView(
      std::make_unique<views::Label>(kNoPageNotesLabel));
  page_notes_empty_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  page_notes_empty_label_->SetAutoColorReadabilityEnabled(false);
  page_notes_empty_label_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(kSubHeaderVerticalPadding,
                      kSubHeaderHorizontalPadding + 8)));
  page_notes_empty_label_->SetVisible(false);

  // All notes section.
  auto* all_notes_section =
      list_container_->AddChildView(std::make_unique<views::View>());
  auto* all_section_layout = all_notes_section->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical, gfx::Insets(),
          kNotesItemSpacing));
  all_section_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  all_notes_label_ = all_notes_section->AddChildView(
      std::make_unique<views::Label>(kAllNotesTitle));
  all_notes_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  all_notes_label_->SetAutoColorReadabilityEnabled(false);
  all_notes_label_->SetFontList(
      all_notes_label_->font_list().DeriveWithSizeDelta(
          kSubHeaderFontSizeDelta));
  all_notes_label_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(kSubHeaderVerticalPadding,
                      kSubHeaderHorizontalPadding)));

  all_notes_container_ = all_notes_section->AddChildView(
      std::make_unique<views::View>());
  auto* all_items_layout = all_notes_container_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical, gfx::Insets(),
          kNotesItemSpacing));
  all_items_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // All notes empty state.
  all_notes_empty_label_ = all_notes_section->AddChildView(
      std::make_unique<views::Label>(kNoNotesLabel));
  all_notes_empty_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  all_notes_empty_label_->SetAutoColorReadabilityEnabled(false);
  all_notes_empty_label_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(kSubHeaderVerticalPadding,
                      kSubHeaderHorizontalPadding + 8)));
  all_notes_empty_label_->SetVisible(false);

  // -- Loading state (hidden by default) --

  loading_view_ = AddChildView(std::make_unique<views::View>());
  auto* loading_layout = loading_view_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets::VH(16, kNotesHeaderHorizontalPadding), 0));
  loading_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  auto* loading_label = loading_view_->AddChildView(
      std::make_unique<views::Label>(u"Loading notes..."));
  loading_label->SetAutoColorReadabilityEnabled(false);
  loading_view_->SetVisible(false);

  // -- Note editor (hidden by default) --

  note_editor_ = AddChildView(std::make_unique<AstraNoteEditorView>(
      AstraNoteEditorView::Mode::kNew));
  note_editor_->set_delegate(this);
  note_editor_->SetVisible(false);

  // Initial state.
  SetExpanded(expanded_);
}

// =========================================================================
// Data population
// =========================================================================

void AstraSidebarNotesView::UpdateFromService() {
  if (!note_service_ || showing_editor_) {
    return;
  }

  // Clear existing items.
  page_notes_container_->RemoveAllChildViews();
  all_notes_container_->RemoveAllChildViews();

  // Get the search query, if any.
  std::u16string search_text_u16;
  if (search_field_) {
    search_text_u16 = search_field_->GetText();
  }
  std::string search_text = base::UTF16ToUTF8(search_text_u16);

  // Populate from service.
  if (search_text.empty()) {
    // No search: show page notes + all notes.
    PopulatePageNotes();
    PopulateAllNotes();
  } else {
    // Search mode: show matching notes in the all notes section,
    // hide page notes section.
    auto results = note_service_->SearchNotes(search_text);
    for (const auto& note : results) {
      all_notes_container_->AddChildView(CreateNoteItemView(note));
    }
    page_notes_section_->SetVisible(false);
  }

  // Show/hide sections based on whether they have items.
  size_t page_count = page_notes_container_->children().size();
  size_t all_count = all_notes_container_->children().size();

  // Show page notes section only if there's a current URL and either
  // there are page notes or we're not in search mode.
  bool has_current_url = current_url_.is_valid();
  page_notes_section_->SetVisible(has_current_url && search_text.empty());

  // Update empty states.
  UpdateEmptyStates();

  InvalidateLayout();
}

void AstraSidebarNotesView::PopulatePageNotes() {
  if (!note_service_ || !current_url_.is_valid()) {
    return;
  }

  auto notes = note_service_->GetNotesForUrl(current_url_);
  for (const auto& note : notes) {
    page_notes_container_->AddChildView(CreateNoteItemView(note));
  }
}

void AstraSidebarNotesView::PopulateAllNotes() {
  if (!note_service_) {
    return;
  }

  auto notes = GetFilteredAllNotes();
  for (const auto& note : notes) {
    all_notes_container_->AddChildView(CreateNoteItemView(note));
  }
}

std::unique_ptr<AstraNoteItemView>
AstraSidebarNotesView::CreateNoteItemView(const AstraNote& note) {
  std::u16 title = base::UTF8ToUTF16(note.title);
  std::u16 snippet = GetContentSnippet(note.content);
  std::u16 time_text = FormatTime(note.modified_time);

  auto item = std::make_unique<AstraNoteItemView>(
      note.id, title, snippet, time_text, note.color);
  item->set_delegate(this);

  return item;
}

// static
std::u16 AstraSidebarNotesView::FormatTime(base::Time time) {
  if (time.is_null()) {
    return u"";
  }

  base::Time now = base::Time::Now();
  base::TimeDelta delta = now - time;

  // Less than 1 minute ago.
  if (delta < base::Minutes(1)) {
    return u"Just now";
  }

  // Less than 1 hour ago.
  if (delta < base::Hours(1)) {
    int minutes = static_cast<int>(delta.InMinutes());
    return base::NumberToString16(minutes) + u"m ago";
  }

  // Less than 24 hours ago.
  if (delta < base::Hours(24)) {
    int hours = static_cast<int>(delta.InHours());
    return base::NumberToString16(hours) + u"h ago";
  }

  // Less than 7 days ago.
  if (delta < base::Days(7)) {
    int days = static_cast<int>(delta.InDays());
    if (days == 1) {
      return u"Yesterday";
    }
    return base::NumberToString16(days) + u"d ago";
  }

  // Older: show as a date.
  // TODO(astra): Use proper locale-aware date formatting.
  // Chromium pattern: base::TimeFormatShortDate or base::TimeFormatWithPattern.
  // For now, use a simple date string.
  return base::TimeFormatShortDate(time);
}

// static
std::u16 AstraSidebarNotesView::GetContentSnippet(
    const std::string& content) {
  if (content.empty()) {
    return u"";
  }

  // Take the first line of content.
  std::vector<std::string> lines = base::SplitString(
      content, "\n", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);

  std::string first_line;
  if (!lines.empty()) {
    first_line = lines[0];
  } else {
    first_line = content;
  }

  // Truncate to max snippet length.
  if (first_line.length() > kMaxSnippetLength) {
    first_line = first_line.substr(0, kMaxSnippetLength) + "...";
  }

  return base::UTF8ToUTF16(first_line);
}

// =========================================================================
// Public API
// =========================================================================

void AstraSidebarNotesView::SetCurrentUrl(const GURL& url) {
  if (current_url_ == url) {
    return;
  }
  current_url_ = url;

  // Refresh page notes section.
  if (!showing_editor_ && expanded_) {
    // Only refresh page notes; all notes are unchanged.
    // TODO(astra): Incremental update instead of full refresh.
    page_notes_container_->RemoveAllChildViews();
    PopulatePageNotes();
    page_notes_section_->SetVisible(current_url_.is_valid() &&
                                    search_field_ &&
                                    search_field_->GetText().empty());
    InvalidateLayout();
  }
}

void AstraSidebarNotesView::ShowNewNoteEditor(bool link_to_current_url) {
  if (!note_editor_) {
    return;
  }

  showing_editor_ = true;
  note_editor_->ClearEditor();
  note_editor_->SetVisible(true);
  list_container_->SetVisible(false);
  search_field_->SetVisible(false);

  // TODO(astra): Pre-link to current URL if requested.
  // The editor doesn't currently display the URL, but the note service
  // will accept a URL parameter when saving.
  if (link_to_current_url && current_url_.is_valid()) {
    // The URL association will happen when saving — we pass it through
    // the delegate. For now, we'll store it in a member or pass it
    // via the editor.
    //
    // TODO(astra): Add URL display/editing to the note editor.
    // For now, the URL is set silently on save.
  }

  InvalidateLayout();
}

void AstraSidebarNotesView::ShowNoteEditor(const std::string& note_id) {
  if (!note_service_ || !note_editor_) {
    return;
  }

  const AstraNote* note = note_service_->GetNote(note_id);
  if (!note) {
    return;
  }

  showing_editor_ = true;
  note_editor_->LoadNote(note->id, note->title, note->content, note->color);
  note_editor_->SetVisible(true);
  list_container_->SetVisible(false);
  search_field_->SetVisible(false);

  InvalidateLayout();
}

void AstraSidebarNotesView::HideEditor() {
  showing_editor_ = false;
  if (note_editor_) {
    note_editor_->SetVisible(false);
  }
  if (list_container_) {
    list_container_->SetVisible(true);
  }
  if (search_field_) {
    search_field_->SetVisible(true);
  }
  UpdateFromService();
}

void AstraSidebarNotesView::SetExpanded(bool expanded) {
  expanded_ = expanded;

  // Show/hide the content below the header.
  if (search_field_) {
    search_field_->SetVisible(expanded_);
  }
  if (list_container_) {
    list_container_->SetVisible(expanded_ && !showing_editor_);
  }
  if (note_editor_) {
    note_editor_->SetVisible(expanded_ && showing_editor_);
  }

  // TODO(astra): Update expand/collapse arrow icon on the header.

  InvalidateLayout();
}

// =========================================================================
// Event handlers
// =========================================================================

void AstraSidebarNotesView::OnHeaderClicked() {
  SetExpanded(!expanded_);
}

// =========================================================================
// AstraNoteServiceObserver
// =========================================================================

void AstraSidebarNotesView::OnNoteAdded(const AstraNote& note) {
  // TODO(astra): Incremental add instead of full rebuild.
  UpdateFromService();
}

void AstraSidebarNotesView::OnNoteUpdated(const AstraNote& note) {
  // TODO(astra): Incremental update instead of full rebuild.
  UpdateFromService();
}

void AstraSidebarNotesView::OnNoteRemoved(const std::string& note_id) {
  // TODO(astra): Incremental remove instead of full rebuild.
  UpdateFromService();
}

void AstraSidebarNotesView::OnNotesReloaded() {
  // Full reload — do a full populate.
  UpdateFromService();
}

// =========================================================================
// AstraNoteItemDelegate
// =========================================================================

void AstraSidebarNotesView::OnNoteItemClicked(const std::string& note_id) {
  if (delegate_) {
    delegate_->OnNoteClicked(note_id);
  }
  ShowNoteEditor(note_id);
}

void AstraSidebarNotesView::OnNoteDeleteRequested(const std::string& note_id) {
  if (delegate_) {
    delegate_->OnNoteDeleteRequested(note_id);
  }
  if (note_service_) {
    note_service_->DeleteNote(note_id);
    // UI updates via OnNoteRemoved observer.
  }
}

// =========================================================================
// AstraNoteEditorDelegate
// =========================================================================

std::string AstraSidebarNotesView::OnNoteEditorSave(
    const std::string& note_id,
    const std::string& title,
    const std::string& content,
    const std::string& color) {
  if (!note_service_) {
    return std::string();
  }

  if (note_id.empty()) {
    // New note.
    // If we have a current URL and the user hasn't explicitly unlinked it,
    // associate the note with the current page.
    // TODO(astra): Add a "link to page" toggle in the editor UI.
    GURL url_to_link;
    // For now, only link if there's a current URL and this is a new note.
    // In the future, this should be controlled by a UI toggle.
    if (current_url_.is_valid()) {
      url_to_link = current_url_;
    }

    std::string new_id = note_service_->AddNote(title, content, url_to_link,
                                                std::string(), color);
    // After saving, stay in the editor (now in edit mode) or return to list?
    // For a good UX, stay in editor so user can keep writing.
    // The editor mode is updated via the LoadNote call below.
    if (note_editor_ && !new_id.empty()) {
      const AstraNote* note = note_service_->GetNote(new_id);
      if (note) {
        note_editor_->LoadNote(note->id, note->title, note->content,
                               note->color);
      }
    }
    return new_id;
  } else {
    // Update existing note.
    // Read current note to preserve url and workspace_id.
    const AstraNote* existing = note_service_->GetNote(note_id);
    if (!existing) {
      return std::string();
    }
    AstraNote updated = *existing;
    updated.title = title;
    updated.content = content;
    updated.color = color;
    note_service_->UpdateNote(updated);
    return note_id;
  }
}

void AstraSidebarNotesView::OnNoteEditorCancel() {
  HideEditor();
}

void AstraSidebarNotesView::OnNoteEditorDelete(const std::string& note_id) {
  if (note_service_) {
    note_service_->DeleteNote(note_id);
  }
  HideEditor();
}

// =========================================================================
// TextfieldController (search)
// =========================================================================

void AstraSidebarNotesView::ContentsChanged(views::Textfield* sender,
                                             const std::u16string& new_contents) {
  // TODO(astra): Debounce search input with a timer to avoid excessive
  // rebuilds while typing. For now, update on every keystroke.
  if (delegate_) {
    delegate_->OnNoteSearchQueryChanged(base::UTF16ToUTF8(new_contents));
  }
  UpdateFromService();
}

// =========================================================================
// views::View overrides
// =========================================================================

gfx::Size AstraSidebarNotesView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return views::View::CalculatePreferredSize(available_size);
}

void AstraSidebarNotesView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  views::View::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kList;
  node_data->SetName("Notes");
}

void AstraSidebarNotesView::OnThemeChanged() {
  views::View::OnThemeChanged();

  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  SkColor header_color =
      color_provider->GetColor(kNotesHeaderTextColorId);
  SkColor sub_header_color =
      color_provider->GetColor(kNotesSubHeaderTextColorId);
  SkColor empty_text_color =
      color_provider->GetColor(kNotesEmptyTextColorId);

  if (header_label_) {
    header_label_->SetEnabledColor(header_color);
  }
  if (page_notes_label_) {
    page_notes_label_->SetEnabledColor(sub_header_color);
  }
  if (all_notes_label_) {
    all_notes_label_->SetEnabledColor(sub_header_color);
  }
  if (page_notes_empty_label_) {
    page_notes_empty_label_->SetEnabledColor(empty_text_color);
  }
  if (all_notes_empty_label_) {
    all_notes_empty_label_->SetEnabledColor(empty_text_color);
  }
}

// =========================================================================
// Sorting
// =========================================================================

void AstraSidebarNotesView::SetSortOrder(NoteSortOrder order) {
  if (sort_order_ == order) {
    return;
  }
  sort_order_ = order;
  if (note_service_ && !showing_editor_ && expanded_) {
    UpdateFromService();
  }
}

// =========================================================================
// Workspace filter
// =========================================================================

void AstraSidebarNotesView::SetWorkspaceFilter(
    const std::string& workspace_id) {
  if (workspace_filter_ == workspace_id) {
    return;
  }
  workspace_filter_ = workspace_id;
  if (note_service_ && !showing_editor_ && expanded_) {
    UpdateFromService();
  }
}

std::vector<AstraNote> AstraSidebarNotesView::GetFilteredAllNotes() const {
  if (!note_service_) {
    return std::vector<AstraNote>();
  }

  std::vector<AstraNote> notes;

  if (!workspace_filter_.empty()) {
    notes = note_service_->GetNotesByWorkspace(workspace_filter_);
  } else {
    notes = note_service_->GetAllNotes();
  }

  // Apply sort order if different from default.
  if (sort_order_ != NoteSortOrder::kDateDescending) {
    notes = note_service_->GetNotesSortedBy(sort_order_);
    // Re-filter by workspace if needed.
    if (!workspace_filter_.empty()) {
      std::vector<AstraNote> filtered;
      for (const auto& note : notes) {
        if (note.workspace_id == workspace_filter_) {
          filtered.push_back(note);
        }
      }
      notes = std::move(filtered);
    }
  } else if (!workspace_filter_.empty()) {
    // Already filtered by workspace above.
  }

  return notes;
}

// =========================================================================
// Loading state
// =========================================================================

void AstraSidebarNotesView::SetLoading(bool loading) {
  if (is_loading_ == loading) {
    return;
  }
  is_loading_ = loading;
  UpdateLoadingState();
}

void AstraSidebarNotesView::UpdateLoadingState() {
  if (loading_view_) {
    loading_view_->SetVisible(is_loading_ && expanded_);
  }
  if (list_container_) {
    list_container_->SetVisible(!is_loading_ && expanded_ && !showing_editor_);
  }
}

// =========================================================================
// Empty states
// =========================================================================

void AstraSidebarNotesView::UpdateEmptyStates() {
  if (!page_notes_container_ || !all_notes_container_) {
    return;
  }

  size_t page_count = page_notes_container_->children().size();
  size_t all_count = all_notes_container_->children().size();

  if (page_notes_empty_label_) {
    page_notes_empty_label_->SetVisible(page_count == 0 &&
                                        current_url_.is_valid() &&
                                        !showing_editor_ &&
                                        search_field_ &&
                                        search_field_->GetText().empty());
  }

  if (all_notes_empty_label_) {
    all_notes_empty_label_->SetVisible(all_count == 0 &&
                                       !is_loading_ &&
                                       !showing_editor_);
  }
}

// =========================================================================
// Count and query helpers
// =========================================================================

int AstraSidebarNotesView::GetNoteCount() const {
  if (!all_notes_container_) {
    return 0;
  }
  return static_cast<int>(all_notes_container_->children().size());
}

int AstraSidebarNotesView::GetPageNoteCount() const {
  if (!page_notes_container_) {
    return 0;
  }
  return static_cast<int>(page_notes_container_->children().size());
}

std::string AstraSidebarNotesView::GetSearchQuery() const {
  if (!search_field_) {
    return std::string();
  }
  return base::UTF16ToUTF8(search_field_->GetText());
}

// =========================================================================
// Delegate-forwarded actions
// =========================================================================
//
// These methods augment the service-based actions with delegate notifications
// so that the parent view (sidebar) can handle browser-level operations.

void AstraSidebarNotesView::OnNewNoteButtonPressed() {
  if (delegate_) {
    delegate_->OnNewNoteRequested();
  }
  ShowNewNoteEditor(/*link_to_current_url=*/true);
}

}  // namespace astra
