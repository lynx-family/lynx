// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shared_data/white_board_delegate.h"

#include <string>
#include <utility>

#include "base/trace/native/trace_event.h"
#include "core/renderer/events/closure_event_listener.h"
#include "core/runtime/common/bindings/event/context_proxy.h"
#include "core/runtime/js/runtime_constant.h"
#include "core/shared_data/lynx_white_board.h"
#include "core/shared_data/shared_data_trace_event_def.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace tasm {

WhiteBoardDelegate::WhiteBoardDelegate(
    const std::shared_ptr<WhiteBoard>& white_board)
    : white_board_(white_board) {}

std::shared_ptr<WhiteBoardInspector> WhiteBoardDelegate::GetInspector() {
  if (white_board_) {
    return white_board_->GetInspector();
  }
  return nullptr;
}

void WhiteBoardDelegate::SetInspector(
    const std::shared_ptr<WhiteBoardInspector>& inspector) {
  if (white_board_) {
    inspector->SetWhiteBoard(white_board_);
    white_board_->SetInspector(inspector);
  }
}

void WhiteBoardDelegate::AddEventListeners(
    runtime::ContextProxy* js_context_proxy) {
  if (!js_context_proxy) {
    return;
  }
  js_context_proxy->AddEventListener(
      runtime::kMessageEventSetSessionStorageItem,
      std::make_unique<event::ClosureEventListener>([this](lepus::Value args) {
        if (args.IsTable()) {
          BASE_STATIC_STRING_DECL(kKey, "key");
          BASE_STATIC_STRING_DECL(kValue, "value");
          const auto& session_key = args.Table()->GetValue(kKey).StdString();
          const auto& session_value = args.Table()->GetValue(kValue);
          SetSessionStorageItem(session_key, session_value);
        }
      }));

  js_context_proxy->AddEventListener(
      runtime::kMessageEventUnSubscribeSessionStorage,
      std::make_unique<event::ClosureEventListener>([this](lepus::Value args) {
        if (args.IsTable()) {
          BASE_STATIC_STRING_DECL(kKey, "key");
          BASE_STATIC_STRING_DECL(kValue, "listenerId");
          const auto& session_key = args.Table()->GetValue(kKey).StdString();
          auto listener_id = args.Table()->GetValue(kValue).Number();
          UnsubscribeJSSessionStorage(session_key, listener_id);
        }
      }));
}

void WhiteBoardDelegate::SetSessionStorageItem(const std::string& key,
                                               const lepus::Value& value) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              WHITE_BOARD_DELEGATE_SET_SESSION_STORAGE_ITEM,
              [&key, &value](lynx::perfetto::EventContext ctx) {
                std::stringstream ss;
                value.PrintValue(ss);
                ctx.event()->add_debug_annotations("key", key);
                ctx.event()->add_debug_annotations("value", ss.str());
              });
  if (white_board_) {
    auto shared_data = std::make_shared<pub::ValueImplLepus>(value);
    white_board_->SetGlobalSharedData(key, std::move(shared_data));
  }
}

lepus::Value WhiteBoardDelegate::GetSessionStorageItem(const std::string& key) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              WHITE_BOARD_DELEGATE_GET_SESSION_STORAGE_ITEM,
              [&key](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_debug_annotations("key", key);
              });
  lepus::Value result;
  if (white_board_) {
    auto value = white_board_->GetGlobalSharedData(key);
    if (value) {
      result = pub::ValueUtils::ConvertValueToLepusValue(*value);
    }
  }
  return result;
}

void WhiteBoardDelegate::SubscribeJSSessionStorage(
    const std::string& key, double listener_id,
    const piper::ApiCallBack& callback) {
  TRACE_EVENT(
      LYNX_TRACE_CATEGORY, WHITE_BOARD_DELEGATE_SUBSCRIBE_JS_SESSION_STORAGE,
      [&key, &listener_id,
       flow_id = callback.trace_flow_id()](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_debug_annotations("key", key);
        ctx.event()->add_debug_annotations("listener_id",
                                           std::to_string(listener_id));
        ctx.event()->add_flow_ids(flow_id);
      });
  if (white_board_) {
    // invoked while session storage changed.
    auto triggered_callback = [weak_self = weak_from_this(),
                               callback](const pub::Value& value) {
      auto self = weak_self.lock();
      if (self) {
        lepus::Value result = pub::ValueUtils::ConvertValueToLepusValue(value);
        self->CallJSApiCallbackWithValue(callback, result);
      }
    };
    // invoked while session storage removed.
    auto removed_callback = [weak_self = weak_from_this(), callback]() {
      auto self = weak_self.lock();
      if (self) {
        self->RemoveJSApiCallback(callback);
      }
    };

    white_board_->RegisterSharedDataListener(
        WhiteBoardStorageType::TYPE_JS, key,
        {listener_id, std::move(triggered_callback),
         std::move(removed_callback)});
  }
}

void WhiteBoardDelegate::UnsubscribeJSSessionStorage(const std::string& key,
                                                     double listener_id) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              WHITE_BOARD_DELEGATE_UNSUBSCRIBE_JS_SESSION_STORAGE,
              [&key, &listener_id](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_debug_annotations("key", key);
                ctx.event()->add_debug_annotations("listener_id",
                                                   std::to_string(listener_id));
              });
  if (white_board_) {
    white_board_->RemoveSharedDataListener(WhiteBoardStorageType::TYPE_JS, key,
                                           listener_id);
  }
}

void WhiteBoardDelegate::SubScribeClientSessionStorage(
    const std::string& key,
    const std::shared_ptr<lynx::shell::PlatformCallBackHolder>& callback) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              WHITE_BOARD_DELEGATE_SUBSCRIBE_CLIENT_SESSION_STORAGE,
              [&key, &callback](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_debug_annotations("key", key);
                ctx.event()->add_debug_annotations(
                    "callback_id", std::to_string(callback->id()));
              });
  if (white_board_) {
    double id = callback->id();
    // invoked while session storage changed.
    auto triggered_callback = [weak_self = weak_from_this(),
                               callback](const pub::Value& value) {
      auto self = weak_self.lock();
      if (self) {
        lepus::Value result = pub::ValueUtils::ConvertValueToLepusValue(value);
        self->CallPlatformCallbackWithValue(callback, result);
      }
    };
    // invoked while session storage removed.
    auto removed_callback = [weak_self = weak_from_this(), callback]() {
      auto self = weak_self.lock();
      if (self) {
        self->RemovePlatformCallback(callback);
      }
    };
    white_board_->RegisterSharedDataListener(
        WhiteBoardStorageType::TYPE_CLIENT, key,
        {id, std::move(triggered_callback), std::move(removed_callback)});
  }
}

void WhiteBoardDelegate::UnsubscribeClientSessionStorage(const std::string& key,
                                                         double callback_id) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              WHITE_BOARD_DELEGATE_UNSUBSCRIBE_CLIENT_SESSION_STORAGE,
              [&key, &callback_id](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_debug_annotations("key", key);
                ctx.event()->add_debug_annotations("callback_id",
                                                   std::to_string(callback_id));
              });
  if (white_board_) {
    white_board_->RemoveSharedDataListener(WhiteBoardStorageType::TYPE_CLIENT,
                                           key, callback_id);
  }
}

}  // namespace tasm
}  // namespace lynx
