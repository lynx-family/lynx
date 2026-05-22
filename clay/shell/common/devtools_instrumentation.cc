// Copyright 2022 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "clay/shell/common/devtools_instrumentation.h"

#include <cmath>
#include <memory>
#include <string>
#include <utility>

#include "clay/fml/logging.h"
#include "third_party/rapidjson/document.h"

#ifndef ENABLE_SKITY
#include "third_party/rapidjson/rapidjson.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/utils/SkBase64.h"
#endif

namespace clay {

const int kMaxImageSize = 10000;

std::string DevtoolsInstrumentation::CreateJsonStringResult() {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  if (raster_document_.Accept(writer)) {
    return buffer.GetString();
  }
  return std::string();
}

#ifndef ENABLE_SKITY
void DevtoolsInstrumentation::UpdateRasterCacheInfo(
    const int64_t& single_cache_size, const int64_t& single_cache_height,
    const int64_t& single_cache_width,
    const std::string& single_cache_color_type, const int64_t& layer_address,
    const int64_t& cache_address, GrImagePtr image) {
  auto& allocator = raster_document_.GetAllocator();
  rapidjson::Value object(rapidjson::kObjectType);
  object.AddMember("size", single_cache_size, allocator);
  object.AddMember("height", single_cache_height, allocator);
  object.AddMember("width", single_cache_width, allocator);
  object.AddMember("color_space", single_cache_color_type, allocator);
  object.AddMember("layer_address", layer_address, allocator);
  if (image) {
    auto b64_data = CompressImageData(image.get());
    rapidjson::Value b64_value(static_cast<const char*>(b64_data->data()),
                               raster_document_.GetAllocator());
    object.AddMember("base64Image", b64_value, raster_document_.GetAllocator());
  }
  if (raster_document_.HasMember("image")) {
    raster_document_["image"].PushBack(object, allocator);
  } else {
    rapidjson::Value cache_member_array(rapidjson::kArrayType);
    cache_member_array.PushBack(object, allocator);
    raster_document_.AddMember("image", cache_member_array, allocator);
  }
}

sk_sp<SkData> DevtoolsInstrumentation::CompressImageData(const SkImage* image) {
  sk_sp<SkData> compressed_data;
  int image_height = image->height();
  int image_width = image->width();
  if (image_height * image_width < kMaxImageSize) {
    compressed_data = image->encodeToData();
  } else {
    float scale_size =
        sqrt(static_cast<float>(image_width * image_height) / kMaxImageSize);
    const auto scaled_image_info = image->imageInfo().makeDimensions(
        SkISize::Make(image_width / scale_size, image_height / scale_size));

    SkBitmap scaled_bitmap;
    if (!scaled_bitmap.tryAllocPixels(scaled_image_info)) {
      FML_LOG(ERROR) << "Failed to allocate memory for bitmap of size "
                     << scaled_image_info.computeMinByteSize() << "B";
      compressed_data = image->encodeToData();
    } else {
      if (image->scalePixels(
              scaled_bitmap.pixmap(),
              SkSamplingOptions(SkFilterMode::kLinear, SkMipmapMode::kNone),
              SkImage::kDisallow_CachingHint)) {
        auto scaled_image = SkImages::RasterFromBitmap(scaled_bitmap);
        compressed_data = scaled_image->encodeToData();
      } else {
        FML_LOG(ERROR) << "Could not scale pixels";
        compressed_data = image->encodeToData();
      }
    }
  }
  size_t b64_size = SkBase64::Encode(compressed_data->data(),
                                     compressed_data->size(), nullptr);
  sk_sp<SkData> b64_data = SkData::MakeUninitialized(b64_size + 1);
  char* b64_char = static_cast<char*>(b64_data->writable_data());
  SkBase64::Encode(compressed_data->data(), compressed_data->size(), b64_char);
  b64_char[b64_size] = 0;  // make it null terminated for printing
  return b64_data;
}
#else
void DevtoolsInstrumentation::UpdateRasterCacheInfo(
    const int64_t& single_cache_size, const int64_t& single_cache_height,
    const int64_t& single_cache_width,
    const std::string& single_cache_color_type, const int64_t& layer_address,
    const int64_t& cache_address, std::shared_ptr<skity::Image> image) {
  // TODO(zhangxiao.ninja) implement later
  FML_UNIMPLEMENTED();
}
#endif  // ENABLE_SKITY

void DevtoolsInstrumentation::UpdateRasterInfoToDevtool() {
  std::string result = CreateJsonStringResult();
  view_holder_->UpdateRasterCacheInfo(result);
}

}  //  namespace clay
