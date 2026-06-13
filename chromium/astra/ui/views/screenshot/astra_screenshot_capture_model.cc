#include "astra/ui/views/screenshot/astra_screenshot_capture_model.h"

#include <algorithm>
#include <string>

#include "base/i18n/time_formatting.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "astra/browser/astra_prefs.h"
#include "astra/browser/astra_screenshot_service.h"
#include "components/prefs/pref_service.h"

namespace astra {

namespace {

// Helper to clamp a value to a range.
template <typename T>
T Clamp(T value, T min_val, T max_val) {
  if (value < min_val) return min_val;
  if (value > max_val) return max_val;
  return value;
}

// Get the file extension for an image format.
const char* GetExtensionForFormat(AstraScreenshotImageFormatModel format) {
  switch (format) {
    case AstraScreenshotImageFormatModel::kPng:
      return "png";
    case AstraScreenshotImageFormatModel::kJpeg:
      return "jpg";
    case AstraScreenshotImageFormatModel::kWebP:
      return "webp";
  }
  return "png";
}

// Get a short label for the capture type (used in filenames).
const char* GetCaptureTypeShortLabel(AstraScreenshotType type) {
  switch (type) {
    case AstraScreenshotType::kVisibleArea:
      return "Visible";
    case AstraScreenshotType::kFullPage:
      return "FullPage";
    case AstraScreenshotType::kRegion:
      return "Region";
  }
  return "Screenshot";
}

}  // namespace

// =========================================================================
// Construction / destruction
// =========================================================================

AstraScreenshotCaptureModel::AstraScreenshotCaptureModel(
    PrefService* pref_service)
    : pref_service_(pref_service),
      capture_type_(static_cast<AstraScreenshotType>(
          prefs::kDefaultScreenshotDefaultCaptureType)) {}

AstraScreenshotCaptureModel::~AstraScreenshotCaptureModel() = default;

// =========================================================================
// Observer management
// =========================================================================

void AstraScreenshotCaptureModel::AddObserver(
    AstraScreenshotCaptureModelObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraScreenshotCaptureModel::RemoveObserver(
    AstraScreenshotCaptureModelObserver* observer) {
  observers_.RemoveObserver(observer);
}

// =========================================================================
// Capture state
// =========================================================================

void AstraScreenshotCaptureModel::SetCaptureType(AstraScreenshotType type) {
  if (capture_type_ == type) return;
  capture_type_ = type;
  NotifyCaptureTypeChanged();
}

void AstraScreenshotCaptureModel::StartCapture(AstraScreenshotType type) {
  capture_type_ = type;
  capture_bitmap_.reset();
  last_error_.clear();
  SetCaptureState(AstraScreenshotCaptureState::kCapturing);

  for (auto& observer : observers_) {
    observer.OnCaptureStarted();
  }
  NotifyCaptureTypeChanged();
}

void AstraScreenshotCaptureModel::CompleteCapture(const SkBitmap& bitmap,
                                                  AstraScreenshotType type,
                                                  const gfx::Rect& source_bounds) {
  capture_bitmap_ = bitmap;
  capture_type_ = type;
  source_bounds_ = source_bounds;
  last_error_.clear();
  SetCaptureState(AstraScreenshotCaptureState::kReady);

  for (auto& observer : observers_) {
    observer.OnCaptureCompleted(bitmap);
  }
}

void AstraScreenshotCaptureModel::FailCapture(const std::string& error_message) {
  last_error_ = error_message;
  SetCaptureState(AstraScreenshotCaptureState::kError);

  for (auto& observer : observers_) {
    observer.OnCaptureFailed(error_message);
  }
}

void AstraScreenshotCaptureModel::ResetToIdle() {
  capture_bitmap_.reset();
  last_error_.clear();
  source_bounds_ = gfx::Rect();
  SetCaptureState(AstraScreenshotCaptureState::kIdle);
}

// =========================================================================
// Save / copy operations
// =========================================================================

void AstraScreenshotCaptureModel::StartSave() {
  SetCaptureState(AstraScreenshotCaptureState::kSaving);
}

void AstraScreenshotCaptureModel::CompleteSave(const base::FilePath& path,
                                               int64_t file_size_bytes) {
  SetCaptureState(AstraScreenshotCaptureState::kReady);

  for (auto& observer : observers_) {
    observer.OnSaveCompleted(path);
  }
}

void AstraScreenshotCaptureModel::CompleteCopy() {
  for (auto& observer : observers_) {
    observer.OnCopyCompleted();
  }
}

// =========================================================================
// Region selection
// =========================================================================

void AstraScreenshotCaptureModel::SetRegionFromPoints(const gfx::Point& start,
                                                      const gfx::Point& end) {
  int x = std::min(start.x(), end.x());
  int y = std::min(start.y(), end.y());
  int width = std::abs(end.x() - start.x());
  int height = std::abs(end.y() - start.y());

  selected_region_ = gfx::Rect(x, y, width, height);
  has_selected_region_ = true;
  NotifyRegionChanged();
}

void AstraScreenshotCaptureModel::SetSelectedRegion(const gfx::Rect& region) {
  selected_region_ = region;
  has_selected_region_ = !region.IsEmpty();
  NotifyRegionChanged();
}

void AstraScreenshotCaptureModel::ConstrainRegionToBounds(const gfx::Rect& bounds) {
  if (!has_selected_region_) return;

  gfx::Rect constrained = selected_region_;

  // Clamp position
  if (constrained.x() < bounds.x()) {
    constrained.set_width(constrained.width() +
                          (constrained.x() - bounds.x()));
    constrained.set_x(bounds.x());
  }
  if (constrained.y() < bounds.y()) {
    constrained.set_height(constrained.height() +
                           (constrained.y() - bounds.y()));
    constrained.set_y(bounds.y());
  }

  // Clamp size
  if (constrained.right() > bounds.right()) {
    constrained.set_width(bounds.right() - constrained.x());
  }
  if (constrained.bottom() > bounds.bottom()) {
    constrained.set_height(bounds.bottom() - constrained.y());
  }

  // Ensure minimum size
  if (constrained.width() < kMinSelectionSize) {
    constrained.set_width(kMinSelectionSize);
  }
  if (constrained.height() < kMinSelectionSize) {
    constrained.set_height(kMinSelectionSize);
  }

  // If after clamping the region is still outside bounds, move it
  if (constrained.right() > bounds.right()) {
    constrained.set_x(bounds.right() - constrained.width());
  }
  if (constrained.bottom() > bounds.bottom()) {
    constrained.set_y(bounds.bottom() - constrained.height());
  }

  selected_region_ = constrained;
  NotifyRegionChanged();
}

void AstraScreenshotCaptureModel::SnapRegionToAspectRatio(
    AstraScreenshotRegionHandle anchor_handle) {
  if (!has_selected_region_) return;

  double ratio = GetCurrentAspectRatio();
  if (ratio <= 0.0) return;  // Free aspect ratio

  gfx::Rect rect = selected_region_;
  int new_width = rect.width();
  int new_height = rect.height();

  // Determine which corner/point stays fixed
  gfx::Point fixed_point;
  switch (anchor_handle) {
    case AstraScreenshotRegionHandle::kTopLeft:
      fixed_point = rect.origin();
      break;
    case AstraScreenshotRegionHandle::kTop:
      fixed_point = rect.top_center();
      break;
    case AstraScreenshotRegionHandle::kTopRight:
      fixed_point = rect.top_right();
      break;
    case AstraScreenshotRegionHandle::kLeft:
      fixed_point = rect.left_center();
      break;
    case AstraScreenshotRegionHandle::kRight:
      fixed_point = rect.right_center();
      break;
    case AstraScreenshotRegionHandle::kBottomLeft:
      fixed_point = rect.bottom_left();
      break;
    case AstraScreenshotRegionHandle::kBottom:
      fixed_point = rect.bottom_center();
      break;
    case AstraScreenshotRegionHandle::kBottomRight:
      fixed_point = rect.bottom_right();
      break;
    case AstraScreenshotRegionHandle::kNone:
      // Center as anchor
      fixed_point = rect.CenterPoint();
      break;
  }

  // Decide whether to adjust width or height based on current proportions
  double current_ratio =
      static_cast<double>(rect.width()) / rect.height();
  if (current_ratio > ratio) {
    // Wider than target ratio — adjust width
    new_width = static_cast<int>(rect.height() * ratio);
  } else {
    // Taller than target ratio — adjust height
    new_height = static_cast<int>(rect.width() / ratio);
  }

  // Ensure minimum size
  if (new_width < kMinSelectionSize) {
    new_width = kMinSelectionSize;
    new_height = static_cast<int>(new_width / ratio);
  }
  if (new_height < kMinSelectionSize) {
    new_height = kMinSelectionSize;
    new_width = static_cast<int>(new_height * ratio);
  }

  // Position the new rect based on the anchor point
  gfx::Rect new_rect(new_width, new_height);
  switch (anchor_handle) {
    case AstraScreenshotRegionHandle::kTopLeft:
      new_rect.set_origin(fixed_point);
      break;
    case AstraScreenshotRegionHandle::kTop:
      new_rect.set_x(fixed_point.x() - new_width / 2);
      new_rect.set_y(fixed_point.y());
      break;
    case AstraScreenshotRegionHandle::kTopRight:
      new_rect.set_x(fixed_point.x() - new_width);
      new_rect.set_y(fixed_point.y());
      break;
    case AstraScreenshotRegionHandle::kLeft:
      new_rect.set_x(fixed_point.x());
      new_rect.set_y(fixed_point.y() - new_height / 2);
      break;
    case AstraScreenshotRegionHandle::kRight:
      new_rect.set_x(fixed_point.x() - new_width);
      new_rect.set_y(fixed_point.y() - new_height / 2);
      break;
    case AstraScreenshotRegionHandle::kBottomLeft:
      new_rect.set_x(fixed_point.x());
      new_rect.set_y(fixed_point.y() - new_height);
      break;
    case AstraScreenshotRegionHandle::kBottom:
      new_rect.set_x(fixed_point.x() - new_width / 2);
      new_rect.set_y(fixed_point.y() - new_height);
      break;
    case AstraScreenshotRegionHandle::kBottomRight:
      new_rect.set_x(fixed_point.x() - new_width);
      new_rect.set_y(fixed_point.y() - new_height);
      break;
    case AstraScreenshotRegionHandle::kNone:
      new_rect.set_x(fixed_point.x() - new_width / 2);
      new_rect.set_y(fixed_point.y() - new_height / 2);
      break;
  }

  selected_region_ = new_rect;
  NotifyRegionChanged();
}

void AstraScreenshotCaptureModel::MoveRegion(const gfx::Vector2d& delta) {
  if (!has_selected_region_) return;
  selected_region_ += delta;
  NotifyRegionChanged();
}

void AstraScreenshotCaptureModel::ResizeRegion(AstraScreenshotRegionHandle handle,
                                               const gfx::Point& point) {
  if (!has_selected_region_ || handle == AstraScreenshotRegionHandle::kNone)
    return;

  gfx::Rect new_rect = selected_region_;

  switch (handle) {
    case AstraScreenshotRegionHandle::kTopLeft:
      new_rect.set_origin(point);
      new_rect.set_width(selected_region_.right() - point.x());
      new_rect.set_height(selected_region_.bottom() - point.y());
      break;
    case AstraScreenshotRegionHandle::kTop:
      new_rect.set_y(point.y());
      new_rect.set_height(selected_region_.bottom() - point.y());
      break;
    case AstraScreenshotRegionHandle::kTopRight:
      new_rect.set_y(point.y());
      new_rect.set_width(point.x() - selected_region_.x());
      new_rect.set_height(selected_region_.bottom() - point.y());
      break;
    case AstraScreenshotRegionHandle::kLeft:
      new_rect.set_x(point.x());
      new_rect.set_width(selected_region_.right() - point.x());
      break;
    case AstraScreenshotRegionHandle::kRight:
      new_rect.set_width(point.x() - selected_region_.x());
      break;
    case AstraScreenshotRegionHandle::kBottomLeft:
      new_rect.set_x(point.x());
      new_rect.set_width(selected_region_.right() - point.x());
      new_rect.set_height(point.y() - selected_region_.y());
      break;
    case AstraScreenshotRegionHandle::kBottom:
      new_rect.set_height(point.y() - selected_region_.y());
      break;
    case AstraScreenshotRegionHandle::kBottomRight:
      new_rect.set_width(point.x() - selected_region_.x());
      new_rect.set_height(point.y() - selected_region_.y());
      break;
    case AstraScreenshotRegionHandle::kNone:
      return;
  }

  // Normalize (in case width/height went negative)
  new_rect = new_rect.Standardized();

  // Enforce minimum size
  if (new_rect.width() < kMinSelectionSize) {
    new_rect.set_width(kMinSelectionSize);
  }
  if (new_rect.height() < kMinSelectionSize) {
    new_rect.set_height(kMinSelectionSize);
  }

  // Apply aspect ratio lock if enabled
  double ratio = GetCurrentAspectRatio();
  if (ratio > 0.0) {
    gfx::Point fixed_point;
    // Determine the fixed point based on the handle being dragged
    // (the opposite corner stays fixed)
    switch (handle) {
      case AstraScreenshotRegionHandle::kTopLeft:
        fixed_point = selected_region_.bottom_right();
        break;
      case AstraScreenshotRegionHandle::kTop:
        fixed_point = selected_region_.bottom_center();
        break;
      case AstraScreenshotRegionHandle::kTopRight:
        fixed_point = selected_region_.bottom_left();
        break;
      case AstraScreenshotRegionHandle::kLeft:
        fixed_point = selected_region_.right_center();
        break;
      case AstraScreenshotRegionHandle::kRight:
        fixed_point = selected_region_.left_center();
        break;
      case AstraScreenshotRegionHandle::kBottomLeft:
        fixed_point = selected_region_.top_right();
        break;
      case AstraScreenshotRegionHandle::kBottom:
        fixed_point = selected_region_.top_center();
        break;
      case AstraScreenshotRegionHandle::kBottomRight:
        fixed_point = selected_region_.origin();
        break;
      case AstraScreenshotRegionHandle::kNone:
        fixed_point = new_rect.CenterPoint();
        break;
    }
    ApplyAspectRatioToResize(handle, new_rect, fixed_point);
  }

  selected_region_ = new_rect;
  NotifyRegionChanged();
}

void AstraScreenshotCaptureModel::NudgeRegion(const gfx::Vector2d& delta) {
  if (!has_selected_region_) return;
  MoveRegion(delta);
}

void AstraScreenshotCaptureModel::NudgeResizeRegion(const gfx::Vector2d& delta) {
  if (!has_selected_region_) return;

  gfx::Rect new_rect = selected_region_;
  new_rect.set_width(selected_region_.width() + delta.x());
  new_rect.set_height(selected_region_.height() + delta.y());

  // Enforce minimum size
  if (new_rect.width() < kMinSelectionSize) {
    new_rect.set_width(kMinSelectionSize);
  }
  if (new_rect.height() < kMinSelectionSize) {
    new_rect.set_height(kMinSelectionSize);
  }

  selected_region_ = new_rect;
  NotifyRegionChanged();
}

void AstraScreenshotCaptureModel::SnapRegionToGrid(int grid_size) {
  if (!has_selected_region_ || grid_size <= 0) return;

  gfx::Rect snapped = selected_region_;

  // Snap left edge
  snapped.set_x((snapped.x() + grid_size / 2) / grid_size * grid_size);
  // Snap top edge
  snapped.set_y((snapped.y() + grid_size / 2) / grid_size * grid_size);
  // Snap right edge
  int right = (snapped.right() + grid_size / 2) / grid_size * grid_size;
  snapped.set_width(right - snapped.x());
  // Snap bottom edge
  int bottom = (snapped.bottom() + grid_size / 2) / grid_size * grid_size;
  snapped.set_height(bottom - snapped.y());

  // Ensure minimum size
  if (snapped.width() < kMinSelectionSize) {
    snapped.set_width(kMinSelectionSize);
  }
  if (snapped.height() < kMinSelectionSize) {
    snapped.set_height(kMinSelectionSize);
  }

  selected_region_ = snapped;
  NotifyRegionChanged();
}

void AstraScreenshotCaptureModel::ClearRegion() {
  selected_region_ = gfx::Rect();
  has_selected_region_ = false;
  NotifyRegionChanged();
}

// =========================================================================
// Aspect ratio utilities
// =========================================================================

// static
double AstraScreenshotCaptureModel::GetAspectRatioValue(
    AstraScreenshotAspectRatioLock mode) {
  switch (mode) {
    case AstraScreenshotAspectRatioLock::kFree:
      return 0.0;
    case AstraScreenshotAspectRatioLock::kRatio4x3:
      return 4.0 / 3.0;
    case AstraScreenshotAspectRatioLock::kRatio16x9:
      return 16.0 / 9.0;
    case AstraScreenshotAspectRatioLock::kRatio1x1:
      return 1.0;
  }
  return 0.0;
}

// =========================================================================
// Capture settings (persisted via PrefService)
// =========================================================================

AstraScreenshotType AstraScreenshotCaptureModel::GetDefaultCaptureType() const {
  if (!pref_service_) {
    return static_cast<AstraScreenshotType>(
        prefs::kDefaultScreenshotDefaultCaptureType);
  }
  int value = pref_service_->GetInteger(prefs::kPrefScreenshotDefaultCaptureType);
  // Clamp to valid enum range.
  value = Clamp(value, 0, 2);
  return static_cast<AstraScreenshotType>(value);
}

void AstraScreenshotCaptureModel::SetDefaultCaptureType(
    AstraScreenshotType type) {
  if (!pref_service_) return;
  if (GetDefaultCaptureType() == type) return;
  pref_service_->SetInteger(prefs::kPrefScreenshotDefaultCaptureType,
                             static_cast<int>(type));
  NotifySettingsChanged();
}

bool AstraScreenshotCaptureModel::GetShowCaptureBubble() const {
  if (!pref_service_) return prefs::kDefaultScreenshotShowCaptureBubble;
  return pref_service_->GetBoolean(prefs::kPrefScreenshotShowCaptureBubble);
}

void AstraScreenshotCaptureModel::SetShowCaptureBubble(bool show) {
  if (!pref_service_) return;
  if (GetShowCaptureBubble() == show) return;
  pref_service_->SetBoolean(prefs::kPrefScreenshotShowCaptureBubble, show);
  NotifySettingsChanged();
}

bool AstraScreenshotCaptureModel::GetAutoDismissBubble() const {
  if (!pref_service_) return prefs::kDefaultScreenshotAutoDismissBubble;
  return pref_service_->GetBoolean(prefs::kPrefScreenshotAutoDismissBubble);
}

void AstraScreenshotCaptureModel::SetAutoDismissBubble(bool auto_dismiss) {
  if (!pref_service_) return;
  if (GetAutoDismissBubble() == auto_dismiss) return;
  pref_service_->SetBoolean(prefs::kPrefScreenshotAutoDismissBubble,
                             auto_dismiss);
  NotifySettingsChanged();
}

int AstraScreenshotCaptureModel::GetAutoDismissDelaySeconds() const {
  if (!pref_service_) {
    return prefs::kDefaultScreenshotAutoDismissDelaySeconds;
  }
  int value =
      pref_service_->GetInteger(prefs::kPrefScreenshotAutoDismissDelaySeconds);
  return ClampAutoDismissDelay(value);
}

void AstraScreenshotCaptureModel::SetAutoDismissDelaySeconds(int seconds) {
  if (!pref_service_) return;
  seconds = ClampAutoDismissDelay(seconds);
  if (GetAutoDismissDelaySeconds() == seconds) return;
  pref_service_->SetInteger(prefs::kPrefScreenshotAutoDismissDelaySeconds,
                             seconds);
  NotifySettingsChanged();
}

AstraScreenshotImageFormatModel AstraScreenshotCaptureModel::GetImageFormat()
    const {
  if (!pref_service_) {
    return static_cast<AstraScreenshotImageFormatModel>(
        prefs::kDefaultScreenshotImageFormat);
  }
  int value = pref_service_->GetInteger(prefs::kPrefScreenshotImageFormat);
  value = Clamp(value, 0, 2);
  return static_cast<AstraScreenshotImageFormatModel>(value);
}

void AstraScreenshotCaptureModel::SetImageFormat(
    AstraScreenshotImageFormatModel format) {
  if (!pref_service_) return;
  if (GetImageFormat() == format) return;
  pref_service_->SetInteger(prefs::kPrefScreenshotImageFormat,
                             static_cast<int>(format));
  NotifySettingsChanged();
}

int AstraScreenshotCaptureModel::GetJpegQuality() const {
  if (!pref_service_) return prefs::kDefaultScreenshotJpegQuality;
  int value = pref_service_->GetInteger(prefs::kPrefScreenshotJpegQuality);
  return ClampJpegQuality(value);
}

void AstraScreenshotCaptureModel::SetJpegQuality(int quality) {
  if (!pref_service_) return;
  quality = ClampJpegQuality(quality);
  if (GetJpegQuality() == quality) return;
  pref_service_->SetInteger(prefs::kPrefScreenshotJpegQuality, quality);
  NotifySettingsChanged();
}

bool AstraScreenshotCaptureModel::GetShowFilenameInBubble() const {
  if (!pref_service_) {
    return prefs::kDefaultScreenshotShowFilenameInBubble;
  }
  return pref_service_->GetBoolean(prefs::kPrefScreenshotShowFilenameInBubble);
}

void AstraScreenshotCaptureModel::SetShowFilenameInBubble(bool show) {
  if (!pref_service_) return;
  if (GetShowFilenameInBubble() == show) return;
  pref_service_->SetBoolean(prefs::kPrefScreenshotShowFilenameInBubble, show);
  NotifySettingsChanged();
}

bool AstraScreenshotCaptureModel::GetShowDimensionsInBubble() const {
  if (!pref_service_) {
    return prefs::kDefaultScreenshotShowDimensionsInBubble;
  }
  return pref_service_->GetBoolean(
      prefs::kPrefScreenshotShowDimensionsInBubble);
}

void AstraScreenshotCaptureModel::SetShowDimensionsInBubble(bool show) {
  if (!pref_service_) return;
  if (GetShowDimensionsInBubble() == show) return;
  pref_service_->SetBoolean(prefs::kPrefScreenshotShowDimensionsInBubble,
                             show);
  NotifySettingsChanged();
}

bool AstraScreenshotCaptureModel::GetShowFileSizeInBubble() const {
  if (!pref_service_) {
    return prefs::kDefaultScreenshotShowFileSizeInBubble;
  }
  return pref_service_->GetBoolean(prefs::kPrefScreenshotShowFileSizeInBubble);
}

void AstraScreenshotCaptureModel::SetShowFileSizeInBubble(bool show) {
  if (!pref_service_) return;
  if (GetShowFileSizeInBubble() == show) return;
  pref_service_->SetBoolean(prefs::kPrefScreenshotShowFileSizeInBubble, show);
  NotifySettingsChanged();
}

AstraScreenshotSaveLocation AstraScreenshotCaptureModel::GetDefaultSaveLocation()
    const {
  if (!pref_service_) {
    // Default to downloads.
    return AstraScreenshotSaveLocation::kDownloads;
  }
  std::string value =
      pref_service_->GetString(prefs::kPrefScreenshotDefaultSaveLocation);
  if (value == "clipboard") return AstraScreenshotSaveLocation::kClipboard;
  if (value == "ask") return AstraScreenshotSaveLocation::kAsk;
  return AstraScreenshotSaveLocation::kDownloads;
}

void AstraScreenshotCaptureModel::SetDefaultSaveLocation(
    AstraScreenshotSaveLocation location) {
  if (!pref_service_) return;
  if (GetDefaultSaveLocation() == location) return;

  std::string value;
  switch (location) {
    case AstraScreenshotSaveLocation::kDownloads:
      value = "downloads";
      break;
    case AstraScreenshotSaveLocation::kClipboard:
      value = "clipboard";
      break;
    case AstraScreenshotSaveLocation::kAsk:
      value = "ask";
      break;
  }
  pref_service_->SetString(prefs::kPrefScreenshotDefaultSaveLocation, value);
  NotifySettingsChanged();
}

bool AstraScreenshotCaptureModel::GetCopyToClipboardAfterCapture() const {
  if (!pref_service_) {
    return prefs::kDefaultScreenshotCopyToClipboardAfterCapture;
  }
  return pref_service_->GetBoolean(
      prefs::kPrefScreenshotCopyToClipboardAfterCapture);
}

void AstraScreenshotCaptureModel::SetCopyToClipboardAfterCapture(bool enabled) {
  if (!pref_service_) return;
  if (GetCopyToClipboardAfterCapture() == enabled) return;
  pref_service_->SetBoolean(
      prefs::kPrefScreenshotCopyToClipboardAfterCapture, enabled);
  NotifySettingsChanged();
}

bool AstraScreenshotCaptureModel::GetShowGridInRegionSelection() const {
  if (!pref_service_) {
    return prefs::kDefaultScreenshotShowGridInRegionSelection;
  }
  return pref_service_->GetBoolean(
      prefs::kPrefScreenshotShowGridInRegionSelection);
}

void AstraScreenshotCaptureModel::SetShowGridInRegionSelection(bool show) {
  if (!pref_service_) return;
  if (GetShowGridInRegionSelection() == show) return;
  pref_service_->SetBoolean(
      prefs::kPrefScreenshotShowGridInRegionSelection, show);
  NotifySettingsChanged();
}

bool AstraScreenshotCaptureModel::GetShowMagnifierInRegionSelection() const {
  if (!pref_service_) {
    return prefs::kDefaultScreenshotShowMagnifierInRegionSelection;
  }
  return pref_service_->GetBoolean(
      prefs::kPrefScreenshotShowMagnifierInRegionSelection);
}

void AstraScreenshotCaptureModel::SetShowMagnifierInRegionSelection(bool show) {
  if (!pref_service_) return;
  if (GetShowMagnifierInRegionSelection() == show) return;
  pref_service_->SetBoolean(
      prefs::kPrefScreenshotShowMagnifierInRegionSelection, show);
  NotifySettingsChanged();
}

AstraScreenshotAspectRatioLock
AstraScreenshotCaptureModel::GetRegionAspectRatioLock() const {
  if (!pref_service_) {
    return static_cast<AstraScreenshotAspectRatioLock>(
        prefs::kDefaultScreenshotRegionAspectRatioLock);
  }
  int value =
      pref_service_->GetInteger(prefs::kPrefScreenshotRegionAspectRatioLock);
  value = Clamp(value, 0, 3);
  return static_cast<AstraScreenshotAspectRatioLock>(value);
}

void AstraScreenshotCaptureModel::SetRegionAspectRatioLock(
    AstraScreenshotAspectRatioLock mode) {
  if (!pref_service_) return;
  if (GetRegionAspectRatioLock() == mode) return;
  pref_service_->SetInteger(prefs::kPrefScreenshotRegionAspectRatioLock,
                             static_cast<int>(mode));
  NotifySettingsChanged();
}

int AstraScreenshotCaptureModel::GetGridSizePixels() const {
  if (!pref_service_) return prefs::kDefaultScreenshotGridSizePixels;
  int value = pref_service_->GetInteger(prefs::kPrefScreenshotGridSizePixels);
  return ClampGridSize(value);
}

void AstraScreenshotCaptureModel::SetGridSizePixels(int size) {
  if (!pref_service_) return;
  size = ClampGridSize(size);
  if (GetGridSizePixels() == size) return;
  pref_service_->SetInteger(prefs::kPrefScreenshotGridSizePixels, size);
  NotifySettingsChanged();
}

bool AstraScreenshotCaptureModel::GetSnapToGrid() const {
  if (!pref_service_) return prefs::kDefaultScreenshotSnapToGrid;
  return pref_service_->GetBoolean(prefs::kPrefScreenshotSnapToGrid);
}

void AstraScreenshotCaptureModel::SetSnapToGrid(bool enabled) {
  if (!pref_service_) return;
  if (GetSnapToGrid() == enabled) return;
  pref_service_->SetBoolean(prefs::kPrefScreenshotSnapToGrid, enabled);
  NotifySettingsChanged();
}

int AstraScreenshotCaptureModel::GetMaxRecentCaptures() const {
  if (!pref_service_) return prefs::kDefaultScreenshotMaxRecentCaptures;
  int value = pref_service_->GetInteger(prefs::kPrefScreenshotMaxRecentCaptures);
  return ClampMaxRecentCaptures(value);
}

void AstraScreenshotCaptureModel::SetMaxRecentCaptures(int max) {
  if (!pref_service_) return;
  max = ClampMaxRecentCaptures(max);
  if (GetMaxRecentCaptures() == max) return;
  pref_service_->SetInteger(prefs::kPrefScreenshotMaxRecentCaptures, max);

  // Truncate existing list if needed.
  if (recent_captures_.size() > static_cast<size_t>(max)) {
    recent_captures_.resize(max);
  }
  NotifySettingsChanged();
}

// =========================================================================
// Recent captures
// =========================================================================

void AstraScreenshotCaptureModel::AddRecentCapture(
    const AstraScreenshotRecentCapture& capture) {
  int max = GetMaxRecentCaptures();
  if (max <= 0) return;

  // Insert at the beginning (most recent first).
  recent_captures_.insert(recent_captures_.begin(), capture);

  // Truncate to max size.
  if (recent_captures_.size() > static_cast<size_t>(max)) {
    recent_captures_.resize(max);
  }
}

void AstraScreenshotCaptureModel::ClearRecentCaptures() {
  recent_captures_.clear();
}

// =========================================================================
// Utility methods
// =========================================================================

int64_t AstraScreenshotCaptureModel::EstimateFileSize() const {
  if (capture_bitmap_.isNull() || capture_bitmap_.empty()) {
    return 0;
  }

  int pixel_count = capture_bitmap_.width() * capture_bitmap_.height();
  AstraScreenshotImageFormatModel format = GetImageFormat();

  switch (format) {
    case AstraScreenshotImageFormatModel::kPng:
      // Rough estimate: ~1 byte per pixel for typical screenshots.
      return pixel_count;
    case AstraScreenshotImageFormatModel::kJpeg: {
      // Estimate based on quality setting.
      // At quality 85: ~0.5 bytes per pixel.
      // At quality 100: ~1.5 bytes per pixel.
      // At quality 1: ~0.05 bytes per pixel.
      double quality_factor = GetJpegQuality() / 100.0;
      double bytes_per_pixel = 0.05 + quality_factor * 1.45;
      return static_cast<int64_t>(pixel_count * bytes_per_pixel);
    }
    case AstraScreenshotImageFormatModel::kWebP:
      // Rough estimate: ~0.6 bytes per pixel for WebP.
      return static_cast<int64_t>(pixel_count * 0.6);
  }
  return pixel_count;
}

// static
std::string AstraScreenshotCaptureModel::GenerateDefaultFilename(
    AstraScreenshotType type,
    AstraScreenshotImageFormatModel format) {
  base::Time now = base::Time::Now();
  std::u16string date_str = base::TimeFormatShortDate(now);
  std::u16string time_str = base::TimeFormatTimeOfDay(now);

  // Build filename: "Screenshot Type YYYY-MM-DD at HH.MM.SS.ext"
  std::string result = "Screenshot ";
  result += GetCaptureTypeShortLabel(type);
  result += " ";
  result += base::UTF16ToUTF8(date_str);
  result += ".";
  result += base::UTF16ToUTF8(time_str);
  // Replace colons with dots for filename safety.
  std::replace(result.begin(), result.end(), ':', '.');
  result += ".";
  result += GetExtensionForFormat(format);
  return result;
}

// static
int AstraScreenshotCaptureModel::ClampJpegQuality(int quality) {
  return Clamp(quality, prefs::kMinScreenshotJpegQuality,
               prefs::kMaxScreenshotJpegQuality);
}

// static
int AstraScreenshotCaptureModel::ClampGridSize(int size) {
  return Clamp(size, prefs::kMinScreenshotGridSizePixels,
               prefs::kMaxScreenshotGridSizePixels);
}

// static
int AstraScreenshotCaptureModel::ClampMaxRecentCaptures(int max) {
  return Clamp(max, prefs::kMinScreenshotMaxRecentCaptures,
               prefs::kMaxScreenshotMaxRecentCaptures);
}

// static
int AstraScreenshotCaptureModel::ClampAutoDismissDelay(int seconds) {
  return Clamp(seconds, kMinAutoDismissSeconds, kMaxAutoDismissSeconds);
}

// =========================================================================
// Private helpers
// =========================================================================

void AstraScreenshotCaptureModel::SetCaptureState(
    AstraScreenshotCaptureState state) {
  if (capture_state_ == state) return;
  capture_state_ = state;
  for (auto& observer : observers_) {
    observer.OnCaptureStateChanged(state);
  }
}

void AstraScreenshotCaptureModel::NotifyRegionChanged() {
  for (auto& observer : observers_) {
    observer.OnRegionChanged(selected_region_);
  }
}

void AstraScreenshotCaptureModel::NotifySettingsChanged() {
  for (auto& observer : observers_) {
    observer.OnCaptureSettingsChanged();
  }
}

void AstraScreenshotCaptureModel::NotifyCaptureTypeChanged() {
  for (auto& observer : observers_) {
    observer.OnCaptureTypeChanged(capture_type_);
  }
}

double AstraScreenshotCaptureModel::GetCurrentAspectRatio() const {
  return GetAspectRatioValue(GetRegionAspectRatioLock());
}

void AstraScreenshotCaptureModel::ApplyAspectRatioToResize(
    AstraScreenshotRegionHandle handle,
    gfx::Rect& rect,
    const gfx::Point& fixed_point) const {
  double ratio = GetCurrentAspectRatio();
  if (ratio <= 0.0) return;

  // Calculate the new dimensions maintaining aspect ratio from the
  // fixed point.
  int new_width, new_height;

  // Determine which dimension drives the aspect ratio based on the handle.
  bool is_corner_handle =
      (handle == AstraScreenshotRegionHandle::kTopLeft) ||
      (handle == AstraScreenshotRegionHandle::kTopRight) ||
      (handle == AstraScreenshotRegionHandle::kBottomLeft) ||
      (handle == AstraScreenshotRegionHandle::kBottomRight);

  if (is_corner_handle) {
    // For corner handles, use the larger delta to determine size.
    int dx = std::abs(rect.right() - fixed_point.x());
    int dy = std::abs(rect.bottom() - fixed_point.y());

    if (dx > dy * ratio) {
      // Width is the driver, compute height from width.
      new_width = dx;
      new_height = static_cast<int>(dx / ratio);
    } else {
      // Height is the driver, compute width from height.
      new_height = dy;
      new_width = static_cast<int>(dy * ratio);
    }
  } else if (handle == AstraScreenshotRegionHandle::kTop ||
             handle == AstraScreenshotRegionHandle::kBottom) {
    // Top/bottom handles: height drives, width is computed.
    new_height = rect.height();
    new_width = static_cast<int>(new_height * ratio);
  } else {
    // Left/right handles: width drives, height is computed.
    new_width = rect.width();
    new_height = static_cast<int>(new_width / ratio);
  }

  // Ensure minimum size
  if (new_width < kMinSelectionSize) {
    new_width = kMinSelectionSize;
    new_height = static_cast<int>(new_width / ratio);
  }
  if (new_height < kMinSelectionSize) {
    new_height = kMinSelectionSize;
    new_width = static_cast<int>(new_height * ratio);
  }

  // Position based on fixed point
  gfx::Rect new_rect(new_width, new_height);
  switch (handle) {
    case AstraScreenshotRegionHandle::kTopLeft:
      // Fixed point is bottom-right
      new_rect.set_x(fixed_point.x() - new_width);
      new_rect.set_y(fixed_point.y() - new_height);
      break;
    case AstraScreenshotRegionHandle::kTop:
      // Fixed point is bottom center
      new_rect.set_x(fixed_point.x() - new_width / 2);
      new_rect.set_y(fixed_point.y() - new_height);
      break;
    case AstraScreenshotRegionHandle::kTopRight:
      // Fixed point is bottom-left
      new_rect.set_x(fixed_point.x());
      new_rect.set_y(fixed_point.y() - new_height);
      break;
    case AstraScreenshotRegionHandle::kLeft:
      // Fixed point is right center
      new_rect.set_x(fixed_point.x() - new_width);
      new_rect.set_y(fixed_point.y() - new_height / 2);
      break;
    case AstraScreenshotRegionHandle::kRight:
      // Fixed point is left center
      new_rect.set_x(fixed_point.x());
      new_rect.set_y(fixed_point.y() - new_height / 2);
      break;
    case AstraScreenshotRegionHandle::kBottomLeft:
      // Fixed point is top-right
      new_rect.set_x(fixed_point.x() - new_width);
      new_rect.set_y(fixed_point.y());
      break;
    case AstraScreenshotRegionHandle::kBottom:
      // Fixed point is top center
      new_rect.set_x(fixed_point.x() - new_width / 2);
      new_rect.set_y(fixed_point.y());
      break;
    case AstraScreenshotRegionHandle::kBottomRight:
      // Fixed point is origin
      new_rect.set_origin(fixed_point);
      break;
    case AstraScreenshotRegionHandle::kNone:
      new_rect.set_x(fixed_point.x() - new_width / 2);
      new_rect.set_y(fixed_point.y() - new_height / 2);
      break;
  }

  rect = new_rect;
}

}  // namespace astra
