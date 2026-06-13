#include "astra/ui/views/split_view/astra_split_view_model.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <utility>

#include "base/check.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"

namespace astra {

// =========================================================================
// Helper functions
// =========================================================================

std::string AstraSplitLayoutModeToString(AstraSplitLayoutMode mode) {
  switch (mode) {
    case AstraSplitLayoutMode::kTwoPaneHorizontal:
      return "two_pane_horizontal";
    case AstraSplitLayoutMode::kTwoPaneVertical:
      return "two_pane_vertical";
    case AstraSplitLayoutMode::kThreePaneHorizontal:
      return "three_pane_horizontal";
    case AstraSplitLayoutMode::kThreePaneVertical:
      return "three_pane_vertical";
    case AstraSplitLayoutMode::kGridTwoByTwo:
      return "grid_two_by_two";
    case AstraSplitLayoutMode::kGridThreeByTwo:
      return "grid_three_by_two";
  }
  return "two_pane_horizontal";
}

AstraSplitLayoutMode AstraSplitLayoutModeFromString(const std::string& value) {
  if (value == "two_pane_vertical") {
    return AstraSplitLayoutMode::kTwoPaneVertical;
  }
  if (value == "three_pane_horizontal") {
    return AstraSplitLayoutMode::kThreePaneHorizontal;
  }
  if (value == "three_pane_vertical") {
    return AstraSplitLayoutMode::kThreePaneVertical;
  }
  if (value == "grid_two_by_two") {
    return AstraSplitLayoutMode::kGridTwoByTwo;
  }
  if (value == "grid_three_by_two") {
    return AstraSplitLayoutMode::kGridThreeByTwo;
  }
  return AstraSplitLayoutMode::kTwoPaneHorizontal;
}

std::string AstraSplitResizeBehaviorToString(AstraSplitResizeBehavior behavior) {
  switch (behavior) {
    case AstraSplitResizeBehavior::kFixedRatio:
      return "fixed_ratio";
    case AstraSplitResizeBehavior::kFixedPixelSize:
      return "fixed_pixel_size";
    case AstraSplitResizeBehavior::kMinSizePriority:
      return "min_size_priority";
  }
  return "fixed_ratio";
}

AstraSplitResizeBehavior AstraSplitResizeBehaviorFromString(
    const std::string& value) {
  if (value == "fixed_pixel_size") {
    return AstraSplitResizeBehavior::kFixedPixelSize;
  }
  if (value == "min_size_priority") {
    return AstraSplitResizeBehavior::kMinSizePriority;
  }
  return AstraSplitResizeBehavior::kFixedRatio;
}

std::string AstraSplitPaneIdToString(AstraSplitPaneId pane_id) {
  switch (pane_id) {
    case AstraSplitPaneId::kPane0:
      return "pane0";
    case AstraSplitPaneId::kPane1:
      return "pane1";
    case AstraSplitPaneId::kPane2:
      return "pane2";
    case AstraSplitPaneId::kPane3:
      return "pane3";
    case AstraSplitPaneId::kPane4:
      return "pane4";
    case AstraSplitPaneId::kPane5:
      return "pane5";
  }
  return "pane0";
}

AstraSplitPaneId AstraSplitPaneIdFromString(const std::string& value) {
  if (value == "pane1") return AstraSplitPaneId::kPane1;
  if (value == "pane2") return AstraSplitPaneId::kPane2;
  if (value == "pane3") return AstraSplitPaneId::kPane3;
  if (value == "pane4") return AstraSplitPaneId::kPane4;
  if (value == "pane5") return AstraSplitPaneId::kPane5;
  return AstraSplitPaneId::kPane0;
}

std::u16string AstraSplitLayoutModeToName(AstraSplitLayoutMode mode) {
  // TODO(astra): Use localized strings (ui::ResourceBundle).
  // For now, use hardcoded English strings as placeholders.
  switch (mode) {
    case AstraSplitLayoutMode::kTwoPaneHorizontal:
      return u"Two Panes (Side by Side)";
    case AstraSplitLayoutMode::kTwoPaneVertical:
      return u"Two Panes (Stacked)";
    case AstraSplitLayoutMode::kThreePaneHorizontal:
      return u"Three Panes (Side by Side)";
    case AstraSplitLayoutMode::kThreePaneVertical:
      return u"Three Panes (Stacked)";
    case AstraSplitLayoutMode::kGridTwoByTwo:
      return u"Grid (2x2)";
    case AstraSplitLayoutMode::kGridThreeByTwo:
      return u"Grid (3x2)";
  }
  return u"Split View";
}

// =========================================================================
// AstraSplitViewModel
// =========================================================================

AstraSplitViewModel::AstraSplitViewModel(AstraSplitLayoutMode mode)
    : layout_mode_(mode) {
  InitializeDividers();
}

AstraSplitViewModel::~AstraSplitViewModel() {
  NotifyDestroyed();
}

// =========================================================================
// Observer management
// =========================================================================

void AstraSplitViewModel::AddObserver(AstraSplitViewModelObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraSplitViewModel::RemoveObserver(AstraSplitViewModelObserver* observer) {
  observers_.RemoveObserver(observer);
}

// =========================================================================
// Layout mode
// =========================================================================

void AstraSplitViewModel::SetLayoutMode(AstraSplitLayoutMode mode) {
  if (layout_mode_ == mode) {
    return;
  }

  layout_mode_ = mode;
  InitializeDividers();

  // Clamp focused pane to valid range.
  int pane_count = GetPaneCount();
  int focused_index = static_cast<int>(focused_pane_);
  if (focused_index >= pane_count) {
    focused_pane_ = static_cast<AstraSplitPaneId>(pane_count - 1);
  }

  NotifyLayoutModeChanged();
}

int AstraSplitViewModel::GetPaneCount() const {
  return PaneCountForMode(layout_mode_);
}

int AstraSplitViewModel::GetDividerCount() const {
  return DividerCountForMode(layout_mode_);
}

bool AstraSplitViewModel::IsGridLayout() const {
  return layout_mode_ == AstraSplitLayoutMode::kGridTwoByTwo ||
         layout_mode_ == AstraSplitLayoutMode::kGridThreeByTwo;
}

int AstraSplitViewModel::GetColumnCount() const {
  switch (layout_mode_) {
    case AstraSplitLayoutMode::kTwoPaneHorizontal:
    case AstraSplitLayoutMode::kThreePaneHorizontal:
      return GetPaneCount();
    case AstraSplitLayoutMode::kTwoPaneVertical:
    case AstraSplitLayoutMode::kThreePaneVertical:
      return 1;
    case AstraSplitLayoutMode::kGridTwoByTwo:
      return 2;
    case AstraSplitLayoutMode::kGridThreeByTwo:
      return 3;
  }
  return 1;
}

int AstraSplitViewModel::GetRowCount() const {
  switch (layout_mode_) {
    case AstraSplitLayoutMode::kTwoPaneHorizontal:
    case AstraSplitLayoutMode::kThreePaneHorizontal:
      return 1;
    case AstraSplitLayoutMode::kTwoPaneVertical:
    case AstraSplitLayoutMode::kThreePaneVertical:
      return GetPaneCount();
    case AstraSplitLayoutMode::kGridTwoByTwo:
      return 2;
    case AstraSplitLayoutMode::kGridThreeByTwo:
      return 2;
  }
  return 1;
}

// =========================================================================
// Divider positions
// =========================================================================

void AstraSplitViewModel::SetDividerRatio(AstraSplitDividerId divider_id,
                                          double ratio) {
  int index = static_cast<int>(divider_id);
  if (!IsValidDividerId(divider_id)) {
    return;
  }

  double clamped = ClampRatio(ratio);
  if (divider_ratios_[index] == clamped) {
    return;
  }

  divider_ratios_[index] = clamped;
  NotifyDividerPositionChanged(divider_id);
}

double AstraSplitViewModel::GetDividerRatio(AstraSplitDividerId divider_id) const {
  int index = static_cast<int>(divider_id);
  if (index < 0 || index >= static_cast<int>(divider_ratios_.size())) {
    return 0.0;
  }
  return divider_ratios_[index];
}

void AstraSplitViewModel::SetEqualRatios() {
  int divider_count = GetDividerCount();
  if (divider_count == 0) {
    return;
  }

  bool changed = false;
  for (int i = 0; i < divider_count; ++i) {
    double expected = static_cast<double>(i + 1) / (divider_count + 1);
    if (divider_ratios_[i] != expected) {
      divider_ratios_[i] = expected;
      changed = true;
    }
  }

  if (changed) {
    for (int i = 0; i < divider_count; ++i) {
      NotifyDividerPositionChanged(static_cast<AstraSplitDividerId>(i));
    }
  }
}

void AstraSplitViewModel::ResetToDefaults() {
  InitializeDividers();

  for (int i = 0; i < static_cast<int>(divider_ratios_.size()); ++i) {
    NotifyDividerPositionChanged(static_cast<AstraSplitDividerId>(i));
  }
}

// =========================================================================
// Drag state
// =========================================================================

void AstraSplitViewModel::StartDividerDrag(AstraSplitDividerId divider_id) {
  if (!IsValidDividerId(divider_id)) {
    return;
  }
  is_dragging_ = true;
  dragged_divider_index_ = static_cast<int>(divider_id);
  NotifyDividerDragStarted(divider_id);
}

void AstraSplitViewModel::UpdateDividerDrag(AstraSplitDividerId divider_id,
                                             double ratio) {
  if (!is_dragging_) {
    return;
  }
  int index = static_cast<int>(divider_id);
  if (index != dragged_divider_index_) {
    return;
  }

  double clamped = ClampRatio(ratio);
  if (divider_ratios_[index] == clamped) {
    return;
  }

  divider_ratios_[index] = clamped;
  NotifyDividerPositionChanging(divider_id);
}

void AstraSplitViewModel::EndDividerDrag(AstraSplitDividerId divider_id) {
  if (!is_dragging_) {
    return;
  }
  int index = static_cast<int>(divider_id);
  if (index != dragged_divider_index_) {
    return;
  }

  is_dragging_ = false;
  dragged_divider_index_ = -1;
  NotifyDividerDragEnded(divider_id);
  NotifyDividerPositionChanged(divider_id);
}

bool AstraSplitViewModel::IsDragging() const {
  return is_dragging_;
}

absl::optional<AstraSplitDividerId> AstraSplitViewModel::GetDraggedDivider() const {
  if (!is_dragging_) {
    return absl::nullopt;
  }
  return static_cast<AstraSplitDividerId>(dragged_divider_index_);
}

// =========================================================================
// Focused pane
// =========================================================================

void AstraSplitViewModel::SetFocusedPane(AstraSplitPaneId pane_id) {
  if (!IsValidPaneId(pane_id)) {
    return;
  }
  if (focused_pane_ == pane_id) {
    return;
  }
  focused_pane_ = pane_id;
  NotifyFocusedPaneChanged();
}

void AstraSplitViewModel::FocusNextPane() {
  int pane_count = GetPaneCount();
  if (pane_count <= 1) {
    return;
  }
  int current = static_cast<int>(focused_pane_);
  int next = (current + 1) % pane_count;
  SetFocusedPane(static_cast<AstraSplitPaneId>(next));
}

void AstraSplitViewModel::FocusPreviousPane() {
  int pane_count = GetPaneCount();
  if (pane_count <= 1) {
    return;
  }
  int current = static_cast<int>(focused_pane_);
  int prev = (current - 1 + pane_count) % pane_count;
  SetFocusedPane(static_cast<AstraSplitPaneId>(prev));
}

// =========================================================================
// Pane operations
// =========================================================================

bool AstraSplitViewModel::ClosePane(AstraSplitPaneId pane_id) {
  if (!IsValidPaneId(pane_id)) {
    return false;
  }

  int pane_count = GetPaneCount();
  if (pane_count <= 1) {
    // Can't close the last pane.
    return false;
  }

  // TODO(astra): Implement actual pane removal.  For now, closing a pane
  // reduces the layout mode to the next smaller one.  The full implementation
  // would need to remove the pane and rebalance the remaining dividers.
  //
  // For the skeleton model, we just notify and return true.
  // The view and controller handle the actual layout adjustment.
  //
  // Chromium owner: views::View hierarchy manipulation.
  NotifyPaneClosed(pane_id);
  return true;
}

void AstraSplitViewModel::SwapPanes(AstraSplitPaneId pane_a,
                                     AstraSplitPaneId pane_b) {
  if (!IsValidPaneId(pane_a) || !IsValidPaneId(pane_b)) {
    return;
  }
  if (pane_a == pane_b) {
    return;
  }

  // The model doesn't own the pane contents — it just tracks which one
  // is focused and notifies observers of the swap.
  // The actual content swap is handled by the view/controller.
  //
  // If the focused pane is one of the swapped panes, swap the focus too.
  if (focused_pane_ == pane_a) {
    focused_pane_ = pane_b;
  } else if (focused_pane_ == pane_b) {
    focused_pane_ = pane_a;
  }

  NotifyPanesSwapped(pane_a, pane_b);
}

void AstraSplitViewModel::SwapFocusedWithNext() {
  int pane_count = GetPaneCount();
  if (pane_count <= 1) {
    return;
  }
  int current = static_cast<int>(focused_pane_);
  int next = (current + 1) % pane_count;
  SwapPanes(focused_pane_, static_cast<AstraSplitPaneId>(next));
}

void AstraSplitViewModel::SwapFocusedWithPrevious() {
  int pane_count = GetPaneCount();
  if (pane_count <= 1) {
    return;
  }
  int current = static_cast<int>(focused_pane_);
  int prev = (current - 1 + pane_count) % pane_count;
  SwapPanes(focused_pane_, static_cast<AstraSplitPaneId>(prev));
}

// =========================================================================
// Layout configuration
// =========================================================================

void AstraSplitViewModel::SetMinPaneSize(int min_size_dips) {
  if (min_size_dips < 0) {
    min_size_dips = 0;
  }
  if (min_pane_size_ == min_size_dips) {
    return;
  }
  min_pane_size_ = min_size_dips;
  NotifyMinPaneSizeChanged();
}

void AstraSplitViewModel::SetResizeBehavior(AstraSplitResizeBehavior behavior) {
  if (resize_behavior_ == behavior) {
    return;
  }
  resize_behavior_ = behavior;
  NotifyResizeBehaviorChanged();
}

// =========================================================================
// Orientation helpers
// =========================================================================

void AstraSplitViewModel::ToggleOrientation() {
  switch (layout_mode_) {
    case AstraSplitLayoutMode::kTwoPaneHorizontal:
      SetLayoutMode(AstraSplitLayoutMode::kTwoPaneVertical);
      break;
    case AstraSplitLayoutMode::kTwoPaneVertical:
      SetLayoutMode(AstraSplitLayoutMode::kTwoPaneHorizontal);
      break;
    case AstraSplitLayoutMode::kThreePaneHorizontal:
      SetLayoutMode(AstraSplitLayoutMode::kThreePaneVertical);
      break;
    case AstraSplitLayoutMode::kThreePaneVertical:
      SetLayoutMode(AstraSplitLayoutMode::kThreePaneHorizontal);
      break;
    case AstraSplitLayoutMode::kGridTwoByTwo:
    case AstraSplitLayoutMode::kGridThreeByTwo:
      // For grids, we don't toggle — it's already both horizontal and vertical.
      break;
  }
}

bool AstraSplitViewModel::IsHorizontal() const {
  switch (layout_mode_) {
    case AstraSplitLayoutMode::kTwoPaneHorizontal:
    case AstraSplitLayoutMode::kThreePaneHorizontal:
      return true;
    case AstraSplitLayoutMode::kTwoPaneVertical:
    case AstraSplitLayoutMode::kThreePaneVertical:
      return false;
    case AstraSplitLayoutMode::kGridTwoByTwo:
    case AstraSplitLayoutMode::kGridThreeByTwo:
      // Grid layouts have both axes; primary axis is considered horizontal.
      return true;
  }
  return true;
}

// =========================================================================
// Validation
// =========================================================================

bool AstraSplitViewModel::IsValidPaneId(AstraSplitPaneId pane_id) const {
  int index = static_cast<int>(pane_id);
  return index >= 0 && index < GetPaneCount();
}

bool AstraSplitViewModel::IsValidDividerId(AstraSplitDividerId divider_id) const {
  int index = static_cast<int>(divider_id);
  return index >= 0 && index < static_cast<int>(divider_ratios_.size());
}

double AstraSplitViewModel::ClampRatio(double ratio) const {
  // Apply ratio-based minimum first.
  double min_ratio = kMinPaneRatio;
  double max_ratio = 1.0 - kMinPaneRatio;

  if (ratio < min_ratio) {
    return min_ratio;
  }
  if (ratio > max_ratio) {
    return max_ratio;
  }

  return ratio;
}

// =========================================================================
// State serialization
// =========================================================================

std::string AstraSplitViewModel::SerializeToString() const {
  std::ostringstream oss;
  oss << AstraSplitLayoutModeToString(layout_mode_);
  oss << "|";
  oss << min_pane_size_;
  oss << "|";
  oss << AstraSplitResizeBehaviorToString(resize_behavior_);
  oss << "|";
  oss << static_cast<int>(focused_pane_);
  oss << "|";
  for (size_t i = 0; i < divider_ratios_.size(); ++i) {
    if (i > 0) {
      oss << ",";
    }
    oss << divider_ratios_[i];
  }
  return oss.str();
}

bool AstraSplitViewModel::DeserializeFromString(const std::string& state) {
  std::vector<std::string> parts = base::SplitString(
      state, "|", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  if (parts.size() < 5) {
    return false;
  }

  AstraSplitLayoutMode mode = AstraSplitLayoutModeFromString(parts[0]);
  int min_size = 0;
  if (!base::StringToInt(parts[1], &min_size)) {
    return false;
  }
  AstraSplitResizeBehavior behavior =
      AstraSplitResizeBehaviorFromString(parts[2]);
  int focused = 0;
  if (!base::StringToInt(parts[3], &focused)) {
    return false;
  }

  std::vector<std::string> ratio_strs = base::SplitString(
      parts[4], ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);

  // Set the layout mode first (initializes dividers).
  layout_mode_ = mode;
  InitializeDividers();

  // Parse ratios.
  for (size_t i = 0; i < ratio_strs.size() && i < divider_ratios_.size(); ++i) {
    double ratio = 0.0;
    if (base::StringToDouble(ratio_strs[i], &ratio)) {
      divider_ratios_[i] = ClampRatio(ratio);
    }
  }

  min_pane_size_ = std::max(0, min_size);
  resize_behavior_ = behavior;

  int pane_count = GetPaneCount();
  if (focused >= 0 && focused < pane_count) {
    focused_pane_ = static_cast<AstraSplitPaneId>(focused);
  } else {
    focused_pane_ = AstraSplitPaneId::kPane0;
  }

  return true;
}

// =========================================================================
// Private helpers
// =========================================================================

void AstraSplitViewModel::InitializeDividers() {
  int divider_count = DividerCountForMode(layout_mode_);
  divider_ratios_.clear();
  divider_ratios_.reserve(divider_count);

  if (divider_count > 0) {
    double step = 1.0 / (divider_count + 1);
    for (int i = 0; i < divider_count; ++i) {
      divider_ratios_.push_back(step * (i + 1));
    }
  }
}

// static
int AstraSplitViewModel::DividerCountForMode(AstraSplitLayoutMode mode) {
  switch (mode) {
    case AstraSplitLayoutMode::kTwoPaneHorizontal:
    case AstraSplitLayoutMode::kTwoPaneVertical:
      return 1;
    case AstraSplitLayoutMode::kThreePaneHorizontal:
    case AstraSplitLayoutMode::kThreePaneVertical:
      return 2;
    case AstraSplitLayoutMode::kGridTwoByTwo:
      return 2;  // 1 column divider + 1 row divider
    case AstraSplitLayoutMode::kGridThreeByTwo:
      return 3;  // 2 column dividers + 1 row divider
  }
  return 0;
}

// static
int AstraSplitViewModel::PaneCountForMode(AstraSplitLayoutMode mode) {
  switch (mode) {
    case AstraSplitLayoutMode::kTwoPaneHorizontal:
    case AstraSplitLayoutMode::kTwoPaneVertical:
      return 2;
    case AstraSplitLayoutMode::kThreePaneHorizontal:
    case AstraSplitLayoutMode::kThreePaneVertical:
      return 3;
    case AstraSplitLayoutMode::kGridTwoByTwo:
      return 4;
    case AstraSplitLayoutMode::kGridThreeByTwo:
      return 6;
  }
  return 1;
}

int AstraSplitViewModel::ValidateDividerIndex(int index) const {
  if (index < 0) return 0;
  int max = static_cast<int>(divider_ratios_.size()) - 1;
  if (index > max) return max;
  return index;
}

int AstraSplitViewModel::ValidatePaneIndex(int index) const {
  if (index < 0) return 0;
  int max = GetPaneCount() - 1;
  if (index > max) return max;
  return index;
}

double AstraSplitViewModel::GetRatioAtIndex(int index) const {
  index = ValidateDividerIndex(index);
  return divider_ratios_[index];
}

void AstraSplitViewModel::SetRatioAtIndex(int index, double ratio) {
  index = ValidateDividerIndex(index);
  double clamped = ClampRatio(ratio);
  if (divider_ratios_[index] == clamped) {
    return;
  }
  divider_ratios_[index] = clamped;
  NotifyDividerPositionChanged(static_cast<AstraSplitDividerId>(index));
}

// =========================================================================
// Observer notification helpers
// =========================================================================

void AstraSplitViewModel::NotifyLayoutModeChanged() {
  for (AstraSplitViewModelObserver& observer : observers_) {
    observer.OnSplitLayoutModeChanged(layout_mode_);
  }
}

void AstraSplitViewModel::NotifyDividerPositionChanged(
    AstraSplitDividerId divider_id) {
  for (AstraSplitViewModelObserver& observer : observers_) {
    observer.OnSplitDividerPositionChanged(divider_id,
        GetDividerRatio(divider_id));
  }
}

void AstraSplitViewModel::NotifyDividerPositionChanging(
    AstraSplitDividerId divider_id) {
  for (AstraSplitViewModelObserver& observer : observers_) {
    observer.OnSplitDividerPositionChanging(divider_id,
        GetDividerRatio(divider_id));
  }
}

void AstraSplitViewModel::NotifyDividerDragStarted(
    AstraSplitDividerId divider_id) {
  for (AstraSplitViewModelObserver& observer : observers_) {
    observer.OnSplitDividerDragStarted(divider_id);
  }
}

void AstraSplitViewModel::NotifyDividerDragEnded(
    AstraSplitDividerId divider_id) {
  for (AstraSplitViewModelObserver& observer : observers_) {
    observer.OnSplitDividerDragEnded(divider_id);
  }
}

void AstraSplitViewModel::NotifyFocusedPaneChanged() {
  for (AstraSplitViewModelObserver& observer : observers_) {
    observer.OnSplitFocusedPaneChanged(focused_pane_);
  }
}

void AstraSplitViewModel::NotifyPaneClosed(AstraSplitPaneId pane_id) {
  for (AstraSplitViewModelObserver& observer : observers_) {
    observer.OnSplitPaneClosed(pane_id);
  }
}

void AstraSplitViewModel::NotifyPanesSwapped(AstraSplitPaneId pane_a,
                                              AstraSplitPaneId pane_b) {
  for (AstraSplitViewModelObserver& observer : observers_) {
    observer.OnSplitPanesSwapped(pane_a, pane_b);
  }
}

void AstraSplitViewModel::NotifyMinPaneSizeChanged() {
  for (AstraSplitViewModelObserver& observer : observers_) {
    observer.OnSplitMinPaneSizeChanged(min_pane_size_);
  }
}

void AstraSplitViewModel::NotifyResizeBehaviorChanged() {
  for (AstraSplitViewModelObserver& observer : observers_) {
    observer.OnSplitResizeBehaviorChanged(resize_behavior_);
  }
}

void AstraSplitViewModel::NotifyDestroyed() {
  for (AstraSplitViewModelObserver& observer : observers_) {
    observer.OnSplitViewModelDestroyed();
  }
}

}  // namespace astra
