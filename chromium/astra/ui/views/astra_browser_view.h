#ifndef ASTRA_UI_VIEWS_ASTRA_BROWSER_VIEW_H_
#define ASTRA_UI_VIEWS_ASTRA_BROWSER_VIEW_H_

#include "base/memory/raw_ptr.h"

class BrowserView;

namespace astra {

class AstraSidebarView;

// Controller that augments Chrome's BrowserView instead of replacing the whole
// desktop UI stack. This keeps toolbar, tab strip, WebUI, DevTools, download UI,
// and profile plumbing on Chromium rails.
class AstraBrowserView {
 public:
  explicit AstraBrowserView(BrowserView* browser_view);
  AstraBrowserView(const AstraBrowserView&) = delete;
  AstraBrowserView& operator=(const AstraBrowserView&) = delete;
  ~AstraBrowserView();

  void Install();
  void UpdateSidebar();

 private:
  raw_ptr<BrowserView> browser_view_;
  raw_ptr<AstraSidebarView> sidebar_view_ = nullptr;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_ASTRA_BROWSER_VIEW_H_
