#include "astra/ui/views/astra_browser_view.h"

#include "astra/ui/views/sidebar/astra_sidebar_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"

#include <memory>

namespace astra {

AstraBrowserView::AstraBrowserView(BrowserView* browser_view)
    : browser_view_(browser_view) {}

AstraBrowserView::~AstraBrowserView() = default;

void AstraBrowserView::Install() {
  if (!browser_view_ || sidebar_view_) {
    return;
  }

  // TODO(astra): Insert the sidebar into BrowserView's layout manager at the
  // same hierarchy level as the toolbar/content area, preserving Chrome-owned
  // WebContents and toolbar behavior.
  sidebar_view_ = browser_view_->AddChildView(std::make_unique<AstraSidebarView>(
      browser_view_->browser()));
}

void AstraBrowserView::UpdateSidebar() {
  if (sidebar_view_) {
    sidebar_view_->SchedulePaint();
  }
}

}  // namespace astra
