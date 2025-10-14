// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_COMMON_PLATFORM_PROVIDED_MENU_H_
#define CLAY_SHELL_PLATFORM_COMMON_PLATFORM_PROVIDED_MENU_H_

namespace clay {

// Enumerates the provided menus that a platform may support.
enum class PlatformProvidedMenu {
  // orderFrontStandardAboutPanel macOS provided menu
  kAbout,

  // terminate macOS provided menu
  kQuit,

  // Services macOS provided submenu.
  kServicesSubmenu,

  // hide macOS provided menu
  kHide,

  // hideOtherApplications macOS provided menu
  kHideOtherApplications,

  // unhideAllApplications macOS provided menu
  kShowAllApplications,

  // startSpeaking macOS provided menu
  kStartSpeaking,

  // stopSpeaking macOS provided menu
  kStopSpeaking,

  // toggleFullScreen macOS provided menu
  kToggleFullScreen,

  // performMiniaturize macOS provided menu
  kMinimizeWindow,

  // performZoom macOS provided menu
  kZoomWindow,

  // arrangeInFront macOS provided menu
  kArrangeWindowsInFront,
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_COMMON_PLATFORM_PROVIDED_MENU_H_
