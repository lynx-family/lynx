// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <arkui/native_node.h>
#include <multimedia/image_framework/image/pixelmap_native.h>

// Embedder headers may include portable NAPI after ArkUI host NAPI.
#undef NAPI_MODULE_INIT
#undef NAPI_MODULE
#undef NAPI_MODULE_X

#include <chrono>
#include <cmath>
#include <memory>
#include <string>
#include <utility>

#include "core/renderer/ui_wrapper/layout/harmony/layout_context_harmony.h"
#include "core/renderer/ui_wrapper/painting/harmony/painting_context_harmony.h"
#include "core/renderer/ui_wrapper/painting/harmony/ui_delegate_harmony.h"
#include "platform/embedder/lynx_ui_renderer.h"
#include "platform/embedder/lynx_view_clients.h"
#include "platform/embedder/lynx_view_priv.h"
#include "platform/embedder/public/capi/lynx_view_harmony_capi.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/shadow_node_owner.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_root.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/lynx_ui_screenshot_helper.h"

namespace lynx {
namespace embedder {
namespace {

void InvokeSnapshotCallback(tasm::TakeSnapshotCompletedCallback callback,
                            std::string data, float width, float height) {
  if (callback) {
    const float timestamp =
        std::chrono::steady_clock::now().time_since_epoch().count();
    callback(data, timestamp, width, height, 1.f);
  }
}

void PostSnapshotFailure(const fml::RefPtr<fml::TaskRunner>& screenshot_runner,
                         tasm::TakeSnapshotCompletedCallback callback) {
  auto task = [callback = std::move(callback)]() mutable {
    InvokeSnapshotCallback(std::move(callback), {}, 0.f, 0.f);
  };
  if (screenshot_runner) {
    screenshot_runner->PostTask(std::move(task));
  } else {
    task();
  }
}

class EmbedderUIDelegateHarmony final
    : public tasm::harmony::UIDelegateHarmony {
 public:
  EmbedderUIDelegateHarmony(
      std::unique_ptr<tasm::harmony::UIOwner> ui_owner,
      std::unique_ptr<tasm::harmony::ShadowNodeOwner> node_owner,
      const std::shared_ptr<tasm::harmony::LynxContext>& context)
      : UIDelegateHarmony(ui_owner.get(), node_owner.get(), context),
        ui_owner_holder_(std::move(ui_owner)),
        node_owner_holder_(std::move(node_owner)) {}

  void AdoptOwners(std::unique_ptr<tasm::harmony::UIOwner> ui_owner,
                   std::unique_ptr<tasm::harmony::ShadowNodeOwner> node_owner,
                   const std::shared_ptr<tasm::harmony::LynxContext>& context) {
    RebindOwners(ui_owner.get(), node_owner.get(), context);
    ui_owner_holder_ = std::move(ui_owner);
    node_owner_holder_ = std::move(node_owner);
  }

  std::unique_ptr<tasm::PaintingCtxPlatformImpl> CreatePaintingContext()
      override {
    auto context = GetLynxContext();
    if (context && context->IsFragmentLayerRenderOn()) {
      return UIDelegateHarmony::CreatePaintingContext();
    }
    // The platform context owns UIOwner in the native View path. Fragment
    // painting only borrows it, so retain ownership in the delegate there.
    return std::make_unique<tasm::PaintingContextHarmony>(
        ui_owner_holder_.release());
  }

  std::unique_ptr<tasm::LayoutCtxPlatformImpl> CreateLayoutContext() override {
    return std::make_unique<tasm::LayoutContextHarmony>(
        node_owner_holder_.release());
  }

  void TakeSnapshot(size_t max_width, size_t max_height, int quality,
                    const std::string& format, float,
                    const fml::RefPtr<fml::TaskRunner>& screenshot_runner,
                    tasm::TakeSnapshotCompletedCallback callback) override {
    auto context = GetLynxContext();
    if (!context) {
      PostSnapshotFailure(screenshot_runner, std::move(callback));
      return;
    }
    context->TakeScreenShot(max_width, max_height, quality, screenshot_runner,
                            std::move(callback), format);
  }

 private:
  std::unique_ptr<tasm::harmony::UIOwner> ui_owner_holder_;
  std::unique_ptr<tasm::harmony::ShadowNodeOwner> node_owner_holder_;
};

class HarmonyLynxUIRenderer final : public LynxUIRenderer,
                                    public tasm::harmony::LynxContextHost {
 public:
  HarmonyLynxUIRenderer(lynx_view_builder_t* builder,
                        ArkUI_ContextHandle ark_ui_context)
      : LynxUIRenderer(builder),
        ark_ui_context_(ark_ui_context),
        embedded_mode_(builder->embedded_mode) {
    Reset();
    ui_delegate_->OnUpdateScreenMetrics(builder->screen_size.width,
                                        builder->screen_size.height,
                                        builder->screen_size.pixel_ratio);
  }

  ~HarmonyLynxUIRenderer() override {
    InvalidateInputEvents();
    PrepareOwnersForDestroy();
  }

  void SetParent(NativeWindow) override {}
  NativeWindow GetNativeWindow() override { return nullptr; }
  void RegisterIMEHandler(void*, void*) override {}

  void AddClient(LynxViewClients* client) override {
    clients_ = client;
    BindIntrinsicContentSizeCallback();
  }

  void Reset() override {
    LynxUIRenderer::Reset();
    PrepareOwnersForDestroy();
    auto ui_owner = std::make_unique<tasm::harmony::UIOwner>();
    auto node_owner = std::make_unique<tasm::harmony::ShadowNodeOwner>();
    context_ = std::make_shared<tasm::harmony::LynxContext>(
        node_owner.get(), ui_owner.get(), nullptr);
    context_->SetEmbeddedMode(embedded_mode_);
    ui_owner->SetArkUIContext(ark_ui_context_);
    ui_owner->SetContext(context_);
    node_owner->SetContext(context_);
    if (parent_ && !ui_owner->Root()->AttachToParent(parent_)) {
      parent_ = nullptr;
    }
    ui_owner_ = ui_owner.get();
    node_owner_ = node_owner.get();
    context_->SetHost(this);
    if (ui_delegate_) {
      ui_delegate_->AdoptOwners(std::move(ui_owner), std::move(node_owner),
                                context_);
    } else {
      ui_delegate_ = std::make_unique<EmbedderUIDelegateHarmony>(
          std::move(ui_owner), std::move(node_owner), context_);
    }
    BindIntrinsicContentSizeCallback();
  }

  void PrepareForDestroy() override {
    PrepareOwnersForDestroy();
    clients_ = nullptr;
    parent_ = nullptr;
  }

  bool AttachPlatformView(void* platform_view) override {
    auto parent = static_cast<ArkUI_NodeHandle>(platform_view);
    auto ark_ui_context = parent ? OH_ArkUI_GetContextByNode(parent) : nullptr;
    if (!ui_owner_ || !ark_ui_context) {
      return false;
    }
    if (parent_ == parent) {
      return true;
    }
    ui_owner_->SetArkUIContext(ark_ui_context);
    ark_ui_context_ = ark_ui_context;
    if (!ui_owner_->Root()->AttachToParent(parent)) {
      parent_ = nullptr;
      return false;
    }
    parent_ = parent;
    return true;
  }

  void DetachPlatformView() override {
    if (ui_owner_ && parent_) {
      ui_owner_->Root()->DetachFromParent();
    }
    parent_ = nullptr;
  }

  void OnEnterForeground() override { ui_owner_->OnEnterForeground(); }
  void OnEnterBackground() override { ui_owner_->OnEnterBackground(); }
  tasm::UIDelegate* GetUIDelegate() override { return ui_delegate_.get(); }

  void ScreenSize(float size[2]) const override {
    size[0] = ui_delegate_->ScreenWidth();
    size[1] = ui_delegate_->ScreenHeight();
  }
  float DevicePixelRatio() const override {
    return ui_delegate_->DevicePixelRatio();
  }
  int32_t GetInstanceId() const override {
    return ui_delegate_->GetInstanceId();
  }

  void TakeSnapshot(size_t, size_t, int quality, const std::string& format,
                    const fml::RefPtr<fml::TaskRunner>& screenshot_runner,
                    tasm::TakeSnapshotCompletedCallback callback,
                    const std::string&) override {
    auto context = context_;
    if (!context) {
      PostSnapshotFailure(screenshot_runner, std::move(callback));
      return;
    }
    context->RunOnUIThread([context = std::move(context), format, quality,
                            screenshot_runner,
                            callback = std::move(callback)]() mutable {
      auto* ui_owner = context->GetUIOwner();
      auto* root = ui_owner ? ui_owner->Root() : nullptr;
      if (!root) {
        PostSnapshotFailure(screenshot_runner, std::move(callback));
        return;
      }
      std::string error;
      auto* pixel_map =
          tasm::harmony::LynxUIScreenshotHelper::TakeSnapshotForNode(
              root->GetProxyNode(), 1.f, error);
      if (!pixel_map) {
        PostSnapshotFailure(screenshot_runner, std::move(callback));
        return;
      }
      auto encode = [pixel_map, format, quality,
                     callback = std::move(callback)]() mutable {
        auto result = tasm::harmony::LynxUIScreenshotHelper::EncodeSnapshot(
            pixel_map, format, quality);
        InvokeSnapshotCallback(std::move(callback),
                               result.success ? std::move(result.data) : "",
                               result.width, result.height);
      };
      if (screenshot_runner) {
        screenshot_runner->PostTask(std::move(encode));
      } else {
        encode();
      }
    });
  }

 private:
  void BindIntrinsicContentSizeCallback() {
    ui_owner_->SetIntrinsicContentSizeChangeCallback(
        [this](float width, float height) {
          if (clients_) {
            clients_->OnIntrinsicContentSizeChanged(width, height);
          }
        });
  }

  void PrepareOwnersForDestroy() {
    if (!context_) {
      return;
    }
    context_->ResetHost();
    node_owner_->PrepareForDestroy();
    context_->ResetNodeOwner();
    node_owner_ = nullptr;
    ui_owner_->Root()->DetachFromParent();
    ui_owner_->PrepareForDestroy();
    context_->ResetUIOwner();
    ui_owner_ = nullptr;
    context_.reset();
  }

  tasm::harmony::UIOwner* ui_owner_{nullptr};
  tasm::harmony::ShadowNodeOwner* node_owner_{nullptr};
  std::unique_ptr<EmbedderUIDelegateHarmony> ui_delegate_;
  std::shared_ptr<tasm::harmony::LynxContext> context_;
  ArkUI_ContextHandle ark_ui_context_;
  const tasm::EmbeddedMode embedded_mode_;
  ArkUI_NodeHandle parent_{nullptr};
  LynxViewClients* clients_{nullptr};
};

}  // namespace
}  // namespace embedder
}  // namespace lynx

LYNX_EXTERN_C lynx_view_builder_t* lynx_view_builder_create_harmony(
    void* ark_ui_context, float viewport_width, float viewport_height,
    float screen_width, float screen_height, float pixel_ratio) {
  const auto is_positive = [](float value) {
    return value > 0 && std::isfinite(value);
  };
  const bool has_viewport = viewport_width != 0 || viewport_height != 0;
  if (!ark_ui_context || !is_positive(screen_width) ||
      !is_positive(screen_height) || !is_positive(pixel_ratio) ||
      (has_viewport &&
       (!is_positive(viewport_width) || !is_positive(viewport_height)))) {
    return nullptr;
  }
  auto* builder = lynx_view_builder_create();
  builder->screen_size = {screen_width, screen_height, pixel_ratio};
  builder->frame = {0, 0, viewport_width, viewport_height};
  builder->has_initial_viewport = has_viewport;
  builder->use_quickjs = true;
  builder->ui_renderer_creator =
      [ark_ui_context](lynx_view_builder_t* builder) {
        return std::make_unique<lynx::embedder::HarmonyLynxUIRenderer>(
            builder, static_cast<ArkUI_ContextHandle>(ark_ui_context));
      };
  return builder;
}

LYNX_EXTERN_C int lynx_view_attach_harmony(lynx_view_t* view,
                                           void* parent_node) {
  return view && view->lynx_ui_renderer && parent_node &&
         view->lynx_ui_renderer->AttachPlatformView(parent_node);
}

LYNX_EXTERN_C void lynx_view_detach_harmony(lynx_view_t* view) {
  if (view && view->lynx_ui_renderer) {
    view->lynx_ui_renderer->DetachPlatformView();
  }
}
