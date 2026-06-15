// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_DEVTOOL_SETTINGS_EMBEDDER_H_
#define PLATFORM_EMBEDDER_DEVTOOL_SETTINGS_EMBEDDER_H_

#include <string>

namespace lynx {
namespace embedder {

class DevToolSettingsEmbedder {
 public:
  static DevToolSettingsEmbedder& GetInstance();
  DevToolSettingsEmbedder() = default;
  DevToolSettingsEmbedder(const DevToolSettingsEmbedder&) = delete;
  DevToolSettingsEmbedder& operator=(const DevToolSettingsEmbedder&) = delete;

  void SyncToNative();

  bool IsDevToolEnabled() const;
  void SetDevToolEnabled(bool enabled);

  bool IsLogBoxEnabled() const;
  void SetLogBoxEnabled(bool enabled);

  bool IsQuickJSDebugEnabled() const;
  void SetQuickJSDebugEnabled(bool enabled);

  bool IsDOMTreeEnabled() const;
  void SetDOMTreeEnabled(bool enabled);

  bool IsLongPressMenuEnabled() const;
  void SetLongPressMenuEnabled(bool enabled);

  bool IsLaunchRecordEnabled() const;
  void SetLaunchRecordEnabled(bool enabled);

#if (OS_WIN || OS_OSX) && JS_ENGINE_TYPE == 0
  bool IsV8Enabled() const;
  void SetV8Enabled(bool enabled);
#endif

 private:
  bool GetPersistedBoolean(const std::string& key, bool default_value) const;
  void SetPersistedBoolean(const std::string& key, bool value);
  void SyncBooleanToNative(const std::string& key, bool value);
};

}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_DEVTOOL_SETTINGS_EMBEDDER_H_
