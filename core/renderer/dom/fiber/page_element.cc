// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/page_element.h"

#include <memory>
#include <utility>

#include "base/include/fml/memory/ref_counted.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/template_assembler.h"

namespace lynx {
namespace tasm {

constexpr const static char kDefaultPageName[] = "page";
constexpr const static char kDefaultPagePath[] = "__PAGE_PATH";

PageElement::PageElement(ElementManager* manager,
                         const base::String& component_id, int32_t css_id)
    : ComponentElement(manager, component_id, css_id,
                       BASE_STATIC_STRING(tasm::DEFAULT_ENTRY_NAME),
                       BASE_STATIC_STRING(kDefaultPageName),
                       BASE_STATIC_STRING(kDefaultPagePath),
                       BASE_STATIC_STRING(kElementPageTag)) {
  MarkCanBeLayoutOnly(false);

  if (manager == nullptr) {
    return;
  }

  // make sure page's default overflow is hidden
  SetDefaultOverflow(false);
  MarkAsLayoutRoot();
  manager->catalyzer()->set_root(this);
  manager->SetRoot(this);
  manager->SetFiberPageElement(fml::RefPtr<PageElement>(this));
  MarkAttached();
  // The page element is always the template element.
  MarkTemplateElement();
  // The parent component unique id of page element is always its own impl id.
  SetParentComponentUniqueIdForFiber(static_cast<int64_t>(impl_id()));
}

PageElement::PageElement(const PageElement& element, bool clone_resolved_props)
    : ComponentElement(element, clone_resolved_props) {
  MarkAttached();
}

void PageElement::AttachToElementManager(
    ElementManager* manager,
    const std::shared_ptr<CSSStyleSheetManager>& style_manager,
    bool keep_element_id) {
  ComponentElement::AttachToElementManager(manager, style_manager,
                                           keep_element_id);
  // make sure page's default overflow is hidden
  SetDefaultOverflow(false);
  MarkAsLayoutRoot();
  manager->catalyzer()->set_root(this);
  manager->SetRoot(this);
  manager->SetFiberPageElement(fml::RefPtr<PageElement>(this));
  set_style_sheet_manager(style_manager);

  MarkAttached();
  // The page element is always the template element.
  MarkTemplateElement();
  // The parent component unique id of page element is always its own impl id.
  SetParentComponentUniqueIdForFiber(static_cast<int64_t>(impl_id()));
}

void PageElement::FlushActionsAsRoot() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "PageElement::FlushActionsAsRoot",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  FiberElement::ParallelFlushAsRoot();
  FiberElement::FlushActions();
}

void PageElement::PostResolveTaskToThreadPool(
    bool is_engine_thread, ParallelReduceTaskQueue& task_queue) {
  // In threaded element flush mode, the PageElement PrepareForCreateOrUpdate
  // should be performed in the TASM thread before dispatching the
  // PrepareForCreateOrUpdate for all the children FiberElement, thus the rem
  // pattern value will be guaranteed to be calculated precisely.

  // Get Tag Info
  EnsureTagInfo();
  // Decode first
  GetRelatedCSSFragment();
  UpdateResolveStatus(AsyncResolveStatus::kSyncResolving);
  ParallelFlushReturn remaining_task = PrepareForCreateOrUpdate();

  if (is_engine_thread) {
    // No need to post layout task to engine thread as OnceTask if this method
    // is invoked on engine thread
    remaining_task();
    return;
  }

  // PageElement style resolving needs to be executed on current thread,
  // but layout related tasks should be guaranteed to execute on Engine Thread
  std::promise<ParallelFlushReturn> promise;
  std::future<ParallelFlushReturn> future = promise.get_future();

  auto task_info_ptr = fml::MakeRefCounted<base::OnceTask<ParallelFlushReturn>>(
      [promise = std::move(promise),
       task = std::move(remaining_task)]() mutable {
        TRACE_EVENT(LYNX_TRACE_CATEGORY,
                    "FiberElement::PrepareForCreateOrUpdateAsync");
        promise.set_value(std::move(task));
      },
      std::move(future));

  base::TaskRunnerManufactor::PostTaskToConcurrentLoop(
      [task_info_ptr]() { task_info_ptr->Run(); },
      base::ConcurrentTaskType::HIGH_PRIORITY);
  task_queue.emplace_back(std::move(task_info_ptr));
}

void PageElement::SetCSSID(int32_t id) {
  ComponentElement::SetComponentCSSID(id);
}

}  // namespace tasm
}  // namespace lynx
