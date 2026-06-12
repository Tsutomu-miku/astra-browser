#include "astra/ui/views/sidebar/astra_sidebar_view.h"

#include "chrome/browser/ui/browser.h"
#include "ui/gfx/geometry/size.h"

namespace astra {

AstraSidebarView::AstraSidebarView(Browser* browser) : browser_(browser) {}

AstraSidebarView::~AstraSidebarView() = default;

gfx::Size AstraSidebarView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  (void)available_size;
  // TODO(astra): Size from user prefs and responsive density settings.
  return gfx::Size(280, 0);
}

}  // namespace astra
