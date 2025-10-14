// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/net/loader/data_image_loader.h"

#include <memory>
#include <utility>

#include "clay/fml/base64.h"
#include "clay/fml/logging.h"

namespace {
static constexpr char kDataURISuffixBase64[] = ";base64,";
static constexpr char kDataURIPrefixImage[] = "data:image/";
static constexpr char kDataURIPrefixOctetStream[] =
    "data:application/octet-stream";
static constexpr size_t kDataURIPrefixImageLen =
    sizeof(kDataURIPrefixImage) - 1;
static constexpr size_t kDataURIPrefixOctetStreamLen =
    sizeof(kDataURIPrefixOctetStream) - 1;

// Checks if the URI starts with "data:image/" or
// "data:application/octet-stream", and is immediately followed by ";base64,"
// (e.g. "data:image/png;base64," or "data:application/octet-stream;base64,").
static bool IsBase64Uri(const std::string& uri) {
  size_t prefix_len = 0;
  if (strncmp(uri.c_str(), kDataURIPrefixImage, kDataURIPrefixImageLen) == 0) {
    prefix_len = kDataURIPrefixImageLen;
  } else if (strncmp(uri.c_str(), kDataURIPrefixOctetStream,
                     kDataURIPrefixOctetStreamLen) == 0) {
    prefix_len = kDataURIPrefixOctetStreamLen;
  } else {
    return false;
  }

  const char* encoding = strstr(uri.c_str() + prefix_len, kDataURISuffixBase64);
  if (!encoding) {
    return false;
  }
  return true;
}

}  // namespace

namespace clay {

// Copy from skia SkResources.cpp
static RawResource DecodeBase64Image(const char* uri) {
  const char* encoding = strstr(uri, kDataURISuffixBase64);
  if (!encoding) {
    return {0, nullptr};
  }
  const char* base64_data = encoding + std::size(kDataURISuffixBase64) - 1;
  size_t base64_data_len = strlen(base64_data);
  size_t data_length;
  if (fml::Base64::Decode(base64_data, base64_data_len, nullptr,
                          &data_length) != fml::Base64::kNoError) {
    return {0, nullptr};
  }

  auto encode_data = std::shared_ptr<uint8_t>(new uint8_t[data_length],
                                              std::default_delete<uint8_t[]>());

  if (fml::Base64::Decode(base64_data, base64_data_len, encode_data.get(),
                          &data_length) != fml::Base64::kNoError) {
    return {0, nullptr};
  }

  return {data_length, encode_data};
}

DataImageLoader::DataImageLoader(fml::RefPtr<fml::TaskRunner> task_runner)
    : task_runner_(task_runner) {}

void DataImageLoader::Load(
    const std::string& src,
    const std::function<void(const uint8_t*, size_t)>& callback,
    const ResourceType resource_type, bool need_redirect) {
  if (!callback) {
    return;
  }

  if (!task_runner_) {
    callback(nullptr, 0);
    return;
  }

  task_runner_->PostTask([uri = src, callback]() {
    if (IsBase64Uri(uri)) {
      auto result = DecodeBase64Image(uri.c_str());
      if (result.data) {
        callback(result.data.get(), result.length);
        return;
      }
    }
    callback(nullptr, 0);
  });
}

RawResource DataImageLoader::LoadSync(const std::string& uri,
                                      const ResourceType resource_type,
                                      bool need_redirect) {
  if (IsBase64Uri(uri)) {
    auto result = DecodeBase64Image(uri.c_str());

    if (result.data) {
      return result;
    }
  }
  FML_LOG(ERROR) << "Data type uri not supported or failed decode.";
  return {0, nullptr};
}

}  // namespace clay
