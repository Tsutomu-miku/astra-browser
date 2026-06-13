#include "astra/ui/views/bookmarks_bar/astra_bookmarks_bar_model.h"

#include <algorithm>

#include "base/i18n/number_formatting.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/text_utils.h"

namespace astra {

AstraBookmarksBarModel::AstraBookmarksBarModel(Profile* profile)
    : profile_(profile) {
  LoadPrefs();
  Refresh();
}

AstraBookmarksBarModel::~AstraBookmarksBarModel() = default;

void AstraBookmarksBarModel::AddObserver(
    AstraBookmarksBarObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraBookmarksBarModel::RemoveObserver(
    AstraBookmarksBarObserver* observer) {
  observers_.RemoveObserver(observer);
}

const AstraBookmarksBarItem* AstraBookmarksBarModel::GetItem(
    int64_t id) const {
  int index = FindItemIndex(id);
  if (index < 0) {
    return nullptr;
  }
  return &items_[index];
}

const AstraBookmarksBarItem* AstraBookmarksBarModel::GetItemAtIndex(
    int index) const {
  if (index < 0 || index >= static_cast<int>(items_.size())) {
    return nullptr;
  }
  return &items_[index];
}

std::vector<AstraBookmarksBarItem>
AstraBookmarksBarModel::GetFolderChildren(int64_t folder_id) const {
  // TODO(astra): Project children from BookmarkModel::GetBookmarksBarNode().
  // For now, return empty list.
  return {};
}

void AstraBookmarksBarModel::SetVisible(bool visible) {
  if (visible_ == visible) {
    return;
  }
  visible_ = visible;
  SavePrefs();
  NotifyVisibilityChanged();
}

void AstraBookmarksBarModel::ToggleVisible() {
  SetVisible(!visible_);
}

void AstraBookmarksBarModel::set_show_on_ntp_only(bool show) {
  if (show_on_ntp_only_ == show) {
    return;
  }
  show_on_ntp_only_ = show;
  SavePrefs();
  NotifyVisibilityChanged();
}

void AstraBookmarksBarModel::AddBookmark(const std::u16string& title,
                                          const GURL& url) {
  // TODO(astra): Delegate to BookmarkModel::AddURL().
  // Chromium method: BookmarkModel::AddURL()
  Refresh();
}

void AstraBookmarksBarModel::AddFolder(const std::u16string& title) {
  // TODO(astra): Delegate to BookmarkModel::AddFolder().
  Refresh();
}

void AstraBookmarksBarModel::RemoveBookmark(int64_t id) {
  // TODO(astra): Delegate to BookmarkModel::Remove().
  Refresh();
}

void AstraBookmarksBarModel::RenameBookmark(int64_t id,
                                             const std::u16string& new_title) {
  // TODO(astra): Delegate to BookmarkModel::SetTitle().
  Refresh();
}

void AstraBookmarksBarModel::ChangeBookmarkURL(int64_t id,
                                                const GURL& new_url) {
  // TODO(astra): Delegate to BookmarkModel::SetURL().
  Refresh();
}

void AstraBookmarksBarModel::MoveBookmark(int64_t id, int new_index) {
  // TODO(astra): Delegate to BookmarkModel::Move().
  Refresh();
}

void AstraBookmarksBarModel::OpenBookmark(int64_t id) {
  // TODO(astra): Delegate to browser-level navigation.
}

void AstraBookmarksBarModel::OpenBookmarkInNewTab(int64_t id) {
  // TODO(astra): Delegate to browser-level new tab navigation.
}

void AstraBookmarksBarModel::OpenFolderInNewTabs(int64_t folder_id) {
  // TODO(astra): Open all children as new tabs.
}

void AstraBookmarksBarModel::set_show_favicons(bool show) {
  if (show_favicons_ == show) {
    return;
  }
  show_favicons_ = show;
  NotifyLoaded();
}

void AstraBookmarksBarModel::set_show_text(bool show) {
  if (show_text_ == show) {
    return;
  }
  show_text_ = show;
  NotifyLoaded();
}

void AstraBookmarksBarModel::set_max_item_width(int width) {
  width = std::clamp(width, kMinItemWidth, kMaxItemWidthLimit);
  if (max_item_width_ == width) {
    return;
  }
  max_item_width_ = width;
  NotifyLoaded();
}

void AstraBookmarksBarModel::set_show_other_bookmarks(bool show) {
  if (show_other_bookmarks_ == show) {
    return;
  }
  show_other_bookmarks_ = show;
  NotifyLoaded();
}

void AstraBookmarksBarModel::Refresh() {
  // TODO(astra): Load items from BookmarkModel.
  // For now, items_ remains empty or as previously set.
  // In production, we'd iterate over the Bookmarks Bar folder's children.
  items_.clear();
  NotifyLoaded();
}

// static
std::u16string AstraBookmarksBarModel::FormatTitle(
    const std::u16string& title,
    int max_width_pixels,
    const gfx::FontList& font_list) {
  if (title.empty()) {
    return u"";
  }
  if (max_width_pixels <= 0) {
    return title;
  }
  int title_width = gfx::GetStringWidth(title, font_list);
  if (title_width <= max_width_pixels) {
    return title;
  }
  // Use elision with "..." at the end.
  return gfx::ElideText(title, font_list, max_width_pixels, gfx::ELIDE_TAIL);
}

bookmarks::BookmarkModel* AstraBookmarksBarModel::GetBookmarkModel() const {
  // TODO(astra): Get BookmarkModel from the profile.
  // Chromium pattern: BookmarkModelFactory::GetForBrowserContext(profile_)
  return nullptr;
}

// static
AstraBookmarksBarItem AstraBookmarksBarModel::ProjectNode(
    const bookmarks::BookmarkNode* node) {
  // TODO(astra): Project actual BookmarkNode data.
  AstraBookmarksBarItem item;
  return item;
}

std::vector<AstraBookmarksBarItem>
AstraBookmarksBarModel::ProjectChildren(
    const bookmarks::BookmarkNode* parent) const {
  // TODO(astra): Project all children of the given parent node.
  return {};
}

void AstraBookmarksBarModel::LoadPrefs() {
  // TODO(astra): Load prefs from PrefService.
  visible_ = true;
  show_on_ntp_only_ = false;
  show_favicons_ = true;
  show_text_ = true;
  max_item_width_ = kDefaultMaxItemWidth;
  show_other_bookmarks_ = true;
}

void AstraBookmarksBarModel::SavePrefs() {
  // TODO(astra): Save prefs to PrefService.
}

void AstraBookmarksBarModel::NotifyLoaded() {
  for (auto& observer : observers_) {
    observer.OnBookmarksBarLoaded(this);
  }
}

void AstraBookmarksBarModel::NotifyBookmarkAdded(int64_t id) {
  for (auto& observer : observers_) {
    observer.OnBookmarkAdded(this, id);
  }
}

void AstraBookmarksBarModel::NotifyBookmarkRemoved(int64_t id) {
  for (auto& observer : observers_) {
    observer.OnBookmarkRemoved(this, id);
  }
}

void AstraBookmarksBarModel::NotifyBookmarkChanged(int64_t id) {
  for (auto& observer : observers_) {
    observer.OnBookmarkChanged(this, id);
  }
}

void AstraBookmarksBarModel::NotifyReordered() {
  for (auto& observer : observers_) {
    observer.OnBookmarksReordered(this);
  }
}

void AstraBookmarksBarModel::NotifyVisibilityChanged() {
  for (auto& observer : observers_) {
    observer.OnBookmarksBarVisibilityChanged(this, visible_);
  }
}

int AstraBookmarksBarModel::FindItemIndex(int64_t id) const {
  for (size_t i = 0; i < items_.size(); i++) {
    if (items_[i].id == id) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

}  // namespace astra
