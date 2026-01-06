// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/template_codec/binary_decoder/parallel_parse_task_scheduler.h"

#include "base/include/fml/concurrent_message_loop.h"
#include "base/trace/native/trace_event.h"
#include "core/renderer/dom/fiber/fiber_element.h"
#include "core/renderer/dom/fiber/tree_resolver.h"
#include "core/renderer/simple_styling/style_object.h"
#include "core/renderer/utils/base/element_template_info.h"
#include "core/template_bundle/template_codec/binary_decoder/binary_decoder_trace_event_def.h"
#include "core/template_bundle/template_codec/binary_decoder/element_binary_reader.h"

namespace lynx {
namespace tasm {

ParallelParseTaskScheduler::ParallelParseTaskScheduler(
    OrderedStringKeyRouter* router, ElementBinaryReader* reader,
    TemplateInfoMap* info_map)
    : router_(router), reader_(reader), info_map_(info_map) {}

ParallelParseTaskScheduler::~ParallelParseTaskScheduler() {
  if (generate_element_template_parse_task_.get() != nullptr) {
    generate_element_template_parse_task_->Run();
    generate_element_template_parse_task_->GetFuture().get();
    generate_element_template_parse_task_ = nullptr;
  }

  for (auto& pair : element_template_parse_task_map_) {
    pair.second->Run();
    pair.second->GetFuture().get();
  }
  element_template_parse_task_map_.clear();

  for (auto& pair : construct_element_task_map_) {
    pair.second->Run();
    pair.second->GetFuture().get();
  }
  construct_element_task_map_.clear();
}

bool ParallelParseTaskScheduler::ParallelParseElementTemplate() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, PARALLEL_READER_PARSE_ELEMENT_TEMPLATE);

  if (router_->start_offsets_.empty()) {
    return true;
  }

  std::promise<int32_t> out_promise;
  std::future<int32_t> out_future = out_promise.get_future();
  auto task = fml::MakeRefCounted<base::OnceTask<int32_t>>(
      [this, router = router_, reader = reader_,
       out_promise = std::move(out_promise)]() mutable {
        base::Vector<base::OnceTaskRefptr<
            std::pair<std::shared_ptr<ElementTemplateInfo>, lepus::Value>>>
            task_vec;
        for (const auto& pair : router->start_offsets_) {
          TRACE_EVENT(LYNX_TRACE_CATEGORY, PARALLEL_READER_GENERATE_PARSE_TASK);
          auto start = router->descriptor_offset_ + pair.second;
          auto sub_reader = reader->DeriveElementBinaryReader();
          std::promise<ElementTemplateResult> promise;
          std::future<ElementTemplateResult> future = promise.get_future();
          auto task_info_ptr =
              fml::MakeRefCounted<base::OnceTask<ElementTemplateResult>>(
                  [sub_reader = std::move(sub_reader), start, key = pair.first,
                   promise = std::move(promise)]() mutable {
                    TRACE_EVENT(
                        LYNX_TRACE_CATEGORY, PARALLEL_READER_RUN_PARSE_TASK,
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

Elements ParallelParseTaskScheduler::GenerateElements(const std::string& key) {
  std::shared_ptr<ElementTemplateInfo> info_ptr = nullptr;
  {
    std::unique_lock<std::mutex> locker(elements_mutex_);
    auto info_iter = info_map_->find(key);
    if (info_iter != info_map_->end()) {
      info_ptr = info_iter->second;
    }
  }

  if (info_ptr == nullptr) {
    if (router_ == nullptr || reader_ == nullptr) {
      return nullptr;
    }
    auto route_iter = router_->start_offsets_.find(key);
    if (route_iter == router_->start_offsets_.end()) {
      return nullptr;
    }
    auto start = router_->descriptor_offset_ + route_iter->second;
    auto sub_reader = reader_->DeriveElementBinaryReader();
    sub_reader->Seek(start);

    info_ptr = sub_reader->DecodeTemplatesInfoWithKey(key);

    if (info_ptr == nullptr) {
      return nullptr;
    }
    {
      std::unique_lock<std::mutex> locker(elements_mutex_);
      info_map_->emplace(key, info_ptr);
    }
  }
  return TreeResolver::FromTemplateInfo(*info_ptr);
}

ElementTemplateResult
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

void ParallelParseTaskScheduler::ConstructElement(const std::string& key,
                                                  bool sync) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, PARALLEL_READER_CONSTRUCT_ELEMENT);
  if (info_map_ == nullptr) {
    return;
  }

  auto info_iter = info_map_->find(key);

  if (info_iter == info_map_->end()) {
    return;
  }

  std::promise<Elements> promise;
  std::future<Elements> future = promise.get_future();
  auto task_info_ptr = fml::MakeRefCounted<base::OnceTask<Elements>>(
      [key, info = info_iter->second, promise = std::move(promise)]() mutable {
        TRACE_EVENT(LYNX_TRACE_CATEGORY,
                    PARALLEL_READER_RUN_CONSTRUCT_ELEMENT_TASK,
                    [key](lynx::perfetto::EventContext ctx) {
                      auto* tagInfo = ctx.event()->add_debug_annotations();
                      tagInfo->set_name("key");
                      tagInfo->set_string_value(key.c_str());
                    });
        promise.set_value(TreeResolver::FromTemplateInfo(*info));
        return;
      },
      std::move(future));
  if (sync) {
    task_info_ptr->Run();
  } else {
    base::TaskRunnerManufactor::PostTaskToConcurrentLoop(
        [task_info_ptr]() { task_info_ptr->Run(); },
        base::ConcurrentTaskType::HIGH_PRIORITY);
  }
  construct_element_task_map_[key] = task_info_ptr;
}

std::optional<Elements> ParallelParseTaskScheduler::TryGetElements(
    const std::string& key) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, PARALLEL_READER_TRY_GET_ELEMENTS);
  std::unique_lock<std::mutex> locker(elements_mutex_, std::try_to_lock);

  if (locker.owns_lock()) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, PARALLEL_READER_TRY_GET_ELEMENTS_GET_LOCK);
    auto iter = construct_element_task_map_.find(key);
    if (iter == construct_element_task_map_.end()) {
      ConstructElement(key, false);
      return std::nullopt;
    }
    TRACE_EVENT(LYNX_TRACE_CATEGORY, PARALLEL_READER_TRY_GET_ELEMENTS_SUCCESS);
    iter->second->Run();
    auto res = iter->second->GetFuture().get();
    construct_element_task_map_.erase(iter);
    ConstructElement(key, false);
    return res;
  }

  return std::nullopt;
}

void ParallelParseTaskScheduler::AsyncDecodeStyleObjects(
    const std::shared_ptr<style::StyleObject*>& style_object_list) {
  base::TaskRunnerManufactor::PostTaskToConcurrentLoop(
      [style_object_list]() mutable {
        TRACE_EVENT(LYNX_TRACE_CATEGORY, "DecodeStyleObjectSectionAsync");
        for (style::StyleObject** p = style_object_list.get(); *p != nullptr;
             p++) {
          (*p)->FromBinary();
        }
      },
      base::ConcurrentTaskType::HIGH_PRIORITY);
}
}  // namespace tasm
}  // namespace lynx
