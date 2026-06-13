#include "astra/browser/astra_focus_mode_service_factory.h"

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_registry_simple.h"
#include "content/public/browser/browser_context.h"

#include "astra/browser/astra_focus_mode_service.h"
#include "astra/browser/astra_prefs.h"

namespace astra {

// =========================================================================
// Construction / destruction
// =========================================================================

AstraFocusModeServiceFactory::AstraFocusModeServiceFactory()
    : ProfileKeyedServiceFactory(
          "AstraFocusModeService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOwnInstance)
              // Incognito: own instance.
              // Focus mode is per-browsing-context.  An incognito window
              // has its own focus session that doesn't affect the main
              // profile's active session.  Prefs are still shared via
              // pref forwarding, but runtime session state is per-instance.
              .WithGuest(ProfileSelection::kOwnInstance)
              // Guest: own instance.
              .WithSystem(ProfileSelection::kNone)
              // System profile: no service needed.
              .Build()) {}

AstraFocusModeServiceFactory::~AstraFocusModeServiceFactory() = default;

// =========================================================================
// Singleton access
// =========================================================================

// static
AstraFocusModeServiceFactory* AstraFocusModeServiceFactory::GetInstance() {
  static base::NoDestructor<AstraFocusModeServiceFactory> instance;
  return instance.get();
}

// static
AstraFocusModeService* AstraFocusModeServiceFactory::GetForProfile(
    Profile* profile) {
  if (!profile) {
    return nullptr;
  }
  return static_cast<AstraFocusModeService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// =========================================================================
// Pref registration
// =========================================================================

// static
void AstraFocusModeServiceFactory::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  // Default focus duration: 25 minutes (Pomodoro-style default).
  registry->RegisterIntegerPref(prefs::kPrefFocusModeDefaultDuration,
                                prefs::kDefaultFocusDefaultDurationMinutes);

  // Distraction site blocklist: list of URL pattern strings.
  // Default: empty list — user populates their own distractions.
  registry->RegisterListPref(prefs::kPrefFocusModeBlocklist);

  // Whether focus mode auto-starts (e.g., during work hours).
  // Default: false — user opts in.
  registry->RegisterBooleanPref(prefs::kPrefFocusModeAutoStart, false);

  // Short break duration for pomodoro mode (minutes).
  registry->RegisterIntegerPref(prefs::kPrefFocusModeShortBreakDuration,
                                prefs::kDefaultFocusShortBreakMinutes);

  // Long break duration for pomodoro mode (minutes).
  registry->RegisterIntegerPref(prefs::kPrefFocusModeLongBreakDuration,
                                prefs::kDefaultFocusLongBreakMinutes);

  // Number of work sessions before a long break.
  registry->RegisterIntegerPref(prefs::kPrefFocusModeLongBreakInterval,
                                prefs::kDefaultFocusLongBreakInterval);

  // Whether to auto-start the next phase in pomodoro mode.
  registry->RegisterBooleanPref(prefs::kPrefFocusModeAutoStartNextPhase,
                                prefs::kDefaultFocusAutoStartNextPhase);

  // Whitelist of sites always allowed during focus mode.
  registry->RegisterListPref(prefs::kPrefFocusModeWhitelist);

  // Total accumulated focus time in seconds (cumulative stat).
  registry->RegisterIntegerPref(prefs::kPrefFocusModeTotalFocusSeconds,
                                prefs::kDefaultFocusTotalFocusSeconds);

  // Total number of completed focus sessions (cumulative stat).
  registry->RegisterIntegerPref(prefs::kPrefFocusModeSessionsCompleted,
                                prefs::kDefaultFocusSessionsCompleted);

  // Total number of completed pomodoro cycles (cumulative stat).
  registry->RegisterIntegerPref(prefs::kPrefFocusModeCyclesCompleted,
                                prefs::kDefaultFocusCyclesCompleted);

  // Whether distraction warnings are enabled.
  registry->RegisterBooleanPref(prefs::kPrefFocusModeWarningsEnabled,
                                prefs::kDefaultFocusWarningsEnabled);

  // Auto-start start time (HH:MM 24h format).
  registry->RegisterStringPref(prefs::kPrefFocusModeAutoStartTime,
                               prefs::kDefaultFocusAutoStartTime);

  // Auto-start end time (HH:MM 24h format).
  registry->RegisterStringPref(prefs::kPrefFocusModeAutoEndTime,
                               prefs::kDefaultFocusAutoEndTime);

  // Auto-start days of week (list of ints, 0=Sun ... 6=Sat).
  {
    base::Value::List default_days;
    default_days.Append(1);  // Monday
    default_days.Append(2);  // Tuesday
    default_days.Append(3);  // Wednesday
    default_days.Append(4);  // Thursday
    default_days.Append(5);  // Friday
    registry->RegisterListPref(prefs::kPrefFocusModeAutoStartDays,
                               std::move(default_days));
  }

  // Session presets (list of dicts).
  registry->RegisterListPref(prefs::kPrefFocusModePresets);

  // Whether the focus session is currently paused.
  registry->RegisterBooleanPref(prefs::kPrefFocusModePaused,
                                prefs::kDefaultFocusPaused);

  // Remaining seconds in a paused session.
  registry->RegisterIntegerPref(prefs::kPrefFocusModePausedRemainingSeconds,
                                prefs::kDefaultFocusPausedRemainingSeconds);
}

// =========================================================================
// Service construction
// =========================================================================

std::unique_ptr<KeyedService>
AstraFocusModeServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  if (!profile) {
    return nullptr;
  }
  return std::make_unique<AstraFocusModeService>(profile);
}

}  // namespace astra
