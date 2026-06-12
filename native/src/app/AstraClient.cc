#include "native/src/app/AstraClient.h"
#include "native/src/app/AstraApp.h"

#include "include/base/cef_logging.h"
#include "include/wrapper/cef_helpers.h"

AstraClient::AstraClient(const std::string& tabId)
    : tab_id_(tabId) {
  DCHECK(!tab_id_.empty());
}

AstraClient::~AstraClient() = default;

// ============================================================
// CefLifeSpanHandler
// ============================================================

void AstraClient::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();
  browser_ = browser;
  DLOG(INFO) << "AstraClient: browser created for tab " << tab_id_;

  if (g_astraApp) {
    g_astraApp->OnClientBrowserCreated(tab_id_, browser);
  }
}

void AstraClient::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();
  DLOG(INFO) << "AstraClient: browser closing for tab " << tab_id_;

  if (g_astraApp) {
    g_astraApp->OnBrowserClosed(tab_id_);
  }

  browser_ = nullptr;
}

// ============================================================
// CefLoadHandler
// ============================================================

void AstraClient::OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
                                       bool isLoading,
                                       bool canGoBack,
                                       bool canGoForward) {
  CEF_REQUIRE_UI_THREAD();
  if (g_astraApp) {
    g_astraApp->OnBrowserLoadingStateChanged(tab_id_, isLoading,
                                              canGoBack, canGoForward);
  }
}

// ============================================================
// CefDisplayHandler
// ============================================================

void AstraClient::OnTitleChange(CefRefPtr<CefBrowser> browser,
                                const CefString& title) {
  CEF_REQUIRE_UI_THREAD();
  if (g_astraApp) {
    g_astraApp->OnBrowserTitleChanged(tab_id_, title.ToString());
  }
}

void AstraClient::OnAddressChange(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefFrame> frame,
                                  const CefString& url) {
  CEF_REQUIRE_UI_THREAD();
  if (frame->IsMain() && g_astraApp) {
    g_astraApp->OnBrowserUrlChanged(tab_id_, url.ToString());
  }
}

// ============================================================
// CefDownloadHandler
// ============================================================

bool AstraClient::OnBeforeDownload(CefRefPtr<CefBrowser> browser,
                                    CefRefPtr<CefDownloadItem> download_item,
                                    const CefString& suggested_name,
                                    CefRefPtr<CefBeforeDownloadCallback> callback) {
  CEF_REQUIRE_UI_THREAD();
  if (!g_astraApp) return false;

  std::string downloadId = std::to_string(download_item->GetId());

  // Check if we already have this download
  auto existing = g_astraApp->GetDownload(downloadId);
  if (existing) {
    callback->Continue(existing->filePath, false);
    return true;
  }

  // Create download item
  auto download = std::make_shared<DownloadItem>();
  download->id = downloadId;
  download->url = download_item->GetURL().ToString();
  download->filename = suggested_name.ToString();
  download->mimeType = download_item->GetMimeType().ToString();
  download->totalBytes = download_item->GetTotalBytes();
  download->receivedBytes = download_item->GetReceivedBytes();
  download->state = DownloadState::InProgress;
  download->startTime = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();

  // Build download path: ~/Downloads/<filename>
  std::string homeDir = getenv("HOME") ? getenv("HOME") : "";
  std::string downloadsDir = homeDir + "/Downloads";
  std::string filePath = downloadsDir + "/" + download->filename;
  download->filePath = filePath;

  g_astraApp->AddDownload(download);

  // Start the download (don't show save dialog)
  callback->Continue(filePath, false);
  return true;
}

void AstraClient::OnDownloadUpdated(CefRefPtr<CefBrowser> browser,
                                     CefRefPtr<CefDownloadItem> download_item,
                                     CefRefPtr<CefDownloadItemCallback> callback) {
  CEF_REQUIRE_UI_THREAD();
  if (!g_astraApp) return;

  std::string downloadId = std::to_string(download_item->GetId());
  auto download = g_astraApp->GetDownload(downloadId);
  if (!download) return;

  // Update progress
  download->receivedBytes = download_item->GetReceivedBytes();
  download->totalBytes = download_item->GetTotalBytes();

  // Update state
  if (download_item->IsComplete()) {
    download->state = DownloadState::Complete;
    download->endTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
  } else if (download_item->IsCanceled()) {
    download->state = DownloadState::Cancelled;
    download->endTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
  } else if (download_item->IsInterrupted()) {
    download->state = DownloadState::Interrupted;
    download->endTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
  }

  // Store callback for cancellation
  g_astraApp->SetDownloadCallback(downloadId, callback);

  g_astraApp->UpdateDownload(download);
}

// ============================================================
// CefFindHandler
// ============================================================

void AstraClient::OnFindResult(CefRefPtr<CefBrowser> browser,
                               int identifier,
                               int count,
                               const CefRect& selectionRect,
                               int activeMatchOrdinal,
                               bool finalUpdate) {
  CEF_REQUIRE_UI_THREAD();
  if (g_astraApp) {
    g_astraApp->OnFindResult(tab_id_, count, activeMatchOrdinal, finalUpdate);
  }
}
