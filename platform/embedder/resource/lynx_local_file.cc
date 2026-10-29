// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cstring>
#include <string>

#include "clay/fml/mapping.h"
#include "clay/fml/paths.h"
#include "clay/net/url/url_helper.h"
#include "platform/embedder/public/capi/lynx_local_file_capi.h"
#include "platform/embedder/public/capi/lynx_memory_capi.h"

LYNX_EXTERN_C uint8_t* lynx_read_local_file(const char* file_url,
                                            size_t* size) {
  if (!file_url || !size) {
    return nullptr;
  }
  *size = 0;

  std::string uri(file_url);
  if (clay::url::ParseUriScheme(uri) != clay::url::UriSchemeType::kLocalFile) {
    return nullptr;
  }

  std::string file_path = fml::paths::AbsolutePath(fml::paths::FromURI(uri));
  auto mapping = fml::FileMapping::CreateReadOnly(file_path);
  if (!mapping || !mapping->IsValid()) {
    return nullptr;
  }

  *size = mapping->GetSize();
  auto* data = static_cast<uint8_t*>(lynx_malloc(*size == 0 ? 1 : *size));
  if (!data) {
    *size = 0;
    return nullptr;
  }
  if (*size != 0) {
    std::memcpy(data, mapping->GetMapping(), *size);
  }
  return data;
}
