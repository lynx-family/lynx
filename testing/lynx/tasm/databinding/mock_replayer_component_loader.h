// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef TESTING_LYNX_TASM_DATABINDING_MOCK_REPLAYER_COMPONENT_LOADER_H_
#define TESTING_LYNX_TASM_DATABINDING_MOCK_REPLAYER_COMPONENT_LOADER_H_

#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/resource/lazy_bundle/lazy_bundle_loader.h"

namespace lynx {
namespace tasm {
namespace test {

class RequireInfo {
 public:
  std::string url_;
  bool sync_;
  RequireInfo(const std::string& url, bool sync) : url_(url), sync_(sync) {}
};

class LoadComponentInfo {
 public:
  std::vector<uint8_t> source_;
  int32_t callback_id_;
  LoadComponentInfo(const std::vector<uint8_t>& source, int32_t callback_id)
      : source_(source), callback_id_(callback_id) {}
};

class MockReplayerComponentLoader : public LazyBundleLoader {
 public:
  MockReplayerComponentLoader(std::weak_ptr<TemplateAssembler> tasm);

  virtual void RequireTemplate(RadonLazyComponent* dynamic_component,
                               const std::string& url,
                               int instance_id) override;

  void InitWithActionList(const rapidjson::Value& action_list);

 private:
  std::list<RequireInfo> require_info_list_;
  std::list<RequireInfo>::iterator list_header_;

  std::weak_ptr<TemplateAssembler> tasm_;

  std::unordered_map<std::string, LoadComponentInfo> component_map_;
};

}  // namespace test
}  // namespace tasm
}  // namespace lynx

#endif  // TESTING_LYNX_TASM_DATABINDING_MOCK_REPLAYER_COMPONENT_LOADER_H_
