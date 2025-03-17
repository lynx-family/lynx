// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/template_codec/binary_decoder/parallel_parse_task_scheduler.h"

#include "base/include/fml/concurrent_message_loop.h"
#include "base/trace/native/internal_trace_category.h"
#include "base/trace/native/trace_event.h"
#include "core/renderer/dom/fiber/tree_resolver.h"
#include "core/renderer/utils/base/element_template_info.h"
#include "core/template_bundle/template_codec/binary_decoder/element_binary_reader.h"

namespace lynx {
namespace tasm {

ParallelParseTaskScheduler::ParallelParseTaskScheduler() {}

ParallelParseTaskScheduler::~ParallelParseTaskScheduler() {
  if (generate_element_template_parse_task_.get() != nullptr) {
    generate_element_template_parse_task_->Run();
    generate_element_template_parse_task_->GetFuture().get();
    generate_element_template_parse_task_ = nullptr;
  }

  for (auto& pair : element_template_parse_task_map_) {
    pair.second->Run();
  }
  element_template_parse_task_map_.clear();
}

bool ParallelParseTaskScheduler::ParallelParseElementTemplate(
    OrderedStringKeyRouter* router, ElementBinaryReader* reader) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "TemplateParallelReader::ParallelParseElementTemplate");

  if (router->start_offsets_.empty()) {
    return true;
  }

  std::promise<int32_t> out_promise;
  std::future<int32_t> out_future = out_promise.get_future();
  auto task = fml::MakeRefCounted<base::OnceTask<int32_t>>(
      [this, router, reader, out_promise = std::move(out_promise)]() mutable {
        base::Vector<base::OnceTaskRefptr<
            std::pair<std::shared_ptr<ElementTemplateInfo>, lepus::Value>>>
            task_vec;
        for (const auto& pair : router->start_offsets_) {
          TRACE_EVENT(
              LYNX_TRACE_CATEGORY,
              "TemplateParallelReader::GenerateElementTemplateParseTask");
          auto start = router->descriptor_offset_ + pair.second;
          auto sub_reader = reader->DeriveElementBinaryReader();
          std::promise<
              std::pair<std::shared_ptr<ElementTemplateInfo>, Elements>>
              promise;
          std::future<std::pair<std::shared_ptr<ElementTemplateInfo>, Elements>>
              future = promise.get_future();
          auto task_info_ptr = fml::MakeRefCounted<base::OnceTask<
              std::pair<std::shared_ptr<ElementTemplateInfo>, Elements>>>(
              [sub_reader = std::move(sub_reader), start, key = pair.first,
               promise = std::move(promise)]() mutable {
                TRACE_EVENT(
                    LYNX_TRACE_CATEGORY,
                    "ElementBinaryReader::RunParseElementTemplateUnitTask",
                    [key = key](lynx::perfetto::EventContext ctx) {
                      auto* tagInfo = ctx.event()->add_debug_annotations();
                      tagInfo->set_name("key");
                      tagInfo->set_string_value(key.c_str());
                    });
                sub_reader->Seek(start);
                auto info = sub_reader->DecodeTemplatesInfoWithKey(key);
                auto result = TreeResolver::FromTemplateInfo(*info);
                promise.set_value({std::move(info), result});
                return;
              },
              std::move(future));
          element_template_parse_task_map_[pair.first] = task_info_ptr;
          base::TaskRunnerManufactor::PostTaskToConcurrentLoop(
              [task_info_ptr]() { task_info_ptr->Run(); },
              base::ConcurrentTaskType::HIGH_PRIORITY);
        }
        out_promise.set_value(0);
      },
      std::move(out_future));
  base::TaskRunnerManufactor::PostTaskToConcurrentLoop(
      [task]() { task->Run(); }, base::ConcurrentTaskType::HIGH_PRIORITY);
  generate_element_template_parse_task_ = task;
  return true;
}

std::pair<std::shared_ptr<ElementTemplateInfo>, Elements>
ParallelParseTaskScheduler::TryGetElementTemplateParseResult(
    const std::string& key) {
  if (generate_element_template_parse_task_.get() != nullptr) {
    generate_element_template_parse_task_->Run();
    generate_element_template_parse_task_->GetFuture().get();
    generate_element_template_parse_task_ = nullptr;
  }

  auto iter = element_template_parse_task_map_.find(key);
  if (iter == element_template_parse_task_map_.end()) {
    return {nullptr, Elements()};
  }
  iter->second->Run();
  auto res = iter->second->GetFuture().get();
  element_template_parse_task_map_.erase(iter);
  return res;
}

}  // namespace tasm
}  // namespace lynx
