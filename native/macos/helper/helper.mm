//
//  helper.mm
//  Astra Browser Helper
//
//  CEF helper process entry point. Same executable is used for
//  renderer, GPU, plugin, utility subprocesses.
//

#import <Cocoa/Cocoa.h>

#include "include/cef_app.h"
#include "include/wrapper/cef_library_loader.h"
#include "native/src/app/AstraApp.h"

namespace {

bool loadCefFramework() {
  NSBundle* bundle = [NSBundle mainBundle];
  NSString* fwDir = [bundle privateFrameworksPath];
  NSString* fwPath =
      [fwDir stringByAppendingPathComponent:@"Chromium Embedded Framework.framework"];
  if (![[NSFileManager defaultManager] fileExistsAtPath:fwPath]) {
    fprintf(stderr, "CEF framework not found in helper at: %s\n",
            [fwPath UTF8String]);
    return false;
  }
  NSString* libPath =
      [fwPath stringByAppendingPathComponent:@"Chromium Embedded Framework"];
  if (!cef_load_library([libPath fileSystemRepresentation])) {
    fprintf(stderr, "Failed to load CEF library in helper: %s\n",
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
      return 1;
    }

    // CEF sandbox initialization (before anything else)
#ifdef CEF_ENABLE_SANDBOX
    cef_sandbox_info_t* sandbox_info = nullptr;
    std::string error;
    if (!cef_sandbox_initialize(argc, argv, &sandbox_info, error)) {
      fprintf(stderr, "Helper sandbox init failed: %s\n", error.c_str());
      return 1;
    }
#endif

    CefMainArgs main_args(argc, argv);
    CefRefPtr<AstraApp> app(new AstraApp());

    // This is a helper process — execute it
    int exit_code = CefExecuteProcess(main_args, app.get(), nullptr);
    if (exit_code >= 0) {
      return exit_code;
    }

    // Should not reach here in a helper process
    return 1;
  }

  return 0;
}
