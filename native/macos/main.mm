//
//  main.mm
//  Astra Browser — Native UI entry point
//
//  Entry point for both browser process and helper processes.
//  Follows Chromium macOS best practices:
//  1. Load CEF framework dynamically
//  2. Call CefExecuteProcess first (handles helper subprocesses)
//  3. For browser process: set up NSApplication, then initialize Chromium
//
//  Architecture: Chromium content module pattern via astra::BrowserApp.
//  CEF is used as the Chromium distribution — the public API (Browser,
//  WebContents, BrowserApp) follows Chromium content module conventions.

#import <Cocoa/Cocoa.h>

#import "native/macos/AppDelegate.h"
#include "browser/core/Browser.h"
#include "browser/chromium/ChromiumBrowserApp.h"

#include "include/cef_command_line.h"
#include "include/cef_sandbox_mac.h"
#include "include/wrapper/cef_library_loader.h"

namespace {

// Load the CEF framework dynamically.
// libcef_dll_wrapper uses a function-pointer table (g_libcef_pointers) that
// must be populated via cef_load_library() before any CEF API is called.
bool loadCefFramework() {
  NSBundle* bundle = [NSBundle mainBundle];
  NSString* fwDir = [bundle privateFrameworksPath];
  NSString* fwPath =
      [fwDir stringByAppendingPathComponent:@"Chromium Embedded Framework.framework"];
  if (![[NSFileManager defaultManager] fileExistsAtPath:fwPath]) {
    fprintf(stderr, "CEF framework not found at: %s\n", [fwPath UTF8String]);
    return false;
  }
  NSString* libPath =
      [fwPath stringByAppendingPathComponent:@"Chromium Embedded Framework"];
  if (!cef_load_library([libPath fileSystemRepresentation])) {
    fprintf(stderr, "Failed to load CEF library: %s\n",
            [libPath UTF8String]);
    return false;
  }
  return true;
}

}  // namespace

int main(int argc, char* argv[]) {
  @autoreleasepool {
    // Load CEF framework BEFORE any CEF API call.
    if (!loadCefFramework()) {
      fprintf(stderr, "Failed to load CEF framework\n");
      return 1;
    }

    // Sandbox initialization (must be first in main)
#ifdef CEF_ENABLE_SANDBOX
    cef_sandbox_info_t* sandbox_info = nullptr;
    std::string sandbox_error;
    if (!cef_sandbox_initialize(argc, argv, &sandbox_info, sandbox_error)) {
      fprintf(stderr, "CEF sandbox initialization failed: %s\n",
              sandbox_error.c_str());
      return 1;
    }
#endif

    // Create the Chromium browser app.
    // This is both the CefApp (for CEF subprocess handling) and the
    // astra::BrowserApp (for the public API).
    CefRefPtr<astra::ChromiumBrowserApp> browser_app(
        new astra::ChromiumBrowserApp());

    // Execute the secondary process, if any.
    // Returns immediately with the process exit code for helper processes.
    CefMainArgs main_args(argc, argv);
    int exit_code = CefExecuteProcess(main_args, browser_app.get(), nullptr);
    if (exit_code >= 0) {
      // This was a helper/renderer/GPU/etc process — exit now.
      return exit_code;
    }

    // === Browser process continues below ===

    // Parse command line
    CefRefPtr<CefCommandLine> command_line = CefCommandLine::CreateCommandLine();
    command_line->InitFromArgv(argc, argv);

    // Enable media autoplay by default (less intrusive UX)
    command_line->AppendSwitchWithValue("autoplay-policy",
                                         "no-user-gesture-required");

    // Initialize Chromium runtime
    // (CefInitialize is called inside ChromiumBrowserApp::Initialize)
    if (!browser_app->Initialize(argc, argv)) {
      fprintf(stderr, "Failed to initialize Chromium browser app\n");
      return 1;
    }

    // Set global accessor (non-owning pointer)
    astra::g_browser_app = browser_app.get();

    // Set up NSApplication
    [NSApplication sharedApplication];

    // Create app delegate (it will create the main window)
    AppDelegate* delegate = [[AppDelegate alloc] init];
    [NSApp setDelegate:delegate];

    // Set activation policy
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    // Run the main event loop
    // Chromium tasks will be pumped via timer on the main runloop
    NSApplicationMain(argc, (const char **)argv);
  }

  return 0;
}
