#include "astra/browser/astra_new_tab_page_service_factory.h"

#include "base/no_destructor.h"
#include "base/strings/stringprintf.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_registry_simple.h"
#include "content/public/browser/browser_context.h"
#include "third_party/skia/include/core/SkColor.h"

#include "astra/browser/astra_new_tab_page_service.h"

namespace astra {

namespace {

// -- Default values (used only for pref registration defaults) ---------------
//
// These mirror the defaults defined in the service .cc file's anonymous
// namespace.  They are duplicated here to avoid exposing them in the header,
// which would be the alternative.
//
// TODO(astra): Consider moving default constants to the service header as
//   public static constexpr for single-source-of-truth.

constexpr int kDefaultLayout = static_cast<int>(AstraNtpLayout::kDefault);
constexpr SkColor kDefaultBackgroundColor = SK_ColorWHITE;
constexpr bool kDefaultShowShortcuts = true;
constexpr bool kDefaultShowWorkspaceCards = true;
constexpr bool kDefaultShowSuggestions = true;
constexpr bool kDefaultShowGoogleLogo = true;
constexpr bool kDefaultShowSearchBox = true;
constexpr int kDefaultShortcutColumns = 4;
constexpr int kDefaultShortcutRows = 2;
constexpr int kDefaultMaxWorkspaceCards = 6;
constexpr int kDefaultMaxSuggestions = 8;
constexpr bool kDefaultShowMostVisited = true;
constexpr bool kDefaultShowRecentlyClosed = false;
constexpr int kDefaultDarkMode = static_cast<int>(AstraNtpDarkMode::kAuto);
constexpr bool kDefaultCustomBackgroundEnabled = false;
constexpr bool kDefaultSuggestionsEnabled = true;

// -- Legacy defaults --

constexpr int kDefaultLayoutMode =
    static_cast<int>(AstraNtpLayoutMode::kStandard);
constexpr bool kDefaultShowRecentlyVisited = true;
constexpr bool kDefaultShowFavorites = true;
constexpr int kDefaultBackgroundType =
    static_cast<int>(AstraNtpBackgroundType::kDefault);

// Converts an SkColor to a "#RRGGBB" hex string.
std::string SkColorToHexString(SkColor color) {
  return base::StringPrintf("#%02X%02X%02X", SkColorGetR(color),
                            SkColorGetG(color), SkColorGetB(color));
}

}  // namespace

// =========================================================================
// Construction / destruction
// =========================================================================

AstraNewTabPageServiceFactory::AstraNewTabPageServiceFactory()
    : ProfileKeyedServiceFactory(
          "AstraNewTabPageService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kRedirectedToOriginal)
              // Incognito: redirect to original profile.
              // NTP data (shortcuts, workspaces, theme) is product-level
              // state shared with the main profile.
              .WithGuest(ProfileSelection::kOwnInstance)
              // Guest: own instance (ephemeral NTP data).
              .WithSystem(ProfileSelection::kNone)
              // System profile: no service needed.
              .Build()) {}

AstraNewTabPageServiceFactory::~AstraNewTabPageServiceFactory() = default;

// =========================================================================
// Singleton access
// =========================================================================

// static
AstraNewTabPageServiceFactory*
AstraNewTabPageServiceFactory::GetInstance() {
  static base::NoDestructor<AstraNewTabPageServiceFactory> instance;
  return instance.get();
}

// static
AstraNewTabPageService* AstraNewTabPageServiceFactory::GetForProfile(
    Profile* profile) {
  if (!profile) {
    return nullptr;
  }
  return static_cast<AstraNewTabPageService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// =========================================================================
// Pref registration
// =========================================================================

// static
void AstraNewTabPageServiceFactory::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  // -- Managed shortcuts --
  registry->RegisterListPref(AstraNewTabPageService::kPrefShortcuts);

  // -- Layout & theme settings --
  registry->RegisterIntegerPref(
      AstraNewTabPageService::kPrefNtpLayout,
      kDefaultLayout);
  registry->RegisterStringPref(
      AstraNewTabPageService::kPrefBackgroundColor,
      SkColorToHexString(kDefaultBackgroundColor));
  registry->RegisterStringPref(
      AstraNewTabPageService::kPrefBackgroundImageUrl,
      std::string());
  registry->RegisterBooleanPref(
      AstraNewTabPageService::kPrefShowShortcuts,
      kDefaultShowShortcuts);
  registry->RegisterBooleanPref(
      AstraNewTabPageService::kPrefShowWorkspaceCards,
      kDefaultShowWorkspaceCards);
  registry->RegisterBooleanPref(
      AstraNewTabPageService::kPrefShowSuggestions,
      kDefaultShowSuggestions);
  registry->RegisterBooleanPref(
      AstraNewTabPageService::kPrefShowGoogleLogo,
      kDefaultShowGoogleLogo);
  registry->RegisterBooleanPref(
      AstraNewTabPageService::kPrefShowSearchBox,
      kDefaultShowSearchBox);
  registry->RegisterIntegerPref(
      AstraNewTabPageService::kPrefShortcutColumns,
      kDefaultShortcutColumns);
  registry->RegisterIntegerPref(
      AstraNewTabPageService::kPrefShortcutRows,
      kDefaultShortcutRows);

  // -- Additional NTP settings --
  registry->RegisterIntegerPref(
      AstraNewTabPageService::kPrefMaxWorkspaceCards,
      kDefaultMaxWorkspaceCards);
  registry->RegisterIntegerPref(
      AstraNewTabPageService::kPrefMaxSuggestions,
      kDefaultMaxSuggestions);
  registry->RegisterBooleanPref(
      AstraNewTabPageService::kPrefShowMostVisited,
      kDefaultShowMostVisited);
  registry->RegisterBooleanPref(
      AstraNewTabPageService::kPrefShowRecentlyClosed,
      kDefaultShowRecentlyClosed);
  registry->RegisterIntegerPref(
      AstraNewTabPageService::kPrefDarkMode,
      kDefaultDarkMode);
  registry->RegisterBooleanPref(
      AstraNewTabPageService::kPrefCustomBackgroundEnabled,
      kDefaultCustomBackgroundEnabled);

  // -- Workspace card visibility & order --
  registry->RegisterDictionaryPref(
      AstraNewTabPageService::kPrefWorkspaceCardVisibility);
  registry->RegisterListPref(
      AstraNewTabPageService::kPrefWorkspaceCardOrder);

  // -- Suggested content --
  registry->RegisterListPref(
      AstraNewTabPageService::kPrefDismissedSuggestions);
  registry->RegisterBooleanPref(
      AstraNewTabPageService::kPrefSuggestionsEnabled,
      kDefaultSuggestionsEnabled);

  // -- Legacy prefs (kept for backward compatibility) --

  // Legacy custom shortcuts list (index-based).
  registry->RegisterListPref("astra.ntp.custom_shortcuts");

  // Legacy layout mode (density: standard / compact / focused).
  registry->RegisterIntegerPref("astra.ntp.layout_mode",
                                 kDefaultLayoutMode);

  // Legacy show-recently-visited toggle.
  registry->RegisterBooleanPref("astra.ntp.show_recently_visited",
                                 kDefaultShowRecentlyVisited);

  // Legacy show-favorites toggle.
  registry->RegisterBooleanPref("astra.ntp.show_favorites",
                                 kDefaultShowFavorites);

  // Legacy background type (default / solid color / custom image).
  registry->RegisterIntegerPref("astra.ntp.background_type",
                                 kDefaultBackgroundType);

  // Note: astra.ntp.background_color is registered above (shared between
  // the old and new API — same pref key).
}

// =========================================================================
// Service construction
// =========================================================================

std::unique_ptr<KeyedService>
AstraNewTabPageServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  if (!profile) {
    return nullptr;
  }
  return std::make_unique<AstraNewTabPageService>(profile);
}

}  // namespace astra
