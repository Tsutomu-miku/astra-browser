#include "browser/chromium/ChromiumBrowserApp.h"

#include "browser/chromium/ChromiumBrowser.h"

#include "include/cef_command_line.h"

namespace astra {

namespace {

ChromiumBrowserApp* g_instance = nullptr;

}  // namespace

ChromiumBrowserApp::ChromiumBrowserApp() {
  DCHECK(!g_instance);
  g_instance = this;
}

ChromiumBrowserApp::~ChromiumBrowserApp() {
  g_instance = nullptr;
}

ChromiumBrowserApp* ChromiumBrowserApp::GetInstance() {
  return g_instance;
}

// ===== BrowserApp interface =====

bool ChromiumBrowserApp::Initialize(int argc, char* argv[]) {
  if (initialized_) return true;

  CefMainArgs main_args(argc, argv);

  // Check if this is a subprocess (renderer, GPU, etc.)
  // CEF will call CefExecuteProcess for subprocesses before we get here
  // in the main process.
  //
  // On macOS, the helper app is a separate bundle, so the main app always
  // runs as the browser process.

  CefSettings settings;
  settings.no_sandbox = true;  // Sandbox requires code signing
  settings.multi_threaded_message_loop = false;  // We pump manually
  settings.remote_debugging_port = 0;  // 0 = disabled by default
  settings.background_color = CefColorSetARGB(255, 255, 255, 255);

  // Log file
  CefString(&settings.log_file).FromASCII("");  // Log to stderr

  app_ref_ = this;  // keep self alive via CEF refcounting

  // Initialize CEF in the browser process
  bool result = CefInitialize(main_args, settings, this, nullptr);
  if (!result) {
    app_ref_ = nullptr;
    return false;
  }

  initialized_ = true;
  return true;
}

void ChromiumBrowserApp::Shutdown() {
  if (!initialized_) return;

  CefShutdown();
  initialized_ = false;
  app_ref_ = nullptr;
}

std::unique_ptr<Browser> ChromiumBrowserApp::CreateBrowser(bool incognito) {
  auto browser = std::make_unique<ChromiumBrowser>(incognito);
  return browser;
}

void ChromiumBrowserApp::RunMessageLoopIteration() {
  // Pump CEF message loop
  CefDoMessageLoopWork();
}

// ===== CefBrowserProcessHandler =====

void ChromiumBrowserApp::OnContextInitialized() {
  // CEF context is ready — can start creating browsers now
  // Create the incognito request context
  CefRequestContextSettings settings;
  settings.persist_session_cookies = false;
  incognito_request_context_ =
      CefRequestContext::CreateContext(settings, nullptr);
}

}  // namespace astra
