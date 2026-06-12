#ifndef ASTRA_BROWSER_ASTRA_COMMAND_DELEGATE_H_
#define ASTRA_BROWSER_ASTRA_COMMAND_DELEGATE_H_

class Browser;

namespace astra {

enum AstraCommandId {
  kAstraCommandToggleSidebar = 60000,
  kAstraCommandNewWorkspace,
  kAstraCommandNextWorkspace,
  kAstraCommandPreviousWorkspace,
  kAstraCommandOpenGlance,
  kAstraCommandToggleSplitView,
};

class AstraCommandDelegate {
 public:
  AstraCommandDelegate() = delete;

  static bool IsAstraCommand(int command_id);
  static bool ExecuteCommand(Browser* browser, int command_id);
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_COMMAND_DELEGATE_H_
