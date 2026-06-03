// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_xelement/markdown/src/main/cpp/impl/resource/markdown_resource_loader_harmony.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "base/include/log/logging.h"
#include "base/include/string/string_utils.h"
#include "core/public/lynx_resource_loader.h"
#include "markdown/element/markdown_drawable.h"
#include "markdown/platform/harmony/serval_markdown_view.h"
#include "markdown/view/markdown_view.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/lynx_base_image.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/lynx_image_constants.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/lynx_pixel_map.h"
#include "platform/harmony/lynx_xelement/markdown/src/main/cpp/impl/shadow_node/markdown_shadow_node.h"

namespace lynx {
namespace tasm {
namespace harmony {
namespace {

bool IsBase64Image(const std::string& source) {
  return base::BeginsWith(source, image::kBase64Scheme);
}

bool IsLocalResourceUrl(const std::string& source) {
  return base::BeginsWith(source, image::kLocalScheme) ||
         base::BeginsWith(source, image::kResourceScheme);
}

bool HasImageSource(const std::string& source) { return !source.empty(); }

std::string NormalizeDecodeSource(std::string source) {
  if (!source.empty() && source[0] == '/') {
    source = std::string("file://") + source;
  }
  return source;
}

float NormalizePositiveSize(float value) {
  return value > 0.f && std::isfinite(value) ? value : 0.f;
}

float NormalizeNonNegativeSize(float value) {
  return value >= 0.f && std::isfinite(value) ? value : 0.f;
}

std::pair<float, float> ResolveImageSize(float desire_width,
                                         float desire_height, float max_width,
                                         float max_height,
                                         float intrinsic_width = 0.f,
                                         float intrinsic_height = 0.f) {
  float width = NormalizePositiveSize(desire_width);
  float height = NormalizePositiveSize(desire_height);
  const float image_width = NormalizePositiveSize(intrinsic_width);
  const float image_height = NormalizePositiveSize(intrinsic_height);
  const bool has_intrinsic_size = image_width > 0.f && image_height > 0.f;
  const bool has_max_width = max_width >= 0.f && std::isfinite(max_width);
  const bool has_max_height = max_height >= 0.f && std::isfinite(max_height);

  if (width <= 0.f && height <= 0.f) {
    if (has_intrinsic_size) {
      width = image_width;
      height = image_height;
    } else if ((has_max_width && max_width == 0.f) ||
               (has_max_height && max_height == 0.f)) {
      return {0.f, 0.f};
    } else {
      width = 1.f;
      height = 1.f;
    }
  } else if (width <= 0.f) {
    width = has_intrinsic_size ? height * image_width / image_height : height;
  } else if (height <= 0.f) {
    height = has_intrinsic_size ? width * image_height / image_width : width;
  }

  if (width <= 0.f || height <= 0.f) {
    return {0.f, 0.f};
  }

  float scale = 1.f;
  if (has_max_width) {
    scale = std::min(scale, max_width / width);
  }
  if (has_max_height) {
    scale = std::min(scale, max_height / height);
  }
  width *= scale;
  height *= scale;
  return {NormalizeNonNegativeSize(width), NormalizeNonNegativeSize(height)};
}

class FixedSizeMarkdownDrawable final
    : public serval::markdown::MarkdownDrawable {
 public:
  FixedSizeMarkdownDrawable(float width, float height)
      : width_(NormalizeNonNegativeSize(width)),
        height_(NormalizeNonNegativeSize(height)) {}

  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override {
    (void)canvas;
    (void)x;
    (void)y;
  }

 protected:
  serval::markdown::MeasureResult OnMeasure(
      serval::markdown::MeasureSpec spec) override {
    (void)spec;
    return {.width_ = width_, .height_ = height_, .baseline_ = height_};
  }

 private:
  float width_{1.f};
  float height_{1.f};
};

}  // namespace

class CanvasImageMarkdownDrawable final
    : public serval::markdown::MarkdownDrawable {
 public:
  CanvasImageMarkdownDrawable(float desire_width, float desire_height,
                              float max_width, float max_height, float width,
                              float height)
      : desire_width_(desire_width),
        desire_height_(desire_height),
        max_width_(max_width),
        max_height_(max_height),
        measured_size_{NormalizeNonNegativeSize(width),
                       NormalizeNonNegativeSize(height)} {}

  void SetImage(std::shared_ptr<LynxBaseImage> image) {
    if (!image) {
      return;
    }

    const float intrinsic_width =
        NormalizePositiveSize(static_cast<float>(image->Width()));
    const float intrinsic_height =
        NormalizePositiveSize(static_cast<float>(image->Height()));
    auto size =
        ResolveImageSize(desire_width_, desire_height_, max_width_, max_height_,
                         intrinsic_width, intrinsic_height);
    std::lock_guard<std::mutex> lock(mutex_);
    image_ = std::move(image);
    intrinsic_size_ = {intrinsic_width, intrinsic_height};
    measured_size_ = {size.first, size.second};
    visible_ = measured_size_.width_ > 0.f && measured_size_.height_ > 0.f;
  }

  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override {
    if (!canvas) {
      return;
    }

    std::shared_ptr<LynxBaseImage> image;
    serval::markdown::SizeF measured_size;
    serval::markdown::SizeF intrinsic_size;
    bool visible = false;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      image = image_;
      measured_size = measured_size_;
      intrinsic_size = intrinsic_size_;
      visible = visible_;
    }

    if (!visible || !image || measured_size.width_ <= 0.f ||
        measured_size.height_ <= 0.f || !std::isfinite(x) ||
        !std::isfinite(y)) {
      return;
    }

    auto* pixel_map = image->FirstFrame();
    if (!pixel_map || !pixel_map->Bitmap()) {
      return;
    }

    const float source_width = intrinsic_size.width_ > 0.f
                                   ? intrinsic_size.width_
                                   : measured_size.width_;
    const float source_height = intrinsic_size.height_ > 0.f
                                    ? intrinsic_size.height_
                                    : measured_size.height_;
    if (source_width <= 0.f || source_height <= 0.f) {
      return;
    }

    canvas->Save();
    canvas->Translate(x, y);
    canvas->Scale(measured_size.width_ / source_width,
                  measured_size.height_ / source_height);
    canvas->DrawImage(reinterpret_cast<const char*>(pixel_map->Bitmap()), 0.f,
                      0.f, source_width, source_height, nullptr);
    canvas->Restore();
  }

  void SetBounds(serval::markdown::RectF bounds) override {
    const bool valid =
        bounds.GetWidth() > 0.f && bounds.GetHeight() > 0.f &&
        std::isfinite(bounds.GetWidth()) && std::isfinite(bounds.GetHeight()) &&
        std::isfinite(bounds.GetLeft()) && std::isfinite(bounds.GetTop());
    std::lock_guard<std::mutex> lock(mutex_);
    visible_ = valid;
    if (valid) {
      measured_size_ = {bounds.GetWidth(), bounds.GetHeight()};
    }
  }

 protected:
  serval::markdown::MeasureResult OnMeasure(
      serval::markdown::MeasureSpec spec) override {
    (void)spec;
    std::lock_guard<std::mutex> lock(mutex_);
    return {.width_ = measured_size_.width_,
            .height_ = measured_size_.height_,
            .baseline_ = measured_size_.height_};
  }

 private:
  float desire_width_{0.f};
  float desire_height_{0.f};
  float max_width_{0.f};
  float max_height_{0.f};
  mutable std::mutex mutex_;
  serval::markdown::SizeF measured_size_{};
  serval::markdown::SizeF intrinsic_size_{};
  std::shared_ptr<LynxBaseImage> image_;
  bool visible_{true};
};

class InlineViewMarkdownDrawable final
    : public serval::markdown::MarkdownDrawable {
 public:
  InlineViewMarkdownDrawable(
      std::weak_ptr<MarkdownResourceLoaderHarmony> resource_loader,
      std::string id_selector, serval::markdown::MeasureResult measure_result)
      : resource_loader_(std::move(resource_loader)),
        id_selector_(std::move(id_selector)),
        measured_size_{NormalizeNonNegativeSize(measure_result.width_),
                       NormalizeNonNegativeSize(measure_result.height_)},
        baseline_(ResolveBaseline(measure_result.baseline_,
                                  measured_size_.height_)) {}

  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override {
    (void)canvas;
    (void)x;
    (void)y;
  }

  void Align(float x, float y) override {
    auto loader = resource_loader_.lock();
    if (!loader || id_selector_.empty() || !std::isfinite(x) ||
        !std::isfinite(y)) {
      return;
    }
    loader->AlignInlineView(id_selector_, x, y);
  }

 protected:
  serval::markdown::MeasureResult OnMeasure(
      serval::markdown::MeasureSpec spec) override {
    (void)spec;
    return {.width_ = measured_size_.width_,
            .height_ = measured_size_.height_,
            .baseline_ = baseline_};
  }

 private:
  static float ResolveBaseline(float baseline, float height) {
    baseline = NormalizeNonNegativeSize(baseline);
    height = NormalizeNonNegativeSize(height);
    if (baseline <= 0.f || baseline > height) {
      return height;
    }
    return baseline;
  }

  std::weak_ptr<MarkdownResourceLoaderHarmony> resource_loader_;
  std::string id_selector_;
  serval::markdown::SizeF measured_size_{};
  float baseline_{0.f};
};

MarkdownResourceLoaderHarmony::MarkdownResourceLoaderHarmony(
    LynxContext* context, MarkdownShadowNode* markdown_node,
    serval::markdown::NativeServalMarkdownView* markdown_view)
    : context_(context),
      markdown_node_(markdown_node),
      markdown_view_(markdown_view) {}

MarkdownResourceLoaderHarmony::~MarkdownResourceLoaderHarmony() { Destroy(); }

std::shared_ptr<serval::markdown::MarkdownDrawable>
MarkdownResourceLoaderHarmony::LoadImage(const char* src, float desire_width,
                                         float desire_height, float max_width,
                                         float max_height,
                                         float border_radius) {
  // The Harmony text canvas does not expose rounded clipping here yet.
  (void)border_radius;
  const std::string source = src ? src : "";
  auto size =
      ResolveImageSize(desire_width, desire_height, max_width, max_height);
  if (!HasImageSource(source) || destroyed_.load()) {
    return CreatePlaceholderDrawable(size.first, size.second);
  }

  auto image_view = std::make_shared<CanvasImageMarkdownDrawable>(
      desire_width, desire_height, max_width, max_height, size.first,
      size.second);
  if (auto cached_image = GetCachedImage(source)) {
    image_view->SetImage(cached_image);
    return image_view;
  }
  LoadImageResource(image_view, source);
  return image_view;
}

std::shared_ptr<serval::markdown::MarkdownDrawable>
MarkdownResourceLoaderHarmony::LoadInlineView(const char* id_selector,
                                              float max_width,
                                              float max_height) {
  if (auto* child = FindChildById(id_selector)) {
    auto measure_result = MeasureChild(child, max_width, max_height);
    return std::make_shared<InlineViewMarkdownDrawable>(
        weak_from_this(), id_selector, measure_result);
  }
  return nullptr;
}

void* MarkdownResourceLoaderHarmony::LoadFont(
    const char* family, serval::markdown::MarkdownFontWeight weight) {
  (void)family;
  (void)weight;
  return nullptr;
}

std::shared_ptr<serval::markdown::MarkdownDrawable>
MarkdownResourceLoaderHarmony::LoadReplacementView(void* ud, int32_t id,
                                                   float max_width,
                                                   float max_height) {
  (void)ud;
  (void)id;
  const float width =
      max_width >= 0.f && std::isfinite(max_width) ? max_width : 1.f;
  const float height =
      max_height >= 0.f && std::isfinite(max_height) ? max_height : 1.f;
  return CreatePlaceholderDrawable(width, height);
}

void MarkdownResourceLoaderHarmony::Destroy() {
  if (destroyed_.exchange(true)) {
    return;
  }
  context_.store(nullptr);
  markdown_node_.store(nullptr);
  markdown_view_.store(nullptr);
  std::lock_guard<std::mutex> lock(image_cache_mutex_);
  image_cache_.clear();
}

void MarkdownResourceLoaderHarmony::SetMeasureContext(
    float width, MeasureMode width_mode, float height, MeasureMode height_mode,
    bool final_measure, float density) {
  measure_width_ = width;
  measure_width_mode_ = width_mode;
  measure_height_ = height;
  measure_height_mode_ = height_mode;
  final_measure_ = final_measure;
  density_ = density > 0.f ? density : 1.f;
}

void MarkdownResourceLoaderHarmony::AlignInlineView(
    const std::string& id_selector, float x, float y) {
  if (destroyed_.load() || id_selector.empty() || !std::isfinite(x) ||
      !std::isfinite(y)) {
    return;
  }
  auto* child = FindChildById(id_selector.c_str());
  if (!child || child->IsDestroyed()) {
    return;
  }
  child->AlignLayoutNode(x / density_, y / density_);
}

std::shared_ptr<serval::markdown::MarkdownDrawable>
MarkdownResourceLoaderHarmony::CreatePlaceholderDrawable(float width,
                                                         float height) {
  return std::make_shared<FixedSizeMarkdownDrawable>(width, height);
}

ShadowNode* MarkdownResourceLoaderHarmony::FindChildById(const char* id) const {
  auto* markdown_node = markdown_node_.load();
  if (!markdown_node || !id || id[0] == '\0') {
    return nullptr;
  }

  const std::string id_selector(id);
  std::vector<ShadowNode*> stack(markdown_node->GetChildren().begin(),
                                 markdown_node->GetChildren().end());
  while (!stack.empty()) {
    auto* child = stack.back();
    stack.pop_back();
    if (!child) {
      continue;
    }
    if (child->GetIdSelector() == id_selector) {
      return child;
    }
    const auto& children = child->GetChildren();
    stack.insert(stack.end(), children.begin(), children.end());
  }
  return nullptr;
}

serval::markdown::MeasureResult MarkdownResourceLoaderHarmony::MeasureChild(
    ShadowNode* child, float max_width, float max_height) const {
  if (!child) {
    return {.width_ = 1.f, .height_ = 1.f, .baseline_ = 1.f};
  }

  const bool has_max_width = max_width >= 0.f && std::isfinite(max_width);
  const bool has_max_height = max_height >= 0.f && std::isfinite(max_height);
  const float width = has_max_width ? max_width / density_ : measure_width_;
  const float height = has_max_height ? max_height / density_ : measure_height_;
  const auto width_mode =
      has_max_width ? MeasureMode::AtMost : measure_width_mode_;
  const auto height_mode =
      has_max_height ? MeasureMode::AtMost : measure_height_mode_;
  auto size = child->MeasureLayoutNode(width, width_mode, height, height_mode,
                                       final_measure_);
  float measured_width = NormalizeNonNegativeSize(size.width_);
  float measured_height = NormalizeNonNegativeSize(size.height_);
  if (has_max_width) {
    measured_width = std::min(measured_width, max_width);
  }
  if (has_max_height) {
    measured_height = std::min(measured_height, max_height);
  }
  float measured_baseline = NormalizeNonNegativeSize(size.baseline_);
  if (measured_baseline <= 0.f || measured_baseline > measured_height) {
    measured_baseline = measured_height;
  }
  return {.width_ = measured_width,
          .height_ = measured_height,
          .baseline_ = measured_baseline};
}

void MarkdownResourceLoaderHarmony::LoadImageResource(
    std::weak_ptr<CanvasImageMarkdownDrawable> drawable,
    const std::string& source) {
  if (source.empty()) {
    return;
  }
  if (IsBase64Image(source) || IsLocalResourceUrl(source)) {
    DecodeImageResource(drawable, source, source, IsBase64Image(source));
    return;
  }

  auto* context = context_.load();
  auto resource_loader = context ? context->GetResourceLoader() : nullptr;
  if (!resource_loader) {
    DecodeImageResource(drawable, source, source, false);
    return;
  }

  auto request =
      pub::LynxResourceRequest{source, pub::LynxResourceType::kImage};
  resource_loader->LoadResourcePath(
      request, [weak_self = weak_from_this(), drawable,
                source](pub::LynxPathResponse& response) {
        auto self = weak_self.lock();
        if (!self) {
          return;
        }
        self->OnImageResourceLoaded(drawable, source, response);
      });
}

void MarkdownResourceLoaderHarmony::DecodeImageResource(
    std::weak_ptr<CanvasImageMarkdownDrawable> drawable,
    const std::string& notify_source, const std::string& decode_source,
    bool is_base64) {
  if (destroyed_.load() || decode_source.empty()) {
    return;
  }
  if (auto cached_image = GetCachedImage(decode_source)) {
    SetCachedImage(notify_source, cached_image);
    if (auto image_view = drawable.lock()) {
      image_view->SetImage(cached_image);
    }
    NotifyImageLoaded(notify_source);
    return;
  }

  auto* context = context_.load();
  if (!context || !context->GetNapiEnv()) {
    return;
  }
  LynxImageHelper::DecodeImageAsync(
      context->GetNapiEnv(), decode_source, is_base64,
      [weak_self = weak_from_this(), drawable, notify_source,
       decode_source](LynxImageHelper::ImageResponse& response) {
        auto self = weak_self.lock();
        if (!self) {
          return;
        }
        self->OnImageDecoded(drawable, notify_source, decode_source, response);
      });
}

void MarkdownResourceLoaderHarmony::OnImageResourceLoaded(
    std::weak_ptr<CanvasImageMarkdownDrawable> drawable,
    const std::string& fallback_source, pub::LynxPathResponse& response) {
  std::string source = response.Success() && !response.path.empty()
                           ? response.path
                           : fallback_source;
  source = NormalizeDecodeSource(std::move(source));
  DecodeImageResource(drawable, fallback_source, source, IsBase64Image(source));
}

void MarkdownResourceLoaderHarmony::OnImageDecoded(
    std::weak_ptr<CanvasImageMarkdownDrawable> drawable,
    const std::string& notify_source, const std::string& decode_source,
    LynxImageHelper::ImageResponse& response) {
  if (destroyed_.load()) {
    return;
  }
  if (!response.Success() || !response.data) {
    LOGE("markdown image decode failed, err_code: "
         << response.err_code << ", source: " << notify_source);
    return;
  }

  std::shared_ptr<LynxBaseImage> image(response.data.release());
  SetCachedImage(decode_source, image);
  SetCachedImage(notify_source, image);
  if (auto image_view = drawable.lock()) {
    image_view->SetImage(image);
  }
  NotifyImageLoaded(notify_source);
}

std::shared_ptr<LynxBaseImage> MarkdownResourceLoaderHarmony::GetCachedImage(
    const std::string& source) {
  if (source.empty()) {
    return nullptr;
  }

  std::lock_guard<std::mutex> lock(image_cache_mutex_);
  auto iter = image_cache_.find(source);
  if (iter == image_cache_.end()) {
    return nullptr;
  }

  auto image = iter->second.lock();
  if (!image) {
    image_cache_.erase(iter);
  }
  return image;
}

void MarkdownResourceLoaderHarmony::SetCachedImage(
    const std::string& source, std::shared_ptr<LynxBaseImage> image) {
  if (source.empty() || !image || IsBase64Image(source)) {
    return;
  }
  std::lock_guard<std::mutex> lock(image_cache_mutex_);
  image_cache_[source] = std::move(image);
}

void MarkdownResourceLoaderHarmony::NotifyImageLoaded(
    const std::string& source) {
  if (destroyed_.load()) {
    return;
  }
  auto* markdown_view = markdown_view_.load();
  if (!markdown_view) {
    return;
  }
  if (auto* view = markdown_view->GetMarkdownView()) {
    view->OnImageLoaded(source);
    view->NeedsMeasure();
  }
  markdown_view->RequestMeasure();
  markdown_view->RequestDraw();
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
