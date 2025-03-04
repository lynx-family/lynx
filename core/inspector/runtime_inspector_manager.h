// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_INSPECTOR_RUNTIME_INSPECTOR_MANAGER_H_
#define CORE_INSPECTOR_RUNTIME_INSPECTOR_MANAGER_H_

#include <memory>
#include <string>

namespace lynx {
namespace piper {
class Runtime;
class InspectorRuntimeObserverNG;

class RuntimeInspectorManager {
 public:
  virtual ~RuntimeInspectorManager() = default;

  virtual void InitInspector(
      Runtime* runtime,
      const std::shared_ptr<InspectorRuntimeObserverNG>& observer) = 0;
  virtual void DestroyInspector() = 0;

  std::string BuildInspectorUrl(const std::string& filename) {
    static constexpr char kUrlPrefixShared[] = "file://shared";
    static constexpr char kUrlPrefixView[] = "file://view";
    static constexpr char kUrlLynxCore[] = "lynx_core";
    static constexpr char kUrlSeparator = '/';

    std::string url = filename;
    if (url.front() != kUrlSeparator) {
      url = kUrlSeparator + url;
    }
    if (filename.find(kUrlLynxCore) != std::string::npos) {
      url = kUrlPrefixShared + url;
    } else {
      url = kUrlPrefixView + std::to_string(instance_id_) + url;
    }
    return url;
  }

  virtual void PrepareForScriptEval() = 0;

 protected:
  int instance_id_{-1};
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_INSPECTOR_RUNTIME_INSPECTOR_MANAGER_H_
