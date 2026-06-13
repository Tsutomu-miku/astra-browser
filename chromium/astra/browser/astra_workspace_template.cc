// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_workspace_template.h"

#include <algorithm>

#include "base/no_destructor.h"
#include "base/ranges/algorithm.h"
#include "url/gurl.h"

namespace astra {

namespace {

// Returns the full list of built-in templates, constructed once and cached.
//
// Built-in templates are a fixed set of workspace blueprints shipped with
// the browser. They are read-only product metadata — users cannot modify
// them, though they may create their own custom templates in the future.
//
// TODO(astra): Allow user-created templates saved to PrefService.
// Chromium owner: PrefService.
// Patch point: add user template pref keys to astra_prefs.h and merge
// user templates with built-ins in GetAllTemplates().
const std::vector<AstraWorkspaceTemplate>& GetTemplatesInternal() {
  static const base::NoDestructor<std::vector<AstraWorkspaceTemplate>>
      kTemplates([]() {
        std::vector<AstraWorkspaceTemplate> templates;
        templates.reserve(12);

        // ------------------------------------------------------------------
        // Work
        // ------------------------------------------------------------------
        {
          AstraWorkspaceTemplate t;
          t.id = "work";
          t.name = "Work";
          t.description =
              "Essential tools for your workday: email, calendar, "
              "documents, and cloud storage.";
          t.icon = "briefcase";
          t.color = "#4285F4";
          t.category = AstraWorkspaceTemplateCategory::kProductivity;
          t.default_tabs = {
              {GURL("https://mail.google.com"), "Gmail"},
              {GURL("https://calendar.google.com"), "Google Calendar"},
              {GURL("https://drive.google.com"), "Google Drive"},
              {GURL("https://docs.google.com"), "Google Docs"},
          };
          templates.push_back(std::move(t));
        }

        // ------------------------------------------------------------------
        // Development
        // ------------------------------------------------------------------
        {
          AstraWorkspaceTemplate t;
          t.id = "development";
          t.name = "Development";
          t.description =
              "Everything you need to build software: code hosting, "
              "documentation, and developer tools.";
          t.icon = "code";
          t.color = "#6C5CE7";
          t.category = AstraWorkspaceTemplateCategory::kDevelopment;
          t.default_tabs = {
              {GURL("https://github.com"), "GitHub"},
              {GURL("https://stackoverflow.com"), "Stack Overflow"},
              {GURL("https://vscode.dev"), "VS Code Web"},
              {GURL("https://developer.mozilla.org"), "MDN Web Docs"},
          };
          templates.push_back(std::move(t));
        }

        // ------------------------------------------------------------------
        // Design
        // ------------------------------------------------------------------
        {
          AstraWorkspaceTemplate t;
          t.id = "design";
          t.name = "Design";
          t.description =
              "Creative tools and inspiration for designers: "
              "prototyping, portfolios, and design communities.";
          t.icon = "palette";
          t.color = "#FF6B9D";
          t.category = AstraWorkspaceTemplateCategory::kDesign;
          t.default_tabs = {
              {GURL("https://figma.com"), "Figma"},
              {GURL("https://dribbble.com"), "Dribbble"},
              {GURL("https://behance.net"), "Behance"},
              {GURL("https://awwwards.com"), "Awwwards"},
          };
          templates.push_back(std::move(t));
        }

        // ------------------------------------------------------------------
        // Social
        // ------------------------------------------------------------------
        {
          AstraWorkspaceTemplate t;
          t.id = "social";
          t.name = "Social";
          t.description =
              "Stay connected with social networks and "
              "professional platforms.";
          t.icon = "people";
          t.color = "#00B894";
          t.category = AstraWorkspaceTemplateCategory::kSocial;
          t.default_tabs = {
              {GURL("https://twitter.com"), "X / Twitter"},
              {GURL("https://linkedin.com"), "LinkedIn"},
              {GURL("https://instagram.com"), "Instagram"},
              {GURL("https://facebook.com"), "Facebook"},
          };
          templates.push_back(std::move(t));
        }

        // ------------------------------------------------------------------
        // Entertainment
        // ------------------------------------------------------------------
        {
          AstraWorkspaceTemplate t;
          t.id = "entertainment";
          t.name = "Entertainment";
          t.description =
              "Streaming, music, gaming, and live content for "
              "your downtime.";
          t.icon = "play_circle";
          t.color = "#E17055";
          t.category = AstraWorkspaceTemplateCategory::kEntertainment;
          t.default_tabs = {
              {GURL("https://youtube.com"), "YouTube"},
              {GURL("https://netflix.com"), "Netflix"},
              {GURL("https://spotify.com"), "Spotify"},
              {GURL("https://twitch.tv"), "Twitch"},
          };
          templates.push_back(std::move(t));
        }

        // ------------------------------------------------------------------
        // News
        // ------------------------------------------------------------------
        {
          AstraWorkspaceTemplate t;
          t.id = "news";
          t.name = "News";
          t.description =
              "Stay informed with top news sources and "
              "community discussions.";
          t.icon = "article";
          t.color = "#2D3436";
          t.category = AstraWorkspaceTemplateCategory::kNews;
          t.default_tabs = {
              {GURL("https://nytimes.com"), "The New York Times"},
              {GURL("https://bbc.com/news"), "BBC News"},
              {GURL("https://cnn.com"), "CNN"},
              {GURL("https://reddit.com"), "Reddit"},
          };
          templates.push_back(std::move(t));
        }

        // ------------------------------------------------------------------
        // Shopping
        // ------------------------------------------------------------------
        {
          AstraWorkspaceTemplate t;
          t.id = "shopping";
          t.name = "Shopping";
          t.description =
              "Marketplaces and handmade goods for all your "
              "online shopping needs.";
          t.icon = "shopping_bag";
          t.color = "#FDCB6E";
          t.category = AstraWorkspaceTemplateCategory::kShopping;
          t.default_tabs = {
              {GURL("https://amazon.com"), "Amazon"},
              {GURL("https://ebay.com"), "eBay"},
              {GURL("https://etsy.com"), "Etsy"},
          };
          templates.push_back(std::move(t));
        }

        // ------------------------------------------------------------------
        // Learning
        // ------------------------------------------------------------------
        {
          AstraWorkspaceTemplate t;
          t.id = "learning";
          t.name = "Learning";
          t.description =
              "Online courses, educational platforms, and "
              "reference resources.";
          t.icon = "school";
          t.color = "#0984E3";
          t.category = AstraWorkspaceTemplateCategory::kLearning;
          t.default_tabs = {
              {GURL("https://coursera.org"), "Coursera"},
              {GURL("https://udemy.com"), "Udemy"},
              {GURL("https://khanacademy.org"), "Khan Academy"},
              {GURL("https://wikipedia.org"), "Wikipedia"},
          };
          templates.push_back(std::move(t));
        }

        // ------------------------------------------------------------------
        // Productivity
        // ------------------------------------------------------------------
        {
          AstraWorkspaceTemplate t;
          t.id = "productivity";
          t.name = "Productivity";
          t.description =
              "Notes, tasks, and organization tools to keep "
              "you focused and productive.";
          t.icon = "check_circle";
          t.color = "#00B894";
          t.category = AstraWorkspaceTemplateCategory::kProductivity;
          t.default_tabs = {
              {GURL("https://notion.so"), "Notion"},
              {GURL("https://todoist.com"), "Todoist"},
              {GURL("https://keep.google.com"), "Google Keep"},
              {GURL("https://calendar.google.com"), "Google Calendar"},
          };
          templates.push_back(std::move(t));
        }

        // ------------------------------------------------------------------
        // Finance
        // ------------------------------------------------------------------
        {
          AstraWorkspaceTemplate t;
          t.id = "finance";
          t.name = "Finance";
          t.description =
              "Banking, investments, and market tracking in "
              "one focused workspace.";
          t.icon = "payments";
          t.color = "#00B894";
          t.category = AstraWorkspaceTemplateCategory::kFinance;
          t.default_tabs = {
              {GURL("https://chase.com"), "Chase"},
              {GURL("https://finance.yahoo.com"), "Yahoo Finance"},
              {GURL("https://coinbase.com"), "Coinbase"},
              {GURL("https://robinhood.com"), "Robinhood"},
          };
          templates.push_back(std::move(t));
        }

        // ------------------------------------------------------------------
        // Travel
        // ------------------------------------------------------------------
        {
          AstraWorkspaceTemplate t;
          t.id = "travel";
          t.name = "Travel";
          t.description =
              "Booking sites, maps, and reviews to plan your "
              "next adventure.";
          t.icon = "flight";
          t.color = "#74B9FF";
          t.category = AstraWorkspaceTemplateCategory::kTravel;
          t.default_tabs = {
              {GURL("https://booking.com"), "Booking.com"},
              {GURL("https://maps.google.com"), "Google Maps"},
              {GURL("https://tripadvisor.com"), "TripAdvisor"},
              {GURL("https://airbnb.com"), "Airbnb"},
          };
          templates.push_back(std::move(t));
        }

        // ------------------------------------------------------------------
        // Fitness
        // ------------------------------------------------------------------
        {
          AstraWorkspaceTemplate t;
          t.id = "fitness";
          t.name = "Fitness";
          t.description =
              "Workout tracking, nutrition, and training "
              "resources to stay healthy.";
          t.icon = "fitness_center";
          t.color = "#E84393";
          t.category = AstraWorkspaceTemplateCategory::kFitness;
          t.default_tabs = {
              {GURL("https://strava.com"), "Strava"},
              {GURL("https://myfitnesspal.com"), "MyFitnessPal"},
              {GURL("https://nike.com/training-apps"), "Nike Training"},
              {GURL("https://bodybuilding.com"), "Bodybuilding.com"},
          };
          templates.push_back(std::move(t));
        }

        return templates;
      }());

  return *kTemplates;
}

}  // namespace

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

std::vector<AstraWorkspaceTemplate> GetBuiltInTemplates() {
  return GetTemplatesInternal();
}

std::vector<AstraWorkspaceTemplate> GetTemplatesByCategory(
    AstraWorkspaceTemplateCategory category) {
  std::vector<AstraWorkspaceTemplate> result;
  for (const auto& t : GetTemplatesInternal()) {
    if (t.category == category) {
      result.push_back(t);
    }
  }
  return result;
}

std::vector<AstraWorkspaceTemplateCategory> GetAllTemplateCategories() {
  return {
      AstraWorkspaceTemplateCategory::kProductivity,
      AstraWorkspaceTemplateCategory::kDevelopment,
      AstraWorkspaceTemplateCategory::kDesign,
      AstraWorkspaceTemplateCategory::kSocial,
      AstraWorkspaceTemplateCategory::kEntertainment,
      AstraWorkspaceTemplateCategory::kNews,
      AstraWorkspaceTemplateCategory::kShopping,
      AstraWorkspaceTemplateCategory::kLearning,
      AstraWorkspaceTemplateCategory::kFinance,
      AstraWorkspaceTemplateCategory::kTravel,
      AstraWorkspaceTemplateCategory::kFitness,
      AstraWorkspaceTemplateCategory::kPersonal,
      AstraWorkspaceTemplateCategory::kCustom,
  };
}

std::string GetTemplateCategoryDisplayName(
    AstraWorkspaceTemplateCategory category) {
  switch (category) {
    case AstraWorkspaceTemplateCategory::kProductivity:
      return "Productivity";
    case AstraWorkspaceTemplateCategory::kDevelopment:
      return "Development";
    case AstraWorkspaceTemplateCategory::kDesign:
      return "Design";
    case AstraWorkspaceTemplateCategory::kSocial:
      return "Social";
    case AstraWorkspaceTemplateCategory::kEntertainment:
      return "Entertainment";
    case AstraWorkspaceTemplateCategory::kNews:
      return "News";
    case AstraWorkspaceTemplateCategory::kShopping:
      return "Shopping";
    case AstraWorkspaceTemplateCategory::kLearning:
      return "Learning";
    case AstraWorkspaceTemplateCategory::kFinance:
      return "Finance";
    case AstraWorkspaceTemplateCategory::kTravel:
      return "Travel";
    case AstraWorkspaceTemplateCategory::kFitness:
      return "Fitness";
    case AstraWorkspaceTemplateCategory::kPersonal:
      return "Personal";
    case AstraWorkspaceTemplateCategory::kCustom:
      return "Custom";
  }
  return "Custom";
}

const AstraWorkspaceTemplate* FindTemplate(const std::string& template_id) {
  const auto& templates = GetTemplatesInternal();
  auto it = base::ranges::find(templates, template_id,
                               &AstraWorkspaceTemplate::id);
  return it == templates.end() ? nullptr : &(*it);
}

}  // namespace astra
