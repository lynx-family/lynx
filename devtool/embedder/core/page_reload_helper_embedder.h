// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_EMBEDDER_CORE_PAGE_RELOAD_HELPER_EMBEDDER_H_
#define DEVTOOL_EMBEDDER_CORE_PAGE_RELOAD_HELPER_EMBEDDER_H_

#include <memory>
#include <string>
#include <vector>

namespace lynx {

namespace tasm {
class TemplateData;
}  // namespace tasm

namespace devtool {

class LynxDevToolProxy;

class PageReloadHelperEmbedder {
 public:
  explicit PageReloadHelperEmbedder(devtool::LynxDevToolProxy* proxy);
  ~PageReloadHelperEmbedder() = default;

  void OnLoadTemplate(const std::string& url, const std::vector<uint8_t>& tem,
                      const std::shared_ptr<tasm::TemplateData>& init_data);

  std::string GetURL();

  void Reload(bool ignore_cache, const std::string& template_binary = "",
              const std::string& reload_url = "",
              bool from_template_fragments = false, int32_t template_size = 0);
  void Navigate(const std::string& url);

  std::shared_ptr<tasm::TemplateData> GetTemplateData();

 private:
  devtool::LynxDevToolProxy* proxy_;
  std::vector<uint8_t> binary_;
  std::string url_;
  std::shared_ptr<tasm::TemplateData> init_data_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_EMBEDDER_CORE_PAGE_RELOAD_HELPER_EMBEDDER_H_
