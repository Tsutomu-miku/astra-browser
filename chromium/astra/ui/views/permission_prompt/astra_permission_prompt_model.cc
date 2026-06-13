// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/permission_prompt/astra_permission_prompt_model.h"

#include <algorithm>

#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"

namespace astra {

namespace {

// Permission metadata struct.
struct PermissionInfo {
  AstraPermissionType type;
  const char* name;
  const char* description;
  const char* icon_name;
  AstraPermissionImportance importance;
  bool supports_one_time;
};

const PermissionInfo kPermissionInfos[] = {
    {AstraPermissionType::kCamera, "Camera",
     "Use your camera", "camera",
     AstraPermissionImportance::kHigh, true},
    {AstraPermissionType::kMicrophone, "Microphone",
     "Use your microphone", "mic",
     AstraPermissionImportance::kHigh, true},
    {AstraPermissionType::kCameraAndMicrophone, "Camera and Microphone",
     "Use your camera and microphone", "camera_mic",
     AstraPermissionImportance::kCritical, true},
    {AstraPermissionType::kGeolocation, "Location",
     "Know your location", "location",
     AstraPermissionImportance::kHigh, true},
    {AstraPermissionType::kNotifications, "Notifications",
     "Show notifications", "notifications",
     AstraPermissionImportance::kMedium, false},
    {AstraPermissionType::kMidi, "MIDI",
     "Use MIDI devices", "midi",
     AstraPermissionImportance::kLow, false},
    {AstraPermissionType::kMidiSysex, "MIDI SysEx",
     "Use MIDI devices with system exclusive messages", "midi",
     AstraPermissionImportance::kMedium, false},
    {AstraPermissionType::kClipboardRead, "Read Clipboard",
     "See text and images copied to the clipboard", "clipboard",
     AstraPermissionImportance::kHigh, true},
    {AstraPermissionType::kClipboardWrite, "Clipboard Write",
     "Modify text and images copied to the clipboard", "clipboard",
     AstraPermissionImportance::kLow, false},
    {AstraPermissionType::kFileSystemRead, "File System Read",
     "Read files you choose", "file",
     AstraPermissionImportance::kMedium, true},
    {AstraPermissionType::kFileSystemWrite, "File System Write",
     "Save changes to files you choose", "file",
     AstraPermissionImportance::kMedium, true},
    {AstraPermissionType::kBluetooth, "Bluetooth",
     "Connect to Bluetooth devices", "bluetooth",
     AstraPermissionImportance::kMedium, true},
    {AstraPermissionType::kUsb, "USB",
     "Connect to USB devices", "usb",
     AstraPermissionImportance::kMedium, true},
    {AstraPermissionType::kSerial, "Serial",
     "Connect to serial ports", "serial",
     AstraPermissionImportance::kMedium, true},
    {AstraPermissionType::kHid, "HID",
     "Connect to HID devices", "hid",
     AstraPermissionImportance::kMedium, true},
    {AstraPermissionType::kNfc, "NFC",
     "Use NFC to communicate with nearby devices", "nfc",
     AstraPermissionImportance::kMedium, true},
    {AstraPermissionType::kSensors, "Sensors",
     "Use motion and light sensors", "sensors",
     AstraPermissionImportance::kLow, false},
    {AstraPermissionType::kIdleDetection, "Idle Detection",
     "Detect when you're actively using your device", "idle",
     AstraPermissionImportance::kMedium, false},
    {AstraPermissionType::kPaymentHandler, "Payment Handler",
     "Handle payments on your behalf", "payment",
     AstraPermissionImportance::kHigh, false},
    {AstraPermissionType::kBackgroundSync, "Background Sync",
     "Send and receive data in the background", "sync",
     AstraPermissionImportance::kLow, false},
    {AstraPermissionType::kStorageAccess, "Storage Access",
     "Access storage on this device", "storage",
     AstraPermissionImportance::kMedium, false},
    {AstraPermissionType::kScreenCapture, "Screen Capture",
     "Share your screen", "screen",
     AstraPermissionImportance::kCritical, true},
    {AstraPermissionType::kWakeLock, "Wake Lock",
     "Prevent your device from sleeping", "wake_lock",
     AstraPermissionImportance::kLow, false},
    {AstraPermissionType::kPictureInPicture, "Picture in Picture",
     "Show picture-in-picture video", "pip",
     AstraPermissionImportance::kLow, false},
    {AstraPermissionType::kFullscreen, "Fullscreen",
     "Open in fullscreen mode", "fullscreen",
     AstraPermissionImportance::kMedium, false},
    {AstraPermissionType::kPointerLock, "Pointer Lock",
     "Lock your mouse pointer", "pointer_lock",
     AstraPermissionImportance::kMedium, false},
    {AstraPermissionType::kWebXR, "WebXR",
     "Use virtual and augmented reality devices", "vr",
     AstraPermissionImportance::kMedium, true},
    {AstraPermissionType::kAutoPlay, "Autoplay",
     "Play audio and video automatically", "autoplay",
     AstraPermissionImportance::kLow, false},
    {AstraPermissionType::kDownloads, "Downloads",
     "Download files", "download",
     AstraPermissionImportance::kLow, false},
    {AstraPermissionType::kMultipleDownloads, "Multiple Downloads",
     "Download multiple files automatically", "download",
     AstraPermissionImportance::kMedium, false},
    {AstraPermissionType::kPopups, "Popups",
     "Show pop-ups and redirects", "popup",
     AstraPermissionImportance::kMedium, false},
    {AstraPermissionType::kRedirects, "Redirects",
     "Allow automatic redirects", "redirect",
     AstraPermissionImportance::kLow, false},
    {AstraPermissionType::kAds, "Ads",
     "Show ads", "ad",
     AstraPermissionImportance::kMedium, false},
    {AstraPermissionType::kInsecureContent, "Insecure Content",
     "Load insecure content on secure pages", "insecure",
     AstraPermissionImportance::kHigh, false},
    {AstraPermissionType::kSound, "Sound",
     "Play sound", "sound",
     AstraPermissionImportance::kLow, false},
    {AstraPermissionType::kImageContentSettings, "Images",
     "Show images", "image",
     AstraPermissionImportance::kLow, false},
    {AstraPermissionType::kJavaScript, "JavaScript",
     "Run JavaScript", "js",
     AstraPermissionImportance::kHigh, false},
    {AstraPermissionType::kCookies, "Cookies",
     "Use cookies and site data", "cookie",
     AstraPermissionImportance::kMedium, false},
};

const PermissionInfo* FindPermissionInfo(AstraPermissionType type) {
  for (const auto& info : kPermissionInfos) {
    if (info.type == type) {
      return &info;
    }
  }
  return nullptr;
}

}  // namespace

// ===========================================================================
// Static helpers
// ===========================================================================

std::u16string AstraPermissionPromptModel::GetPermissionName(
    AstraPermissionType type) {
  const auto* info = FindPermissionInfo(type);
  return info ? base::UTF8ToUTF16(info->name) : std::u16string();
}

std::u16string AstraPermissionPromptModel::GetPermissionDescription(
    AstraPermissionType type) {
  const auto* info = FindPermissionInfo(type);
  return info ? base::UTF8ToUTF16(info->description) : std::u16string();
}

std::string AstraPermissionPromptModel::GetPermissionIconName(
    AstraPermissionType type) {
  const auto* info = FindPermissionInfo(type);
  return info ? info->icon_name : std::string();
}

AstraPermissionImportance
AstraPermissionPromptModel::GetPermissionImportance(
    AstraPermissionType type) {
  const auto* info = FindPermissionInfo(type);
  return info ? info->importance : AstraPermissionImportance::kMedium;
}

bool AstraPermissionPromptModel::SupportsOneTimePermission(
    AstraPermissionType type) {
  const auto* info = FindPermissionInfo(type);
  return info ? info->supports_one_time : false;
}

// ===========================================================================
// AstraPermissionPromptModel
// ===========================================================================

AstraPermissionPromptModel::AstraPermissionPromptModel() = default;

AstraPermissionPromptModel::~AstraPermissionPromptModel() {
  for (auto& observer : observers_) {
    observer.OnPermissionPromptModelShutdown(this);
  }
}

void AstraPermissionPromptModel::AddObserver(
    AstraPermissionPromptObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraPermissionPromptModel::RemoveObserver(
    AstraPermissionPromptObserver* observer) {
  observers_.RemoveObserver(observer);
}

std::string AstraPermissionPromptModel::AddRequest(
    const AstraPermissionRequest& request) {
  AstraPermissionRequest req = request;
  if (req.id.empty()) {
    req.id = "perm_" + base::NumberToString(next_request_id_++);
  }
  if (req.request_time.is_null()) {
    req.request_time = base::Time::Now();
  }

  requests_.push_back(std::move(req));
  const std::string& new_id = requests_.back().id;

  // Set as active if this is the first request.
  if (requests_.size() == 1) {
    active_request_index_ = 0;
    NotifyActiveRequestChanged(new_id);
  }

  NotifyPermissionRequested(new_id);
  return new_id;
}

void AstraPermissionPromptModel::RemoveRequest(
    const std::string& request_id) {
  int index = FindRequestIndex(request_id);
  if (index < 0) {
    return;
  }

  requests_.erase(requests_.begin() + index);

  // Update active index.
  if (active_request_index_ >= static_cast<int>(requests_.size())) {
    active_request_index_ = static_cast<int>(requests_.size()) - 1;
    if (active_request_index_ >= 0) {
      NotifyActiveRequestChanged(requests_[active_request_index_].id);
    }
  } else if (active_request_index_ == index) {
    // Active request was removed, move to next or previous.
    UpdateActiveRequestIndex();
  }

  NotifyPermissionDismissed(request_id);
}

const AstraPermissionRequest* AstraPermissionPromptModel::GetRequest(
    const std::string& request_id) const {
  for (const auto& req : requests_) {
    if (req.id == request_id) {
      return &req;
    }
  }
  return nullptr;
}

const std::vector<AstraPermissionRequest>&
AstraPermissionPromptModel::GetAllRequests() const {
  return requests_;
}

size_t AstraPermissionPromptModel::GetRequestCount() const {
  return requests_.size();
}

const AstraPermissionRequest*
AstraPermissionPromptModel::GetActiveRequest() const {
  if (active_request_index_ < 0 ||
      active_request_index_ >= static_cast<int>(requests_.size())) {
    return nullptr;
  }
  return &requests_[active_request_index_];
}

std::string AstraPermissionPromptModel::GetActiveRequestId() const {
  const auto* req = GetActiveRequest();
  return req ? req->id : std::string();
}

void AstraPermissionPromptModel::SetActiveRequest(
    const std::string& request_id) {
  int index = FindRequestIndex(request_id);
  if (index < 0 || index == active_request_index_) {
    return;
  }
  active_request_index_ = index;
  NotifyActiveRequestChanged(request_id);
}

void AstraPermissionPromptModel::NextRequest() {
  if (requests_.empty()) {
    return;
  }
  int next = active_request_index_ + 1;
  if (next >= static_cast<int>(requests_.size())) {
    next = 0;
  }
  active_request_index_ = next;
  NotifyActiveRequestChanged(requests_[active_request_index_].id);
}

void AstraPermissionPromptModel::PreviousRequest() {
  if (requests_.empty()) {
    return;
  }
  int prev = active_request_index_ - 1;
  if (prev < 0) {
    prev = static_cast<int>(requests_.size()) - 1;
  }
  active_request_index_ = prev;
  NotifyActiveRequestChanged(requests_[active_request_index_].id);
}

void AstraPermissionPromptModel::Allow() {
  if (active_request_index_ < 0) {
    return;
  }
  std::string id = requests_[active_request_index_].id;
  Decide(id, AstraPermissionAction::kAllow);
}

void AstraPermissionPromptModel::Block() {
  if (active_request_index_ < 0) {
    return;
  }
  std::string id = requests_[active_request_index_].id;
  Decide(id, AstraPermissionAction::kBlock);
}

void AstraPermissionPromptModel::AllowOnce() {
  if (active_request_index_ < 0) {
    return;
  }
  std::string id = requests_[active_request_index_].id;
  Decide(id, AstraPermissionAction::kAllowOnce);
}

void AstraPermissionPromptModel::Dismiss() {
  if (active_request_index_ < 0) {
    return;
  }
  std::string id = requests_[active_request_index_].id;
  Decide(id, AstraPermissionAction::kDismiss);
}

void AstraPermissionPromptModel::Decide(const std::string& request_id,
                                        AstraPermissionAction action) {
  auto* req = FindRequest(request_id);
  if (!req) {
    return;
  }

  // TODO(astra): Wire to PermissionRequestManager::Accept() / Deny().
  // Patch point: chrome/browser/permissions/permission_request_manager.cc

  NotifyPermissionDecided(request_id, action);

  // After decision, remove the request from the queue.
  RemoveRequest(request_id);
}

void AstraPermissionPromptModel::SetRememberDecision(bool remember) {
  remember_decision_ = remember;
}

bool AstraPermissionPromptModel::IsLoading() const {
  return loading_;
}

void AstraPermissionPromptModel::SetLoading(bool loading) {
  loading_ = loading;
}

bool AstraPermissionPromptModel::IsPromptVisible() const {
  return !requests_.empty();
}

void AstraPermissionPromptModel::ClearAllRequests() {
  requests_.clear();
  active_request_index_ = -1;
}

// ===========================================================================
// Private helpers
// ===========================================================================

void AstraPermissionPromptModel::NotifyPermissionRequested(
    const std::string& request_id) {
  for (auto& observer : observers_) {
    observer.OnPermissionRequested(this, request_id);
  }
}

void AstraPermissionPromptModel::NotifyPermissionDecided(
    const std::string& request_id,
    AstraPermissionAction action) {
  for (auto& observer : observers_) {
    observer.OnPermissionDecided(this, request_id, action);
  }
}

void AstraPermissionPromptModel::NotifyPermissionDismissed(
    const std::string& request_id) {
  for (auto& observer : observers_) {
    observer.OnPermissionDismissed(this, request_id);
  }
}

void AstraPermissionPromptModel::NotifyActiveRequestChanged(
    const std::string& request_id) {
  for (auto& observer : observers_) {
    observer.OnActiveRequestChanged(this, request_id);
  }
}

AstraPermissionRequest* AstraPermissionPromptModel::FindRequest(
    const std::string& request_id) {
  for (auto& req : requests_) {
    if (req.id == request_id) {
      return &req;
    }
  }
  return nullptr;
}

int AstraPermissionPromptModel::FindRequestIndex(
    const std::string& request_id) const {
  for (size_t i = 0; i < requests_.size(); ++i) {
    if (requests_[i].id == request_id) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

void AstraPermissionPromptModel::UpdateActiveRequestIndex() {
  if (requests_.empty()) {
    active_request_index_ = -1;
    return;
  }
  if (active_request_index_ < 0) {
    active_request_index_ = 0;
  } else if (active_request_index_ >=
             static_cast<int>(requests_.size())) {
    active_request_index_ = static_cast<int>(requests_.size()) - 1;
  }
  NotifyActiveRequestChanged(requests_[active_request_index_].id);
}

}  // namespace astra
