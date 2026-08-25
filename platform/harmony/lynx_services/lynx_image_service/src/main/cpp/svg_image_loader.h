// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_SERVICES_LYNX_IMAGE_SERVICE_SRC_MAIN_CPP_SVG_IMAGE_LOADER_H_
#define PLATFORM_HARMONY_LYNX_SERVICES_LYNX_IMAGE_SERVICE_SRC_MAIN_CPP_SVG_IMAGE_LOADER_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "platform/harmony/lynx_harmony/src/main/cpp/public/image_service.h"

namespace lynx {
namespace service {
class ImageServiceHarmony;

class SvgImageLoaderImpl final
    : public tasm::harmony::SvgImageLoader,
      public std::enable_shared_from_this<SvgImageLoaderImpl> {
 public:
  SvgImageLoaderImpl(
      ImageServiceHarmony* image_service,
      std::unique_ptr<tasm::harmony::SvgResourceFetcher> resource_fetcher,
      std::function<void()> invalidation_callback, float density);
  ~SvgImageLoaderImpl() override;
  void FetchSvgImage(
      const tasm::harmony::ImageRequestInfo& info,
      tasm::harmony::ImageSuccessCallback on_load_success,
      tasm::harmony::ImageFailedCallback on_load_failed) override;
  void UpdateLayout(float width, float height, float padding_left,
                    float padding_top, float padding_right,
                    float padding_bottom) override;
  void Render(OH_Drawing_Canvas* canvas) override;

 private:
  static bool DecodeDataUri(const std::string& uri, std::string& data);
  static OH_PixelmapNative* ProvideImage(void* user_data, const char* source);
  void Reset();
  void LoadSvgSource();
  void UpdateSvgDocument();
  void ReportError(int error_code, const std::string& error_message);
  OH_PixelmapNative* GetImage(const std::string& source);

  ImageServiceHarmony* image_service_{nullptr};
  std::unique_ptr<tasm::harmony::SvgResourceFetcher> resource_fetcher_;
  float density_{1.f};
  void* svg_handle_{nullptr};
  std::string svg_source_;
  std::string svg_content_;
  bool source_loading_{false};
  bool document_ready_{false};
  float content_left_{0.f};
  float content_top_{0.f};
  float content_width_{0.f};
  float content_height_{0.f};
  tasm::harmony::ImageSuccessCallback on_load_success_;
  tasm::harmony::ImageFailedCallback on_load_failed_;
  std::function<void()> invalidation_callback_;
  std::unordered_map<std::string, std::shared_ptr<tasm::harmony::ImageData>>
      images_;
};

}  // namespace service
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_SERVICES_LYNX_IMAGE_SERVICE_SRC_MAIN_CPP_SVG_IMAGE_LOADER_H_
