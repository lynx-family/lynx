// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_XELEMENT_MARKDOWN_SRC_MAIN_CPP_IMPL_RESOURCE_MARKDOWN_RESOURCE_LOADER_HARMONY_H_
#define PLATFORM_HARMONY_LYNX_XELEMENT_MARKDOWN_SRC_MAIN_CPP_IMPL_RESOURCE_MARKDOWN_RESOURCE_LOADER_HARMONY_H_

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

#include "markdown/parser/markdown_resource_loader.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/measure_mode.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/lynx_image_helper.h"

namespace serval {
namespace markdown {
class NativeServalMarkdownView;
}  // namespace markdown
}  // namespace serval

namespace lynx {
namespace pub {
struct LynxPathResponse;
}  // namespace pub
namespace tasm {
namespace harmony {

class CanvasImageMarkdownDrawable;
class LynxBaseImage;
class LynxContext;
class MarkdownShadowNode;
class ShadowNode;

class MarkdownResourceLoaderHarmony final
    : public serval::markdown::MarkdownResourceLoader,
      public std::enable_shared_from_this<MarkdownResourceLoaderHarmony> {
 public:
  MarkdownResourceLoaderHarmony(
      LynxContext* context, MarkdownShadowNode* markdown_node,
      serval::markdown::NativeServalMarkdownView* markdown_view);
  ~MarkdownResourceLoaderHarmony() override;

  MarkdownResourceLoaderHarmony(const MarkdownResourceLoaderHarmony&) = delete;
  MarkdownResourceLoaderHarmony& operator=(
      const MarkdownResourceLoaderHarmony&) = delete;

  std::shared_ptr<serval::markdown::MarkdownDrawable> LoadImage(
      const char* src, float desire_width, float desire_height, float max_width,
      float max_height, float border_radius) override;
  std::shared_ptr<serval::markdown::MarkdownDrawable> LoadInlineView(
      const char* id_selector, float max_width, float max_height) override;
  void* LoadFont(const char* family,
                 serval::markdown::MarkdownFontWeight weight) override;
  std::shared_ptr<serval::markdown::MarkdownDrawable> LoadReplacementView(
      void* ud, int32_t id, float max_width, float max_height) override;

  void Destroy();
  void SetMeasureContext(float width, MeasureMode width_mode, float height,
                         MeasureMode height_mode, bool final_measure,
                         float density);
  void AlignInlineView(const std::string& id_selector, float x, float y);

 private:
  std::shared_ptr<serval::markdown::MarkdownDrawable> CreatePlaceholderDrawable(
      float width = 1.f, float height = 1.f);
  ShadowNode* FindChildById(const char* id) const;
  serval::markdown::MeasureResult MeasureChild(ShadowNode* child,
                                               float max_width,
                                               float max_height) const;
  void LoadImageResource(std::weak_ptr<CanvasImageMarkdownDrawable> drawable,
                         const std::string& source);
  void DecodeImageResource(std::weak_ptr<CanvasImageMarkdownDrawable> drawable,
                           const std::string& notify_source,
                           const std::string& decode_source, bool is_base64);
  void OnImageResourceLoaded(
      std::weak_ptr<CanvasImageMarkdownDrawable> drawable,
      const std::string& fallback_source, pub::LynxPathResponse& response);
  void OnImageDecoded(std::weak_ptr<CanvasImageMarkdownDrawable> drawable,
                      const std::string& notify_source,
                      const std::string& decode_source,
                      LynxImageHelper::ImageResponse& response);
  std::shared_ptr<LynxBaseImage> GetCachedImage(const std::string& source);
  void SetCachedImage(const std::string& source,
                      std::shared_ptr<LynxBaseImage> image);
  void NotifyImageLoaded(const std::string& source);

  std::atomic<LynxContext*> context_{nullptr};
  std::atomic<MarkdownShadowNode*> markdown_node_{nullptr};
  std::atomic<serval::markdown::NativeServalMarkdownView*> markdown_view_{
      nullptr};
  std::atomic<bool> destroyed_{false};
  mutable std::mutex image_cache_mutex_;
  std::unordered_map<std::string, std::weak_ptr<LynxBaseImage>> image_cache_;
  float measure_width_{0.f};
  MeasureMode measure_width_mode_{MeasureMode::Indefinite};
  float measure_height_{0.f};
  MeasureMode measure_height_mode_{MeasureMode::Indefinite};
  bool final_measure_{false};
  float density_{1.f};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_XELEMENT_MARKDOWN_SRC_MAIN_CPP_IMPL_RESOURCE_MARKDOWN_RESOURCE_LOADER_HARMONY_H_
