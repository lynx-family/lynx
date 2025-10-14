// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_BASE_IMAGE_VIEW_H_
#define CLAY_UI_COMPONENT_BASE_IMAGE_VIEW_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "base/include/fml/memory/weak_ptr.h"
#include "base/include/fml/time/timer.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/rendering/render_image.h"
#include "clay/ui/rendering/render_object.h"
#include "clay/ui/resource/image_resource_fetcher.h"

namespace clay {

enum class ImageTransitionStyle {
  kNone,
  kFadeIn,
};

class BaseImageView : public WithTypeInfo<BaseImageView, BaseView>,
                      AnimatorUpdateListener,
                      RenderImageClient {
 public:
  class Listener {
   public:
    virtual void OnImageLoadSuccess(BaseImageView* image, int width,
                                    int height) = 0;
    virtual void OnImageDecodedSuccess(BaseImageView* image) = 0;
    virtual void OnImageLoadError(BaseImageView* image,
                                  const std::string& error_msg) = 0;
  };
  BaseImageView(std::unique_ptr<RenderObject> render_object,
                PageView* page_view);
  BaseImageView(uint32_t id, std::string tag,
                std::unique_ptr<RenderObject> render_object,
                PageView* page_view);
  ~BaseImageView() override;

  void SetAttribute(const char* attr_c, const clay::Value& value) override;
  void OnNodeReady() override;

  void SetLocalCache(bool use_local_cache);
  void SetSkipRedirection(bool skip_redirection);
  void SetSource(std::string original_url);
  void SetSource(const std::string&& url, const uint8_t* source, const int len);
  void SetRepeat(ImageRepeat repeat);
  void SetPlaceholder(std::string original_url);
  void SetMode(const std::string& fill_mode);
  void SetMode(FillMode fill_mode);
  void SetEffect(ImageEffect effect);
  void SetCapInsets(const std::string& cap_insets);
  void SetCapInsetsScale(float scale);
  void SetImageTransitionStyle(const std::string& style);
  void SetImageLoadListener(Listener* listener);
  void SetAutoPlay(bool auto_play);
  void SetLoopCount(int loop_count);
  void SetAutoSize(bool auto_size);

  std::string GetSource() const { return source_; }
  virtual bool IsSVG() const { return false; }

  void DidUpdateAttributes() override;

#ifdef ENABLE_ACCESSIBILITY
  bool EnableAccessibilityElement() const override { return true; }
#endif

  void startAnimate();
  void stopAnimation();
  void pauseAnimation();
  void resumeAnimation();

  void OnDecodeFinished(bool success) override;
  void RegisterUploadTask(OneShotCallback<>&& task, int image_id) override;
  void OnStartPlay() override;
  void OnCurrentLoopComplete() override;
  void OnFinalLoopComplete() override;
  void AdjustSizeIfNeeded(bool auto_size, float bitmap_width,
                          float bitmap_height) override;

  void TryDecodeImmediately() override;

#ifndef NDEBUG
  std::string ToString() const override;
#endif

 protected:
  RenderImage* GetRenderImage();
  void NotifyLoadSuccess(int width, int height);

 private:
  void NotifyLoadError(const std::string& error_msg);
  void NotifyDecodedSuccess();
  void NotifyStartPlay();
  void NotifyCurrentLoopComplete();
  void NotifyFinalLoopComplete();
  void FetchPlaceholder();
  void FetchSource();
  void TryCancelFetch(const std::string&, ImageFetchID&);

  void OnAnimationUpdate(ValueAnimator& animation) override;

  void TriggerTransitionIfNeeded();
  void TryEndTransition();

  bool should_redirect_url_ = true;
  bool prevent_loading_on_list_scroll_ = false;

  std::string source_;
  ImageFetchID source_fetch_id_ = kDefaultImageFetchID;
  std::string placeholder_;
  ImageFetchID placeholder_fetch_id_ = kDefaultImageFetchID;
  fml::WeakPtrFactory<BaseImageView> weak_factory_;

  ImageTransitionStyle transition_style_ = ImageTransitionStyle::kNone;
  std::unique_ptr<ValueAnimator> transition_animator_;
  std::string incoming_source_ = "";
  bool fetch_delay_source_ = false;
  std::string incoming_placeholder_ = "";
  bool fetch_delay_placeholder_ = false;
  bool defer_src_invalidation_ = false;
  Listener* listener_ = nullptr;
};

inline RenderImage* BaseImageView::GetRenderImage() {
  return static_cast<RenderImage*>(render_object_.get());
}

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_BASE_IMAGE_VIEW_H_
