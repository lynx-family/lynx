// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/base_image_view.h"

#include <array>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/include/fml/make_copyable.h"
#include "base/include/fml/memory/ref_counted.h"
#include "base/include/string/string_number_convert.h"
#include "base/include/string/string_utils.h"
#include "base/include/timer/time_utils.h"
#include "clay/common/graphics/shared_image_external_texture.h"
#include "clay/fml/logging.h"
#include "clay/gfx/animation/value_animator.h"
#include "clay/gfx/style/length.h"
#include "clay/net/loader/resource_loader_intercept.h"
#include "clay/net/url/url_helper.h"
#include "clay/public/value.h"
#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/component_constants.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/painter/image_painter.h"
#include "clay/ui/shadow/image_shadow_node.h"

#ifndef ENABLE_NATIVE_LIST
#include "clay/ui/component/list/base_list_view.h"
#endif

#ifdef ENABLE_SKITY
#include "clay/gfx/image/base_image.h"
#endif

#ifdef ENABLE_NET_LOADER
#include "clay/net/net_loader_manager.h"
#endif

namespace clay {

constexpr double kImageFadeInDuration = 300;

LYNX_UI_METHOD_BEGIN(BaseImageView) {
  LYNX_UI_METHOD(BaseImageView, startAnimate);
  LYNX_UI_METHOD(BaseImageView, stopAnimation);
  LYNX_UI_METHOD(BaseImageView, pauseAnimation);
  LYNX_UI_METHOD(BaseImageView, resumeAnimation);
}
LYNX_UI_METHOD_END(BaseImageView);

BaseImageView::BaseImageView(std::unique_ptr<RenderObject> render_object,
                             PageView* page_view)
    : WithTypeInfo(std::move(render_object), page_view), weak_factory_(this) {
#if FORCE_IMAGEVIEW_FOCUSABLE
  SetFocusable(true);
#endif
  transition_animator_ = std::make_unique<ValueAnimator>();
  transition_animator_->SetDuration(kImageFadeInDuration);
  transition_animator_->SetAnimationHandler(GetAnimationHandler());
  transition_animator_->AddUpdateListener(this);
  GetRenderImage()->AddClient(this);
}

BaseImageView::BaseImageView(uint32_t id, std::string tag,
                             std::unique_ptr<RenderObject> render_object,
                             PageView* page_view)
    : WithTypeInfo(id, std::move(tag), std::move(render_object), page_view),
      weak_factory_(this) {
#if FORCE_IMAGEVIEW_FOCUSABLE
  SetFocusable(true);
#endif
  transition_animator_ = std::make_unique<ValueAnimator>();
  transition_animator_->SetDuration(kImageFadeInDuration);
  transition_animator_->SetAnimationHandler(GetAnimationHandler());
  transition_animator_->AddUpdateListener(this);
  GetRenderImage()->AddClient(this);
}

BaseImageView::~BaseImageView() {
  GetRenderImage()->RemoveClient();
  TryEndTransition();
  transition_animator_.reset();
  TryCancelFetch(source_, source_fetch_id_);
  TryCancelFetch(placeholder_, placeholder_fetch_id_);
}

void BaseImageView::SetAttribute(const char* attr_c, const clay::Value& value) {
  auto kw = GetKeywordID(attr_c);
  switch (kw) {
    case KeywordID::kMode:
      SetMode(attribute_utils::GetCString(value));
      break;
    case KeywordID::kSrc:
      incoming_source_ = attribute_utils::GetCString(value);
      fetch_delay_source_ = true;
      break;
    case KeywordID::kRepeat:
      if (attribute_utils::GetBool(value)) {
        SetRepeat(ImageRepeat::kRepeat);
      } else {
        SetRepeat(ImageRepeat::kNoRepeat);
      }
      break;
    case KeywordID::kPlaceholder:
      incoming_placeholder_ = attribute_utils::GetCString(value);
      fetch_delay_placeholder_ = true;
      break;
    case KeywordID::kSkipRedirection:
      SetSkipRedirection(attribute_utils::GetBool(value));
      break;
    case KeywordID::kImageTransitionStyle:
      SetImageTransitionStyle(attribute_utils::GetCString(value));
      break;
    case KeywordID::kPreventLoadingOnListScroll:
      prevent_loading_on_list_scroll_ = attribute_utils::GetBool(value);
      break;
    case KeywordID::kBlurRadius: {
      std::string valueWithUnit = attribute_utils::GetCString(value);
      float blur =
          attribute_utils::ToPxWithDisplayMetrics(valueWithUnit, page_view());
      GetRenderImage()->SetBlurRadius(blur);
    } break;
    case KeywordID::kLowQuality: {
      bool enable_low_quality_image = attribute_utils::GetBool(value);
      GetRenderImage()->SetEnableLowQuality(enable_low_quality_image);
    } break;
    case KeywordID::kImageConfig: {
      std::string image_config = attribute_utils::GetCString(value);
      if (lynx::base::EqualsIgnoreCase(image_config, "RGB_565")) {
        GetRenderImage()->SetEnableLowQuality(true);
      } else if (lynx::base::EqualsIgnoreCase(image_config, "ARGB_8888")) {
        GetRenderImage()->SetEnableLowQuality(false);
      }
    } break;
    case KeywordID::kDownsampling: {
      bool down_sampling = attribute_utils::GetBool(value);
      GetRenderImage()->SetDownSampling(down_sampling);
    } break;
    case KeywordID::kCapInsets: {
      std::string cap_insets = attribute_utils::GetCString(value);
      SetCapInsets(cap_insets);
    } break;
    case KeywordID::kCapInsetsScale:
      SetCapInsetsScale(attribute_utils::GetDouble(value));
      break;
    case KeywordID::kDeferSrcInvalidation:
      defer_src_invalidation_ = attribute_utils::GetBool(value);
      break;
    case KeywordID::kAutoplay:
      SetAutoPlay(attribute_utils::GetBool(value));
      break;
    case KeywordID::kLoopCount:
      SetLoopCount(attribute_utils::GetInt(value));
      break;
    case KeywordID::kAutoSize:
      SetAutoSize(attribute_utils::GetBool(value));
      break;
    case KeywordID::kTintColor: {
      std::string tint_color = attribute_utils::GetCString(value);
      Color color;
      if (Color::Parse(tint_color, &color)) {
        GetRenderImage()->SetTintColor(color);
      }
      break;
    }
    case KeywordID::kEnableReportInfo:
      report_info_.enable_report_info = attribute_utils::GetBool(value);
      break;
    default:
      BaseView::SetAttribute(attr_c, value);
      break;
  }
}

void BaseImageView::OnNodeReady() {
  BaseView::OnNodeReady();
  GetRenderImage()->OnNodeReady();
}

void BaseImageView::SetLocalCache(bool use_local_cache) {}

void BaseImageView::SetSkipRedirection(bool skip_redirection) {
  bool should_redirect_url = !skip_redirection;
  if (should_redirect_url_ != should_redirect_url) {
    should_redirect_url_ = should_redirect_url;
    if (!source_.empty()) {
      TryCancelFetch(source_, source_fetch_id_);
      FetchSource();
    }
    if (!placeholder_.empty()) {
      TryCancelFetch(placeholder_, placeholder_fetch_id_);
      FetchPlaceholder();
    }
  }
}

void BaseImageView::SetImageTransitionStyle(const std::string& style) {
  if (style == attr_value::kImageTransitionFadeIn) {
    transition_style_ = ImageTransitionStyle::kFadeIn;
    SetRepaintBoundary(true);
  } else {
    transition_style_ = ImageTransitionStyle::kNone;
  }
}

void BaseImageView::SetImageLoadListener(Listener* listener) {
  listener_ = listener;
}

void BaseImageView::SetPlaceholder(std::string original_url) {
  if (original_url.compare(placeholder_) == 0) {
    return;
  }

  TryCancelFetch(placeholder_, placeholder_fetch_id_);
  placeholder_ = std::move(original_url);

  TryEndTransition();
  auto render_image = GetRenderImage();
  render_image->SetPlaceholderImage(nullptr);
  if (!render_image->GetImage()) {
    FetchPlaceholder();
  }
}

void BaseImageView::SetSource(std::string original_url) {
  if (original_url.compare(source_) == 0) {
    return;
  }

  TryCancelFetch(source_, source_fetch_id_);
  TryEndTransition();
  auto render_image = GetRenderImage();
  if (!defer_src_invalidation_ || original_url.empty()) {
    render_image->SetImage(nullptr);
  }
  if (!render_image->GetPlaceholderImage() &&
      placeholder_fetch_id_ == kDefaultImageFetchID) {
    FetchPlaceholder();
  }

  source_ = std::move(original_url);

  FetchSource();
}

void BaseImageView::SetSource(const std::string&& url, const uint8_t* source,
                              const int len) {
#ifndef ENABLE_SKITY
  page_view_->GetImageResourceFetcher()->GetImageResource(
      url,
      [self = weak_factory_.GetWeakPtr(),
       ui_task_runner = page_view_->GetTaskRunners().GetUITaskRunner()](
          std::unique_ptr<ImageResource> resource, bool from_cache) {
        FML_DCHECK(ui_task_runner->RunsTasksOnCurrentThread());
        if (self && resource) {
          self->TriggerTransitionIfNeeded();
          RenderImage* render_image = self->GetRenderImage();
          render_image->SetImage(std::move(resource));
        }
      },
      source, len, page_view_->UseTextureBackend(),
      page_view_->DeferredImageDecode(), page_view_->ImageDecodeWithPriority(),
      GetRenderImage() && GetRenderImage()->EnableLowQuality());
#endif  // ENABLE_SKITY
}

void BaseImageView::SetRepeat(ImageRepeat repeat) {
  auto render_image = GetRenderImage();
  render_image->SetRepeat(repeat);
}

void BaseImageView::SetMode(const std::string& mode) {
  FillMode fill_mode = FillMode::kScaleToFill;
  if (mode.compare(attr_value::kModeScaleToFill) == 0) {
    fill_mode = FillMode::kScaleToFill;
  } else if (mode.compare(attr_value::kModeAspectFit) == 0) {
    fill_mode = FillMode::kAspectFit;
  } else if (mode.compare(attr_value::kModeAspectFill) == 0) {
    fill_mode = FillMode::kAspectFill;
  } else if (mode.compare(attr_value::kModeCenter) == 0) {
    fill_mode = FillMode::kCenter;
  } else {
    return;
  }
  SetMode(fill_mode);
}

void BaseImageView::SetMode(FillMode fill_mode) {
  GetRenderImage()->SetMode(fill_mode);
}

void BaseImageView::SetEffect(ImageEffect effect) {
  GetRenderImage()->SetEffect(effect);
}

void BaseImageView::SetCapInsets(const std::string& cap_insets) {
  std::vector<std::string_view> cap_insets_strs =
      lynx::base::SplitToStringViews(cap_insets, " ");
  std::array<Length, 4> cap_insets_arr;
  size_t count = 0;
  for (const auto& s : cap_insets_strs) {
    float value;
    Length length;
    std::string percent_str(s.substr(0, s.length() - 1));
    std::string px_str(s.substr(0, s.length() - 2));
    if (s.length() >= 2 && lynx::base::EndsWith(s, "%") &&
        lynx::base::StringToFloat(percent_str, value)) {
      length.SetValue(value / 100.f);
      length.SetUnit(LengthUnit::kPercent);
    } else if (s.length() >= 3 && lynx::base::EndsWith(s, "px") &&
               lynx::base::StringToFloat(px_str, value)) {
      length.SetValue(value);
      length.SetUnit(LengthUnit::kNum);
    } else {
      FML_DLOG(ERROR) << "ImageView set invalid cap-insets!";
      return;
    }

    cap_insets_arr[count] = length;
    if (++count >= cap_insets_arr.size()) {
      break;
    }
  }
  if (count != 4) {
    FML_DLOG(ERROR) << "ImageView set invalid cap-insets!";
    return;
  }
  GetRenderImage()->SetCapInsets(cap_insets_arr);
}

void BaseImageView::SetCapInsetsScale(float scale) {
  auto render_image = GetRenderImage();
  render_image->SetCapInsetsScale(scale);
}

void BaseImageView::SetAutoPlay(bool auto_play) {
  auto render_image = GetRenderImage();
  render_image->SetAutoPlay(auto_play);
}

void BaseImageView::SetLoopCount(int loop_count) {
  auto render_image = GetRenderImage();
  render_image->SetLoopCount(loop_count);
}

void BaseImageView::startAnimate() {
  auto render_image = GetRenderImage();
  render_image->StartAnimate();
}
void BaseImageView::stopAnimation() {
  auto render_image = GetRenderImage();
  render_image->StopAnimation();
}
void BaseImageView::pauseAnimation() {
  auto render_image = GetRenderImage();
  render_image->PauseAnimation();
}
void BaseImageView::resumeAnimation() {
  auto render_image = GetRenderImage();
  render_image->ResumeAnimation();
}

void BaseImageView::SetAutoSize(bool auto_size) {
  auto render_image = GetRenderImage();
  render_image->SetAutoSize(auto_size);
}

void BaseImageView::NotifyLoadError(const std::string& error_msg) {
  if (HasEvent(event_attr::kEventError)) {
    page_view()->SendEvent(id(), event_attr::kEventError, {"errMsg"},
                           error_msg.c_str());
  }
  if (listener_) {
    listener_->OnImageLoadError(this, error_msg);
  }
}

void BaseImageView::NotifyLoadSuccess(int width, int height) {
  // Only send once if enable_report_info or extra_load_info is true.
  if (HasEvent(event_attr::kEventLoad) && !report_info_.enable_report_info) {
    page_view()->SendEvent(id(), event_attr::kEventLoad, {"width", "height"},
                           width, height);
  }
  if (listener_) {
    listener_->OnImageLoadSuccess(this, width, height);
  }
}

void BaseImageView::NotifyDecodedSuccess() {
  if (listener_) {
    listener_->OnImageDecodedSuccess(this);
  }
}

void BaseImageView::NotifyStartPlay() {
  if (HasEvent(event_attr::kEventStartPlay)) {
    page_view()->SendEvent(id(), event_attr::kEventStartPlay, {});
  }
}
void BaseImageView::NotifyCurrentLoopComplete() {
  if (HasEvent(event_attr::kEventCurrentLoopComplete)) {
    page_view()->SendEvent(id(), event_attr::kEventCurrentLoopComplete, {});
  }
}
void BaseImageView::NotifyFinalLoopComplete() {
  if (HasEvent(event_attr::kEventFinalLoopComplete)) {
    page_view()->SendEvent(id(), event_attr::kEventFinalLoopComplete, {});
  }
}

void BaseImageView::FetchPlaceholder() {
  if (placeholder_.empty()) {
    return;
  }
#ifndef ENABLE_SKITY
  placeholder_fetch_id_ =
      page_view_->GetImageResourceFetcher()->FetchImageAsync(
          placeholder_,
          [self = weak_factory_.GetWeakPtr()](
              std::unique_ptr<ImageResource> resource, bool hit_cache) {
            if (!self) {
              return;
            }
            self->placeholder_fetch_id_ = kDefaultImageFetchID;
            if (!resource || self->placeholder_ != resource->GetUrl()) {
              return;
            }

            if (!hit_cache) {
              self->TriggerTransitionIfNeeded();
            } else {
              self->report_info_.image_origin =
                  ReportInfo::ImageOrigin::kImageMemoryDecoded;
            }

            RenderImage* render_image = self->GetRenderImage();
            render_image->SetPlaceholderImage(std::move(resource));
          },
          page_view_->UseTextureBackend(),
          page_view_->DeferredImageDecode() && !defer_src_invalidation_,
          page_view_->ImageDecodeWithPriority(), should_redirect_url_,
          GetRenderImage() && GetRenderImage()->EnableLowQuality());
#else
  placeholder_fetch_id_ = page_view_->GetImageResourceFetcher()->FetchImage(
      placeholder_, IsSVG(),
      [self = weak_factory_.GetWeakPtr()](
          std::unique_ptr<BaseImageInstance> image_instance, bool hit_cache) {
        if (!self) {
          return;
        }
        self->placeholder_fetch_id_ = kDefaultImageFetchID;

        if (!image_instance) {
          return;
        }

        if (!hit_cache) {
          self->TriggerTransitionIfNeeded();
        } else {
          self->report_info_.image_origin =
              ReportInfo::ImageOrigin::kImageMemoryDecoded;
        }

        image_instance->SetAnimationFrameCallback([self]() {
          if (!self) {
            return;
          }
          self->GetRenderImage()->MarkNeedsPaint();
        });
        auto render_image = self->GetRenderImage();
        render_image->SetPlaceholderImage(std::move(image_instance));
      },
      should_redirect_url_);
#endif  // ENABLE_SKITY
}

void BaseImageView::FetchSource() {
  if (source_.empty()) {
    return;
  }
  report_info_.download_start_time =
      lynx::base::CurrentSystemTimeMilliseconds();
#ifndef ENABLE_SKITY
  source_fetch_id_ = page_view_->GetImageResourceFetcher()->FetchImageAsync(
      source_,
      [self = weak_factory_.GetWeakPtr()](
          std::unique_ptr<ImageResource> resource, bool hit_cache) {
        if (!self) {
          return;
        }
        self->source_fetch_id_ = kDefaultImageFetchID;
        if (!resource) {
          std::string error_msg = "{\"error\":\"failed to fetch resource\"";
#if ENABLE_NET_LOADER && (OS_WIN || OS_MAC || OS_LINUX)
          std::string lookup_url = url::TrimUrl(self->GetSource());
          if (self->should_redirect_url_ && self->page_view()) {
            auto intercept = self->page_view()->GetResourceLoaderIntercept();
            if (intercept) {
              lookup_url =
                  intercept->ShouldInterceptUrl(self->GetSource(), false);
            }
          }

          std::string response =
              NetLoaderManager::Instance().TakeLastResponse(lookup_url);
          if (!response.empty()) {
            error_msg += ",\"response\":";
            error_msg += response;
          }
#endif
          error_msg += "}";
          self->NotifyLoadError(error_msg);
          return;
        }

        if (!ImageResourceFetcher::SameImage(self->GetSource(),
                                             resource->GetUrl())) {
          return;
        }

        // Give real image size rather than the paint size.
        self->NotifyLoadSuccess(resource->GetWidth(), resource->GetHeight());

        if (self->prevent_loading_on_list_scroll_) {
#ifndef ENABLE_NATIVE_LIST
          auto parent_list = self->Parent();
          while (parent_list && !parent_list->Is<BaseListView>()) {
            parent_list = parent_list->Parent();
          }
          if (parent_list) {
            static_cast<BaseListView*>(parent_list)
                ->PostLowPriorityTask(fml::MakeCopyable(
                    [self, hit_cache,
                     resource = std::move(resource)]() mutable {
                      if (!self) {
                        return;
                      }

                      if (!hit_cache) {
                        self->TriggerTransitionIfNeeded();
                      }

                      RenderImage* render_image = self->GetRenderImage();
                      render_image->SetImage(std::move(resource));
                    }));
            return;
          }
#endif
        }

        if (!hit_cache) {
          self->TriggerTransitionIfNeeded();
        }

        RenderImage* render_image = self->GetRenderImage();
        render_image->SetImage(std::move(resource));
      },
      page_view_->UseTextureBackend(),
      page_view_->DeferredImageDecode() && !defer_src_invalidation_,
      page_view_->ImageDecodeWithPriority(), should_redirect_url_,
      GetRenderImage() && GetRenderImage()->EnableLowQuality(), false, IsSVG());
#else
  source_fetch_id_ = page_view_->GetImageResourceFetcher()->FetchImage(
      source_, IsSVG(),
      [self = weak_factory_.GetWeakPtr()](
          std::unique_ptr<BaseImageInstance> image_instance, bool hit_cache) {
        if (!self) {
          return;
        }
        self->source_fetch_id_ = kDefaultImageFetchID;

        if (!image_instance) {
          FML_LOG(ERROR) << "image is null";
          self->NotifyLoadError("resource fetch fail");
          return;
        }
        self->NotifyLoadSuccess(image_instance->GetWidth(),
                                image_instance->GetHeight());

        image_instance->SetAnimationFrameCallback([self]() {
          if (!self) {
            return;
          }
          self->GetRenderImage()->MarkNeedsPaint();
        });
        if (!hit_cache) {
          self->TriggerTransitionIfNeeded();
        }
#if OS_WIN || OS_MAC
        // FIXME(songchengjiang.real): Theoretically, whether to enable mipmap
        // should be determined by the Image component. However, this setting is
        // not currently exposed, so we temporarily enable it on Windows and
        // macOS.
        image_instance->GetImage()->SetMipmapped(true);
#endif
        auto render_image = self->GetRenderImage();
        render_image->SetImage(std::move(image_instance));
        self->ReportImageLoadInfo();
      },
      should_redirect_url_);
#endif  // ENABLE_SKITY
}

void BaseImageView::TryCancelFetch(const std::string& url,
                                   ImageFetchID& fetch_id) {
  if (url.empty() || fetch_id == kDefaultImageFetchID) {
    return;
  }

  page_view_->GetImageResourceFetcher()->TryCancelAsyncFetch(url, fetch_id);
  fetch_id = kDefaultImageFetchID;
}

void BaseImageView::OnAnimationUpdate(ValueAnimator& animation) {
  float opacity = 1.0f * animation.GetAnimatedFraction();
  GetRenderImage()->SetImageOpacity(opacity);
}

void BaseImageView::TriggerTransitionIfNeeded() {
  if (transition_animator_->IsRunning()) {
    transition_animator_->End();
    return;
  }

  auto render_image = GetRenderImage();
  if (!render_image->GetImage() && !render_image->GetPlaceholderImage() &&
      transition_style_ == ImageTransitionStyle::kFadeIn) {
    transition_animator_->Start();
  }
}

void BaseImageView::TryEndTransition() {
  if (transition_animator_->IsRunning()) {
    transition_animator_->End();
  }
}

void BaseImageView::DidUpdateAttributes() {
  BaseView::DidUpdateAttributes();
  if (fetch_delay_source_) {
    fetch_delay_source_ = false;
    SetSource(std::move(incoming_source_));
  }
  if (fetch_delay_placeholder_) {
    fetch_delay_placeholder_ = false;
    SetPlaceholder(std::move(incoming_placeholder_));
  }
}

// If decoding has not occurred but graphics image is prepared, it will also
// calls this callback
void BaseImageView::OnDecodeFinished(bool success, const std::string& url) {
  if (!success) {
    NotifyLoadError(GetSource());
  } else {
    NotifyDecodedSuccess();
    // Only report load info if this image is not placeholder.
    if (url == source_) {
      ReportImageLoadInfo();
    }
  }
}

void BaseImageView::RegisterUploadTask(OneShotCallback<>&& task, int image_id) {
  page_view()->RegisterUploadTask(std::move(task), image_id);
}

void BaseImageView::OnStartPlay() { NotifyStartPlay(); }
void BaseImageView::OnCurrentLoopComplete() { NotifyCurrentLoopComplete(); }
void BaseImageView::OnFinalLoopComplete() { NotifyFinalLoopComplete(); }
void BaseImageView::AdjustSizeIfNeeded(bool auto_size, float bitmap_width,
                                       float bitmap_height) {
  auto* shadow_node = page_view()->GetShadowNodeById(id_);
  if (shadow_node && shadow_node->IsImageShadowNode()) {
    static_cast<ImageShadowNode*>(shadow_node)
        ->AdjustSizeIfNeeded(auto_size, bitmap_width, bitmap_height);
  }
}

void BaseImageView::TryDecodeImmediately() {
  GetRenderImage()->SetNeedDecodeImmediately(true);
  GetRenderImage()->TryDecodeImmediately();
}

#ifndef NDEBUG
std::string BaseImageView::ToString() const {
  std::stringstream ss;
  ss << BaseView::ToString();
  ss << " source_=(" << source_ << ")";
  if (!placeholder_.empty()) {
    ss << " placeholder_=(" << placeholder_ << ")";
  }
  if (!incoming_source_.empty()) {
    ss << " incoming_source_=(" << incoming_source_ << ")";
  }
  if (!incoming_placeholder_.empty()) {
    ss << " incoming_placeholder_=(" << incoming_placeholder_ << ")";
  }
  return ss.str();
}
#endif

void BaseImageView::ReportImageLoadInfo() {
  if (!HasEvent(event_attr::kEventLoad) || !report_info_.enable_report_info) {
    return;
  }

  // load finish time includes decode cost.
  uint64_t load_finish = lynx::base::CurrentSystemTimeMilliseconds();
  uint64_t cost = load_finish - report_info_.download_start_time;
  int width = GetRenderImage()->GetImage()->GetWidth();
  int height = GetRenderImage()->GetImage()->GetHeight();
  size_t memory_cost =
      GetRenderImage()->GetImage()->GetGraphicsImageAllocSize();
  bool downsampled = GetRenderImage()->DownSampling();
  std::string url = source_;
  float view_width = Width();
  float view_height = Height();

  page_view()->SendEvent(
      id(), event_attr::kEventLoad,
      {"load_start", "load_finish", "cost", "src", "width", "height",
       "memory_cost", "downsampled", "view_width", "view_height", "origin"},
      std::to_string(report_info_.download_start_time),
      std::to_string(load_finish), std::to_string(cost), url, width, height,
      std::to_string(memory_cost), downsampled, view_width, view_height,
      static_cast<int>(report_info_.image_origin));
}

}  // namespace clay
