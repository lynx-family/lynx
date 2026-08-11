// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/ios/native_painting_context_darwin.h"

#include <algorithm>
#include <array>

#include "base/include/debug/lynx_error.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/dom/fragment/display_list.h"
#include "core/renderer/ui_wrapper/common/ios/platform_extra_bundle_darwin.h"
#include "core/renderer/ui_wrapper/layout/ios/text_layout_darwin.h"
#include "core/renderer/ui_wrapper/layout/textra/text_layout_textra.h"
#include "core/renderer/ui_wrapper/painting/ios/native_painting_context_platform_darwin_ref.h"
#include "core/renderer/ui_wrapper/painting/ios/paint_image_darwin.h"
#include "core/renderer/ui_wrapper/painting/ios/painting_context_darwin_utils.h"
#include "core/renderer/ui_wrapper/painting/ios/platform_renderer_context_darwin.h"
#include "core/renderer/ui_wrapper/painting/ios/platform_renderer_darwin_factory.h"
#include "core/renderer/ui_wrapper/painting/platform_renderer_impl.h"
#include "core/shell/dynamic_ui_operation_queue.h"
#include "core/value_wrapper/value_wrapper_utils.h"

#import <Foundation/Foundation.h>
#import <Lynx/LUIBodyView.h>
#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxShadowNodeOwner.h>
#import <Lynx/LynxUIOwner+Private.h>
#import <Lynx/LynxURL.h>
#include <cstdint>

namespace lynx {
namespace tasm {

namespace {

std::array<float, 4> CopyMetrics(const float *source) {
  std::array<float, 4> result = {0.f, 0.f, 0.f, 0.f};
  if (source != nullptr) {
    std::copy_n(source, result.size(), result.data());
  }
  return result;
}

}  // namespace

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

void NativePaintingCtxDarwin::UpdatePaintingNode(int id, bool tend_to_flatten,
                                                 const fml::RefPtr<PropBundle> &painting_data) {
  if (!painting_data) {
    return;
  }

  Enqueue([ref = platform_ref_, id, tend_to_flatten, painting_data]() mutable {
    auto darwin_ref = std::static_pointer_cast<NativePaintingCtxPlatformDarwinRef>(ref);
    if (!darwin_ref) {
      return;
    }

    darwin_ref->UpdateAttributes(id, painting_data, tend_to_flatten);
  });
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
  (void)id;
  float size[2] = {0.f, 0.f};
  auto darwin_ref = std::static_pointer_cast<NativePaintingCtxPlatformDarwinRef>(platform_ref_);
  if (darwin_ref) {
    darwin_ref->GetScreenSize(size);
  }
  return {size[0], size[1]};
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

void NativePaintingCtxDarwin::UpdateLayout(int tag, float x, float y, float width, float height,
                                           const float *paddings, const float *margins,
                                           const float *borders, const float *bounds,
                                           const float *sticky, float max_height,
                                           uint32_t node_index, bool display_none) {
  (void)bounds;
  (void)sticky;
  (void)max_height;
  (void)node_index;
  (void)display_none;
  auto padding_values = CopyMetrics(paddings);
  auto margin_values = CopyMetrics(margins);
  auto border_values = CopyMetrics(borders);
  Enqueue([ref = platform_ref_, tag, x, y, width, height, padding_values, margin_values,
           border_values]() {
    auto darwin_ref = std::static_pointer_cast<NativePaintingCtxPlatformDarwinRef>(ref);
    if (darwin_ref) {
      darwin_ref->UpdateLayoutMetrics(tag, x, y, width, height, padding_values.data(),
                                      margin_values.data(), border_values.data());
    }
  });
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

  if (options->need_timestamps && !options->pipeline_id.empty()) {
    Enqueue([ref = platform_ref_, pipeline_id = options->pipeline_id]() {
      ref->SetNeedMarkPaintEndTiming(pipeline_id);
    });
  }

  if (queue_ != nullptr &&
      options->native_update_data_order_ == queue_->GetNativeUpdateDataOrder()) {
    queue_->UpdateStatus(shell::UIOperationStatus::LAYOUT_FINISH);
  }
}

void NativePaintingCtxDarwin::Flush() { queue_->Flush(); }

void NativePaintingCtxDarwin::OnFirstScreen() { has_first_screen_ = true; }

void NativePaintingCtxDarwin::CreatePlatformRenderer(
    int id, PlatformRendererType type, const fml::RefPtr<PropBundle> &init_data,
    const PlatformRendererInitConfig &init_config) {
  Enqueue([ref = platform_ref_, id, type, init_config = init_config, init_data]() {
    std::static_pointer_cast<NativePaintingCtxPlatformDarwinRef>(ref)->CreatePlatformRenderer(
        id, type, init_data, init_config);
  });
}

void NativePaintingCtxDarwin::CreatePlatformExtendedRenderer(
    int id, const base::String &tag_name, const fml::RefPtr<PropBundle> &init_data,
    const PlatformRendererInitConfig &init_config) {
  Enqueue([ref = platform_ref_, id, tag_name, init_config = init_config, init_data]() {
    std::static_pointer_cast<NativePaintingCtxPlatformDarwinRef>(ref)
        ->CreatePlatformExtendedRenderer(id, tag_name, init_data, init_config);
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

void NativePaintingCtxDarwin::ReconstructEventTargetTreeRecursively() {
  auto platform_ref = std::static_pointer_cast<NativePaintingCtxPlatformRef>(platform_ref_);
  if (platform_ref && platform_ref->HasScheduledEventTargetTreeUpdate()) {
    return;
  }
  bool expected = false;
  if (!event_target_tree_update_enqueued_->compare_exchange_strong(expected, true)) {
    return;
  }
  Enqueue([enqueued = event_target_tree_update_enqueued_, ref = std::move(platform_ref)]() {
    auto darwin_ref = std::static_pointer_cast<NativePaintingCtxPlatformDarwinRef>(ref);
    if (darwin_ref) {
      darwin_ref->ScheduleEnsureEventTargetTree(kRootId);
    }
    enqueued->store(false);
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

fml::RefPtr<PaintImage> NativePaintingCtxDarwin::CreateImage(int id, base::String src,
                                                             ImageFitMode mode, float width,
                                                             float height, int32_t event_mask,
                                                             bool disable_default_resize) {
  static_cast<void>(disable_default_resize);
  LynxURL *sourceUrl = [[LynxURL alloc] init];
  sourceUrl.url = [[NSURL alloc] initWithString:[[NSString alloc] initWithUTF8String:src.c_str()]];
  sourceUrl.imageSize = CGSizeMake(width, height);
  int32_t image_key = GenerateUniqueImageKey();

  [context_->GetRendererContext() createImageManager:id
                                       withSourceURL:sourceUrl
                                   andPlaceholderURL:nil
                                                mode:static_cast<int32_t>(mode)
                                           eventMask:event_mask
                                            imageKey:image_key];
  return fml::MakeRefCounted<PaintImageDarwin>(image_key, context_->GetRendererContext());
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
