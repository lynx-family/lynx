// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/DevToolSettings.h>

#include "base/include/no_destructor.h"
#include "platform/embedder/devtool_settings_embedder.h"

namespace lynx {
namespace embedder {

DevToolSettingsEmbedder& DevToolSettingsEmbedder::GetInstance() {
  static base::NoDestructor<DevToolSettingsEmbedder> instance;
  return *instance;
}

// iOS shares the existing DevTool settings and their native synchronization.
void DevToolSettingsEmbedder::SyncToNative() {}

bool DevToolSettingsEmbedder::IsDevToolEnabled() const {
  return [DevToolSettings sharedInstance].devToolEnabled;
}

void DevToolSettingsEmbedder::SetDevToolEnabled(bool enabled) {
  [DevToolSettings sharedInstance].devToolEnabled = enabled;
}

bool DevToolSettingsEmbedder::IsLogBoxEnabled() const {
  return [DevToolSettings sharedInstance].logBoxEnabled;
}

void DevToolSettingsEmbedder::SetLogBoxEnabled(bool enabled) {
  [DevToolSettings sharedInstance].logBoxEnabled = enabled;
}

bool DevToolSettingsEmbedder::IsQuickJSDebugEnabled() const {
  return [DevToolSettings sharedInstance].quickjsDebugEnabled;
}

void DevToolSettingsEmbedder::SetQuickJSDebugEnabled(bool enabled) {
  [DevToolSettings sharedInstance].quickjsDebugEnabled = enabled;
}

bool DevToolSettingsEmbedder::IsDOMTreeEnabled() const {
  return [DevToolSettings sharedInstance].domTreeEnabled;
}

void DevToolSettingsEmbedder::SetDOMTreeEnabled(bool enabled) {
  [DevToolSettings sharedInstance].domTreeEnabled = enabled;
}

bool DevToolSettingsEmbedder::IsLongPressMenuEnabled() const {
  return [DevToolSettings sharedInstance].longPressMenuEnabled;
}

void DevToolSettingsEmbedder::SetLongPressMenuEnabled(bool enabled) {
  [DevToolSettings sharedInstance].longPressMenuEnabled = enabled;
}

bool DevToolSettingsEmbedder::IsLaunchRecordEnabled() const {
  return [DevToolSettings sharedInstance].launchRecordEnabled;
}

void DevToolSettingsEmbedder::SetLaunchRecordEnabled(bool enabled) {
  [DevToolSettings sharedInstance].launchRecordEnabled = enabled;
}

}  // namespace embedder
}  // namespace lynx
