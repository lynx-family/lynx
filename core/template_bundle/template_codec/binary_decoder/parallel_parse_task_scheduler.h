// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_PARALLEL_PARSE_TASK_SCHEDULER_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_PARALLEL_PARSE_TASK_SCHEDULER_H_

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/vector.h"
#include "core/base/thread/once_task.h"
#include "core/renderer/dom/element.h"
#include "core/template_bundle/template_codec/template_binary.h"

namespace lynx {
namespace style {
class StyleObject;
}
namespace tasm {

class ElementBinaryReader;
struct ElementTemplateInfo;
using Elements = base::Vector<fml::RefPtr<Element>>;
using ElementTemplateResult =
    std::pair<std::shared_ptr<ElementTemplateInfo>, Elements>;

class ParallelParseTaskScheduler {
 public:
  ParallelParseTaskScheduler();
  ~ParallelParseTaskScheduler();

  bool ParallelParseElementTemplate(OrderedStringKeyRouter* router,
                                    ElementBinaryReader* reader);

  ElementTemplateResult TryGetElementTemplateParseResult(
      const std::string& key);

  void ConstructElement(const std::string& key,
                        const std::shared_ptr<ElementTemplateInfo>& info,
                        bool sync);
  std::optional<Elements> TryGetElements(
      const std::string& key, const std::shared_ptr<ElementTemplateInfo>& info);

  void AsyncDecodeStyleObjects(
      const std::shared_ptr<style::StyleObject*>& style_object_list);

 private:
  base::OnceTaskRefptr<int32_t> generate_element_template_parse_task_;
  std::unordered_map<std::string,
                     base::OnceTaskRefptr<std::pair<
                         std::shared_ptr<ElementTemplateInfo>, Elements>>>
      element_template_parse_task_map_;

  std::mutex elements_mutex_;

  std::unordered_map<std::string, base::OnceTaskRefptr<Elements>>
      construct_element_task_map_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_PARALLEL_PARSE_TASK_SCHEDULER_H_
