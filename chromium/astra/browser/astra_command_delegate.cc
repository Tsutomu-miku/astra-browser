#include "astra/browser/astra_command_delegate.h"

#include "chrome/browser/ui/browser.h"

namespace astra {

bool AstraCommandDelegate::IsAstraCommand(int command_id) {
  return command_id >= kAstraCommandToggleSidebar &&
         command_id <= kAstraCommandToggleSplitView;
}

bool AstraCommandDelegate::ExecuteCommand(Browser* browser, int command_id) {
  if (!browser || !IsAstraCommand(command_id)) {
    return false;
  }

  // TODO(astra): Route only Astra-specific commands here. Standard browser
  // commands must continue through Chrome's command controller.
  switch (command_id) {
    case kAstraCommandToggleSidebar:
    case kAstraCommandNewWorkspace:
    case kAstraCommandNextWorkspace:
    case kAstraCommandPreviousWorkspace:
    case kAstraCommandOpenGlance:
    case kAstraCommandToggleSplitView:
      return false;
    default:
      return false;
  }
}

}  // namespace astra
