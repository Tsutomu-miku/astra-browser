#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_VIEW_H_

#include "base/memory/raw_ptr.h"
#include "ui/views/view.h"

class Browser;

namespace astra {

class AstraSidebarView final : public views::View {
 public:
  explicit AstraSidebarView(Browser* browser);
  AstraSidebarView(const AstraSidebarView&) = delete;
  AstraSidebarView& operator=(const AstraSidebarView&) = delete;
  ~AstraSidebarView() override;

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;

 private:
  raw_ptr<Browser> browser_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_VIEW_H_
