// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_xelement/markdown/src/main/cpp/impl/resource/markdown_resource_loader_harmony.h"

#include <algorithm>
#include <chrono>
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
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"
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
    : public serval::markdown::MarkdownDrawable,
      public std::enable_shared_from_this<CanvasImageMarkdownDrawable> {
 public:
  CanvasImageMarkdownDrawable(
      std::weak_ptr<MarkdownResourceLoaderHarmony> loader,
      fml::RefPtr<fml::TaskRunner> ui_runner, float desire_width,
      float desire_height, float max_width, float max_height, float width,
      float height, float density)
      : loader_(std::move(loader)),
        ui_runner_(std::move(ui_runner)),
        desire_width_(desire_width),
        desire_height_(desire_height),
        max_width_(max_width),
        max_height_(max_height),
        density_(density),
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
    // Bitmap dimensions are source pixels; Markdown layout uses device pixels.
    // Explicit sizes and constraints have already been converted by Serval.
    auto size = ResolveImageSize(desire_width_, desire_height_, max_width_,
                                 max_height_, intrinsic_width * density_,
                                 intrinsic_height * density_);
    std::lock_guard<std::mutex> lock(mutex_);
    image_ = std::move(image);
    frame_ends_.clear();
    int64_t duration = 0;
    if (image_->FrameCount() > 1) {
      frame_ends_.reserve(image_->FrameCount());
      for (uint32_t i = 0; i < image_->FrameCount(); ++i) {
        const int32_t frame_duration = image_->FrameDuration(i);
        duration += frame_duration > 0 ? frame_duration : 100;
        frame_ends_.push_back(duration);
      }
    }
    animation_start_ = {};
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
    int frame_index = 0;
    int64_t next_frame_delay = 0;
    auto loader = loader_.lock();
    {
      std::lock_guard<std::mutex> lock(mutex_);
      image = image_;
      measured_size = measured_size_;
      intrinsic_size = intrinsic_size_;
      visible = visible_;
      if (visible && loader && loader->EnableGif() && !frame_ends_.empty()) {
        const auto now = std::chrono::steady_clock::now();
        if (animation_start_ == std::chrono::steady_clock::time_point{}) {
          animation_start_ = now;
        }
        const auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                now - animation_start_)
                .count();
        const auto in_loop = elapsed % frame_ends_.back();
        const auto frame =
            std::upper_bound(frame_ends_.begin(), frame_ends_.end(), in_loop);
        frame_index = static_cast<int>(frame - frame_ends_.begin());
        next_frame_delay = *frame - in_loop;
      } else {
        animation_start_ = {};
      }
    }

    if (!visible || !image || measured_size.width_ <= 0.f ||
        measured_size.height_ <= 0.f || !std::isfinite(x) ||
        !std::isfinite(y)) {
      return;
    }

    auto* pixel_map = image->FramePixelMap(frame_index);
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
    if (next_frame_delay > 0 && !frame_pending_ && ui_runner_) {
      frame_pending_ = true;
      // Only a visible draw schedules another frame; pending work holds no
      // view or drawable alive after a page or its image is removed.
      ui_runner_->PostDelayedTask(
          [weak_self = weak_from_this()] {
            if (auto self = weak_self.lock()) {
              self->frame_pending_ = false;
              if (auto loader = self->loader_.lock()) {
                if (loader->EnableGif()) {
                  loader->RequestImageDraw();
                }
              }
            }
          },
          fml::TimeDelta::FromMilliseconds(next_frame_delay));
    }
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
  std::weak_ptr<MarkdownResourceLoaderHarmony> loader_;
  fml::RefPtr<fml::TaskRunner> ui_runner_;
  std::vector<int64_t> frame_ends_;
  std::chrono::steady_clock::time_point animation_start_{};
  bool frame_pending_{false};  // Accessed only on the UI thread.
  float desire_width_{0.f};
  float desire_height_{0.f};
  float max_width_{0.f};
  float max_height_{0.f};
  const float density_;
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
      std::string id_selector, int sign,
      serval::markdown::MeasureResult measure_result)
      : resource_loader_(std::move(resource_loader)),
        id_selector_(std::move(id_selector)),
        sign_(sign),
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

  void SetBounds(serval::markdown::RectF bounds) override {
    if (auto loader = resource_loader_.lock()) {
      loader->UpdateInlineViewBounds(sign_, bounds);
    }
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
  int sign_;
  serval::markdown::SizeF measured_size_{};
  float baseline_{0.f};
};

MarkdownResourceLoaderHarmony::MarkdownResourceLoaderHarmony(
    LynxContext* context, MarkdownShadowNode* markdown_node,
    serval::markdown::NativeServalMarkdownView* markdown_view)
    : context_(context),
      markdown_node_(markdown_node),
      markdown_view_(markdown_view),
      markdown_sign_(markdown_node->Signature()) {}

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
  auto* context = context_.load();
  if (!HasImageSource(source) || !context || destroyed_.load()) {
    return CreatePlaceholderDrawable(size.first, size.second);
  }

  auto image_view = std::make_shared<CanvasImageMarkdownDrawable>(
      weak_from_this(), context->GetUITaskRunner(), desire_width, desire_height,
      max_width, max_height, size.first, size.second, context->ScaledDensity());
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
        weak_from_this(), id_selector, child->Signature(), measure_result);
  }
  return nullptr;
}

void* MarkdownResourceLoaderHarmony::LoadFont(
    const char* family, serval::markdown::MarkdownFontWeight weight) {
  (void)family;
  (void)weight;
  return nullptr;
}

serval::markdown::MarkdownReplacementView
MarkdownResourceLoaderHarmony::LoadReplacementView(void* ud, int32_t id,
                                                   float max_width,
                                                   float max_height) {
  (void)ud;
  (void)id;
  const float width =
      max_width >= 0.f && std::isfinite(max_width) ? max_width : 1.f;
  const float height =
      max_height >= 0.f && std::isfinite(max_height) ? max_height : 1.f;
  return {CreatePlaceholderDrawable(width, height), {}};
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

void MarkdownResourceLoaderHarmony::UpdateInlineViewBounds(
    int sign, serval::markdown::RectF bounds) {
  // Serval updates cursor bounds on the renderer thread. Resolve the UI here;
  // shadow-node alignment belongs to the layout thread.
  auto* context = context_.load();
  if (!context || destroyed_.load()) {
    return;
  }
  auto* child = context->FindUIBySign(sign);
  auto* markdown_ui = context->FindUIBySign(markdown_sign_);
  if (!child || !markdown_ui) {
    return;
  }
  const bool visible = bounds.GetWidth() > 0 && bounds.GetHeight() > 0;
  child->SetVisibility(visible);
  if (!visible) {
    return;
  }
  const float density = context->ScaledDensity();
  float left = bounds.GetLeft() / density + markdown_ui->padding_left_ +
               markdown_ui->GetBorderLeftWidth();
  float top = bounds.GetTop() / density + markdown_ui->padding_top_ +
              markdown_ui->GetBorderTopWidth();
  if (child->Parent() && child->Parent() != markdown_ui) {
    float markdown_rect[4] = {0};
    float parent_rect[4] = {0};
    markdown_ui->GetBoundingClientRect(markdown_rect);
    child->Parent()->GetBoundingClientRect(parent_rect);
    left += markdown_rect[0] - parent_rect[0];
    top += markdown_rect[1] - parent_rect[1];
  }
  const float width = bounds.GetWidth() / density;
  const float height = bounds.GetHeight() / density;
  if (left == child->left_ && top == child->top_ && width == child->width_ &&
      height == child->height_) {
    return;
  }
  const float paddings[] = {child->padding_left_, child->padding_top_,
                            child->padding_right_, child->padding_bottom_};
  const float margins[] = {child->margin_left_, child->margin_top_,
                           child->margin_right_, child->margin_bottom_};
  child->UpdateLayout(left, top, width, height, paddings, margins, nullptr,
                      height, 0);
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
    LOGE("markdown image resource loader is unavailable, source: " << source);
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
  if (!response.Success() || response.path.empty()) {
    LOGE("markdown image resource load failed, source: " << fallback_source);
    return;
  }
  std::string source = response.path;
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

void MarkdownResourceLoaderHarmony::RequestImageDraw() {
  if (!destroyed_.load()) {
    if (auto* view = markdown_view_.load()) {
      view->RequestDraw();
    }
  }
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
