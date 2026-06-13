// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_PERMISSION_PROMPT_ASTRA_PERMISSION_PROMPT_MODEL_H_
#define ASTRA_UI_VIEWS_PERMISSION_PROMPT_ASTRA_PERMISSION_PROMPT_MODEL_H_

#include <string>
#include <vector>

#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/time/time.h"
#include "ui/gfx/image/image_skia.h"

namespace astra {

// Types of permissions that can be requested.
enum class AstraPermissionType {
  kCamera,
  kMicrophone,
  kCameraAndMicrophone,
  kGeolocation,
  kNotifications,
  kMidi,
  kMidiSysex,
  kClipboardRead,
  kClipboardWrite,
  kFileSystemRead,
  kFileSystemWrite,
  kBluetooth,
  kUsb,
  kSerial,
  kHid,  // Human Interface Device
  kNfc,
  kSensors,  // Generic sensor
  kIdleDetection,
  kPaymentHandler,
  kBackgroundSync,
  kStorageAccess,
  kScreenCapture,
  kWakeLock,
  kPictureInPicture,
  kFullscreen,
  kPointerLock,
  kWebXR,
  kAutoPlay,
  kDownloads,
  kMultipleDownloads,
  kPopups,
  kRedirects,
  kAds,
  kInsecureContent,
  kSound,
  kImageContentSettings,
  kJavaScript,
  kCookies,
};

// Permission action/decision.
enum class AstraPermissionAction {
  kAllow,
  kBlock,
  kAllowOnce,         // Allow for this session only
  kAllowEveryTime,    // Persistent allow
  kDeny,              // Persistent block
  kDismiss,           // User dismissed the prompt
  kIgnore,            // No decision yet
};

// Permission importance level.
enum class AstraPermissionImportance {
  kLow,
  kMedium,
  kHigh,
  kCritical,
};

// A single permission request.
struct AstraPermissionRequest {
  std::string id;
  AstraPermissionType type = AstraPermissionType::kNotifications;
  std::u16string title;        // e.g. "Use your camera"
  std::u16string message;      // e.g. "example.com wants to use your camera"
  std::u16string origin;       // e.g. "https://example.com"
  std::u16string display_name; // e.g. "Example.com"
  gfx::ImageSkia favicon;
  std::string favicon_url;

  // Whether this is a request that was already partially answered.
  bool is_persistent = true;   // Remember decision
  bool is_one_time_allowed = false;  // This request can be allowed once
  bool has_remember_option = true;   // Show "Remember this decision" checkbox

  AstraPermissionAction default_action = AstraPermissionAction::kIgnore;
  AstraPermissionImportance importance = AstraPermissionImportance::kMedium;

  base::Time request_time;
  base::TimeDelta auto_dismiss_timeout;
  bool auto_dismiss = false;

  // Embedding info.
  bool is_embedded = false;     // Permission requested by iframe
  std::string requesting_origin;
  std::string embedding_origin;

  // Additional details for specific permission types.
  std::u16string device_name;   // e.g. "FaceTime HD Camera"
  std::vector<std::u16string> device_options;  // Multiple devices to choose from
  int selected_device_index = 0;
};

// Observer for AstraPermissionPromptModel.
class AstraPermissionPromptObserver : public base::CheckedObserver {
 public:
  // Called when a new permission request is added.
  virtual void OnPermissionRequested(AstraPermissionPromptModel* model,
                                     const std::string& request_id) {}

  // Called when a permission decision is made.
  virtual void OnPermissionDecided(AstraPermissionPromptModel* model,
                                   const std::string& request_id,
                                   AstraPermissionAction action) {}

  // Called when a request is dismissed/removed.
  virtual void OnPermissionDismissed(AstraPermissionPromptModel* model,
                                     const std::string& request_id) {}

  // Called when the currently displayed request changes.
  virtual void OnActiveRequestChanged(AstraPermissionPromptModel* model,
                                      const std::string& request_id) {}

  // Called when the model is about to be destroyed.
  virtual void OnPermissionPromptModelShutdown(
      AstraPermissionPromptModel* model) {}

 protected:
  ~AstraPermissionPromptObserver() override = default;
};

// Model for permission prompts.
//
// Owns the active permission requests and decision logic.
// Permission data comes from Chromium's PermissionManager and
// permission_request_manager — this model projects and augments it
// with Astra-specific UX patterns.
//
// Chromium owner: PermissionManager / PermissionRequestManager
//   (components/permissions/permission_manager.h)
//   (chrome/browser/permissions/permission_request_manager.h)
//
// TODO(astra): Wire up to Chromium's PermissionRequestManager.
// Patch point: chrome/browser/permissions/permission_request_manager.cc
// or chrome/browser/ui/views/permissions/permission_prompt_bubble_view.h.
class AstraPermissionPromptModel {
 public:
  AstraPermissionPromptModel();
  ~AstraPermissionPromptModel();

  AstraPermissionPromptModel(const AstraPermissionPromptModel&) = delete;
  AstraPermissionPromptModel& operator=(const AstraPermissionPromptModel&) =
      delete;

  // -- Observer management --------------------------------------------------

  void AddObserver(AstraPermissionPromptObserver* observer);
  void RemoveObserver(AstraPermissionPromptObserver* observer);

  // -- Request management ---------------------------------------------------

  // Add a new permission request.
  std::string AddRequest(const AstraPermissionRequest& request);

  // Remove a request by ID.
  void RemoveRequest(const std::string& request_id);

  // Get a request by ID.
  const AstraPermissionRequest* GetRequest(
      const std::string& request_id) const;

  // Get all pending requests.
  const std::vector<AstraPermissionRequest>& GetAllRequests() const;

  // Get the number of pending requests.
  size_t GetRequestCount() const;

  // Get the active (currently displayed) request.
  const AstraPermissionRequest* GetActiveRequest() const;
  std::string GetActiveRequestId() const;

  // Set the active request.
  void SetActiveRequest(const std::string& request_id);

  // Go to next request.
  void NextRequest();

  // Go to previous request.
  void PreviousRequest();

  // -- Decision logic -------------------------------------------------------

  // Allow the active request.
  void Allow();

  // Block the active request.
  void Block();

  // Allow the active request once (session-only).
  void AllowOnce();

  // Dismiss the active request (no decision).
  void Dismiss();

  // Apply a specific action to a specific request.
  void Decide(const std::string& request_id, AstraPermissionAction action);

  // Set whether to remember the decision.
  void SetRememberDecision(bool remember);
  bool GetRememberDecision() const { return remember_decision_; }

  // -- Permission helpers ---------------------------------------------------

  // Get display name for a permission type.
  static std::u16string GetPermissionName(AstraPermissionType type);

  // Get description for a permission type.
  static std::u16string GetPermissionDescription(AstraPermissionType type);

  // Get icon name for a permission type.
  static std::string GetPermissionIconName(AstraPermissionType type);

  // Get importance level for a permission type.
  static AstraPermissionImportance GetPermissionImportance(
      AstraPermissionType type);

  // Whether a permission supports one-time allow.
  static bool SupportsOneTimePermission(AstraPermissionType type);

  // -- State ----------------------------------------------------------------

  bool IsLoading() const { return loading_; }
  void SetLoading(bool loading);

  // Whether the prompt should be visible (has pending requests).
  bool IsPromptVisible() const;

  // Clear all pending requests.
  void ClearAllRequests();

 private:
  // Notify helpers.
  void NotifyPermissionRequested(const std::string& request_id);
  void NotifyPermissionDecided(const std::string& request_id,
                               AstraPermissionAction action);
  void NotifyPermissionDismissed(const std::string& request_id);
  void NotifyActiveRequestChanged(const std::string& request_id);

  // Find a non-const request by ID.
  AstraPermissionRequest* FindRequest(const std::string& request_id);

  // Find the index of a request by ID.
  int FindRequestIndex(const std::string& request_id) const;

  // Update active request index.
  void UpdateActiveRequestIndex();

  // All pending requests.
  std::vector<AstraPermissionRequest> requests_;

  // Index of the currently active request.
  int active_request_index_ = -1;

  // Whether to remember decisions.
  bool remember_decision_ = true;

  // Loading state.
  bool loading_ = false;

  // Next request ID counter.
  int next_request_id_ = 1;

  base::ObserverList<AstraPermissionPromptObserver> observers_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_PERMISSION_PROMPT_ASTRA_PERMISSION_PROMPT_MODEL_H_
