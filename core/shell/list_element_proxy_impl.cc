// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/list_element_proxy_impl.h"

#include "base/include/base_defines.h"

namespace lynx {
namespace shell {

void ListElementProxyImpl::ScrollByListContainer(int32_t tag, float x, float y,
                                                 float original_x,
                                                 float original_y) {
  engine_actor_->Act([tag, x, y, original_x, original_y](auto& engine) {
    auto list_element = engine->GetElementById(tag);
    if (!list_element) {
      return;
    }
    list_element->ScrollByListContainer(x, y, original_x, original_y);
  });
}

void ListElementProxyImpl::ScrollToPosition(int32_t tag, int index,
                                            float offset, int align,
                                            bool smooth) {
  engine_actor_->Act([tag, index, offset, align, smooth](auto& engine) {
    auto list_element = engine->GetElementById(tag);
    if (!list_element) {
      return;
    }
    list_element->ScrollToPosition(index, offset, align, smooth);
  });
}

void ListElementProxyImpl::ScrollStopped(int32_t tag) {
  engine_actor_->Act([tag](auto& engine) {
    auto list_element = engine->GetElementById(tag);
    if (!list_element) {
      return;
    }
    list_element->ScrollStopped();
  });
}

// Legacy
int32_t ListElementProxyImpl::LegacyImpl::ObtainListChild(
    int32_t tag, uint32_t index, int64_t operation_id,
    bool enable_reuse_notification) {
  if (!engine_actor_->CanRunNow()) {
    LOGE("ObtainListChild failed since current thread is not on engine thread");
    return -1;
  }
  auto* impl = engine_actor_->Impl();
  if (unlikely(!impl)) {
    LOGE("ObtainListChild failed since LynxEngine is nullptr");
    return -1;
  }
  auto* list_node = engine_actor_->Impl()->GetListNode(tag);
  if (!list_node) {
    return -1;
  }
  return list_node->ComponentAtIndex(index, operation_id,
                                     enable_reuse_notification);
}

void ListElementProxyImpl::LegacyImpl::ObtainListChildAsync(
    int32_t tag, uint32_t index, int64_t operation_id,
    bool enable_reuse_notification) {
  engine_actor_->ActAsync([tag, index, operation_id,
                           enable_reuse_notification](auto& engine) {
    tasm::ListNode* listNode = engine->GetListNode(tag);
    if (!listNode) {
      return;
    }
    listNode->ComponentAtIndex(index, operation_id, enable_reuse_notification);
  });
}

void ListElementProxyImpl::LegacyImpl::RecycleListChild(int32_t tag,
                                                        uint32_t sign) {
  engine_actor_->Act([tag, sign](auto& engine) {
    auto* list_node = engine->GetListNode(tag);
    if (!list_node) {
      return;
    }
    list_node->EnqueueComponent(sign);
  });
}

void ListElementProxyImpl::LegacyImpl::RecycleListChildAsync(int32_t tag,
                                                             uint32_t sign) {
  engine_actor_->ActAsync([tag, sign](auto& engine) {
    auto* list_node = engine->GetListNode(tag);
    if (!list_node) {
      return;
    }
    list_node->EnqueueComponent(sign);
  });
}

void ListElementProxyImpl::LegacyImpl::RenderListChild(int32_t tag,
                                                       uint32_t index,
                                                       int64_t operation_id) {
  engine_actor_->Act([tag, index, operation_id](auto& engine) {
    auto* list_node = engine->GetListNode(tag);
    if (!list_node) {
      return;
    }
    list_node->RenderComponentAtIndex(index, operation_id);
  });
}

void ListElementProxyImpl::LegacyImpl::UpdateListChild(int32_t tag,
                                                       uint32_t sign,
                                                       uint32_t index,
                                                       int64_t operation_id) {
  engine_actor_->Act([tag, sign, index, operation_id](auto& engine) {
    auto* list_node = engine->GetListNode(tag);
    if (!list_node) {
      return;
    }
    list_node->UpdateComponent(sign, index, operation_id);
  });
}

tasm::ListData ListElementProxyImpl::LegacyImpl::GetListData(int view_id) {
  auto result = tasm::ListData();
  if (!engine_actor_->CanRunNow()) {
    LOGE("GetListData failed since current thread is not on engine thread");
    return result;
  }
  auto* impl = engine_actor_->Impl();
  if (unlikely(!impl)) {
    LOGE("GetListData failed since LynxEngine is nullptr");
    return result;
  }
  auto* node = engine_actor_->Impl()->GetListNode(view_id);
  if (!node) {
    return result;
  }
  result.SetViewTypeNames(node->component_info());
  result.SetNewArch(node->NewArch());
  result.SetDiffable(node->Diffable());
  result.SetFullSpan(node->fullspan());
  result.SetStickyTop(node->sticky_top());
  result.SetStickyBottom(node->sticky_bottom());

  result.SetInsertions(node->DiffResult().insertions_);
  result.SetRemovals(node->DiffResult().removals_);
  result.SetUpdateFrom(node->DiffResult().update_from_);
  result.SetUpdateTo(node->DiffResult().update_to_);
  result.SetMoveFrom(node->DiffResult().move_from_);
  result.SetMoveTo(node->DiffResult().move_to_);

  return result;
}

}  // namespace shell
}  // namespace lynx
