// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/performance/astra_performance_dashboard_view.h"

#include <algorithm>
#include <memory>
#include <string>

#include "base/strings/utf_string_conversions.h"
#include "skia/core/SkColor.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/style/typography.h"
#include "ui/views/view_class_properties.h"

namespace astra {

namespace {

constexpr int kBubbleWidth = 380;
constexpr int kRowHeight = 56;
constexpr int kSectionPadding = 16;
constexpr int kRowSpacing = 6;
constexpr int kMaxVisibleRows = 7;
constexpr int kMemoryBarHeight = 4;

// Format memory size (e.g. "256 MB", "1.2 GB").
std::u16string FormatMemoryBytes(int64_t bytes) {
  if (bytes < 1024) return u"0 KB";
  if (bytes < 1024 * 1024) {
    return base::UTF8ToUTF16(
        std::to_string(bytes / 1024) + " KB");
  }
  if (bytes < 1024LL * 1024 * 1024) {
    int mb = bytes / (1024 * 1024);
    return base::UTF8ToUTF16(std::to_string(mb) + " MB");
  }
  int gb = bytes / (1024 * 1024 * 1024);
  int remainder_mb = (bytes % (1024 * 1024 * 1024)) / (1024 * 1024);
  if (remainder_mb >= 100) {
    return base::UTF8ToUTF16(
        std::to_string(gb) + "." + std::to_string(remainder_mb / 100) + " GB");
  }
  return base::UTF8ToUTF16(std::to_string(gb) + " GB");
}

// Get icon emoji for process type.
std::u16string GetProcessIcon(const std::string& type) {
  if (type == "tab") return u"📄";
  if (type == "extension") return u"🧩";
  if (type == "gpu") return u"🎮";
  if (type == "browser") return u"🌐";
  if (type == "utility") return u"⚙️";
  return u"📄";
}

}  // namespace

// ===========================================================================
// AstraProcessRowView
// ===========================================================================

AstraProcessRowView::AstraProcessRowView(const ProcessInfo& info)
    : process_id_(info.process_id),
      title_(info.title),
      domain_(info.domain),
      type_(info.type),
      memory_bytes_(info.memory_bytes),
      cpu_percent_(info.cpu_percent),
      max_memory_bytes_(info.memory_bytes) {
  BuildLayout();
}

AstraProcessRowView::~AstraProcessRowView() = default;

void AstraProcessRowView::BuildLayout() {
  SetPreferredSize(gfx::Size(kBubbleWidth - kSectionPadding * 2, kRowHeight));
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(8, 12),
      10));
  SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  SetBorder(views::CreateRoundedRectBorder(
      1, 6, SK_ColorGRAY));

  // Icon.
  auto* icon_label = AddChildView(
      std::make_unique<views::Label>(GetProcessIcon(type_)));
  icon_label->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  icon_label->SetAutoColorReadabilityEnabled(false);
  icon_label->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  // Text column.
  auto* text_col = AddChildView(std::make_unique<views::View>());
  text_col->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets(), 2));
  text_col->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  title_label_ = text_col->AddChildView(
      std::make_unique<views::Label>(title_));
  title_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title_label_->SetAutoColorReadabilityEnabled(false);
  title_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));
  title_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);

  detail_label_ = text_col->AddChildView(
      std::make_unique<views::Label>(base::UTF8ToUTF16(domain_)));
  detail_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  detail_label_->SetAutoColorReadabilityEnabled(false);
  detail_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));
  detail_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);

  // Right column: memory + CPU.
  auto* right_col = AddChildView(std::make_unique<views::View>());
  right_col->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets(), 2));
  right_col->SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kEnd);

  memory_label_ = right_col->AddChildView(
      std::make_unique<views::Label>(FormatMemoryBytes(memory_bytes_)));
  memory_label_->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  memory_label_->SetAutoColorReadabilityEnabled(false);
  memory_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  cpu_label_ = right_col->AddChildView(
      std::make_unique<views::Label>(base::UTF8ToUTF16(
          std::to_string(static_cast<int>(cpu_percent_)) + "% CPU")));
  cpu_label_->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  cpu_label_->SetAutoColorReadabilityEnabled(false);
  cpu_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));
}

void AstraProcessRowView::UpdateMemory(
    int64_t memory_bytes, double cpu_percent) {
  memory_bytes_ = memory_bytes;
  cpu_percent_ = cpu_percent;
  if (memory_bytes > max_memory_bytes_) {
    max_memory_bytes_ = memory_bytes;
  }
  if (memory_label_) {
    memory_label_->SetText(FormatMemoryBytes(memory_bytes));
  }
  if (cpu_label_) {
    cpu_label_->SetText(base::UTF8ToUTF16(
        std::to_string(static_cast<int>(cpu_percent)) + "% CPU"));
  }
  SchedulePaint();
}

void AstraProcessRowView::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);

  // Draw memory usage bar at the bottom.
  if (max_memory_bytes_ > 0 && memory_bytes_ > 0) {
    gfx::Rect bounds = GetContentsBounds();
    int bar_width = bounds.width() - 24;
    int bar_x = bounds.x() + 12;
    int bar_y = bounds.bottom() - kMemoryBarHeight - 4;

    double ratio = static_cast<double>(memory_bytes_) /
                   static_cast<double>(max_memory_bytes_);
    int filled_width = static_cast<int>(bar_width * ratio);

    // Background bar.
    cc::PaintFlags bg_flags;
    bg_flags.setColor(SK_ColorGRAY);
    bg_flags.setStyle(cc::PaintFlags::kFill_Style);
    bg_flags.setAntiAlias(true);
    canvas->DrawRoundRect(
        gfx::Rect(bar_x, bar_y, bar_width, kMemoryBarHeight),
        kMemoryBarHeight / 2, bg_flags);

    // Filled bar.
    cc::PaintFlags fill_flags;
    SkColor fill_color = SK_ColorBLUE;
    if (GetColorProvider()) {
      fill_color = GetColorProvider()->GetColor(
          ui::kColorButtonBackgroundProminent);
    }
    fill_flags.setColor(fill_color);
    fill_flags.setStyle(cc::PaintFlags::kFill_Style);
    fill_flags.setAntiAlias(true);
    canvas->DrawRoundRect(
        gfx::Rect(bar_x, bar_y, filled_width, kMemoryBarHeight),
        kMemoryBarHeight / 2, fill_flags);
  }
}

void AstraProcessRowView::OnThemeChanged() {
  views::View::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  title_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForeground));
  detail_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));
  memory_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForeground));
  cpu_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));

  SetBackground(views::CreateRoundedRectBackground(
      cp->GetColor(ui::kColorDialogBackground), 6));
}

// ===========================================================================
// AstraPerformanceDashboardView
// ===========================================================================

AstraPerformanceDashboardView::AstraPerformanceDashboardView(
    views::View* anchor_view)
    : views::BubbleDialogDelegateView(anchor_view,
                                       views::BubbleBorder::TOP_RIGHT) {
  SetShowCloseButton(true);
  SetButtons(ui::DIALOG_BUTTON_NONE);
  set_fixed_width(kBubbleWidth);

  BuildUI();
}

AstraPerformanceDashboardView::~AstraPerformanceDashboardView() = default;

void AstraPerformanceDashboardView::SetProcesses(
    const std::vector<AstraProcessRowView::ProcessInfo>& processes) {
  processes_ = processes;
  RefreshProcessList();
}

void AstraPerformanceDashboardView::SetTotalMemory(int64_t bytes) {
  total_memory_bytes_ = bytes;
  RefreshStats();
}

void AstraPerformanceDashboardView::SetTotalCpu(double percent) {
  total_cpu_percent_ = percent;
  RefreshStats();
}

void AstraPerformanceDashboardView::SetTabCount(int count) {
  tab_count_ = count;
  RefreshStats();
}

void AstraPerformanceDashboardView::SetExtensionCount(int count) {
  extension_count_ = count;
  RefreshStats();
}

void AstraPerformanceDashboardView::SetMemorySavedBySleep(int64_t bytes) {
  memory_saved_by_sleep_ = bytes;
  RefreshStats();
}

void AstraPerformanceDashboardView::SetSortBy(SortBy sort_by) {
  sort_by_ = sort_by;
  RefreshProcessList();
}

void AstraPerformanceDashboardView::SetForceSleepCallback(
    ForceSleepCallback callback) {
  force_sleep_callback_ = std::move(callback);
}

void AstraPerformanceDashboardView::SetSortChangedCallback(
    SortChangedCallback callback) {
  sort_changed_callback_ = std::move(callback);
}

void AstraPerformanceDashboardView::BuildUI() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  set_margins(gfx::Insets::VH(0, 0));

  BuildStatsSection();
  BuildProcessList();
  BuildActionButtons();
}

void AstraPerformanceDashboardView::BuildStatsSection() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), 8));
  section->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));

  // Row 1: total memory + total CPU.
  auto* row1 = section->AddChildView(std::make_unique<views::View>());
  row1->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(), 16));

  total_memory_label_ = row1->AddChildView(
      std::make_unique<views::Label>(u"🧠 Memory: —"));
  total_memory_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  total_memory_label_->SetAutoColorReadabilityEnabled(false);
  total_memory_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));
  total_memory_label_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  total_cpu_label_ = row1->AddChildView(
      std::make_unique<views::Label>(u"💻 CPU: —"));
  total_cpu_label_->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  total_cpu_label_->SetAutoColorReadabilityEnabled(false);
  total_cpu_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  // Row 2: tab count + extension count.
  auto* row2 = section->AddChildView(std::make_unique<views::View>());
  row2->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(), 16));

  tab_count_label_ = row2->AddChildView(
      std::make_unique<views::Label>(u"📑 Tabs: 0"));
  tab_count_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  tab_count_label_->SetAutoColorReadabilityEnabled(false);
  tab_count_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));
  tab_count_label_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  ext_count_label_ = row2->AddChildView(
      std::make_unique<views::Label>(u"🧩 Extensions: 0"));
  ext_count_label_->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  ext_count_label_->SetAutoColorReadabilityEnabled(false);
  ext_count_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));

  // Row 3: memory saved by sleep.
  memory_saved_label_ = section->AddChildView(
      std::make_unique<views::Label>(u"💤 Tab sleep saves: 0 MB"));
  memory_saved_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  memory_saved_label_->SetAutoColorReadabilityEnabled(false);
  memory_saved_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));
}

void AstraPerformanceDashboardView::BuildProcessList() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), kRowSpacing));
  section->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  auto* header = section->AddChildView(
      std::make_unique<views::Label>(u"Top Processes"));
  header->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  header->SetAutoColorReadabilityEnabled(false);
  header->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  scroll_view_ = section->AddChildView(
      std::make_unique<views::ScrollView>());
  scroll_view_->SetClipHeight(kRowHeight * kMaxVisibleRows +
                                kRowSpacing * (kMaxVisibleRows - 1));
  scroll_view_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  process_list_ = scroll_view_->SetContents(
      std::make_unique<views::View>());
  process_list_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets(), kRowSpacing));
}

void AstraPerformanceDashboardView::BuildActionButtons() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), 8));
  section->SetBorder(views::CreateSolidSidedBorder(
      1, 0, 0, 0, SK_ColorGRAY));

  // Sort buttons row.
  auto* sort_row = section->AddChildView(std::make_unique<views::View>());
  sort_row->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(), 8));

  sort_memory_button_ = sort_row->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraPerformanceDashboardView::OnSortByMemory,
              base::Unretained(this)),
          u"Sort by memory"));

  sort_cpu_button_ = sort_row->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraPerformanceDashboardView::OnSortByCpu,
              base::Unretained(this)),
          u"Sort by CPU"));

  // Force sleep button.
  force_sleep_button_ = section->AddChildView(
      views::MdTextButton::CreateProminent(
          base::BindRepeating(
              &AstraPerformanceDashboardView::OnForceSleepHeaviest,
              base::Unretained(this)),
          u"💤 Sleep heaviest tabs"));
}

void AstraPerformanceDashboardView::RefreshProcessList() {
  if (!process_list_) return;

  process_list_->RemoveAllChildViews();
  process_rows_.clear();

  // Sort processes.
  std::vector<AstraProcessRowView::ProcessInfo> sorted = processes_;
  switch (sort_by_) {
    case SortBy::kMemory:
      std::sort(sorted.begin(), sorted.end(),
                [](const auto& a, const auto& b) {
                  return a.memory_bytes > b.memory_bytes;
                });
      break;
    case SortBy::kCpu:
      std::sort(sorted.begin(), sorted.end(),
                [](const auto& a, const auto& b) {
                  return a.cpu_percent > b.cpu_percent;
                });
      break;
    case SortBy::kName:
      std::sort(sorted.begin(), sorted.end(),
                [](const auto& a, const auto& b) {
                  return a.title < b.title;
                });
      break;
  }

  for (const auto& proc : sorted) {
    auto* row = process_list_->AddChildView(
        std::make_unique<AstraProcessRowView>(proc));
    process_rows_.push_back(row);
  }

  InvalidateLayout();
}

void AstraPerformanceDashboardView::RefreshStats() {
  if (total_memory_label_) {
    total_memory_label_->SetText(
        u"🧠 Memory: " + FormatMemoryBytes(total_memory_bytes_));
  }
  if (total_cpu_label_) {
    total_cpu_label_->SetText(base::UTF8ToUTF16(
        u"💻 CPU: " + std::to_string(
            static_cast<int>(total_cpu_percent_)) + "%"));
  }
  if (tab_count_label_) {
    tab_count_label_->SetText(base::UTF8ToUTF16(
        u"📑 Tabs: " + std::to_string(tab_count_)));
  }
  if (ext_count_label_) {
    ext_count_label_->SetText(base::UTF8ToUTF16(
        u"🧩 Extensions: " + std::to_string(extension_count_)));
  }
  if (memory_saved_label_) {
    memory_saved_label_->SetText(
        u"💤 Tab sleep saves: " + FormatMemoryBytes(memory_saved_by_sleep_));
  }
}

void AstraPerformanceDashboardView::OnSortByMemory() {
  sort_by_ = SortBy::kMemory;
  RefreshProcessList();
  if (sort_changed_callback_) {
    sort_changed_callback_.Run("memory");
  }
}

void AstraPerformanceDashboardView::OnSortByCpu() {
  sort_by_ = SortBy::kCpu;
  RefreshProcessList();
  if (sort_changed_callback_) {
    sort_changed_callback_.Run("cpu");
  }
}

void AstraPerformanceDashboardView::OnForceSleepHeaviest() {
  if (force_sleep_callback_) {
    force_sleep_callback_.Run();
  }
}

std::u16string AstraPerformanceDashboardView::GetWindowTitle() const {
  return u"Performance";
}

void AstraPerformanceDashboardView::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  total_memory_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForeground));
  total_cpu_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForeground));
  tab_count_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));
  ext_count_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));
  memory_saved_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));
}

}  // namespace astra
