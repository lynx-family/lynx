// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/android/native_painting_context_android.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/vector.h"
#include "core/renderer/dom/fragment/display_list.h"
#include "core/renderer/ui_wrapper/layout/android/text_layout_android.h"
#include "core/renderer/ui_wrapper/painting/android/platform_renderer_android.h"
#include "core/renderer/ui_wrapper/painting/android/platform_renderer_context.h"
#include "core/renderer/utils/diff_algorithm.h"
#include "platform/android/lynx_android/src/main/jni/gen/NativePaintingContext_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/NativePaintingContext_register_jni.h"

// TODO: implement necessary functions for native ui renderer.
jlong CreatePaintingContext(JNIEnv *env, jobject jcaller, jobject jThis,
                            jlong platformRendererContextPtr,
                            jobject textLayout) {
  // This native object will be managed by NativePaintingContextAndroid with
  // unique_ptr.
  return reinterpret_cast<jlong>(new lynx::tasm::NativePaintingCtxAndroid(
      env, textLayout,
      reinterpret_cast<lynx::tasm::PlatformRendererContext *>(
          platformRendererContextPtr)));
}
namespace lynx {
namespace jni {
bool RegisterJNIForNativePaintingContext(JNIEnv *env) {
  return RegisterNativesImpl(env);
}
}  // namespace jni

namespace tasm {

class NativePaintingCtxAndroidRef : public PaintingCtxPlatformRef {
 public:
  explicit NativePaintingCtxAndroidRef(PlatformRendererContext *view_manager)
      : view_factory_(view_manager) {}

  ~NativePaintingCtxAndroidRef() override = default;

  void CreatePlatformRenderer(int id, PlatformRendererType type) {
    renderers_.insert_or_assign(id, view_factory_.CreateRenderer(id, type));
  }

  void UpdateDisplayList(int id, DisplayList &&display_list) {
    auto it = renderers_.find(id);
    if (it == renderers_.end()) {
      return;
    }

    const auto &layer = it->second;
    // Rebuild the sublayers according to the new SubLayers in the display list
    // with MyersDiff. And generate actual addChild and removeChild actions for
    // PlatformRenderer here.
    RebuildSubLayers(layer, display_list.SubLayers());

    layer->UpdateDisplayList(std::move(display_list));
  }

  void RemovePaintingNode(int parent, int child, int index,
                          bool is_move) override {
    if (auto it_child = renderers_.find(child); it_child != renderers_.end()) {
      it_child->second->RemoveFromParent();
    }
  }

  void DestroyPaintingNode(int parent, int child, int index) override {
    if (auto it_child = renderers_.find(child); it_child != renderers_.end()) {
      it_child->second->RemoveFromParent();
      renderers_.erase(child);
    }
  }

  void UpdateNodeReadyPatching(std::vector<int32_t> ready_ids,
                               std::vector<int32_t> remove_ids) override {
    PaintingCtxPlatformRef::UpdateNodeReadyPatching(ready_ids, remove_ids);
  }

  void UpdateFlattenStatus(int id, bool flatten) override {
    PaintingCtxPlatformRef::UpdateFlattenStatus(id, flatten);
  }

 private:
  void RebuildSubLayers(const fml::RefPtr<PlatformRenderer> &renderer,
                        const base::InlineVector<int, 16> &new_children);

  PlatformRendererAndroidFactory view_factory_;
  base::InlineOrderedFlatMap<int32_t, fml::RefPtr<PlatformRenderer>, 64>
      renderers_;
};

void NativePaintingCtxAndroidRef::RebuildSubLayers(
    const fml::RefPtr<PlatformRenderer> &renderer,
    const base::InlineVector<int, 16> &new_children) {
  const auto &existing_children = renderer->Children();

  if (existing_children.empty()) {
    // If there are no existing children, simply add all new sublayers.
    for (int child_id : new_children) {
      auto child_it = renderers_.find(child_id);
      if (child_it != renderers_.end()) {
        renderer->AddChild(child_it->second);
      }
    }
    return;
  }

  // Use MyersDiff to compare existing children with new display_list
  // SubLayers Custom comparator: compare existing child's ID with new child
  // ID
  auto id_compare = [](const fml::RefPtr<PlatformRenderer> &existing_child,
                       int new_child_id) {
    return existing_child->GetId() == new_child_id;
  };

  // Perform diff
  auto diff_result = myers_diff::MyersDiffWithoutUpdate(
      existing_children.begin(), existing_children.end(), new_children.begin(),
      new_children.end(), id_compare);

  // Apply removals: process in reverse order to avoid index shifting
  std::sort(diff_result.removals_.begin(), diff_result.removals_.end(),
            std::greater<>());
  for (int idx : diff_result.removals_) {
    if (idx >= 0 && static_cast<size_t>(idx) < existing_children.size()) {
      existing_children[idx]->RemoveFromParent();
    }
  }

  // Apply insertions
  for (int insert_pos : diff_result.insertions_) {
    if (insert_pos < 0 ||
        static_cast<size_t>(insert_pos) >= new_children.size()) {
      continue;
    }
    int child_id = new_children[insert_pos];
    auto child_it = renderers_.find(child_id);
    if (child_it != renderers_.end()) {
      renderer->AddChild(child_it->second);
    }
  }
}

NativePaintingCtxAndroid::NativePaintingCtxAndroid(
    JNIEnv *env, jobject text_layout, PlatformRendererContext *view_manager)
    : view_manager_(std::unique_ptr<PlatformRendererContext>(view_manager)) {
  platform_ref_ = std::make_shared<NativePaintingCtxAndroidRef>(view_manager);
  text_layout_impl_ = std::make_unique<TextLayoutAndroid>(env, text_layout);
}

void NativePaintingCtxAndroid::SetUIOperationQueue(
    const std::shared_ptr<shell::UIOperationQueueInterface> &queue) {
  queue_ = std::static_pointer_cast<shell::DynamicUIOperationQueue>(queue);
}

void NativePaintingCtxAndroid::CreatePaintingNode(
    int id, const std::string &tag,
    const fml::RefPtr<PropBundle> &painting_data, bool flatten,
    bool create_node_async, uint32_t node_index) {}

void NativePaintingCtxAndroid::UpdatePaintingNode(
    int id, bool tend_to_flatten,
    const fml::RefPtr<PropBundle> &painting_data) {}

std::unique_ptr<pub::Value> NativePaintingCtxAndroid::GetTextInfo(
    const std::string &content, const pub::Value &info) {
  return std::unique_ptr<pub::Value>();
}

void NativePaintingCtxAndroid::StopExposure(const pub::Value &options) {}

void NativePaintingCtxAndroid::ResumeExposure() {}

void NativePaintingCtxAndroid::UpdateLayout(
    int tag, float x, float y, float width, float height, const float *paddings,
    const float *margins, const float *borders, const float *bounds,
    const float *sticky, float max_height, uint32_t node_index) {}

void NativePaintingCtxAndroid::UpdatePlatformExtraBundle(
    int32_t id, PlatformExtraBundle *bundle) {
  PaintingCtxPlatformImpl::UpdatePlatformExtraBundle(id, bundle);
}

void NativePaintingCtxAndroid::SetKeyframes(
    fml::RefPtr<PropBundle> keyframes_data) {}

void NativePaintingCtxAndroid::Flush() { queue_->Flush(); }

void NativePaintingCtxAndroid::HandleValidate(int tag) {}

void NativePaintingCtxAndroid::FinishTasmOperation(
    const std::shared_ptr<PipelineOptions> &options) {}

void NativePaintingCtxAndroid::FinishLayoutOperation(
    const std::shared_ptr<PipelineOptions> &options) {}

std::vector<float> NativePaintingCtxAndroid::getBoundingClientOrigin(int id) {
  return std::vector<float>();
}

std::vector<float> NativePaintingCtxAndroid::getWindowSize(int id) {
  return std::vector<float>();
}

std::vector<float> NativePaintingCtxAndroid::GetRectToWindow(int id) {
  return std::vector<float>();
}

std::vector<float> NativePaintingCtxAndroid::GetRectToLynxView(int64_t id) {
  return std::vector<float>();
}

std::vector<float> NativePaintingCtxAndroid::ScrollBy(int64_t id, float width,
                                                      float height) {
  return std::vector<float>();
}

void NativePaintingCtxAndroid::Invoke(
    int64_t id, const std::string &method, const pub::Value &params,
    const std::function<void(int32_t, const pub::Value &)> &callback) {}

int32_t NativePaintingCtxAndroid::GetTagInfo(const std::string &tag_name) {
  return 0;
}

bool NativePaintingCtxAndroid::IsFlatten(
    base::MoveOnlyClosure<bool, bool> func) {
  return false;
}

bool NativePaintingCtxAndroid::NeedAnimationProps() { return false; }

void NativePaintingCtxAndroid::CreatePlatformRenderer(
    int id, PlatformRendererType type) {
  Enqueue([ref = platform_ref_, id, type]() {
    std::static_pointer_cast<NativePaintingCtxAndroidRef>(ref)
        ->CreatePlatformRenderer(id, type);
  });
}

void NativePaintingCtxAndroid::UpdateDisplayList(int id,
                                                 DisplayList display_list) {
  Enqueue([ref = platform_ref_, id, dl = std::move(display_list)]() mutable {
    std::static_pointer_cast<NativePaintingCtxAndroidRef>(ref)
        ->UpdateDisplayList(id, std::move(dl));
  });
}

void NativePaintingCtxAndroid::CreateImage(int id, base::String src,
                                           float width, float height) {
  view_manager_->CreateImage(id, src, width, height);
}

}  // namespace tasm
}  // namespace lynx
