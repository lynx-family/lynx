// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/ios/native_painting_context_darwin.h"
#include "base/include/debug/lynx_error.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/dom/fragment/display_list.h"
#include "core/renderer/ui_wrapper/common/ios/platform_extra_bundle_darwin.h"
#include "core/renderer/ui_wrapper/layout/ios/text_layout_darwin.h"
#include "core/renderer/ui_wrapper/layout/textra/text_layout_textra.h"
#include "core/renderer/ui_wrapper/painting/ios/native_painting_context_platform_darwin_ref.h"
#include "core/renderer/ui_wrapper/painting/ios/painting_context_darwin_utils.h"
#include "core/renderer/ui_wrapper/painting/ios/platform_renderer_context_darwin.h"
#include "core/renderer/ui_wrapper/painting/ios/platform_renderer_darwin_factory.h"
#include "core/shell/dynamic_ui_operation_queue.h"
#include "core/value_wrapper/value_wrapper_utils.h"

#import <Foundation/Foundation.h>
#import <Lynx/LUIBodyView.h>
#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxShadowNodeOwner.h>
#import <Lynx/LynxUIOwner+Private.h>

namespace lynx {
namespace tasm {

NativePaintingCtxDarwin::NativePaintingCtxDarwin(LynxUIOwner *owner, void *textra)
    : context_(
          std::make_unique<PlatformRendererContextDarwin>([owner tryGetContainerView], owner)) {
  platform_ref_ = std::make_shared<NativePaintingCtxPlatformDarwinRef>(
      std::make_unique<PlatformRendererDarwinFactory>(context_.get()));

  context_->GetRendererContext().uiContext = owner.uiContext;
  if (textra != 0) {
    text_layout_impl_ = std::make_unique<TextLayoutTextra>(reinterpret_cast<intptr_t>(textra));
  } else {
    context_->GetRendererContext().textRenderManager = owner.textRenderManager;
    text_layout_impl_ =
        std::make_unique<TextLayoutDarwin>(owner.textRenderManager, owner.fontFaceContext);
  }
}

std::unique_ptr<pub::Value> NativePaintingCtxDarwin::GetTextInfo(const std::string &content,
                                                                 const pub::Value &info) {
  // TODO: impl this function later.
  return std::unique_ptr<pub::Value>();
}

std::vector<float> NativePaintingCtxDarwin::getBoundingClientOrigin(int id) {
  // TODO: impl this function later.
  return std::vector<float>();
}

std::vector<float> NativePaintingCtxDarwin::getWindowSize(int id) {
  // TODO: impl this function later.
  return std::vector<float>();
}

std::vector<float> NativePaintingCtxDarwin::GetRectToWindow(int id) {
  // TODO: impl this function later.
  return std::vector<float>();
}

std::vector<float> NativePaintingCtxDarwin::GetRectToLynxView(int64_t id) {
  // TODO: impl this function later.
  return std::vector<float>();
}

std::vector<float> NativePaintingCtxDarwin::ScrollBy(int64_t id, float width, float height) {
  // TODO: impl this function later.
  return std::vector<float>();
}

int32_t NativePaintingCtxDarwin::GetTagInfo(const std::string &tag_name) {
  NSInteger result = 0;
  BOOL supported = YES;
  NSString *nsTagName = [NSString stringWithUTF8String:tag_name.c_str()];
  Class clazz = [LynxComponentRegistry shadowNodeClassWithName:nsTagName accessible:&supported];
  if (supported) {
    LynxShadowNode *node = nil;
    NSInteger sign = 0;
    if (clazz) {
      node = [[clazz alloc] initWithSign:sign tagName:nsTagName];
    }
    if (node != nil) {
      result |= LynxShadowNodeTypeCustom;
      if ([node isVirtual]) {
        result |= LynxShadowNodeTypeVirtual;
      }
    } else {
      result |= LynxShadowNodeTypeCommon;
    }
  }
  return static_cast<int32_t>(result);
}

bool NativePaintingCtxDarwin::IsFlatten(base::MoveOnlyClosure<bool, bool> func) { return false; }

bool NativePaintingCtxDarwin::NeedAnimationProps() { return false; }

// For BTS
void NativePaintingCtxDarwin::Invoke(
    int64_t id, const std::string &method, const pub::Value &params,
    const std::function<void(int32_t, const pub::Value &)> &callback) {
  auto runner = base::UIThread::GetRunner();
  if (!runner) {
    return;
  }
  const auto &lepus_params = pub::ValueUtils::ConvertValueToLepusValue(params);
  base::MoveOnlyClosure<void, int32_t, const pub::Value &> cb =
      [callback](int32_t code, const pub::Value &data) { callback(code, data); };
  runner->PostTask([ref = platform_ref_, id, method, params = lepus_params.ToLepusValue(),
                    cb = std::move(cb)]() mutable {
    auto darwin_ref = std::static_pointer_cast<NativePaintingCtxPlatformDarwinRef>(ref);
    if (darwin_ref) {
      darwin_ref->InvokeUIMethod(static_cast<int32_t>(id), method, params, std::move(cb));
    }
  });
}

// For MTS
void NativePaintingCtxDarwin::EnqueueInvoke(
    int64_t id, const std::string &method, const pub::Value &params,
    const std::function<void(int32_t, const pub::Value &)> &callback) {
  const auto &lepus_params = pub::ValueUtils::ConvertValueToLepusValue(params);
  base::MoveOnlyClosure<void, int32_t, const pub::Value &> cb =
      [callback](int32_t code, const pub::Value &data) { callback(code, data); };
  Enqueue([ref = platform_ref_, id, method, params = lepus_params.ToLepusValue(),
           cb = std::move(cb)]() mutable {
    auto darwin_ref = std::static_pointer_cast<NativePaintingCtxPlatformDarwinRef>(ref);
    if (darwin_ref) {
      darwin_ref->InvokeUIMethod(static_cast<int32_t>(id), method, params, std::move(cb));
    }
  });
}

void NativePaintingCtxDarwin::StopExposure(const pub::Value &options) {
  auto runner = base::UIThread::GetRunner();
  if (!runner) {
    return;
  }
  auto lepus_options = pub::ValueUtils::ConvertValueToLepusValue(options);
  runner->PostTask([ref = platform_ref_, options = std::move(lepus_options)]() mutable {
    auto darwin_ref = std::static_pointer_cast<NativePaintingCtxPlatformDarwinRef>(ref);
    if (darwin_ref) {
      darwin_ref->StopExposure(options);
    }
  });
}

void NativePaintingCtxDarwin::ResumeExposure() {
  auto runner = base::UIThread::GetRunner();
  if (!runner) {
    return;
  }
  runner->PostTask([ref = platform_ref_]() mutable {
    auto darwin_ref = std::static_pointer_cast<NativePaintingCtxPlatformDarwinRef>(ref);
    if (darwin_ref) {
      darwin_ref->ResumeExposure();
    }
  });
}

void NativePaintingCtxDarwin::UpdatePlatformExtraBundle(int32_t id, PlatformExtraBundle *bundle) {
  if (!bundle) {
    return;
  }

  auto platform_bundle = static_cast<PlatformExtraBundleDarwin *>(bundle);
  Enqueue([ref = platform_ref_, id, value = platform_bundle->PlatformBundle()]() {
    std::static_pointer_cast<NativePaintingCtxPlatformDarwinRef>(ref)
        ->UpdatePlatformRendererExtraBundle(id, value);
  });
}

void NativePaintingCtxDarwin::FinishTasmOperation(const std::shared_ptr<PipelineOptions> &options) {
  if (queue_ != nullptr &&
      options->native_update_data_order_ == queue_->GetNativeUpdateDataOrder()) {
    queue_->UpdateStatus(shell::UIOperationStatus::TASM_FINISH);
  }
}

void NativePaintingCtxDarwin::FinishLayoutOperation(
    const std::shared_ptr<PipelineOptions> &options) {
  if (!has_first_screen_) {
    return;
  }

  __weak LynxUIOwner *ui_owner = context_ != nullptr ? context_->GetUIOwner() : nil;
  Enqueue(
      [ui_owner, weak_queue = std::weak_ptr<shell::DynamicUIOperationQueue>(queue_), options]() {
        if (auto queue = weak_queue.lock()) {
          [ui_owner finishLayoutOperation:options->operation_id componentID:options->list_comp_id_];
          if (options->has_layout) {
            [ui_owner layoutDidFinish];
          }
          if (options->native_update_data_order_ == queue->GetNativeUpdateDataOrder()) {
            queue->UpdateStatus(shell::UIOperationStatus::ALL_FINISH);
          }
        }
      });

  if (queue_ != nullptr &&
      options->native_update_data_order_ == queue_->GetNativeUpdateDataOrder()) {
    queue_->UpdateStatus(shell::UIOperationStatus::LAYOUT_FINISH);
  }
}

void NativePaintingCtxDarwin::Flush() { queue_->Flush(); }

void NativePaintingCtxDarwin::OnFirstScreen() { has_first_screen_ = true; }

void NativePaintingCtxDarwin::CreatePlatformRenderer(int id, PlatformRendererType type,
                                                     const fml::RefPtr<PropBundle> &init_data) {
  Enqueue([ref = platform_ref_, id, type, init_data]() {
    std::static_pointer_cast<NativePaintingCtxPlatformDarwinRef>(ref)->CreatePlatformRenderer(
        id, type, init_data);
  });
}

void NativePaintingCtxDarwin::CreatePlatformExtendedRenderer(
    int id, const base::String &tag_name, const fml::RefPtr<PropBundle> &init_data) {
  Enqueue([ref = platform_ref_, id, tag_name, init_data]() {
    std::static_pointer_cast<NativePaintingCtxPlatformDarwinRef>(ref)
        ->CreatePlatformExtendedRenderer(id, tag_name, init_data);
  });
}

void NativePaintingCtxDarwin::UpdateDisplayList(int id, DisplayList display_list) {
  Enqueue([ref = platform_ref_, id, dl = std::move(display_list)]() {
    std::static_pointer_cast<NativePaintingCtxPlatformDarwinRef>(ref)->UpdateDisplayList(
        id, std::move(const_cast<DisplayList &>(dl)));
  });
}

void NativePaintingCtxDarwin::UpdateTextBundle(int id, intptr_t bundle) {
  Enqueue([ref = platform_ref_, id, bundle]() {
    auto darwin_ref = std::static_pointer_cast<NativePaintingCtxPlatformDarwinRef>(ref);
    if (darwin_ref) {
      [darwin_ref->GetRendererContext() updateTextBundle:id
                                              withBundle:reinterpret_cast<void *>(bundle)];
    }
  });
}

void NativePaintingCtxDarwin::DestroyTextBundle(int id) {
  Enqueue([ref = platform_ref_, id]() {
    auto darwin_ref = std::static_pointer_cast<NativePaintingCtxPlatformDarwinRef>(ref);
    if (darwin_ref) {
      [darwin_ref->GetRendererContext() destroyTextBundle:id];
    }
  });
}

void NativePaintingCtxDarwin::InsertListItemPaintingNode(int32_t list_id, int32_t child_id) {}

void NativePaintingCtxDarwin::RemoveListItemPaintingNode(int32_t list_id, int32_t child_id) {}

void NativePaintingCtxDarwin::UpdateContentOffsetForListContainer(int32_t container_id,
                                                                  float content_size, float delta_x,
                                                                  float delta_y,
                                                                  bool is_init_scroll_offset,
                                                                  bool from_layout) {}

void NativePaintingCtxDarwin::ReconstructEventTargetTreeRecursively() {
  Enqueue([ref = platform_ref_]() {
    auto darwin_ref = std::static_pointer_cast<NativePaintingCtxPlatformDarwinRef>(ref);
    if (darwin_ref) {
      darwin_ref->ReconstructEventTargetTreeRecursively();
    }
  });
}

void NativePaintingCtxDarwin::UpdatePlatformEventBundle(int32_t id, PlatformEventBundle bundle) {
  Enqueue([ref = platform_ref_, id, bundle = std::move(bundle)]() {
    auto darwin_ref = std::static_pointer_cast<NativePaintingCtxPlatformDarwinRef>(ref);
    if (darwin_ref) {
      darwin_ref->UpdatePlatformEventBundle(id, std::move(bundle));
    }
  });
}

void NativePaintingCtxDarwin::CreateImage(int id, base::String src, float width, float height,
                                          int32_t event_mask) {
  LynxURL *sourceUrl = [[LynxURL alloc] init];
  sourceUrl.url = [[NSURL alloc] initWithString:[[NSString alloc] initWithUTF8String:src.c_str()]];
  sourceUrl.imageSize = CGSizeMake(width, height);

  [context_->GetRendererContext() createImageManager:id
                                       withSourceURL:sourceUrl
                                   andPlaceholderURL:nil
                                           eventMask:event_mask];
}

template <typename F>
void NativePaintingCtxDarwin::Enqueue(F &&func) {
  queue_->EnqueueUIOperation([func = std::move(func)]() mutable {
    @autoreleasepool {
      PaintingContextDarwinUtils::ExecuteSafely(func);
    }
  });
}

}  // namespace tasm
}  // namespace lynx
