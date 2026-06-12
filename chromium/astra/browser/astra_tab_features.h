#ifndef ASTRA_BROWSER_ASTRA_TAB_FEATURES_H_
#define ASTRA_BROWSER_ASTRA_TAB_FEATURES_H_

#include <string>
#include <utility>

#include "content/public/browser/web_contents_user_data.h"

namespace content {
class WebContents;
}

namespace astra {

// Astra-only metadata attached to Chromium-owned WebContents.
// Do not mirror navigation state, title, favicon, zoom, mute, or loading here;
// those remain Chromium data.
class AstraTabFeatures final
    : public content::WebContentsUserData<AstraTabFeatures> {
 public:
  ~AstraTabFeatures() override;

  const std::string& workspace_id() const { return workspace_id_; }
  void set_workspace_id(std::string workspace_id) {
    workspace_id_ = std::move(workspace_id);
  }

  bool is_favorite() const { return is_favorite_; }
  void set_is_favorite(bool is_favorite) { is_favorite_ = is_favorite; }

  bool is_in_split_view() const { return is_in_split_view_; }
  void set_is_in_split_view(bool is_in_split_view) {
    is_in_split_view_ = is_in_split_view;
  }

 private:
  friend class content::WebContentsUserData<AstraTabFeatures>;

  explicit AstraTabFeatures(content::WebContents* web_contents);

  std::string workspace_id_ = "default";
  bool is_favorite_ = false;
  bool is_in_split_view_ = false;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_TAB_FEATURES_H_
