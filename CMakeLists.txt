cmake_minimum_required(VERSION 3.24)
project(AstraBrowser CXX C OBJC OBJCXX)

# ============================================================
# Configuration
# ============================================================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OBJCXX_STANDARD 20)
set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0")
set(CMAKE_OSX_ARCHITECTURES "arm64")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

# CEF root directory
if(NOT DEFINED CEF_ROOT)
  set(CEF_ROOT "${CMAKE_SOURCE_DIR}/third_party/cef")
endif()

# Find CEF
if(NOT IS_DIRECTORY "${CEF_ROOT}/cmake")
  message(WARNING "========================================================")
  message(WARNING "CEF not found at: ${CEF_ROOT}")
  message(WARNING "Run: ./scripts/setup-cef.sh")
  message(WARNING "Or:  cmake -DCEF_ROOT=/path/to/cef")
  message(WARNING "========================================================")
  return()
endif()

# Include CEF cmake modules directly (FindCEF.cmake doesn't set CEF_FOUND)
set(_CEF_ROOT "${CEF_ROOT}")
set(_CEF_ROOT_EXPLICIT 1)
set(CMAKE_MODULE_PATH "${CEF_ROOT}/cmake" ${CMAKE_MODULE_PATH})
include("cef_variables")
include("cef_macros")

# Build libcef_dll_wrapper (C API → C++ wrapper)
add_subdirectory("${CEF_ROOT}/libcef_dll" libcef_dll_build)

# ============================================================
# Source files
# ============================================================

# Browser core (platform-independent abstract interfaces)
set(BROWSER_CORE_SOURCES
  browser/core/Browser.cc
)

set(BROWSER_CORE_HEADERS
  browser/core/Browser.h
)

# Browser Chromium implementation (CEF-backed, content-module pattern)
set(BROWSER_CHROMIUM_SOURCES
  browser/chromium/ChromiumWebContents.cc
  browser/chromium/ChromiumWebContentsClient.cc
  browser/chromium/ChromiumBrowser.cc
  browser/chromium/ChromiumBrowserApp.cc
)

set(BROWSER_CHROMIUM_HEADERS
  browser/chromium/ChromiumWebContents.h
  browser/chromium/ChromiumWebContentsClient.h
  browser/chromium/ChromiumBrowser.h
  browser/chromium/ChromiumBrowserApp.h
)

set(APP_CPP_SOURCES
  ${BROWSER_CORE_SOURCES}
  ${BROWSER_CHROMIUM_SOURCES}
  native/src/app/AstraApp.cc
  native/src/app/AstraClient.cc
)

set(APP_OBJC_SOURCES
  native/macos/main.mm
  native/macos/AppDelegate.mm
  native/macos/MainWindowController.mm
  native/macos/SidebarViewController.mm
  native/macos/BrowserContainerView.mm
  native/macos/AddressBar.mm
  native/macos/TabRowView.m
  native/macos/FindBarView.m
)

set(APP_HEADERS
  ${BROWSER_CORE_HEADERS}
  ${BROWSER_CHROMIUM_HEADERS}
  native/src/app/AstraApp.h
  native/src/app/AstraClient.h
  native/src/common/AstraIds.h
  native/macos/AppDelegate.h
  native/macos/MainWindowController.h
  native/macos/SidebarViewController.h
  native/macos/BrowserContainerView.h
  native/macos/AddressBar.h
  native/macos/TabRowView.h
  native/macos/FindBarView.h
)

set(APP_RESOURCES
  native/macos/Resources/Info.plist
)

# ============================================================
# Astra Browser — main app target
# ============================================================

# Set CEF target output directory
SET_CEF_TARGET_OUT_DIR()

add_executable(AstraBrowser MACOSX_BUNDLE
  ${APP_CPP_SOURCES}
  ${APP_OBJC_SOURCES}
  ${APP_HEADERS}
  ${APP_RESOURCES}
)

target_include_directories(AstraBrowser PRIVATE
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/native/src
  ${CMAKE_SOURCE_DIR}/native/macos
)

# Apply standard CEF executable properties (compiler flags, defines, linker flags)
SET_EXECUTABLE_TARGET_PROPERTIES(AstraBrowser)

# Override C/C++ standard to C++20 / C17 (CEF defaults to C++17 / C99)
# Disable CEF's strict ObjC property synthesis warning (modern ObjC auto-synthesis is fine)
set_target_properties(AstraBrowser PROPERTIES
  XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "gnu++20"
  XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "gnu17"
  XCODE_ATTRIBUTE_CLANG_WARN_OBJC_MISSING_PROPERTY_SYNTHESIS "NO"
)

# CEF wrapper library + framework
target_link_libraries(AstraBrowser PRIVATE libcef_dll_wrapper)
target_link_libraries(AstraBrowser PRIVATE
  "${CEF_BINARY_DIR_RELEASE}/Chromium Embedded Framework.framework/Chromium Embedded Framework"
)

# macOS frameworks
target_link_libraries(AstraBrowser PRIVATE
  "-framework Cocoa"
  "-framework AppKit"
  "-framework Foundation"
  "-framework QuartzCore"
  "-framework IOSurface"
)

# macOS bundle properties
set_target_properties(AstraBrowser PROPERTIES
  MACOSX_BUNDLE TRUE
  MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/native/macos/Resources/Info.plist"
  MACOSX_BUNDLE_GUI_IDENTIFIER "com.astra.browser"
  OUTPUT_NAME "Astra"
  XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.astra.browser"
  XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC "YES"
  XCODE_ATTRIBUTE_CLANG_ENABLE_MODULES "YES"
  XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
  XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO"
  XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
)

set_source_files_properties(${APP_RESOURCES}
  PROPERTIES MACOSX_PACKAGE_LOCATION "Resources"
)

# Copy CEF framework into the app bundle's Frameworks directory
# COPY_MAC_FRAMEWORK(target, binary_dir_containing_framework, app_bundle_path)
COPY_MAC_FRAMEWORK(AstraBrowser "${CEF_BINARY_DIR_RELEASE}" "$<TARGET_BUNDLE_DIR:AstraBrowser>")

# Copy CEF resources (.pak files, icudtl.dat, etc.) into the app bundle's Resources
set(APP_RESOURCES_DIR "$<TARGET_BUNDLE_CONTENT_DIR:AstraBrowser>/Resources")
add_custom_command(TARGET AstraBrowser POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CEF_BINARY_DIR_RELEASE}/Chromium Embedded Framework.framework/Resources"
    "${APP_RESOURCES_DIR}/"
  COMMENT "Copying CEF resources to app bundle"
)

# Copy helper app into the main app bundle's Frameworks directory.
# CEF expects helper apps to be nested inside the main app bundle.
# Use cp -R because cmake -E copy_directory fails with macOS bundles
# that contain frameworks with versioned symlinks.
set(APP_FRAMEWORKS_DIR "$<TARGET_BUNDLE_CONTENT_DIR:AstraBrowser>/Frameworks")
add_custom_command(TARGET AstraBrowser POST_BUILD
  COMMAND rm -rf "${APP_FRAMEWORKS_DIR}/Astra Helper.app"
  COMMAND cp -R "$<TARGET_BUNDLE_DIR:AstraHelper>" "${APP_FRAMEWORKS_DIR}/"
  COMMENT "Copying helper app into main bundle"
)

# ============================================================
# Helper app (required for Chromium multi-process architecture)
# ============================================================

add_executable(AstraHelper MACOSX_BUNDLE
  native/macos/helper/helper.mm
  ${BROWSER_CORE_SOURCES}
  ${BROWSER_CHROMIUM_SOURCES}
  native/src/app/AstraApp.cc
  native/src/app/AstraClient.cc
)

SET_EXECUTABLE_TARGET_PROPERTIES(AstraHelper)

set_target_properties(AstraHelper PROPERTIES
  MACOSX_BUNDLE TRUE
  MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/native/macos/Resources/Helper-Info.plist"
  OUTPUT_NAME "Astra Helper"
  XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.astra.browser.helper"
  XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC "YES"
  XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
  XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO"
  XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
)

target_link_libraries(AstraHelper PRIVATE libcef_dll_wrapper)
target_link_libraries(AstraHelper PRIVATE
  "${CEF_BINARY_DIR_RELEASE}/Chromium Embedded Framework.framework/Chromium Embedded Framework"
)
target_link_libraries(AstraHelper PRIVATE
  "-framework Cocoa"
  "-framework Foundation"
  "-framework AppKit"
)

target_include_directories(AstraHelper PRIVATE
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/native/src
  ${CMAKE_SOURCE_DIR}/native/macos
)

COPY_MAC_FRAMEWORK(AstraHelper "${CEF_BINARY_DIR_RELEASE}" "$<TARGET_BUNDLE_DIR:AstraHelper>")

add_dependencies(AstraBrowser AstraHelper)

# ============================================================
# Summary
# ============================================================

message(STATUS "")
message(STATUS "=== Astra Browser (Native UI + Chromium CEF) ===")
message(STATUS "  Platform:  macOS ${CMAKE_OSX_DEPLOYMENT_TARGET} (${CMAKE_OSX_ARCHITECTURES})")
message(STATUS "  Build:     ${CMAKE_BUILD_TYPE}")
message(STATUS "  Target:    Astra.app")
message(STATUS "")
