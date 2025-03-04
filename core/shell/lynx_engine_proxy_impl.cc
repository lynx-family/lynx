// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/lynx_engine_proxy_impl.h"

#include <utility>

#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace shell {

bool LynxEngineProxyImpl::SendTouchEvent(const std::string& name, int32_t tag,
                                         float x, float y, float client_x,
                                         float client_y, float page_x,
                                         float page_y, int64_t timestamp) {
  if (engine_actor_ == nullptr) {
    LOGE(
        "LynxEngineProxy::SendTouchEvent failed since engine_actor_ is "
        "nullptr");
    return false;
  }
  tasm::EventInfo info(tag, x, y, client_x, client_y, page_x, page_y,
                       timestamp);
  engine_actor_->Act([name, info = std::move(info)](auto& engine) mutable {
    (void)engine->SendTouchEvent(name, info);
  });
  return false;
}

bool LynxEngineProxyImpl::SendTouchEvent(const std::string& name,
                                         const pub::Value& params,
                                         int64_t timestamp) {
  if (engine_actor_ == nullptr) {
    LOGE(
        "LynxEngineProxy::SendTouchEvent failed since engine_actor_ is "
        "nullptr");
    return false;
  }
  tasm::EventInfo info(pub::ValueUtils::ConvertValueToLepusValue(params),
                       timestamp);
  engine_actor_->Act([name, info = std::move(info)](auto& engine) mutable {
    engine->SendTouchEvent(name, info);
  });
  return false;
}

void LynxEngineProxyImpl::SendCustomEvent(const std::string& name, int32_t tag,
                                          const pub::Value& params,
                                          const std::string& params_name) {
  if (engine_actor_ == nullptr) {
    LOGE(
        "LynxEngineProxy::SendCustomEvent failed since engine_actor_ is "
        "nullptr");
    return;
  }
  auto params_value = pub::ValueUtils::ConvertValueToLepusValue(params);
  engine_actor_->Act([name, tag, params_value, params_name](auto& engine) {
    engine->SendCustomEvent(name, tag, params_value, params_name);
  });
}

void LynxEngineProxyImpl::SendGestureEvent(int tag, int gesture_id,
                                           std::string name,
                                           const pub::Value& params) {
  // Check if engine_actor_ is available.
  if (engine_actor_ == nullptr) {
    LOGE(
        "LynxEngineProxy::SendGestureEvent failed since engine_actor_ is "
        "nullptr");
    return;
  }

  // Perform the action using the engine_actor_.
  auto params_value = pub::ValueUtils::ConvertValueToLepusValue(params);
  engine_actor_->Act([tag, gesture_id, name, params_value](auto& engine) {
    engine->SendGestureEvent(tag, gesture_id, name, params_value);
  });
}

void LynxEngineProxyImpl::OnPseudoStatusChanged(int32_t id, int32_t pre_status,
                                                int32_t current_status) {
  if (engine_actor_ == nullptr) {
    LOGE(
        "LynxEngineProxy::OnPseudoStatusChanged failed since engine_actor_ is "
        "nullptr");
    return;
  }
  engine_actor_->Act([id, pre_status, current_status](auto& engine) {
    engine->OnPseudoStatusChanged(id, pre_status, current_status);
  });
}

void LynxEngineProxyImpl::SendBubbleEvent(const std::string& name, int32_t tag,
                                          const pub::Value& params) {
  if (engine_actor_ == nullptr) {
    LOGE(
        "LynxEngineProxy::SendBubbleEvent failed since engine_actor_ is "
        "nullptr");
    return;
  }

  auto params_value = pub::ValueUtils::ConvertValueToLepusValue(params).Table();
  engine_actor_->Act([name, tag, params_value](auto& engine) {
    engine->SendBubbleEvent(name, tag, params_value);
  });
}

void LynxEngineProxyImpl::ScrollByListContainer(int32_t tag, float x, float y,
                                                float original_x,
                                                float original_y) {
  if (engine_actor_ == nullptr) {
    LOGE("ScrollByListContainer failed since engine_actor_ is nullptr");
    return;
  }
  engine_actor_->Act([tag, x, y, original_x, original_y](auto& engine) {
    engine->ScrollByListContainer(tag, x, y, original_x, original_y);
  });
}

void LynxEngineProxyImpl::ScrollToPosition(int32_t tag, int index, float offset,
                                           int align, bool smooth) {
  if (engine_actor_ == nullptr) {
    LOGE("ScrollToPosition failed since engine_actor_ is nullptr");
    return;
  }
  engine_actor_->Act([tag, index, offset, align, smooth](auto& engine) {
    engine->ScrollToPosition(tag, index, offset, align, smooth);
  });
}

void LynxEngineProxyImpl::ScrollStopped(int32_t tag) {
  if (engine_actor_ == nullptr) {
    LOGE("ScrollStopped failed since engine_actor_ is nullptr");
    return;
  }
  engine_actor_->Act([tag](auto& engine) { engine->ScrollStopped(tag); });
}

int32_t LynxEngineProxyImpl::ObtainListChild(int32_t tag, uint32_t index,
                                             int64_t operation_id,
                                             bool enable_reuse_notification) {
  if (engine_actor_ == nullptr) {
    LOGE("ObtainListChild failed since engine_actor_ is nullptr");
    return -1;
  }

  DCHECK(engine_actor_->CanRunNow());
  if (!engine_actor_->CanRunNow()) {
    LOGE("ObtainListChild failed since current thread is not on engine thread");
    return -1;
  }
  auto* list_node = engine_actor_->Impl()->GetListNode(tag);
  if (list_node == nullptr) {
    return -1;
  }
  return list_node->ComponentAtIndex(index, operation_id,
                                     enable_reuse_notification);
}

void LynxEngineProxyImpl::RecycleListChild(int32_t tag, uint32_t sign) {
  if (engine_actor_ == nullptr) {
    LOGE("RecycleListChild failed since engine_actor_ is nullptr");
    return;
  }
  engine_actor_->Act([tag, sign](auto& engine) {
    auto* list_node = engine->GetListNode(tag);
    if (list_node != nullptr) {
      list_node->EnqueueComponent(sign);
    }
  });
}

void LynxEngineProxyImpl::RenderListChild(int32_t tag, uint32_t index,
                                          int64_t operation_id) {
  if (engine_actor_ == nullptr) {
    LOGE("RenderListChild failed since engine_actor_ is nullptr");
    return;
  }
  engine_actor_->Act([tag, index, operation_id](auto& engine) {
    auto* list_node = engine->GetListNode(tag);
    if (list_node != nullptr) {
      list_node->RenderComponentAtIndex(index, operation_id);
    }
  });
}

void LynxEngineProxyImpl::UpdateListChild(int32_t tag, uint32_t sign,
                                          uint32_t index,
                                          int64_t operation_id) {
  if (engine_actor_ == nullptr) {
    LOGE("UpdateListChild failed since engine_actor_ is nullptr");
    return;
  }
  engine_actor_->Act([tag, sign, index, operation_id](auto& engine) {
    auto* list_node = engine->GetListNode(tag);
    if (list_node != nullptr) {
      list_node->UpdateComponent(sign, index, operation_id);
    }
  });
}

tasm::ListData LynxEngineProxyImpl::GetListData(int view_id) {
  auto result = tasm::ListData();
  if (engine_actor_ == nullptr) {
    LOGE("GetListData failed since engine_actor_ is nullptr");
    return result;
  }
  DCHECK(engine_actor_->CanRunNow());
  if (!engine_actor_->CanRunNow()) {
    LOGE("GetListData failed since current thread is not on engine thread");
    return result;
  }
  auto* node = engine_actor_->Impl()->GetListNode(view_id);
  if (node == nullptr) {
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

void LynxEngineProxyImpl::MarkLayoutDirty(int sign) {
  if (engine_actor_ == nullptr) {
    LOGE(
        "LynxEngineProxy::MarkLayoutDirty failed since engine_actor_ is "
        "nullptr");
    return;
  }
  engine_actor_->Act([sign](auto& engine) {
    const auto& client = engine->GetTasm()->page_proxy()->element_manager();
    if (!client) return;
    tasm::Element* element = client->node_manager()->Get(sign);
    if (element) {
      element->MarkLayoutDirty();
    }
  });
}

bool LynxEngineProxyImpl::EnableRasterAnimation() {
  if (engine_actor_ == nullptr) {
    LOGE(
        "LynxEngineProxy::EnableRasterAnimation failed since engine_actor_ is "
        "nullptr");
    return false;
  }

  return engine_actor_->ActSync([](auto& engine) {
    return engine->GetTasm()
        ->page_proxy()
        ->element_manager()
        ->GetEnableRasterAnimation();
  });
}

float LynxEngineProxyImpl::GetDensity() const {
  if (engine_actor_ == nullptr) {
    LOGE(
        "LynxEngineProxy::GetDensity failed since engine_actor_ is "
        "nullptr, defaults to 1");
    return 1.f;
  }

  return engine_actor_->ActSync([](auto& engine) {
    return engine->GetTasm()
        ->page_proxy()
        ->element_manager()
        ->GetLynxEnvConfig()
        .LayoutsUnitPerPx();
  });
}

// TODO(huzhanbo.luc): remove this later
void LynxEngineProxyImpl::OnFirstMeaningfulPaint(){};

}  // namespace shell

}  // namespace lynx
