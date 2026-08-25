// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_services/lynx_image_service/src/main/cpp/svg_image_loader.h"

#include <dlfcn.h>

#include <algorithm>
#include <utility>
#include <vector>

#include "base/include/datauri_utils.h"
#include "base/include/log/logging.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/lynx_image_constants.h"
#include "platform/harmony/lynx_services/lynx_image_service/src/main/cpp/image_service_harmony.h"

namespace lynx {
namespace service {
namespace {
constexpr char kSvgDataUriPrefix[] = "data:image/svg+xml;base64,";
constexpr char kServalSvgSoName[] = "libservalsvg.so";
using ServalSvgImageProvider = OH_PixelmapNative* (*)(void*, const char*);
template <typename Function>
Function ResolveServalSvgSymbol(void* handle, const char* symbol) {
  void* address = dlsym(handle, symbol);
  if (address == nullptr) {
    LOGE("Serval SVG API is unavailable: " << symbol);
  }
  return reinterpret_cast<Function>(address);
}

struct ServalSvgApi {
  using Create = void* (*)(ServalSvgImageProvider, void*);
  using Destroy = int (*)(void*);
  using Update = int (*)(void*, const char*, size_t, float, float, float, float,
                         bool, const char*);
  using Render = int (*)(void*, OH_Drawing_Canvas*);

  ServalSvgApi() : handle(dlopen(kServalSvgSoName, RTLD_NOW | RTLD_LOCAL)) {
    if (handle == nullptr) {
      LOGE("Serval SVG library is unavailable: " << kServalSvgSoName);
      return;
    }
    create = ResolveServalSvgSymbol<Create>(handle, "serval_svg_create");
    destroy = ResolveServalSvgSymbol<Destroy>(handle, "serval_svg_destroy");
    update = ResolveServalSvgSymbol<Update>(handle, "serval_svg_update");
    render = ResolveServalSvgSymbol<Render>(handle, "serval_svg_render");
  }

  bool IsAvailable() const {
    return create != nullptr && destroy != nullptr && update != nullptr &&
           render != nullptr;
  }

  void* handle = nullptr;
  Create create = nullptr;
  Destroy destroy = nullptr;
  Update update = nullptr;
  Render render = nullptr;
};

const ServalSvgApi& GetServalSvgApi() {
  static const ServalSvgApi api;
  return api;
}
}  // namespace

SvgImageLoaderImpl::SvgImageLoaderImpl(
    ImageServiceHarmony* image_service,
    std::unique_ptr<tasm::harmony::SvgResourceFetcher> resource_fetcher,
    std::function<void()> invalidation_callback, float density)
    : image_service_(image_service),
      resource_fetcher_(std::move(resource_fetcher)),
      density_(density),
      invalidation_callback_(std::move(invalidation_callback)) {
  const auto& api = GetServalSvgApi();
  if (api.IsAvailable()) {
    svg_handle_ = api.create(ProvideImage, this);
  }
}

SvgImageLoaderImpl::~SvgImageLoaderImpl() {
  if (svg_handle_) {
    GetServalSvgApi().destroy(svg_handle_);
  }
}

bool SvgImageLoaderImpl::DecodeDataUri(const std::string& uri,
                                       std::string& data) {
  const int32_t size =
      base::DataURIUtil::DecodeDataURI(uri, [&data](size_t buffer_size) {
        data.resize(buffer_size);
        return data.data();
      });
  if (size <= 0) {
    data.clear();
    return false;
  }
  data.resize(static_cast<size_t>(size));
  return true;
}

void SvgImageLoaderImpl::FetchSvgImage(
    const tasm::harmony::ImageRequestInfo& info,
    tasm::harmony::ImageSuccessCallback on_load_success,
    tasm::harmony::ImageFailedCallback on_load_failed) {
  if (svg_source_ != info.url) {
    Reset();
    svg_source_ = info.url;
  }
  on_load_success_ = std::move(on_load_success);
  on_load_failed_ = std::move(on_load_failed);
  if (!svg_handle_) {
    ReportError(tasm::harmony::image::kPathErrorCode,
                "Serval SVG renderer is unavailable.");
    return;
  }
  if (document_ready_ || source_loading_) {
    return;
  }
  if (svg_content_.empty()) {
    source_loading_ = true;
    LoadSvgSource();
  } else {
    UpdateSvgDocument();
  }
}

void SvgImageLoaderImpl::Reset() {
  svg_source_.clear();
  svg_content_.clear();
  source_loading_ = false;
  document_ready_ = false;
  images_.clear();
}

void SvgImageLoaderImpl::UpdateLayout(float width, float height,
                                      float padding_left, float padding_top,
                                      float padding_right,
                                      float padding_bottom) {
  const float content_left = padding_left * density_;
  const float content_top = padding_top * density_;
  const float content_width =
      std::max(0.f, width - padding_left - padding_right) * density_;
  const float content_height =
      std::max(0.f, height - padding_top - padding_bottom) * density_;
  if (content_left_ == content_left && content_top_ == content_top &&
      content_width_ == content_width && content_height_ == content_height) {
    return;
  }
  content_left_ = content_left;
  content_top_ = content_top;
  content_width_ = content_width;
  content_height_ = content_height;
  UpdateSvgDocument();
}

void SvgImageLoaderImpl::Render(OH_Drawing_Canvas* canvas) {
  if (document_ready_ && svg_handle_ && canvas) {
    GetServalSvgApi().render(svg_handle_, canvas);
  }
}

void SvgImageLoaderImpl::LoadSvgSource() {
  if (svg_source_.compare(0, sizeof(kSvgDataUriPrefix) - 1,
                          kSvgDataUriPrefix) == 0) {
    source_loading_ = false;
    if (!DecodeDataUri(svg_source_, svg_content_)) {
      ReportError(tasm::harmony::image::kPathErrorCode,
                  "Failed to decode SVG data URI.");
      return;
    }
    UpdateSvgDocument();
    return;
  }
  if (!resource_fetcher_) {
    source_loading_ = false;
    ReportError(tasm::harmony::image::kPathErrorCode,
                "SVG resource loader is unavailable.");
    return;
  }
  const std::string source = svg_source_;
  resource_fetcher_->Fetch(source, [weak_loader = weak_from_this(), source](
                                       int error_code,
                                       const std::string& error_message,
                                       std::vector<uint8_t> data) {
    auto loader = weak_loader.lock();
    if (!loader || loader->svg_source_ != source) {
      return;
    }
    loader->source_loading_ = false;
    if (error_code != 0 || data.empty()) {
      loader->ReportError(
          error_code == 0 ? tasm::harmony::image::kPathErrorCode : error_code,
          error_message.empty() ? "Failed to fetch SVG resource."
                                : error_message);
      return;
    }
    loader->svg_content_.assign(reinterpret_cast<const char*>(data.data()),
                                data.size());
    loader->UpdateSvgDocument();
  });
}

void SvgImageLoaderImpl::UpdateSvgDocument() {
  if (!svg_handle_ || svg_content_.empty() || content_width_ <= 0.f ||
      content_height_ <= 0.f) {
    return;
  }
  const auto& api = GetServalSvgApi();
  const int result = api.update(
      svg_handle_, svg_content_.data(), svg_content_.size(), content_left_,
      content_top_, content_width_, content_height_, true, nullptr);
  if (result < 0) {
    document_ready_ = false;
    ReportError(
        tasm::harmony::image::kPathErrorCode,
        "Failed to update SVG document, result: " + std::to_string(result));
    svg_content_.clear();
    return;
  }
  document_ready_ = true;
  if (invalidation_callback_) {
    invalidation_callback_();
  }
  if (on_load_success_) {
    on_load_success_(content_width_, content_height_);
  }
}

void SvgImageLoaderImpl::ReportError(int error_code,
                                     const std::string& error_message) {
  LOGE("Failed to load SVG, src: " << svg_source_
                                   << ", error: " << error_message);
  if (on_load_failed_) {
    on_load_failed_(error_code, error_message);
  }
}

OH_PixelmapNative* SvgImageLoaderImpl::ProvideImage(void* user_data,
                                                    const char* source) {
  if (!user_data || !source) {
    return nullptr;
  }
  return static_cast<SvgImageLoaderImpl*>(user_data)->GetImage(source);
}

OH_PixelmapNative* SvgImageLoaderImpl::GetImage(const std::string& source) {
  auto [entry, inserted] = images_.try_emplace(source, nullptr);
  if (inserted) {
    tasm::harmony::ImageRequestInfo request{.url = source};
    image_service_->DecodeImage(
        request,
        [weak_loader = weak_from_this(),
         source](const std::shared_ptr<tasm::harmony::ImageData>& image) {
          auto loader = weak_loader.lock();
          if (!loader || !image || !image->Pixelmap()) {
            return;
          }
          loader->images_.insert_or_assign(source, image);
          if (loader->invalidation_callback_) {
            loader->invalidation_callback_();
          }
        },
        [](float, float) {},
        [source](int error_code, const std::string& error_message) {
          LOGE("Failed to load embedded SVG image, src: "
               << source << ", error_code: " << error_code
               << ", error: " << error_message);
        });
  }
  return entry->second ? entry->second->Pixelmap() : nullptr;
}

}  // namespace service
}  // namespace lynx
