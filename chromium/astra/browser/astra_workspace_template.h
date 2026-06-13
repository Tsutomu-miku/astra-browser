// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_BROWSER_ASTRA_WORKSPACE_TEMPLATE_H_
#define ASTRA_BROWSER_ASTRA_WORKSPACE_TEMPLATE_H_

#include <string>
#include <vector>

#include "url/gurl.h"

namespace astra {

// Categories for organizing workspace templates.
//
// Templates are grouped by category in the UI to help users quickly find
// relevant workspace blueprints. New categories can be added as needed.
enum class AstraWorkspaceTemplateCategory {
  kProductivity,
  kDevelopment,
  kDesign,
  kSocial,
  kEntertainment,
  kNews,
  kShopping,
  kLearning,
  kFinance,
  kTravel,
  kFitness,
  kPersonal,
  kCustom,
};

// A single default tab entry within a workspace template.
//
// Contains the URL and a human-readable title for a tab that should be
// opened when a workspace is created from this template.
struct AstraTemplateTab {
  GURL url;
  std::string title;
};

// A predefined workspace template for quick workspace creation.
//
// Templates are read-only product metadata that describe how to bootstrap
// a new workspace. They include default tabs, a suggested name, accent
// color, icon, and category. Templates do not persist user state — they
// are blueprints that the workspace service uses when creating new
// workspace metadata.
//
// TODO(astra): Allow user-created templates saved to PrefService.
// Chromium owner: PrefService + user pref storage.
// Patch point: extend astra_prefs.h with user template pref keys and
// register them via AstraWorkspaceServiceFactory::RegisterProfilePrefs.
struct AstraWorkspaceTemplate {
  // Stable identifier for the template (e.g. "work", "development").
  std::string id;

  // Human-readable display name for the template.
  std::string name;

  // Short description of what the template is for.
  std::string description;

  // Optional icon identifier (e.g. a Chrome side panel icon name).
  std::string icon;

  // Suggested accent color for workspaces created from this template.
  std::string color;

  // Default tabs to open when creating a workspace from this template.
  std::vector<AstraTemplateTab> default_tabs;

  // Category this template belongs to.
  AstraWorkspaceTemplateCategory category;
};

// Returns all built-in workspace templates.
//
// Built-in templates are hard-coded product metadata. They are the same
// for all users and cannot be modified at runtime.
//
// TODO(astra): Combine built-in templates with user-created templates
// from PrefService when user templates are supported.
std::vector<AstraWorkspaceTemplate> GetBuiltInTemplates();

// Returns all templates belonging to the given category.
std::vector<AstraWorkspaceTemplate> GetTemplatesByCategory(
    AstraWorkspaceTemplateCategory category);

// Returns all template categories, in recommended display order.
std::vector<AstraWorkspaceTemplateCategory> GetAllTemplateCategories();

// Returns the human-readable display name for a template category.
std::string GetTemplateCategoryDisplayName(
    AstraWorkspaceTemplateCategory category);

// Finds a built-in template by its id.
// Returns nullptr if no template with the given id exists.
const AstraWorkspaceTemplate* FindTemplate(const std::string& template_id);

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_WORKSPACE_TEMPLATE_H_
