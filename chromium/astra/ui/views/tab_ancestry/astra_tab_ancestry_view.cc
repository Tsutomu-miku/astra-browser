// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/tab_ancestry/astra_tab_ancestry_view.h"

#include <algorithm>
#include <memory>
#include <string>

#include "base/strings/utf_string_conversions.h"
#include "skia/core/SkColor.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/point.h"
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

constexpr int kBubbleWidth = 360;
constexpr int kNodeHeight = 32;
constexpr int kIndentPerLevel = 20;
constexpr int kSectionPadding = 16;
constexpr int kMaxVisibleNodes = 12;
constexpr int kToggleSize = 16;

// Format relative time.
std::u16string FormatRelativeTime(base::Time time) {
  if (time.is_null()) return u"";
  base::TimeDelta delta = base::Time::Now() - time;
  if (delta.InMinutes() < 1) return u"just now";
  if (delta.InHours() < 1) {
    return base::UTF8ToUTF16(
        std::to_string(delta.InMinutes()) + "m ago");
  }
  if (delta.InDays() < 1) {
    return base::UTF8ToUTF16(
        std::to_string(delta.InHours()) + "h ago");
  }
  return base::UTF8ToUTF16(
      std::to_string(delta.InDays()) + "d ago");
}

}  // namespace

// ===========================================================================
// AstraTabAncestryNodeView
// ===========================================================================

AstraTabAncestryNodeView::AstraTabAncestryNodeView(
    const TabNode& node,
    TabClickCallback click_callback,
    CollapseCallback collapse_callback)
    : tab_id_(node.tab_id),
      title_(node.title),
      domain_(node.domain),
      depth_(node.depth),
      expanded_(true),
      is_active_(node.is_active),
      node_(node),
      click_callback_(std::move(click_callback)),
      collapse_callback_(std::move(collapse_callback)) {
  BuildLayout();
}

AstraTabAncestryNodeView::~AstraTabAncestryNodeView() = default;

void AstraTabAncestryNodeView::BuildLayout() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));

  // Main row.
  auto* row = AddChildView(std::make_unique<views::View>());
  row->SetPreferredSize(gfx::Size(kBubbleWidth - kSectionPadding * 2, kNodeHeight));
  row->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(4, kIndentPerLevel * depth_ + 4),
      8));
  row->SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  row->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  // Toggle button (expand/collapse).
  toggle_button_ = row->AddChildView(std::make_unique<views::View>());
  toggle_button_->SetPreferredSize(gfx::Size(kToggleSize, kToggleSize));
  toggle_button_->SetCursor(ui::CursorType::kHand);
  toggle_button_->SetPaintCallback(
      base::BindRepeating(
          [](AstraTabAncestryNodeView* view, gfx::Canvas* canvas) {
            gfx::Rect bounds = view->toggle_button_->GetContentsBounds();
            SkColor color = SK_ColorGRAY;
            if (view->GetColorProvider()) {
              color = view->GetColorProvider()->GetColor(
                  ui::kColorIcon);
            }
            cc::PaintFlags flags;
            flags.setColor(color);
            flags.setStyle(cc::PaintFlags::kStroke_Style);
            flags.setStrokeWidth(2);
            flags.setAntiAlias(true);

            int cx = bounds.x() + bounds.width() / 2;
            int cy = bounds.y() + bounds.height() / 2;
            int s = 5;

            if (view->expanded_) {
              // Down arrow (▼).
              canvas->DrawLine(gfx::Point(cx - s, cy - 2),
                               gfx::Point(cx, cy + 3), flags);
              canvas->DrawLine(gfx::Point(cx, cy + 3),
                               gfx::Point(cx + s, cy - 2), flags);
            } else {
              // Right arrow (▶).
              canvas->DrawLine(gfx::Point(cx - 2, cy - s),
                               gfx::Point(cx + 3, cy), flags);
              canvas->DrawLine(gfx::Point(cx + 3, cy),
                               gfx::Point(cx - 2, cy + s), flags);
            }
          },
          base::Unretained(this)));

  // If no children, hide toggle button.
  if (node_.children.empty()) {
    toggle_button_->SetVisible(false);
  }

  // Tab icon.
  auto* icon_label = row->AddChildView(
      std::make_unique<views::Label>(u"📄"));
  icon_label->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  icon_label->SetAutoColorReadabilityEnabled(false);
  icon_label->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));

  // Title label.
  title_label_ = row->AddChildView(
      std::make_unique<views::Label>(title_));
  title_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title_label_->SetAutoColorReadabilityEnabled(false);
  title_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          is_active_ ? views::style::STYLE_PRIMARY
                     : views::style::STYLE_SECONDARY));
  title_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);
  title_label_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  // Make row clickable.
  row->SetCursor(ui::CursorType::kHand);

  // Children container.
  children_container_ = AddChildView(std::make_unique<views::View>());
  children_container_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical));

  if (!node_.children.empty()) {
    BuildChildren();
  }
}

void AstraTabAncestryNodeView::BuildChildren() {
  if (!children_container_) return;

  children_container_->RemoveAllChildViews();
  child_nodes_.clear();

  for (const auto& child : node_.children) {
    auto* child_node = children_container_->AddChildView(
        std::make_unique<AstraTabAncestryNodeView>(
            child,
            base::BindRepeating(
                &AstraTabAncestryNodeView::OnTabClicked,
                base::Unretained(this)),
            base::BindRepeating(
                &AstraTabAncestryNodeView::OnNodeCollapsed,
                base::Unretained(this))));
    child_nodes_.push_back(child_node);
  }
}

void AstraTabAncestryNodeView::SetExpanded(bool expanded) {
  if (expanded_ == expanded) return;
  expanded_ = expanded;
  if (children_container_) {
    children_container_->SetVisible(expanded_);
  }
  if (toggle_button_) {
    toggle_button_->SchedulePaint();
  }
  if (parent()) {
    parent()->InvalidateLayout();
  }
}

void AstraTabAncestryNodeView::SetActive(bool active) {
  is_active_ = active;
  if (title_label_) {
    title_label_->SetFontList(
        views::style::GetFont(
            views::style::CONTEXT_LABEL,
            is_active ? views::style::STYLE_PRIMARY
                       : views::style::STYLE_SECONDARY));
  }
  SchedulePaint();
}

bool AstraTabAncestryNodeView::OnMousePressed(
    const ui::MouseEvent& event) {
  // Check if click is on toggle button.
  if (toggle_button_ && toggle_button_->GetVisible()) {
    gfx::Point toggle_point = event.location();
    ConvertPointToTarget(this, toggle_button_, &toggle_point);
    if (toggle_button_->GetContentsBounds().Contains(toggle_point)) {
      SetExpanded(!expanded_);
      if (collapse_callback_) {
        collapse_callback_.Run(tab_id_);
      }
      return true;
    }
  }

  // Otherwise, treat as tab click.
  OnTabClicked(tab_id_);
  return true;
}

void AstraTabAncestryNodeView::OnToggleClicked() {
  SetExpanded(!expanded_);
  if (collapse_callback_) {
    collapse_callback_.Run(tab_id_);
  }
}

void AstraTabAncestryNodeView::OnTabClicked() {
  if (click_callback_) {
    click_callback_.Run(tab_id_);
  }
}

void AstraTabAncestryNodeView::OnTabClicked(const std::string& tab_id) {
  if (click_callback_) {
    click_callback_.Run(tab_id);
  }
}

void AstraTabAncestryNodeView::OnNodeCollapsed(const std::string& tab_id) {
  if (collapse_callback_) {
    collapse_callback_.Run(tab_id);
  }
}

void AstraTabAncestryNodeView::OnThemeChanged() {
  views::View::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  title_label_->SetEnabledColor(
      cp->GetColor(is_active_ ? ui::kColorLabelForeground
                              : ui::kColorLabelForegroundSecondary));

  if (toggle_button_) {
    toggle_button_->SchedulePaint();
  }
}

// ===========================================================================
// AstraTabAncestryView
// ===========================================================================

AstraTabAncestryView::AstraTabAncestryView(views::View* anchor_view)
    : views::BubbleDialogDelegateView(anchor_view,
                                       views::BubbleBorder::TOP_RIGHT) {
  SetShowCloseButton(true);
  SetButtons(ui::DIALOG_BUTTON_NONE);
  set_fixed_width(kBubbleWidth);

  BuildUI();
}

AstraTabAncestryView::~AstraTabAncestryView() = default;

void AstraTabAncestryView::SetRootNodes(
    const std::vector<AstraTabAncestryNodeView::TabNode>& roots) {
  root_nodes_ = roots;
  RefreshTree();
}

void AstraTabAncestryView::SetActiveTabId(const std::string& tab_id) {
  active_tab_id_ = tab_id;
  // TODO(astra): Update active node visual state recursively.
}

void AstraTabAncestryView::SetTabCount(int count) {
  tab_count_ = count;
  RefreshStats();
}

void AstraTabAncestryView::SetBranchCount(int count) {
  branch_count_ = count;
  RefreshStats();
}

void AstraTabAncestryView::SetMaxDepth(int depth) {
  max_depth_ = depth;
  RefreshStats();
}

void AstraTabAncestryView::SetTabActivatedCallback(
    TabActivatedCallback callback) {
  tab_activated_callback_ = std::move(callback);
}

void AstraTabAncestryView::SetCollapseAllCallback(
    CollapseAllCallback callback) {
  collapse_all_callback_ = std::move(callback);
}

void AstraTabAncestryView::SetExpandAllCallback(
    ExpandAllCallback callback) {
  expand_all_callback_ = std::move(callback);
}

void AstraTabAncestryView::BuildUI() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  set_margins(gfx::Insets::VH(0, 0));

  BuildToolbar();
  BuildTreeView();
  BuildStatsFooter();
}

void AstraTabAncestryView::BuildToolbar() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(8, kSectionPadding), 8));
  section->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));
  section->SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  collapse_all_button_ = section->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraTabAncestryView::OnCollapseAllClicked,
              base::Unretained(this)),
          u"Collapse all"));

  expand_all_button_ = section->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraTabAncestryView::OnExpandAllClicked,
              base::Unretained(this)),
          u"Expand all"));

  // Spacer.
  auto* spacer = section->AddChildView(std::make_unique<views::View>());
  spacer->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  auto* tree_icon = section->AddChildView(
      std::make_unique<views::Label>(u"🌳 Tree"));
  tree_icon->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  tree_icon->SetAutoColorReadabilityEnabled(false);
  tree_icon->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));
}

void AstraTabAncestryView::BuildTreeView() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(8, kSectionPadding), 4));
  section->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  scroll_view_ = section->AddChildView(
      std::make_unique<views::ScrollView>());
  scroll_view_->SetClipHeight(kNodeHeight * kMaxVisibleNodes);
  scroll_view_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  tree_container_ = scroll_view_->SetContents(
      std::make_unique<views::View>());
  tree_container_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets(), 2));
}

void AstraTabAncestryView::BuildStatsFooter() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(8, kSectionPadding), 16));
  section->SetBorder(views::CreateSolidSidedBorder(
      1, 0, 0, 0, SK_ColorGRAY));

  stats_label_ = section->AddChildView(
      std::make_unique<views::Label>(u"0 tabs · 0 branches · depth 0"));
  stats_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  stats_label_->SetAutoColorReadabilityEnabled(false);
  stats_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));
}

void AstraTabAncestryView::RefreshTree() {
  if (!tree_container_) return;

  tree_container_->RemoveAllChildViews();
  root_node_views_.clear();

  for (const auto& root : root_nodes_) {
    auto* node_view = tree_container_->AddChildView(
        std::make_unique<AstraTabAncestryNodeView>(
            root,
            base::BindRepeating(
                &AstraTabAncestryView::OnTabClicked,
                base::Unretained(this)),
            base::BindRepeating(
                &AstraTabAncestryView::OnNodeCollapsed,
                base::Unretained(this))));
    root_node_views_.push_back(node_view);
  }

  InvalidateLayout();
}

void AstraTabAncestryView::RefreshStats() {
  if (!stats_label_) return;

  stats_label_->SetText(base::UTF8ToUTF16(
      std::to_string(tab_count_) + " tabs · " +
      std::to_string(branch_count_) + " branches · depth " +
      std::to_string(max_depth_)));
}

void AstraTabAncestryView::OnTabClicked(const std::string& tab_id) {
  if (tab_activated_callback_) {
    tab_activated_callback_.Run(tab_id);
  }
}

void AstraTabAncestryView::OnNodeCollapsed(const std::string& tab_id) {
  // Propagate or handle.
}

void AstraTabAncestryView::OnCollapseAllClicked() {
  for (auto* root : root_node_views_) {
    root->SetExpanded(false);
  }
  if (collapse_all_callback_) {
    collapse_all_callback_.Run();
  }
}

void AstraTabAncestryView::OnExpandAllClicked() {
  for (auto* root : root_node_views_) {
    root->SetExpanded(true);
  }
  if (expand_all_callback_) {
    expand_all_callback_.Run();
  }
}

std::u16string AstraTabAncestryView::GetWindowTitle() const {
  return u"Tab Family Tree";
}

void AstraTabAncestryView::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  stats_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));
}

}  // namespace astra
